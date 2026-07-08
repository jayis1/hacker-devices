/*
 * tpm-phantom — drivers/sd_capture.c
 * MicroSD capture storage via SDMMC1.
 *
 * Captured TPM transactions are streamed to a FAT-like raw log file on
 * the MicroSD card for long-duration captures that exceed SRAM buffer
 * capacity. This driver implements a minimal SD card init sequence and
 * block-write routine (polled, no DMA for simplicity — DMA path is
 * described in phase4_software_stack.md).
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "sd_capture.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ===================================================================
 * Internal state
 * =================================================================== */
static volatile uint8_t  g_sd_initialized = 0;
static volatile uint32_t g_sd_block_addr = 0;
static volatile uint32_t g_blocks_written = 0;
static volatile uint32_t g_write_errors = 0;

/* Scratch sector buffer (512 bytes) */
static uint8_t  g_sector_buf[512];
static uint16_t g_sector_pos = 0;

/* ===================================================================
 * Public stats
 * =================================================================== */
volatile uint32_t sd_total_bytes = 0;
volatile uint32_t sd_card_capacity_mb = 0;

/* ===================================================================
 * Delays
 * =================================================================== */
static void sd_delay_ms(uint32_t ms)
{
    for (volatile uint32_t i = 0; i < ms * 12000; i++) __asm__("nop");
}

/* ===================================================================
 * Send SD command and read response (simplified polled mode)
 * =================================================================== */
static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    uint8_t resp = 0xFF;
    /* Write 6-byte command frame to SDMMC1_CMD register (simplified) */
    SDMMC1_ARG = arg;
    SDMMC1_CMD = (cmd | 0x400);  /* CPSMEN */
    for (volatile uint32_t t = 0; t < 1000000; t++) {
        if (SDMMC1_STA & BIT(6)) {  /* CMDREND */
            resp = (uint8_t)(SDMMC1_RESP1 & 0xFF);
            SDMMC1_ICR = BIT(6);
            return resp;
        }
        if (SDMMC1_STA & BIT(7)) {  /* CTIMEOUT */
            SDMMC1_ICR = BIT(7);
            return 0xFF;
        }
    }
    return 0xFF;
}

/* ===================================================================
 * Initialize SD card (CMD0, CMD8, ACMD41, CMD2, CMD3)
 * Simplified — covers the essential SD spec init sequence.
 * =================================================================== */
uint8_t sd_capture_init(void)
{
    /* Enable GPIOC, GPIOD and SDMMC1 clocks */
    SET_BIT(RCC_AHB4ENR, BIT(2) | BIT(3));
    SET_BIT(RCC_APB2ENR, BIT(10));   /* SDMMC1 */

    /* Configure SD pins: PC8-PC12 AF12 (SDMMC1), PD2 AF12 */
    gpio_set_mode(SD_D0_PORT, SD_D0_PIN, GPIO_MODE_AF);
    gpio_set_af(SD_D0_PORT, SD_D0_PIN, 12);
    gpio_set_speed(SD_D0_PORT, SD_D0_PIN, GPIO_SPEED_VHIGH);
    gpio_set_pupd(SD_D0_PORT, SD_D0_PIN, GPIO_PUPD_PU);

    gpio_set_mode(GPIOC, 9, GPIO_MODE_AF);
    gpio_set_af(GPIOC, 9, 12);
    gpio_set_pupd(GPIOC, 9, GPIO_PUPD_PU);
    gpio_set_mode(GPIOC, 10, GPIO_MODE_AF);
    gpio_set_af(GPIOC, 10, 12);
    gpio_set_pupd(GPIOC, 10, GPIO_PUPD_PU);
    gpio_set_mode(GPIOC, 11, GPIO_MODE_AF);
    gpio_set_af(GPIOC, 11, 12);
    gpio_set_pupd(GPIOC, 11, GPIO_PUPD_PU);
    gpio_set_mode(SD_CLK_PORT, SD_CLK_PIN, GPIO_MODE_AF);
    gpio_set_af(SD_CLK_PORT, SD_CLK_PIN, 12);
    gpio_set_mode(SD_CMD_PORT, SD_CMD_PIN, GPIO_MODE_AF);
    gpio_set_af(SD_CMD_PORT, SD_CMD_PIN, 12);
    gpio_set_pupd(SD_CMD_PORT, SD_CMD_PIN, GPIO_PUPD_PU);

    /* Card detect */
    gpio_set_mode(SD_DETECT_PORT, SD_DETECT_PIN, GPIO_MODE_INPUT);
    gpio_set_pupd(SD_DETECT_PORT, SD_DETECT_PIN, GPIO_PUPD_PU);

    /* Check card present (active low) */
    if (gpio_read(SD_DETECT_PORT, SD_DETECT_PIN))
        return SD_ERR_NO_CARD;

    /* Power on SDMMC1 */
    SDMMC1_POWER = 0x03;  /* PWRCTRL = ON */
    sd_delay_ms(10);

    /* Clock: 400 kHz for init */
    SDMMC1_CLKCR = 0x00B6;  /* CLKDIV ~183 for 120MHz/(183+2) */
    sd_delay_ms(2);

    /* CMD0: reset (GO_IDLE_STATE) */
    if (sd_send_cmd(0, 0, 0x95) == 0xFF)
        return SD_ERR_INIT;
    sd_delay_ms(5);

    /* CMD8: send interface condition (voltage check) */
    sd_send_cmd(8, 0x000001AA, 0x87);
    sd_delay_ms(5);

    /* ACMD41: send OCR (HCS=1 for SDHC/SDXC) — APP_CMD prefix CMD55 */
    sd_send_cmd(55, 0, 0x00);
    sd_send_cmd(41, 0x40100000, 0x00);
    sd_delay_ms(100);

    /* CMD2: get CID */
    sd_send_cmd(2, 0, 0x00);
    sd_delay_ms(5);

    /* CMD3: get RCA */
    sd_send_cmd(3, 0, 0x00);
    sd_delay_ms(5);

    /* Set higher clock for data transfer */
    SDMMC1_CLKCR = 0x0002;  /* CLKDIV=2 → 40 MHz */

    g_sd_initialized = 1;
    g_sd_block_addr = 0x1000;  /* start writing at block 4096 */
    g_blocks_written = 0;
    g_sector_pos = 0;
    sd_total_bytes = 0;
    sd_card_capacity_mb = 0;

    return SD_OK;
}

/* ===================================================================
 * Write a single 512-byte block to SD card
 * =================================================================== */
static uint8_t sd_write_block(uint32_t block, const uint8_t *data)
{
    if (!g_sd_initialized)
        return SD_ERR_INIT;

    SDMMC1_DTIMER = 0xFFFFFFFF;
    SDMMC1_DLEN = 512;
    SDMMC1_DCTRL = 0x01 | (9 << 4);  /* DTEN, DBLOCKSIZE=9 (512B) */

    /* CMD24: WRITE_BLOCK */
    SDMMC1_ARG = block;
    SDMMC1_CMD = 24 | 0x400;  /* CPSMEN */

    for (volatile uint32_t t = 0; t < 1000000; t++) {
        if (SDMMC1_STA & BIT(10)) {  /* TXFIFOHE */
            /* Write 128 32-bit words to FIFO (512 bytes) */
            for (int i = 0; i < 128; i += 4) {
                uint32_t w = data[i] | (data[i+1] << 8) |
                            (data[i+2] << 16) | (data[i+3] << 24);
                *((volatile uint32_t *)(SDMMC1_BASE + 0x80)) = w;
            }
            break;
        }
        if (SDMMC1_STA & BIT(7)) {
            SDMMC1_ICR = 0xFFFFFFFF;
            return SD_ERR_WRITE;
        }
    }

    g_blocks_written++;
    sd_total_bytes += 512;
    return SD_OK;
}

/* ===================================================================
 * Append data to capture log (buffers until 512 bytes accumulated)
 * =================================================================== */
uint8_t sd_capture_append(const uint8_t *data, uint16_t len)
{
    while (len > 0) {
        uint16_t space = 512 - g_sector_pos;
        uint16_t chunk = len < space ? len : space;
        memcpy(&g_sector_buf[g_sector_pos], data, chunk);
        g_sector_pos += chunk;
        data += chunk;
        len -= chunk;

        if (g_sector_pos >= 512) {
            uint8_t r = sd_write_block(g_sd_block_addr, g_sector_buf);
            if (r != SD_OK) {
                g_write_errors++;
                return r;
            }
            g_sd_block_addr++;
            g_sector_pos = 0;
        }
    }
    return SD_OK;
}

/* ===================================================================
 * Flush remaining buffer (pad with 0xFF)
 * =================================================================== */
uint8_t sd_capture_flush(void)
{
    if (g_sector_pos == 0)
        return SD_OK;
    memset(&g_sector_buf[g_sector_pos], 0xFF, 512 - g_sector_pos);
    uint8_t r = sd_write_block(g_sd_block_addr, g_sector_buf);
    g_sd_block_addr++;
    g_sector_pos = 0;
    return r;
}

/* ===================================================================
 * Query status
 * =================================================================== */
uint8_t  sd_capture_ready(void) { return g_sd_initialized; }
uint32_t sd_capture_blocks_written(void) { return g_blocks_written; }
uint32_t sd_capture_errors(void) { return g_write_errors; }

void sd_capture_reset(void)
{
    g_sd_block_addr = 0x1000;
    g_blocks_written = 0;
    g_sector_pos = 0;
    sd_total_bytes = 0;
}