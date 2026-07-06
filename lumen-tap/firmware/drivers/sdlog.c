/*
 * lumen-tap/firmware/drivers/sdlog.c
 * SD/MMC card logging via SDMMC1 peripheral with a minimal FatFs-style
 * block writer. Writes a raw + demodulated audio stream to a ring file
 * on the microSD card.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

#include "sdlog.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

static ltm_sdlog_state_t g_state = SDLOG_IDLE;
static ltm_sdlog_stats_t g_stats;
static ltm_session_hdr_t g_hdr;
static uint32_t g_file_offset = 0;

/* Sector buffer (512 bytes) for unaligned writes. */
static uint8_t  g_sector[512];
static uint16_t g_sector_pos = 0;

/* ---- SDMMC1 low-level (polled, 4-bit) ------------------------------ */
static void sdmmc1_init_hw(void)
{
    /* Enable GPIOC/PORTB/PORTC + SDMMC1 clocks */
    RCC_AHB4ENR  |= (1U << 2) | (1U << 1);   /* GPIOCEN, GPIOBEN */
    RCC_AHB3ENR  |= (1U << 12);              /* SDMMC1EN on H7 */

    /* PC8..PC11 = SDMMC1 D0..D3 (AF9), PC12 = CK, PD2 = CMD (AF9) */
    for (int p = 8; p <= 12; ++p) {
        GPIOC->MODER = (GPIOC->MODER & ~(0x3 << (p * 2))) | (0x2 << (p * 2));
        GPIOC->AFRH  = (GPIOC->AFRH  & ~(0xF << ((p - 8) * 4))) |
                       (0x9 << ((p - 8) * 4));
        GPIOC->OSPEEDR |= (0x3 << (p * 2));   /* high speed */
        GPIOC->PUPDR  = (GPIOC->PUPDR & ~(0x3 << (p * 2))) | (0x1 << (p * 2));
    }
    /* PD2 = CMD (AF9) */
    GPIOD->MODER  = (GPIOD->MODER  & ~(0x3 << (2 * 2)))  | (0x2 << (2 * 2));
    GPIOD->AFRL   = (GPIOD->AFRL   & ~(0xF << (2 * 4)))  | (0x9 << (2 * 4));
    GPIOD->OSPEEDR |= (0x3 << (2 * 2));
    GPIOD->PUPDR  = (GPIOD->PUPDR  & ~(0x3 << (2 * 2)))  | (0x1 << (2 * 2));

    /* Power on SDMMC1 */
    SDMMC1_POWER = 0x03;   /* ON */
    /* Clock at ~400 kHz for init (CLKDIV = 118 from 120 MHz) */
    SDMMC1_CLKCR = (118U << 0) | (1U << 11);  /* PWRSAV off, HWFC_EN off */
    SDMMC1_DTIMER = 0xFFFFFFFFU;
}

/* Send a command and wait for response. Returns 1 on success. */
static int sdmmc1_send_cmd(uint32_t cmd, uint32_t arg, uint32_t wait_flags)
{
    SDMMC1_ARG  = arg;
    SDMMC1_CMD  = cmd | (wait_flags << 8) | (1U << 10);  /* CPSMEN */
    uint32_t to = 1000000;
    while (!(SDMMC1_STA & (SDMMC_STA_CMDREND | SDMMC_STA_CTIMEOUT)) && --to) {}
    if (SDMMC1_STA & SDMMC_STA_CTIMEOUT) {
        SDMMC1_ICR = SDMMC_STA_CTIMEOUT;
        return 0;
    }
    SDMMC1_ICR = SDMMC_STA_CMDREND;
    return 1;
}

/* Read a single 512-byte sector (polled). */
static int sdmmc1_read_block(uint32_t lba, uint8_t *buf)
{
    SDMMC1_DTIMER = 0xFFFFFFFFU;
    SDMMC1_DLEN   = 512;
    SDMMC1_DCTRL  = (9 << 4) | (1U << 1) | (1U << 3);  /* 2^9=512, DTEN, DTDIR */
    if (!sdmmc1_send_cmd(17, lba, 0x05)) return 0;     /* CMD17 read single */
    uint32_t to = 2000000;
    while (!(SDMMC1_STA & (SDMMC_STA_DATAEND | SDMMC_STA_DTIMEOUT)) && --to) {}
    if (SDMMC1_STA & SDMMC_STA_DTIMEOUT) { SDMMC1_ICR = 0xFFFFFFFFU; return 0; }
    /* Drain FIFO (32-bit) */
    volatile uint32_t *fifo = (volatile uint32_t *)(SDMMC1_BASE + 0x80);
    for (int i = 0; i < 128; ++i) ((uint32_t *)buf)[i] = fifo[i];
    SDMMC1_ICR = 0xFFFFFFFFU;
    return 1;
}

/* Write a single 512-byte sector (polled). */
static int sdmmc1_write_block(uint32_t lba, const uint8_t *buf)
{
    SDMMC1_DTIMER = 0xFFFFFFFFU;
    SDMMC1_DLEN   = 512;
    SDMMC1_DCTRL  = (9 << 4) | (1U << 1);              /* 2^9=512, DTEN, write */
    if (!sdmmc1_send_cmd(24, lba, 0x05)) return 0;     /* CMD24 write single */
    volatile uint32_t *fifo = (volatile uint32_t *)(SDMMC1_BASE + 0x80);
    for (int i = 0; i < 128; ++i) fifo[i] = ((const uint32_t *)buf)[i];
    uint32_t to = 2000000;
    while (!(SDMMC1_STA & (SDMMC_STA_DATAEND | SDMMC_STA_DTIMEOUT)) && --to) {}
    if (SDMMC1_STA & SDMMC_STA_DTIMEOUT) { SDMMC1_ICR = 0xFFFFFFFFU; return 0; }
    SDMMC1_ICR = 0xFFFFFFFFU;
    return 1;
}

/* ---- Sector-aligned byte stream writer ----------------------------- */
static int stream_write(const uint8_t *data, int len)
{
    while (len > 0) {
        int space = 512 - g_sector_pos;
        int n = len < space ? len : space;
        memcpy(g_sector + g_sector_pos, data, n);
        g_sector_pos += n;
        data += n;
        len -= n;
        if (g_sector_pos == 512) {
            if (!sdmmc1_write_block(g_file_offset / 512, g_sector)) {
                g_stats.write_errors++;
                return -1;
            }
            g_file_offset += 512;
            g_stats.blocks_written++;
            g_stats.bytes_written += 512;
            g_sector_pos = 0;
        }
    }
    return 0;
}

/* ---- Public API ---------------------------------------------------- */
void sdlog_init(void)
{
    memset(&g_stats, 0, sizeof(g_stats));
    memset(&g_hdr,   0, sizeof(g_hdr));
    sdmmc1_init_hw();
    /* Card init sequence (simplified): CMD0, CMD8, CMD55, ACMD41, CMD2, CMD3 */
    sdmmc1_send_cmd(0,  0, 0x05);              /* GO_IDLE */
    sdmmc1_send_cmd(8,  0x1AA, 0x05);          /* SEND_IF_COND */
    uint32_t to = 1000;
    do {
        sdmmc1_send_cmd(55, 0, 0x05);          /* APP_CMD */
    } while (sdmmc1_send_cmd(41, 0x1000000, 0x05) && --to && !(SDMMC1_RESP1 & (1U << 31)));
    sdmmc1_send_cmd(2, 0, 0x05);               /* ALL_SEND_CID */
    sdmmc1_send_cmd(3, 0, 0x05);               /* SEND_REL_ADDR */
    /* raise clock to 25 MHz */
    SDMMC1_CLKCR = (3U << 0) | (1U << 11);
    g_state = SDLOG_IDLE;
    g_file_offset = 0;
    g_sector_pos = 0;
}

int sdlog_begin_session(const ltm_session_hdr_t *hdr)
{
    if (g_state == SDLOG_RECORDING) return -1;
    memcpy(&g_hdr, hdr, sizeof(g_hdr));
    g_hdr.magic[0]='L'; g_hdr.magic[1]='T'; g_hdr.magic[2]='M';
    g_hdr.magic[3]='L'; g_hdr.magic[4]='O'; g_hdr.magic[5]='G';
    g_hdr.magic[6]='0'; g_hdr.magic[7]='1';
    g_hdr.header_size = sizeof(g_hdr);
    g_hdr.adc_rate = ADC_SAMPLE_RATE_HZ;
    g_hdr.out_rate = OUTPUT_SAMPLE_RATE_HZ;
    g_hdr.channels = 1;
    g_hdr.bits_per_sample = 16;
    g_file_offset = 0;
    g_sector_pos  = 0;
    stream_write((const uint8_t *)&g_hdr, sizeof(g_hdr));
    g_state = SDLOG_RECORDING;
    return 0;
}

int sdlog_write_block(const uint8_t *raw_adc, int raw_len,
                      const uint8_t *demod, int demod_len)
{
    if (g_state != SDLOG_RECORDING) return -1;
    /* Write a record: [u16 raw_len][u16 demod_len][raw][demod] */
    uint8_t hdr[4] = {
        (uint8_t)(raw_len & 0xFF), (uint8_t)((raw_len >> 8) & 0xFF),
        (uint8_t)(demod_len & 0xFF), (uint8_t)((demod_len >> 8) & 0xFF)
    };
    if (stream_write(hdr, 4) < 0)        return -1;
    if (raw_len > 0 && stream_write(raw_adc, raw_len) < 0)   return -1;
    if (demod_len > 0 && stream_write(demod, demod_len) < 0) return -1;
    return 0;
}

void sdlog_end_session(void)
{
    /* flush partial sector */
    if (g_sector_pos > 0) {
        memset(g_sector + g_sector_pos, 0, 512 - g_sector_pos);
        sdmmc1_write_block(g_file_offset / 512, g_sector);
        g_file_offset += 512;
        g_stats.bytes_written += g_sector_pos;
        g_sector_pos = 0;
    }
    g_state = SDLOG_IDLE;
}

ltm_sdlog_state_t sdlog_state(void) { return g_state; }

void sdlog_get_stats(ltm_sdlog_stats_t *st) { *st = g_stats; }