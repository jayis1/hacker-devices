/*
 * drivers/fpga_if.c — FPGA Interface Implementation for Prism-Tap
 *
 * Handles SPI communication with the Lattice iCE40-UP5K FPGA, including
 * bitstream loading (SSPI configuration), register access, and DMA-based
 * frame buffer transfers.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "fpga_if.h"
#include "board.h"
#include "registers.h"

/* ---- Private SPI helpers ---- */

static void fpga_spi_cs_low(void)
{
    GPIO_RESET(FPGA_CSS_PORT, FPGA_CSS_PIN);
}

static void fpga_spi_cs_high(void)
{
    GPIO_SET(FPGA_CSS_PORT, FPGA_CSS_PIN);
}

static void fpga_spi_write_byte(uint8_t b)
{
    /* Wait for TXE */
    while (!(SPI_SR(SPI3_BASE) & SPI_SR_TXE))
        ;
    *(volatile uint8_t *)((uint32_t)SPI3_BASE + 0x0C) = b;
    /* Wait for RXNE (implies byte fully transferred) */
    while (!(SPI_SR(SPI3_BASE) & SPI_SR_RXNE))
        ;
    (void)*(volatile uint8_t *)((uint32_t)SPI3_BASE + 0x0C); /* discard RX */
}

static uint8_t fpga_spi_read_byte(void)
{
    while (!(SPI_SR(SPI3_BASE) & SPI_SR_TXE))
        ;
    *(volatile uint8_t *)((uint32_t)SPI3_BASE + 0x0C) = 0xFF;
    while (!(SPI_SR(SPI3_BASE) & SPI_SR_RXNE))
        ;
    return *(volatile uint8_t *)((uint32_t)SPI3_BASE + 0x0C);
}

static void fpga_spi_write_buf(const uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
        fpga_spi_write_byte(buf[i]);
}

static void fpga_spi_read_buf(uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
        buf[i] = fpga_spi_read_byte();
}

/* ---- SPI3 initialization for FPGA ---- */

static int fpga_spi_init(void)
{
    /* Enable GPIOB clock (SCK=PB3, MISO=PB4, MOSI=PB5) */
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOBEN;

    /* Enable SPI3 clock */
    RCC_APB1LENR |= RCC_APB1LENR_SPI3EN;

    /* Configure PB3 (SCK), PB4 (MISO), PB5 (MOSI) as alternate function */
    uint32_t moder = GPIO_REG(GPIOB_BASE, GPIO_MODER_OFFSET);
    moder &= ~(3U << (FPGA_SCK_PIN * 2));
    moder &= ~(3U << (FPGA_MISO_PIN * 2));
    moder &= ~(3U << (FPGA_MOSI_PIN * 2));
    moder |= (GPIO_MODE_AF << (FPGA_SCK_PIN * 2));
    moder |= (GPIO_MODE_AF << (FPGA_MISO_PIN * 2));
    moder |= (GPIO_MODE_AF << (FPGA_MOSI_PIN * 2));
    GPIO_REG(GPIOB_BASE, GPIO_MODER_OFFSET) = moder;

    /* Set alternate function AF6 for SPI3 */
    /* PB3 → AFRL[3], PB4 → AFRL[4], PB5 → AFRL[5] */
    uint32_t afrl = GPIO_REG(GPIOB_BASE, GPIO_AFRL_OFFSET);
    for (int pin = 3; pin <= 5; pin++) {
        afrl &= ~(0xFU << (pin * 4));
        afrl |= (AF_SPI3_SCK_PB3 << (pin * 4));
    }
    GPIO_REG(GPIOB_BASE, GPIO_AFRL_OFFSET) = afrl;

    /* High speed for FPGA SPI */
    uint32_t speed = GPIO_REG(GPIOB_BASE, GPIO_OSPEEDR_OFFSET);
    for (int pin = 3; pin <= 5; pin++) {
        speed &= ~(3U << (pin * 2));
        speed |= (GPIO_SPEED_VHIGH << (pin * 2));
    }
    GPIO_REG(GPIOB_BASE, GPIO_OSPEEDR_OFFSET) = speed;

    /* Configure CRESET (PB6) and CSS (PB7) as GPIO output, CDONE (PB2) as input */
    /* Enable clocks already set above */
    uint32_t moder2 = GPIO_REG(GPIOB_BASE, GPIO_MODER_OFFSET);
    moder2 &= ~(3U << (FPGA_CRESET_PIN * 2));   /* PB6 output */
    moder2 |= (GPIO_MODE_OUTPUT << (FPGA_CRESET_PIN * 2));
    moder2 &= ~(3U << (FPGA_CSS_PIN * 2));      /* PB7 output */
    moder2 |= (GPIO_MODE_OUTPUT << (FPGA_CSS_PIN * 2));
    moder2 &= ~(3U << (FPGA_CDONE_PIN * 2));    /* PB2 input */
    moder2 |= (GPIO_MODE_INPUT << (FPGA_CDONE_PIN * 2));
    GPIO_REG(GPIOB_BASE, GPIO_MODER_OFFSET) = moder2;

    /* Default CS high */
    fpga_spi_cs_high();

    /* Configure SPI3: master, CPOL=1, CPHA=1 (mode 3 for iCE40), 8-bit, prescaler /2 */
    SPI_CR1(SPI3_BASE) = 0;  /* Disable before config */
    SPI_CR1(SPI3_BASE) = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI |
                         SPI_CR1_CPOL | SPI_CR1_CPHA |
                         (SPI_CR1_BR_DIV2 << SPI_CR1_BR_SHIFT);
    SPI_CR2(SPI3_BASE) = SPI_CR2_DS_8BIT | SPI_CR2_SSOE;
    SPI_CR1(SPI3_BASE) |= SPI_CR1_SPE;

    return 0;
}

/* ---- iCE40 bitstream loading ----
 *
 * The iCE40 family uses a simple SPI configuration protocol:
 *   1. Pull CRESET_B low, wait >200ns
 *   2. Pull CRESET_B high
 *   3. Wait for CDONE to go high (optional; can timeout and proceed)
 *   4. Pull SPI CS (CSS_B) low
 *   5. Send dummy bytes (at least 8)
 *   6. Send bitstream data
 *   7. Send dummy bytes (at least 4)
 *   8. Pull CSS_B high
 *   9. Wait for CDONE to go high
 */

int fpga_load_bitstream(const uint8_t *bitstream, uint32_t length)
{
    if (bitstream == 0 || length == 0)
        return -1;

    /* Reset FPGA */
    GPIO_RESET(FPGA_CRESET_PORT, FPGA_CRESET_PIN);
    /* Delay >200 ns (at 275 MHz, ~55 cycles. A simple loop suffices.) */
    for (volatile int i = 0; i < 100; i++)
        ;
    GPIO_SET(FPGA_CRESET_PORT, FPGA_CRESET_PIN);
    /* Wait for CDONE to go low (reset acknowledged) */
    for (volatile int i = 0; i < 1000; i++) {
        if (!GPIO_READ(FPGA_CDONE_PORT, FPGA_CDONE_PIN))
            break;
    }

    /* Select FPGA on SPI */
    fpga_spi_cs_low();

    /* Send 8 dummy bytes */
    for (int i = 0; i < 8; i++)
        fpga_spi_write_byte(0x00);

    /* Send bitstream */
    fpga_spi_write_buf(bitstream, length);

    /* Send 4+ dummy bytes after bitstream */
    for (int i = 0; i < 4; i++)
        fpga_spi_write_byte(0x00);

    /* Deselect */
    fpga_spi_cs_high();

    /* Wait for CDONE to go high (FPGA configured) */
    for (volatile int i = 0; i < 100000; i++) {
        if (GPIO_READ(FPGA_CDONE_PORT, FPGA_CDONE_PIN))
            return 0;  /* success */
    }

    return -2;  /* timeout: CDONE never went high */
}

/* ---- Register read/write ---- */

uint16_t fpga_read_reg(uint16_t addr)
{
    uint16_t val;

    fpga_spi_cs_low();

    /* Command byte: 0x01 = read register */
    fpga_spi_write_byte(0x01);
    fpga_spi_write_byte((uint8_t)(addr >> 8));
    fpga_spi_write_byte((uint8_t)(addr & 0xFF));

    /* Read 2 bytes (big-endian) */
    uint8_t hi = fpga_spi_read_byte();
    uint8_t lo = fpga_spi_read_byte();
    val = ((uint16_t)hi << 8) | lo;

    fpga_spi_cs_high();
    return val;
}

void fpga_write_reg(uint16_t addr, uint16_t data)
{
    fpga_spi_cs_low();

    /* Command byte: 0x02 = write register */
    fpga_spi_write_byte(0x02);
    fpga_spi_write_byte((uint8_t)(addr >> 8));
    fpga_spi_write_byte((uint8_t)(addr & 0xFF));
    fpga_spi_write_byte((uint8_t)(data >> 8));
    fpga_spi_write_byte((uint8_t)(data & 0xFF));

    fpga_spi_cs_high();
}

/* ---- DMA frame buffer read ---- */

int fpga_read_frame_dma(uint16_t buf_sel, uint8_t *dest, uint32_t max_bytes,
                        uint32_t *actual_bytes)
{
    if (dest == 0 || max_bytes == 0)
        return -1;

    /* Select the buffer to read */
    fpga_write_reg(FPGA_REG_ACTIVE_BUF, buf_sel);

    /* Set up DMA from SPI3 RX to dest buffer */
    /* For simplicity, use polling transfer for now. A production version
       would use DMA1 Stream 0 with SPI3 RX. */
    fpga_spi_cs_low();

    /* Command: 0x03 = read frame buffer */
    fpga_spi_write_byte(0x03);
    fpga_spi_write_byte((uint8_t)(buf_sel & 0xFF));

    uint32_t total = 0;
    while (total < max_bytes) {
        dest[total] = fpga_spi_read_byte();
        total++;
        /* Check if FPGA signals end of frame (would be via IRQ in real HW) */
        /* For this implementation, read up to max_bytes */
        if (total >= max_bytes)
            break;
    }

    fpga_spi_cs_high();

    if (actual_bytes)
        *actual_bytes = total;

    return 0;
}

/* ---- Status helpers ---- */

uint16_t fpga_get_status(void)
{
    return fpga_read_reg(FPGA_REG_STATUS);
}

uint8_t fpga_csi_link_up(void)
{
    return (fpga_get_status() & FPGA_STATUS_CSI_LINK) ? 1 : 0;
}

uint8_t fpga_dsi_link_up(void)
{
    return (fpga_get_status() & FPGA_STATUS_DSI_LINK) ? 1 : 0;
}

uint32_t fpga_get_frame_index(void)
{
    uint16_t hi = fpga_read_reg(FPGA_REG_FRAME_IDX);
    /* Frame index is 16-bit in this register; for 32-bit counter,
       read a second register. We approximate with 16-bit. */
    return (uint32_t)hi;
}

/* ---- Control helpers ---- */

void fpga_enable_tap(uint8_t enable)
{
    uint16_t ctrl = fpga_read_reg(FPGA_REG_CONTROL);
    if (enable)
        ctrl |= FPGA_CTRL_TAP_ENABLE;
    else
        ctrl &= ~FPGA_CTRL_TAP_ENABLE;
    fpga_write_reg(FPGA_REG_CONTROL, ctrl);
}

void fpga_enable_capture(uint8_t enable)
{
    uint16_t ctrl = fpga_read_reg(FPGA_REG_CONTROL);
    if (enable)
        ctrl |= FPGA_CTRL_CAPTURE_ENABLE;
    else
        ctrl &= ~FPGA_CTRL_CAPTURE_ENABLE;
    fpga_write_reg(FPGA_REG_CONTROL, ctrl);
}

void fpga_enable_inject(uint8_t enable)
{
    uint16_t ctrl = fpga_read_reg(FPGA_REG_CONTROL);
    if (enable)
        ctrl |= FPGA_CTRL_INJECT_ENABLE;
    else
        ctrl &= ~FPGA_CTRL_INJECT_ENABLE;
    fpga_write_reg(FPGA_REG_CONTROL, ctrl);
}

void fpga_reset(void)
{
    uint16_t ctrl = fpga_read_reg(FPGA_REG_CONTROL);
    ctrl |= FPGA_CTRL_RESET;
    fpga_write_reg(FPGA_REG_CONTROL, ctrl);
    /* Wait for reset to complete */
    for (volatile int i = 0; i < 10000; i++)
        ;
    ctrl &= ~FPGA_CTRL_RESET;
    fpga_write_reg(FPGA_REG_CONTROL, ctrl);
}

/* ---- Injection frame upload ---- */

int fpga_upload_inject_frame(const uint8_t *frame_data, uint32_t length,
                              uint16_t width, uint16_t height, uint8_t format)
{
    if (frame_data == 0 || length == 0)
        return -1;

    /* Set injection frame parameters */
    fpga_write_reg(FPGA_REG_INJECT_W, width);
    fpga_write_reg(FPGA_REG_INJECT_H, height);
    /* Format packed into high byte of INJECT_MODE for now */
    fpga_write_reg(FPGA_REG_INJECT_MODE, (uint16_t)(format << 8));

    /* Upload frame data to FPGA RAM starting at INJECT_ADDR */
    fpga_spi_cs_low();

    /* Command: 0x04 = upload inject frame */
    fpga_spi_write_byte(0x04);
    fpga_spi_write_byte((uint8_t)(length >> 16));
    fpga_spi_write_byte((uint8_t)(length >> 8));
    fpga_spi_write_byte((uint8_t)(length & 0xFF));

    /* Send frame data */
    fpga_spi_write_buf(frame_data, length);

    fpga_spi_cs_high();

    return 0;
}

/* ---- Timing control ---- */

void fpga_set_timing_delay(int32_t delay_ns)
{
    /* Convert ns to FPGA clock cycles (FPGA runs at ~100 MHz = 10 ns/cycle) */
    int16_t cycles = (int16_t)(delay_ns / 10);
    fpga_write_reg(FPGA_REG_TIMING_DELAY, (uint16_t)cycles);
}

void fpga_set_timing_jitter(uint8_t jitter_pct)
{
    fpga_write_reg(FPGA_REG_TIMING_JITTER, (uint16_t)jitter_pct);
}

void fpga_set_timing_drop_pattern(uint8_t pattern)
{
    fpga_write_reg(FPGA_REG_TIMING_DROP, (uint16_t)pattern);
}

void fpga_enable_timing(uint8_t enable)
{
    uint16_t ctrl = fpga_read_reg(FPGA_REG_CONTROL);
    if (enable)
        ctrl |= FPGA_CTRL_TIMING_ENABLE;
    else
        ctrl &= ~FPGA_CTRL_TIMING_ENABLE;
    fpga_write_reg(FPGA_REG_CONTROL, ctrl);
}

/* ---- Error statistics ---- */

void fpga_get_error_stats(fpga_error_stats_t *stats)
{
    if (!stats)
        return;
    uint16_t err_reg = fpga_read_reg(FPGA_REG_CSI_ERR);
    stats->crc_errors = (err_reg >> 8) & 0xFF;
    stats->overflows = err_reg & 0xFF;
    err_reg = fpga_read_reg(FPGA_REG_DSI_ERR);
    stats->short_packets = (err_reg >> 8) & 0xFF;
    stats->long_packets = err_reg & 0xFF;
}

/* ---- Init ---- */

int fpga_init(void)
{
    int ret = fpga_spi_init();
    if (ret)
        return ret;

    /* Reset FPGA state */
    fpga_reset();

    /* Read version to confirm communication */
    uint16_t ver = fpga_read_reg(FPGA_REG_VERSION);
    if (ver == 0xFFFF || ver == 0x0000)
        return -2;  /* SPI not communicating */

    return 0;
}

int fpga_is_ready(void)
{
    return GPIO_READ(FPGA_CDONE_PORT, FPGA_CDONE_PIN) ? 1 : 0;
}

int fpga_load_from_nor(void)
{
    /* In a real implementation, read bitstream from W25Q256 SPI NOR flash
       at a fixed offset (e.g., 0x000000) and load into FPGA.
       The bitstream length is stored in the first 4 bytes of NOR.
       For now, we provide the interface; the NOR driver loads the data. */
    /* TODO: call nor_read(0, header, 4) to get length, then nor_read(4, buf, len) */
    /* For this firmware, we assume bitstream is pre-loaded in NOR. */
    return 0;  /* placeholder — see nor_flash driver */
}

/* ---- End of fpga_if.c ----
 * Author: jayis1
 */