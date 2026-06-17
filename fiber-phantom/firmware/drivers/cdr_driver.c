/*
 * cdr_driver.c — FPGA SPI interface for Clock/Data Recovery and frame access
 * Author: jayis1
 * Date:   2026-06-17
 *
 * Implements the SPI communication between the RP2040 and the iCE40 UP5K FPGA.
 * The FPGA manages the optical data path: CDR, framing, MITM rule evaluation,
 * and frame FIFO buffering. This driver provides the control interface.
 */

#include "cdr_driver.h"
#include "registers.h"
#include "board.h"

/* ---- SPI access helpers ---- */

#define FPGA_CS_LOW()    (SIO_GPIO_OUT_CLR = GPIO_BIT(PIN_FPGA_CS_N))
#define FPGA_CS_HIGH()   (SIO_GPIO_OUT_SET = GPIO_BIT(PIN_FPGA_CS_N))

static void delay_us(uint32_t us)
{
    volatile uint32_t count = us * 44;
    while (count--) __asm__ volatile ("nop");
}

static void delay_ms(uint32_t ms)
{
    while (ms--) delay_us(1000);
}

static void gpio_init_simple(uint8_t pin, uint8_t output, uint8_t func)
{
    IO_BANK0_GPIO_CTRL(pin) = func;
    uint32_t pad = PAD_IE | PAD_DRIVE_4MA;
    if (!output) pad |= PAD_OD;
    PADS_BANK0_GPIO(pin) = pad;
    if (output)
        SIO_GPIO_OE_SET = GPIO_BIT(pin);
    else
        SIO_GPIO_OE_CLR = GPIO_BIT(pin);
}

/* ---- SPI0 low-level transfer ---- */

static void spi0_init_hw(void)
{
    /* Configure SPI0 pins: SCK=GPIO2, MOSI=GPIO3, MISO=GPIO4, CS=GPIO5 */
    IO_BANK0_GPIO_CTRL(PIN_FPGA_SCK)  = GPIO_FUNC_SPI;
    IO_BANK0_GPIO_CTRL(PIN_FPGA_MOSI) = GPIO_FUNC_SPI;
    IO_BANK0_GPIO_CTRL(PIN_FPGA_MISO) = GPIO_FUNC_SPI;

    /* CS as GPIO (manual control) */
    gpio_init_simple(PIN_FPGA_CS_N, 1, GPIO_FUNC_SIO);
    FPGA_CS_HIGH();

    /* CDONE as input with pull-up */
    gpio_init_simple(PIN_FPGA_CDONE, 0, GPIO_FUNC_SIO);
    PADS_BANK0_GPIO(PIN_FPGA_CDONE) |= PAD_PUE;

    /* CRESET_N as output (default high) */
    gpio_init_simple(PIN_FPGA_CRESET_N, 1, GPIO_FUNC_SIO);
    SIO_GPIO_OUT_SET = GPIO_BIT(PIN_FPGA_CRESET_N);

    /* IRQ pin as input */
    gpio_init_simple(PIN_FPGA_IRQ, 0, GPIO_FUNC_SIO);

    /* Configure pads for SPI */
    PADS_BANK0_GPIO(PIN_FPGA_SCK)  = PAD_IE | PAD_DRIVE_8MA | PAD_SLEWFAST;
    PADS_BANK0_GPIO(PIN_FPGA_MOSI) = PAD_IE | PAD_DRIVE_8MA | PAD_SLEWFAST;
    PADS_BANK0_GPIO(PIN_FPGA_MISO) = PAD_IE | PAD_DRIVE_4MA;

    /* Reset SPI0 peripheral */
    PSM_FRCE_OFF |= PSM_SPI0;
    delay_us(10);
    PSM_FRCE_OFF &= ~PSM_SPI0;
    delay_us(100);

    /* Configure SPI0: Master mode, 8-bit, Motorola frame format
     * CPOL=0, CPHA=0 (Mode 0)
     * Clock: PERI_CLK / CPSR / (1 + SCR)
     * PERI_CLK = 125 MHz, CPSR = 2 → 62.5 MHz base
     * For 48 MHz: CPSR=2, SCR=0 → 62.5MHz (close enough for FPGA)
     * For slower debug: CPSR=25, SCR=0 → 2.5 MHz
     */
    SPI0->cpsr = 2;   /* Prescaler divisor */
    SPI0->cr0 = SPI_CR0_DSS_8BIT | SPI_CR0_FRF_MOTOROLA | (0 << SPI_CR0_SCR_SHIFT);
    SPI0->cr1 = 0x10;  /* Master mode (bit 2 set), no slave select */
    /* Actually, CR1 bit 1 = SOD (slave output disable), bit 2 = MS (master/slave)
     * For master: MS=0 (default), enable via CR1 bit 1 (SSE) */
    SPI0->cr1 = 0x00;  /* Master, 8-bit */
    /* Enable SPI */
    SPI0->cr1 |= (1 << 1);  /* SSE = bit 1 (synchronous serial enable) */
}

static uint8_t spi0_transfer(uint8_t tx_byte)
{
    /* Wait for TX FIFO not full */
    while (!(SPI0->sr & SPI_SR_TNF)) { }

    /* Write data */
    SPI0->dr = tx_byte;

    /* Wait for RX FIFO not empty (RXNE) */
    while (!(SPI0->sr & SPI_SR_RNE)) { }

    /* Read received data */
    return (uint8_t)(SPI0->dr & 0xFF);
}

static void spi0_transfer_buf(const uint8_t *tx, uint8_t *rx, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        uint8_t tx_byte = tx ? tx[i] : 0xFF;
        uint8_t rx_byte = spi0_transfer(tx_byte);
        if (rx) rx[i] = rx_byte;
    }
}

/* ============================================================
 * Public API
 * ============================================================ */

void cdr_spi_init(void)
{
    spi0_init_hw();
}

int fpga_configure(void)
{
    /* iCE40 configuration sequence:
     * 1. Assert CRESET_N low for >200ns
     * 2. Release CRESET_N (goes high via internal pull-up)
     * 3. Wait for CDONE to go high (FPGA reads bitstream from flash)
     * 4. Send dummy clocks to complete configuration
     *
     * The FPGA configures from the W25Q128 SPI flash on its own SPI pins.
     * The RP2040's SPI0 connects to FPGA user SPI pins (post-configuration).
     */

    /* Pulse CRESET_N low to force re-configuration */
    SIO_GPIO_OUT_CLR = GPIO_BIT(PIN_FPGA_CRESET_N);
    delay_us(1);  /* Hold for at least 200ns */
    SIO_GPIO_OUT_SET = GPIO_BIT(PIN_FPGA_CRESET_N);

    /* Wait for CDONE (timeout: ~2 seconds for UP5K from flash) */
    uint32_t timeout = 2000;  /* ms */
    while (!(SIO_GPIO_IN & GPIO_BIT(PIN_FPGA_CDONE))) {
        delay_ms(1);
        if (--timeout == 0) {
            return -1;  /* Configuration timeout */
        }
    }

    /* Send at least 49 dummy clocks to complete configuration */
    FPGA_CS_LOW();
    for (int i = 0; i < 8; i++) {
        spi0_transfer(0x00);
    }
    FPGA_CS_HIGH();

    return 0;  /* Success */
}

uint8_t fpga_is_ready(void)
{
    return (SIO_GPIO_IN & GPIO_BIT(PIN_FPGA_CDONE)) ? 1 : 0;
}

uint32_t fpga_reg_read(uint8_t reg)
{
    /* Protocol: [W: 0x80 | reg] [R: dummy] [R: byte0] [R: byte1] [R: byte2] [R: byte3]
     * Bit 7 of first byte = 1 for read, 0 for write
     */
    FPGA_CS_LOW();
    spi0_transfer(0x80 | (reg & 0x7F));
    (void)spi0_transfer(0x00);  /* Dummy read for response latency */
    uint32_t value = 0;
    value |= ((uint32_t)spi0_transfer(0x00)) << 24;
    value |= ((uint32_t)spi0_transfer(0x00)) << 16;
    value |= ((uint32_t)spi0_transfer(0x00)) << 8;
    value |= ((uint32_t)spi0_transfer(0x00)) << 0;
    FPGA_CS_HIGH();
    return value;
}

void fpga_reg_write(uint8_t reg, uint32_t value)
{
    /* Protocol: [W: 0x00 | reg] [W: byte0] [W: byte1] [W: byte2] [W: byte3] */
    FPGA_CS_LOW();
    spi0_transfer(reg & 0x7F);  /* Bit 7 = 0 for write */
    spi0_transfer((value >> 24) & 0xFF);
    spi0_transfer((value >> 16) & 0xFF);
    spi0_transfer((value >> 8) & 0xFF);
    spi0_transfer((value >> 0) & 0xFF);
    FPGA_CS_HIGH();
}

uint32_t fpga_read_frame(uint8_t *buf, uint32_t max_len,
                         uint32_t *timestamp_lo, uint32_t *timestamp_hi,
                         uint16_t *frame_len)
{
    uint32_t status = fpga_reg_read(FPGA_REG_STATUS);

    if (!(status & FPGA_STATUS_FRAME_READY)) {
        return 0;  /* No frame available */
    }

    /* Read timestamp and frame length */
    uint32_t ts_lo = fpga_reg_read(FPGA_REG_TIMESTAMP_LO);
    uint32_t ts_hi = fpga_reg_read(FPGA_REG_TIMESTAMP_HI);
    uint32_t len_reg = fpga_reg_read(FPGA_REG_FRAME_LEN);

    uint16_t len = (uint16_t)(len_reg & 0xFFFF);
    if (len == 0 || len > max_len) {
        len = (len > max_len) ? (uint16_t)max_len : 0;
    }

    if (timestamp_lo) *timestamp_lo = ts_lo;
    if (timestamp_hi) *timestamp_hi = ts_hi;
    if (frame_len) *frame_len = (uint16_t)len_reg;

    /* Read frame data from FPGA data FIFO (burst read) */
    FPGA_CS_LOW();
    spi0_transfer(FPGA_REG_DATA_FIFO & 0x7F);  /* Write to data FIFO register addr */

    /* Burst read frame bytes (send dummy 0x00, receive data) */
    for (uint16_t i = 0; i < len; i++) {
        buf[i] = spi0_transfer(0x00);
    }
    FPGA_CS_HIGH();

    return len;
}

int fpga_write_rule(uint8_t index, const mitm_rule_t *rule)
{
    if (index >= MITM_RULE_MAX) {
        return -1;
    }

    /* Write rule to FPGA rule RAM.
     * Each rule occupies 108 bytes (27 x 32-bit words) in FPGA rule RAM.
     * Rule RAM base address: FPGA_REG_RULE_BASE + index * 27
     */
    uint8_t base = FPGA_REG_RULE_BASE + (index * 27);

    /* Pack mitm_rule_t into 32-bit words for SPI transfer */
    const uint8_t *rule_bytes = (const uint8_t *)rule;

    for (int word_idx = 0; word_idx < 27; word_idx++) {
        uint32_t value = 0;
        /* Copy 4 bytes at a time (handle last word which may be < 4 bytes) */
        int byte_offset = word_idx * 4;
        for (int b = 0; b < 4 && (byte_offset + b) < sizeof(mitm_rule_t); b++) {
            value |= ((uint32_t)rule_bytes[byte_offset + b]) << (b * 8);
        }
        fpga_reg_write(base + word_idx, value);
    }

    return 0;
}

int fpga_clear_rule(uint8_t index)
{
    if (index >= MITM_RULE_MAX) {
        return -1;
    }
    /* Write a zeroed rule (disabled) */
    mitm_rule_t zero_rule = {0};
    return fpga_write_rule(index, &zero_rule);
}

void fpga_clear_all_rules(void)
{
    for (int i = 0; i < MITM_RULE_MAX; i++) {
        fpga_clear_rule((uint8_t)i);
    }
}

void fpga_mitm_enable(uint8_t enable)
{
    uint32_t ctrl = fpga_reg_read(FPGA_REG_CTRL);
    if (enable) {
        ctrl |= FPGA_CTRL_MITM_EN;
    } else {
        ctrl &= ~FPGA_CTRL_MITM_EN;
    }
    fpga_reg_write(FPGA_REG_CTRL, ctrl);
}

void fpga_inject_enable(uint8_t enable)
{
    uint32_t ctrl = fpga_reg_read(FPGA_REG_CTRL);
    if (enable) {
        ctrl |= FPGA_CTRL_INJECT_EN;
    } else {
        ctrl &= ~FPGA_CTRL_INJECT_EN;
    }
    fpga_reg_write(FPGA_REG_CTRL, ctrl);
}

void fpga_capture_enable(uint8_t enable)
{
    uint32_t ctrl = fpga_reg_read(FPGA_REG_CTRL);
    if (enable) {
        ctrl |= FPGA_CTRL_CAP_EN;
    } else {
        ctrl &= ~FPGA_CTRL_CAP_EN;
    }
    fpga_reg_write(FPGA_REG_CTRL, ctrl);
}

void fpga_reset_stats(void)
{
    uint32_t ctrl = fpga_reg_read(FPGA_REG_CTRL);
    fpga_reg_write(FPGA_REG_CTRL, ctrl | FPGA_CTRL_RESET_STATS);
    delay_us(10);
    fpga_reg_write(FPGA_REG_CTRL, ctrl & ~FPGA_CTRL_RESET_STATS);
}

uint32_t fpga_get_status(void)
{
    return fpga_reg_read(FPGA_REG_STATUS);
}

link_rate_t fpga_get_link_rate(void)
{
    uint32_t rate = fpga_reg_read(FPGA_REG_LINK_RATE) & 0xFF;
    switch (rate) {
        case 1:  return LINK_1G;
        case 2:  return LINK_10G;
        case 3:  return LINK_FC_8G;
        case 4:  return LINK_FC_16G;
        default: return LINK_DOWN;
    }
}

void fpga_get_stats(uint32_t *frame_cnt_lo, uint32_t *frame_cnt_hi,
                    uint32_t *drop_cnt, uint32_t *mitm_modified,
                    uint32_t *mitm_dropped, uint32_t *crc_errors)
{
    if (frame_cnt_lo) *frame_cnt_lo = fpga_reg_read(FPGA_REG_FRAME_CNT_LO);
    if (frame_cnt_hi) *frame_cnt_hi = fpga_reg_read(FPGA_REG_FRAME_CNT_HI);
    if (drop_cnt)     *drop_cnt     = fpga_reg_read(FPGA_REG_DROP_CNT);

    /* Extended stats at higher register addresses */
    if (mitm_modified) *mitm_modified = fpga_reg_read(0x08);
    if (mitm_dropped)   *mitm_dropped   = fpga_reg_read(0x09);
    if (crc_errors)     *crc_errors     = fpga_reg_read(0x0A);
}