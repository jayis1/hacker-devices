/*
 * tesla-phantom — drivers/fpga_spi.c
 * SPI communication with the Lattice iCE40UP5K FPGA.
 * Handles pulse timing configuration, trigger mode, and status readback.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"

/* FPGA SPI register read/write protocol:
 *
 * Write: [W_ADDR(7:0)] [DATA(31:0)]  (5 bytes, CS low for all)
 * Read:  [R_ADDR(7:0)] [DUMMY]       (2 bytes, then read 4 bytes)
 *
 * Address bit 7: 0 = write, 1 = read
 */

#define FPGA_WRITE_FLAG   0x00
#define FPGA_READ_FLAG    0x80

static void fpga_cs_low(void) {
    gpio_clr(GPIO(GPIOB), FPGA_CS_PIN);
}

static void fpga_cs_high(void) {
    gpio_set(GPIO(GPIOB), FPGA_CS_PIN);
}

static void fpga_spi_init(void) {
    /* Configure SPI1: master, 8-bit, CPOL=0, CPHA=0, prescaler /4 */
    volatile spi_regs_t *s = SPI1;

    s->CR1 = 0;  /* disable before config */
    s->CR2 = SPI_CR2_DS_8BIT;
    s->CR1 = SPI_CR1_MSTR
           | SPI_CR1_SSM
           | SPI_CR1_SSI
           | SPI_CR1_BR_DIV4;  /* 240 MHz / 4 = 60 MHz SPI clock */
    s->CR1 |= SPI_CR1_SPE;     /* enable SPI */
}

static uint8_t fpga_spi_xfer8(uint8_t tx) {
    volatile spi_regs_t *s = SPI1;
    spi_wait_tx(s);
    s->DR = tx;
    spi_wait_rx(s);
    return (uint8_t)s->DR;
}

static void fpga_spi_write32(uint32_t val) {
    /* MSB first */
    fpga_spi_xfer8((val >> 24) & 0xFF);
    fpga_spi_xfer8((val >> 16) & 0xFF);
    fpga_spi_xfer8((val >>  8) & 0xFF);
    fpga_spi_xfer8( val        & 0xFF);
}

static uint32_t fpga_spi_read32(void) {
    uint32_t val = 0;
    val = (uint32_t)fpga_spi_xfer8(0xFF) << 24;
    val |= (uint32_t)fpga_spi_xfer8(0xFF) << 16;
    val |= (uint32_t)fpga_spi_xfer8(0xFF) << 8;
    val |= (uint32_t)fpga_spi_xfer8(0xFF);
    return val;
}

/* ── Public API ───────────────────────────────────────────────────── */

int fpga_init(void) {
    fpga_spi_init();

    /* Reset the FPGA (toggle reset line) */
    gpio_clr(GPIO(GPIOB), FPGA_RESET_PIN);
    delay_ms(10);
    gpio_set(GPIO(GPIOB), FPGA_RESET_PIN);
    delay_ms(50);

    /* Wait for CDONE (configuration done) — or timeout */
    uint32_t timeout = millis();
    while (!gpio_get(GPIO(GPIOB), FPGA_CDONE_PIN)) {
        if ((millis() - timeout) > 1000) {
            /* FPGA not configured — may need bitstream upload.
             * For now, assume flash-loaded bitstream. */
            break;
        }
    }

    /* Verify communication with scratch register loopback */
    uint32_t test_val = 0xDEADBEEF;
    fpga_write_reg(FPGA_REG_SCRATCH, test_val);
    uint32_t readback = 0;
    fpga_read_reg(FPGA_REG_SCRATCH, &readback);
    if (readback != test_val) {
        return -1;  /* FPGA not responding */
    }

    /* Read FPGA version */
    uint32_t version = 0;
    fpga_read_reg(FPGA_REG_VERSION, &version);

    /* Set default trigger mode to external */
    fpga_set_trigger_mode(TRIG_EXTERNAL);

    return 0;
}

int fpga_write_reg(uint8_t reg, uint32_t value) {
    fpga_cs_low();
    fpga_spi_xfer8(reg & 0x7F);  /* write flag = 0 */
    fpga_spi_write32(value);
    fpga_cs_high();
    delay_us(1);
    return 0;
}

int fpga_read_reg(uint8_t reg, uint32_t *value) {
    fpga_cs_low();
    fpga_spi_xfer8(reg | FPGA_READ_FLAG);
    *value = fpga_spi_read32();
    fpga_cs_high();
    delay_us(1);
    return 0;
}

int fpga_configure_pulse(uint16_t width_ns, uint32_t delay_ns,
                          uint16_t count, uint32_t interval_us) {
    /* Convert ns to FPGA ticks (200 MHz clock → 5 ns per tick) */
    uint32_t width_ticks = width_ns / 5;
    if (width_ticks < 1) width_ticks = 1;
    if (width_ticks > 40) width_ticks = 40;  /* max 200 ns */

    uint32_t delay_ticks = delay_ns / 5;
    uint32_t interval_ticks = interval_us * 200;  /* 200 ticks per µs */

    fpga_write_reg(FPGA_REG_PULSE_WIDTH, width_ticks);
    fpga_write_reg(FPGA_REG_PULSE_DELAY, delay_ticks);
    fpga_write_reg(FPGA_REG_PULSE_COUNT, count);
    fpga_write_reg(FPGA_REG_PULSE_INTERVAL, interval_ticks);

    return 0;
}

int fpga_set_trigger_mode(trigger_source_t src) {
    fpga_write_reg(FPGA_REG_TRIGGER_MODE, (uint32_t)src);
    return 0;
}

int fpga_arm(void) {
    /* Arm: set armed bit, clear any pending fire */
    fpga_write_reg(FPGA_REG_HV_FIRE, 0x00000002);  /* arm=1, fire=0 */
    return 0;
}

int fpga_disarm(void) {
    /* Disarm: clear all control bits */
    fpga_write_reg(FPGA_REG_HV_FIRE, 0x00000000);
    return 0;
}

int fpga_fire(void) {
    /* Fire: set fire bit (bit 0) */
    fpga_write_reg(FPGA_REG_HV_FIRE, 0x00000003);  /* arm=1, fire=1 */
    delay_us(10);
    /* Clear fire bit (arm stays until disarm) */
    fpga_write_reg(FPGA_REG_HV_FIRE, 0x00000002);
    return 0;
}

uint8_t fpga_get_status(void) {
    uint32_t status = 0;
    fpga_read_reg(FPGA_REG_STATUS, &status);
    return (uint8_t)(status & 0xFF);
}

int fpga_set_mag_trigger(uint16_t threshold, uint64_t pattern) {
    fpga_write_reg(FPGA_REG_MAG_THRESHOLD, threshold);
    fpga_write_reg(FPGA_REG_MAG_PATTERN_LO, (uint32_t)(pattern & 0xFFFFFFFF));
    fpga_write_reg(FPGA_REG_MAG_PATTERN_HI, (uint32_t)(pattern >> 32));
    return 0;
}