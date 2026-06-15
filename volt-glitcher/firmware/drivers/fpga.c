/*
 * drivers/fpga.c — Lattice iCE40HX1K-TQ144 FPGA driver
 * SPI configuration, register access, waveform upload, PLL reconfig
 *
 * Copyright (c) 2026 Hacker Devices
 * SPDX-License-Identifier: MIT
 */

#include "fpga.h"
#include "../board.h"
#include "../registers.h"

/* ========================================================================
 * SPI Transfer Helpers
 * ======================================================================== */

static void spi_cs_assert(void) {
    GPIOA->ODR &= ~(1UL << 4);  /* PA4 low = CS asserted */
    __asm__ volatile ("nop; nop; nop; nop;");  /* 4-cycle CS setup */
}

static void spi_cs_deassert(void) {
    __asm__ volatile ("nop; nop; nop; nop;");  /* CS hold time */
    GPIOA->ODR |= (1UL << 4);   /* PA4 high = CS deasserted */
}

static uint8_t spi_xfer_byte(uint8_t tx) {
    /* Wait for TX empty */
    while (!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = (uint16_t)tx;

    /* Wait for RX not empty */
    while (!(SPI1->SR & SPI_SR_RXNE));
    return (uint8_t)(SPI1->DR & 0xFF);
}

static void spi_xfer_buf(const uint8_t *tx_buf, uint8_t *rx_buf, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        uint8_t tx = tx_buf ? tx_buf[i] : 0xFF;
        uint8_t rx = spi_xfer_byte(tx);
        if (rx_buf) rx_buf[i] = rx;
    }
}

/* ========================================================================
 * Register Access
 * ======================================================================== */

uint16_t fpga_read_reg(uint8_t addr) {
    uint8_t cmd = FPGA_SPI_CMD_READ | ((addr << FPGA_SPI_CMD_ADDR_SHIFT) & 0x7E);
    uint8_t tx[4] = { cmd, 0x00, 0x00, 0x00 };
    uint8_t rx[4] = { 0 };

    spi_cs_assert();
    spi_xfer_buf(tx, rx, 4);
    spi_cs_deassert();

    return (uint16_t)((rx[2] << 8) | rx[3]);
}

void fpga_write_reg(uint8_t addr, uint16_t value) {
    uint8_t cmd = FPGA_SPI_CMD_WRITE | ((addr << FPGA_SPI_CMD_ADDR_SHIFT) & 0x7E);
    uint8_t tx[4] = { cmd, (uint8_t)(value >> 8), (uint8_t)(value & 0xFF), 0x00 };

    spi_cs_assert();
    spi_xfer_buf(tx, NULL, 3);
    spi_cs_deassert();

    /* Small delay for register write to propagate inside FPGA */
    __asm__ volatile ("nop; nop; nop; nop; nop; nop;");
}

void fpga_read_reg_burst(uint8_t start_addr, uint16_t *buf, uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        buf[i] = fpga_read_reg(start_addr + i);
    }
}

void fpga_write_reg_burst(uint8_t start_addr, const uint16_t *buf, uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        fpga_write_reg(start_addr + i, buf[i]);
    }
}

/* ========================================================================
 * FPGA Configuration from Flash
 * ======================================================================== */

/* External flash read command (W25Q128: 0x03 = read, 24-bit addr) */
static void flash_read_begin(uint32_t addr) {
    /* Select flash on SPI2 (PB12) */
    GPIOB->ODR &= ~(1UL << 12);
    spi_xfer_byte(0x03);                /* READ command */
    spi_xfer_byte((addr >> 16) & 0xFF); /* Address high */
    spi_xfer_byte((addr >> 8) & 0xFF);  /* Address mid */
    spi_xfer_byte(addr & 0xFF);         /* Address low */
}

static void flash_read_end(void) {
    GPIOB->ODR |= (1UL << 12);  /* Deselect flash */
}

static uint8_t flash_read_byte(void) {
    return spi_xfer_byte(0xFF);  /* Clock out dummy, read data */
}

int fpga_configure(void) {
    /* Step 1: Assert CRESET_B to put FPGA in config mode */
    GPIOB->ODR &= ~(1UL << 6);  /* PB6 low = CRESET_B asserted */
    delay_us(10);               /* Hold reset for 10 µs (spec: >200ns) */

    /* Step 2: While CRESET_B is low, set SS high (deselected for config) */
    GPIOA->ODR |= (1UL << 4);   /* SPI1 CS high */

    /* Step 3: Release CRESET_B */
    GPIOB->ODR |= (1UL << 6);   /* PB6 high = CRESET_B released */
    delay_us(5);                 /* Wait for FPGA to be ready */

    /* Step 4: Send dummy clocks before bitstream
     * iCE40 requires 8+ dummy SCLK cycles with SS high */
    for (int i = 0; i < FPGA_CFG_DUMMY_PREFIX; i++) {
        spi_xfer_byte(0xFF);
    }

    /* Step 5: Assert SS and stream bitstream from flash
     * The FPGA bitstream is stored in W25Q128 flash at offset 0.
     * We read it byte-by-byte and clock it into the FPGA via SPI1. */
    flash_read_begin(FPGA_BIT_FLASH_ADDR);

    spi_cs_assert();  /* Assert FPGA SS */

    uint32_t bytes_sent = 0;
    uint32_t start_tick = g_tick_ms;  /* Use global tick for timeout */

    while (bytes_sent < FPGA_BIT_MAX_SIZE) {
        uint8_t byte = flash_read_byte();
        spi_xfer_byte(byte);
        bytes_sent++;

        /* Check CDONE after every 1KB — it goes high on successful config */
        if ((bytes_sent % 1024) == 0) {
            if (GPIOB->IDR & (1UL << 7)) {
                /* CDONE is high — configuration complete! */
                break;
            }
        }

        /* Timeout check */
        if ((g_tick_ms - start_tick) > FPGA_CFG_TIMEOUT_MS) {
            flash_read_end();
            spi_cs_deassert();
            return -1;  /* Timeout */
        }
    }

    flash_read_end();

    /* Step 6: Send dummy clocks after bitstream
     * iCE40 requires 49+ additional clock cycles after CDONE goes high
     * for the FPGA to fully initialize its I/O buffers */
    for (int i = 0; i < FPGA_CFG_DUMMY_SUFFIX; i++) {
        spi_xfer_byte(0xFF);
    }

    spi_cs_deassert();

    /* Step 7: Verify CDONE is high */
    if (!(GPIOB->IDR & (1UL << 7))) {
        return -2;  /* Configuration failed */
    }

    /* Step 8: Wait for I/O initialization (spec: 1.5ms typical) */
    delay_ms(2);

    /* Step 9: Verify FPGA responds to register read */
    uint16_t ver = fpga_read_reg(FPGA_REG_VERSION);
    if (ver == 0xFFFF || ver == 0x0000) {
        return -3;  /* FPGA not responding */
    }

    return 0;  /* Success */
}

int fpga_configure_from_buf(const uint8_t *bitstream, uint32_t length) {
    if (!bitstream || length == 0 || length > FPGA_BIT_MAX_SIZE) {
        return -1;
    }

    /* Assert CRESET_B */
    GPIOB->ODR &= ~(1UL << 6);
    delay_us(10);
    GPIOA->ODR |= (1UL << 4);

    /* Release CRESET_B */
    GPIOB->ODR |= (1UL << 6);
    delay_us(5);

    /* Dummy prefix clocks */
    for (int i = 0; i < FPGA_CFG_DUMMY_PREFIX; i++) {
        spi_xfer_byte(0xFF);
    }

    /* Stream bitstream from buffer */
    spi_cs_assert();

    for (uint32_t i = 0; i < length; i++) {
        spi_xfer_byte(bitstream[i]);

        /* Early exit if CDONE goes high */
        if ((i % 256) == 0 && (GPIOB->IDR & (1UL << 7))) {
            break;
        }
    }

    /* Dummy suffix clocks */
    for (int i = 0; i < FPGA_CFG_DUMMY_SUFFIX; i++) {
        spi_xfer_byte(0xFF);
    }

    spi_cs_deassert();

    /* Verify CDONE */
    if (!(GPIOB->IDR & (1UL << 7))) {
        return -2;
    }

    delay_ms(2);
    return 0;
}

/* ========================================================================
 * FPGA Lifecycle
 * ======================================================================== */

void fpga_reset(void) {
    GPIOB->ODR &= ~(1UL << 6);  /* CRESET_B low */
    delay_us(10);
    GPIOB->ODR |= (1UL << 6);   /* CRESET_B high */
    delay_ms(5);                 /* Wait for internal init */
}

int fpga_is_configured(void) {
    return (GPIOB->IDR & (1UL << 7)) ? 1 : 0;
}

void fpga_sleep(void) {
    GPIOC->ODR |= (1UL << 3);   /* PC3 high = FPGA_SUSPEND (sleep mode) */
    fpga_write_reg(FPGA_REG_CTRL, 0);  /* Disable all FPGA logic */
}

void fpga_wake(void) {
    GPIOC->ODR |= (1UL << 2);   /* PC2 high = FPGA_WAKE pulse */
    delay_us(10);
    GPIOC->ODR &= ~(1UL << 2);  /* Wake pulse done */
    GPIOC->ODR &= ~(1UL << 3);  /* Release suspend */
    delay_ms(5);                 /* Wait for FPGA to stabilize */
}

void fpga_suspend(void) {
    /* Freeze PLL and gate all internal clocks */
    fpga_write_reg(FPGA_REG_CTRL, FPGA_CTRL_RESET);
    GPIOC->ODR |= (1UL << 3);   /* Suspend pin high */
}

void fpga_resume(void) {
    GPIOC->ODR &= ~(1UL << 3);  /* Release suspend */
    fpga_write_reg(FPGA_REG_CTRL, FPGA_CTRL_ENABLE);
    delay_ms(10);                /* Wait for PLL re-lock */

    /* Verify PLL lock */
    uint16_t status = fpga_read_reg(FPGA_REG_STATUS);
    if (!(status & FPGA_STATUS_PLL_LOCKED)) {
        /* PLL didn't re-lock — try again */
        fpga_write_reg(FPGA_REG_CTRL, FPGA_CTRL_PLL_RECONFIG);
        delay_ms(5);
    }
}

/* ========================================================================
 * Trigger Pattern Interface
 * ======================================================================== */

void fpga_feed_uart_trigger(uint8_t byte) {
    /* Write byte to FPGA UART trigger pattern matcher
     * The FPGA shifts this into its internal comparator and
     * checks against the configured pattern/mask */
    fpga_write_reg(FPGA_REG_UART_PATTERN0, (uint16_t)byte);
}

void fpga_set_uart_pattern(const uint8_t pattern[4], uint8_t mask) {
    fpga_write_reg(FPGA_REG_UART_PATTERN0, (uint16_t)pattern[0]);
    fpga_write_reg(FPGA_REG_UART_PATTERN1, (uint16_t)pattern[1]);
    fpga_write_reg(FPGA_REG_UART_PATTERN2, (uint16_t)pattern[2]);
    fpga_write_reg(FPGA_REG_UART_PATTERN3, (uint16_t)pattern[3]);
    fpga_write_reg(FPGA_REG_UART_MASK, (uint16_t)mask);
}

void fpga_set_jtag_trigger(uint8_t tap_state) {
    fpga_write_reg(FPGA_REG_JTAG_STATE, (uint16_t)tap_state);
}

void fpga_set_gpio_trigger(uint8_t edge_config) {
    fpga_write_reg(FPGA_REG_GPIO_TRIG_CFG, (uint16_t)edge_config);
}

/* ========================================================================
 * Waveform RAM Upload
 * ======================================================================== */

int fpga_load_waveform(const uint16_t *samples, uint16_t count) {
    if (!samples || count == 0 || count > WAVEFORM_MAX_SAMPLES) {
        return -1;
    }

    /* Stop any running waveform first */
    fpga_stop_waveform();

    /* Write samples to waveform BRAM via address auto-increment */
    for (uint16_t i = 0; i < count; i++) {
        fpga_write_reg(FPGA_REG_WAVEFORM_ADDR, i);
        fpga_write_reg(FPGA_REG_WAVEFORM_DATA, samples[i]);
    }

    /* Store count in waveform control register (upper bits) */
    uint16_t ctrl = (count & 0x3FF);
    fpga_write_reg(FPGA_REG_WAVEFORM_CTRL, ctrl);

    return 0;
}

void fpga_start_waveform(int loop, int trigger) {
    uint16_t ctrl = fpga_read_reg(FPGA_REG_WAVEFORM_CTRL);
    ctrl |= WAVEFORM_CTRL_PLAY;
    if (loop)   ctrl |= WAVEFORM_CTRL_LOOP;
    if (trigger) ctrl |= WAVEFORM_CTRL_TRIGGER;
    fpga_write_reg(FPGA_REG_WAVEFORM_CTRL, ctrl);
}

void fpga_stop_waveform(void) {
    uint16_t ctrl = fpga_read_reg(FPGA_REG_WAVEFORM_CTRL);
    ctrl &= ~(WAVEFORM_CTRL_PLAY | WAVEFORM_CTRL_LOOP | WAVEFORM_CTRL_TRIGGER);
    fpga_write_reg(FPGA_REG_WAVEFORM_CTRL, ctrl);
}

/* ========================================================================
 * PLL Reconfiguration
 * ======================================================================== */

int fpga_pll_reconfig(uint16_t div_int, uint16_t div_frac) {
    if (div_int == 0 || div_int > 128) {
        return -1;
    }

    /* Write new divisor values */
    fpga_write_reg(FPGA_REG_PLL_CTRL, div_int & 0x7F);
    fpga_write_reg(FPGA_REG_PLL_FRAC_LO, div_frac & 0xFFFF);
    fpga_write_reg(FPGA_REG_PLL_FRAC_HI, (div_frac >> 16) & 0xFF);

    /* Trigger PLL reconfig */
    uint16_t ctrl = fpga_read_reg(FPGA_REG_CTRL);
    ctrl |= FPGA_CTRL_PLL_RECONFIG;
    fpga_write_reg(FPGA_REG_CTRL, ctrl);

    /* Wait for PLL lock (typical: 100 µs, max: 1 ms) */
    uint32_t start = g_tick_ms;
    while (!(fpga_read_reg(FPGA_REG_STATUS) & FPGA_STATUS_PLL_LOCKED)) {
        if ((g_tick_ms - start) > 10) {
            return -2;  /* PLL lock timeout */
        }
    }

    /* Clear reconfig bit */
    ctrl &= ~FPGA_CTRL_PLL_RECONFIG;
    fpga_write_reg(FPGA_REG_CTRL, ctrl);

    return 0;
}

int fpga_pll_locked(void) {
    return (fpga_read_reg(FPGA_REG_STATUS) & FPGA_STATUS_PLL_LOCKED) ? 1 : 0;
}

/* ========================================================================
 * Status & Diagnostics
 * ======================================================================== */

int8_t fpga_read_temperature(void) {
    /* The FPGA bitstream can include a temperature sensor IP.
     * We read it from a custom register (extended address space).
     * Returns temperature in °C, or -128 on error. */
    uint16_t val = fpga_read_reg(0x20);  /* Custom temp register */
    if (val == 0xFFFF) return -128;
    return (int8_t)(val & 0xFF);
}

uint16_t fpga_get_version(void) {
    return fpga_read_reg(FPGA_REG_VERSION);
}

uint16_t fpga_get_status(void) {
    return fpga_read_reg(FPGA_REG_STATUS);
}

uint32_t fpga_get_glitch_count(void) {
    uint16_t lo = fpga_read_reg(FPGA_REG_GLITCH_COUNT_LO);
    uint16_t hi = fpga_read_reg(FPGA_REG_GLITCH_COUNT_HI);
    return ((uint32_t)hi << 16) | lo;
}

uint32_t fpga_get_last_timestamp(void) {
    uint16_t lo = fpga_read_reg(FPGA_REG_TIMESTAMP_LO);
    uint16_t hi = fpga_read_reg(FPGA_REG_TIMESTAMP_HI);
    return ((uint32_t)hi << 16) | lo;
}

/* ========================================================================
 * Fan Control
 * ======================================================================== */

void fpga_set_fan_speed(uint8_t speed) {
    fpga_write_reg(FPGA_REG_FAN_CTRL, (uint16_t)speed);
}

void fpga_fan_auto(void) {
    int8_t temp = fpga_read_temperature();
    if (temp < 0) {
        fpga_set_fan_speed(128);  /* Default medium on read error */
        return;
    }

    uint8_t speed;
    if (temp < 30) {
        speed = 0;       /* Off below 30°C */
    } else if (temp < 45) {
        speed = 64;      /* Low: 30-45°C */
    } else if (temp < 60) {
        speed = 128;     /* Medium: 45-60°C */
    } else if (temp < 75) {
        speed = 200;     /* High: 60-75°C */
    } else {
        speed = 255;     /* Max: 75°C+ */
    }

    fpga_set_fan_speed(speed);
}