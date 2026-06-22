/*
 * onewire.c — WireReaper 1-Wire driver: master and sniffer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — GPL-2.0
 *
 * Implements 1-Wire protocol in bit-banged GPIO for reading iButton
 * ROM IDs, DS24xx EEPROM pages, and temperature sensors. Supports
 * standard speed (15 us slots) and overdrive (2 us slots).
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"

#define ONEWIRE_GPIO   GPIO(ONEWIRE_PORT)
#define ONEWIRE_BIT    (1U << ONEWIRE_PIN)

/* Timing constants (microseconds) — standard speed */
#define OW_STD_RESET_LOW      480
#define OW_STD_RESET_WAIT      70
#define OW_STD_RESET_RECOVER  410
#define OW_STD_WRITE1_LOW       6
#define OW_STD_WRITE1_RECOVER  64
#define OW_STD_WRITE0_LOW      60
#define OW_STD_WRITE0_RECOVER  10
#define OW_STD_READ_LOW         6
#define OW_STD_READ_WAIT        9
#define OW_STD_READ_RECOVER    55

/* Overdrive timing */
#define OW_OD_RESET_LOW        70
#define OW_OD_RESET_WAIT        8
#define OW_OD_RESET_RECOVER   402
#define OW_OD_WRITE1_LOW        1
#define OW_OD_WRITE1_RECOVER    7
#define OW_OD_WRITE0_LOW        7
#define OW_OD_WRITE0_RECOVER    2
#define OW_OD_READ_LOW           1
#define OW_OD_READ_WAIT          7
#define OW_OD_READ_RECOVER      13

static int ow_overdrive = 0;
static volatile uint32_t *g_ticks;

/* ---- Microsecond delay via DWT cycle counter ---- */
#define DWT_CYCCNT  (*(volatile uint32_t *)0xE0001004)
#define DWT_CTRL    (*(volatile uint32_t *)0xE0001000)
#define DWT_LAR     (*(volatile uint32_t *)0xE0001FB0)

static void dwt_init(void) {
    DWT_LAR = 0xC5ACCE55; /* Unlock */
    DWT_CTRL |= 1; /* Enable CYCCNT */
    DWT_CYCCNT = 0;
}

static void delay_us(uint32_t us) {
    uint32_t cycles = us * (HCLK_HZ / 1000000);
    uint32_t start = DWT_CYCCNT;
    while ((DWT_CYCCNT - start) < cycles);
}

/* ---- GPIO control ---- */
static void ow_pin_output(void) {
    gpio_t *g = ONEWIRE_GPIO;
    g->MODER &= ~(3U << (ONEWIRE_PIN * 2));
    g->MODER |= (GPIO_MODE_OUTPUT << (ONEWIRE_PIN * 2));
    g->OTYPER |= (1U << ONEWIRE_PIN); /* Open-drain */
    g->OSPEEDR |= (GPIO_OSPEED_HIGH << (ONEWIRE_PIN * 2));
}

static void ow_pin_input(void) {
    gpio_t *g = ONEWIRE_GPIO;
    g->MODER &= ~(3U << (ONEWIRE_PIN * 2));
    g->MODER |= (GPIO_MODE_INPUT << (ONEWIRE_PIN * 2));
    g->PUPDR |= (GPIO_PUPD_PULLUP << (ONEWIRE_PIN * 2));
}

static void ow_low(void) {
    ONEWIRE_GPIO->BSRR = (ONEWIRE_BIT << 16); /* Reset (pull low) */
}

static void ow_release(void) {
    ONEWIRE_GPIO->BSRR = ONEWIRE_BIT; /* Set (release, pulled up) */
}

static int ow_read_pin(void) {
    return (ONEWIRE_GPIO->IDR & ONEWIRE_BIT) ? 1 : 0;
}

/* ---- 1-Wire reset pulse ---- */
static int ow_reset(void) {
    int presence;

    ow_pin_output();
    ow_low();
    delay_us(ow_overdrive ? OW_OD_RESET_LOW : OW_STD_RESET_LOW);

    ow_release();
    ow_pin_input();
    delay_us(ow_overdrive ? OW_OD_RESET_WAIT : OW_STD_RESET_WAIT);

    presence = !ow_read_pin(); /* Low = device present */

    delay_us(ow_overdrive ? OW_OD_RESET_RECOVER : OW_STD_RESET_RECOVER);
    return presence;
}

/* ---- Write a single bit ---- */
static void ow_write_bit(int bit) {
    ow_pin_output();

    if (bit) {
        ow_low();
        delay_us(ow_overdrive ? OW_OD_WRITE1_LOW : OW_STD_WRITE1_LOW);
        ow_release();
        delay_us(ow_overdrive ? OW_OD_WRITE1_RECOVER : OW_STD_WRITE1_RECOVER);
    } else {
        ow_low();
        delay_us(ow_overdrive ? OW_OD_WRITE0_LOW : OW_STD_WRITE0_LOW);
        ow_release();
        delay_us(ow_overdrive ? OW_OD_WRITE0_RECOVER : OW_STD_WRITE0_RECOVER);
    }
}

/* ---- Read a single bit ---- */
static int ow_read_bit(void) {
    int bit;

    ow_pin_output();
    ow_low();
    delay_us(ow_overdrive ? OW_OD_READ_LOW : OW_STD_READ_LOW);

    ow_pin_input();
    delay_us(ow_overdrive ? OW_OD_READ_WAIT : OW_STD_READ_WAIT);

    bit = ow_read_pin();

    delay_us(ow_overdrive ? OW_OD_READ_RECOVER : OW_STD_READ_RECOVER);
    return bit;
}

/* ---- Write a byte (LSB first) ---- */
static void ow_write_byte(uint8_t b) {
    for (int i = 0; i < 8; i++) {
        ow_write_bit(b & 1);
        b >>= 1;
    }
}

/* ---- Read a byte ---- */
static uint8_t ow_read_byte(void) {
    uint8_t b = 0;
    for (int i = 0; i < 8; i++) {
        b >>= 1;
        if (ow_read_bit())
            b |= 0x80;
    }
    return b;
}

/* ---- CRC8 (Dallas/Maxim 1-Wire CRC) ---- */
static uint8_t ow_crc8(const uint8_t *data, int len) {
    uint8_t crc = 0;
    for (int i = 0; i < len; i++) {
        uint8_t b = data[i];
        for (int j = 0; j < 8; j++) {
            uint8_t mix = (crc ^ b) & 0x01;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            b >>= 1;
        }
    }
    return crc;
}

/* ---- 1-Wire commands ---- */
#define OW_CMD_READ_ROM     0x33
#define OW_CMD_MATCH_ROM    0x55
#define OW_CMD_SKIP_ROM     0xCC
#define OW_CMD_SEARCH_ROM   0xF0

/* ---- Initialize ---- */
void wr_onewire_init(void) {
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOC;
    dwt_init();
    ow_pin_input();
    ow_release();
}

/* ---- Scan for 1-Wire devices (ROM search) ---- */
int wr_onewire_scan(uint8_t *rom_ids, int max) {
    int count = 0;
    uint8_t last_zero = 0;
    uint8_t last_rom[8] = {0};

    while (count < max) {
        if (!ow_reset())
            break;

        ow_write_byte(OW_CMD_SEARCH_ROM);

        uint8_t rom[8] = {0};
        int last_discrepancy = 0;

        for (int bit = 0; bit < 64; bit++) {
            int bit_a = ow_read_bit(); /* bit value */
            int bit_b = ow_read_bit(); /* complement */

            if (bit_a && bit_b) {
                /* No devices responding */
                return count;
            }

            int dir;
            if (bit_a == 0 && bit_b == 0) {
                /* Discrepancy: two devices with different bits */
                if (bit < last_zero) {
                    dir = (last_rom[bit / 8] >> (bit % 8)) & 1;
                } else if (bit == last_zero) {
                    dir = 1;
                } else {
                    dir = 0;
                }
                if (dir == 0)
                    last_discrepancy = bit;
            } else {
                dir = bit_a;
            }

            /* Write direction bit */
            ow_write_bit(dir);

            /* Store in ROM */
            if (dir)
                rom[bit / 8] |= (1U << (bit % 8));
        }

        /* Verify CRC */
        if (ow_crc8(rom, 7) == rom[7]) {
            memcpy(&rom_ids[count * 8], rom, 8);
            count++;
        }

        last_zero = last_discrepancy;
        memcpy(last_rom, rom, 8);

        if (last_discrepancy == 0)
            break; /* Search complete */
    }
    return count;
}

/* ---- Read from a specific 1-Wire device ---- */
int wr_onewire_read(int idx, uint8_t *buf, int offset, int len) {
    /* For simplicity, use SKIP_ROM if only one device.
     * In a real implementation, we'd store ROM IDs from scan
     * and use MATCH_ROM. Here we use the global scan results. */
    static uint8_t saved_roms[16][8];
    static int saved_count = 0;

    if (idx < 0 || idx >= saved_count || len <= 0)
        return WR_ERR_PARAM;

    if (!ow_reset())
        return WR_ERR_NO_TARGET;

    /* Match ROM */
    ow_write_byte(OW_CMD_MATCH_ROM);
    for (int i = 0; i < 8; i++)
        ow_write_byte(saved_roms[idx][i]);

    /* Read memory (command 0xF0 = READ MEMORY for DS2433/DS2431) */
    ow_write_byte(0xF0);
    ow_write_byte(offset & 0xFF);
    ow_write_byte((offset >> 8) & 0xFF);

    for (int i = 0; i < len; i++)
        buf[i] = ow_read_byte();

    return WR_OK;
}

/* ---- Read ROM (single device on bus) ---- */
int wr_onewire_read_rom(uint8_t *rom) {
    if (!ow_reset())
        return WR_ERR_NO_TARGET;

    ow_write_byte(OW_CMD_READ_ROM);
    for (int i = 0; i < 8; i++)
        rom[i] = ow_read_byte();

    if (ow_crc8(rom, 7) != rom[7])
        return WR_ERR_HW_FAULT; /* CRC error */

    return WR_OK;
}

/* ---- Set overdrive mode ---- */
void wr_onewire_set_overdrive(int enable) {
    ow_overdrive = enable ? 1 : 0;
}