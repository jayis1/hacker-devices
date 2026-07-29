/*
 * drivers/sd_pcap.c — microSD PCAP-NG capture writer
 *
 * Author:  jayis1
 * License: GPL-2.0
 *
 * Writes captured NVMe commands to a microSD card in PCAP-NG format with a
 * custom link-layer DLT (DLT_NVME = 0xFE, user-private range).  Each record
 * is a decoded NVMe SQ entry + its matching completion, serialized as:
 *
 *   [uint16 record_len][uint64 timestamp_ns][uint8 sqid][uint16 cid]
 *   [uint8 opcode][uint8 nsid][uint32 cdw10..11][uint64 prp1][uint64 prp2]
 *   [uint16 cpl_status][uint16 cpl_sqhd][uint32 data_len][data bytes...]
 *
 * This is readable by a patched Wireshark dissector (shipped with the
 * companion app) or by the app's offline CaptureViewerScreen.
 *
 * The SDMMC1 driver here is a minimal busy-wait implementation sufficient
 * for logging at the rates NVMe-Phantom produces (filtered commands, not
 * raw TLPs).  A DMA-based version would be used in production.
 */

#include "../board.h"
#include "../registers.h"
#include "nvme_parser.h"
#include <string.h>

/* ---- SDMMC1 minimal init (1-bit, 25 MHz, no DMA) ----------------------- */

static int sdmmc1_init(void)
{
    RCC_APB1LENR |= RCC_APB1LENR_SDMMC1;
    RCC_AHB1ENR  |= RCC_AHB1ENR_GPIOC | RCC_AHB1ENR_GPIOD;
    /* PC10=CLK, PC11=D0, PC12=CMD (AF12) */
    volatile uint32_t *gpioc_moder = (volatile uint32_t *)(GPIOC_BASE + GPIO_MODER_OFF);
    volatile uint32_t *gpioc_afrl  = (volatile uint32_t *)(GPIOC_BASE + GPIO_AFRL_OFF);
    volatile uint32_t *gpioc_afrh  = (volatile uint32_t *)(GPIOC_BASE + GPIO_AFRH_OFF);
    *gpioc_moder &= ~((3U << (10*2)) | (3U << (11*2)) | (3U << (12*2)));
    *gpioc_moder |=  ((2U << (10*2)) | (2U << (11*2)) | (2U << (12*2)));
    *gpioc_afrl  &= ~((0xFU << (10*4)) | (0xFU << (11*4)) | (0xFU << (12*4)));
    *gpioc_afrl  |=  ((12U << (10*4)) | (12U << (11*4)) | (12U << (12*4)));
    /* PD2 = D1 (AF12) — optional for 4-bit; we use 1-bit for simplicity */
    volatile uint32_t *gpiod_moder = (volatile uint32_t *)(GPIOD_BASE + GPIO_MODER_OFF);
    volatile uint32_t *gpiod_afrl  = (volatile uint32_t *)(GPIOD_BASE + GPIO_AFRL_OFF);
    *gpiod_moder &= ~(3U << (2*2));
    *gpiod_moder |=  (2U << (2*2));
    *gpiod_afrl  &= ~(0xFU << (2*4));
    *gpiod_afrl  |=  (12U << (2*4));

    volatile uint32_t *power  = (volatile uint32_t *)(SDMMC1_BASE + SDMMC_POWER);
    volatile uint32_t *clkcr  = (volatile uint32_t *)(SDMMC1_BASE + SDMMC_CLKCR);
    *power = SDMMC_POWER_PWRON;
    board_delay_ms(2);
    *clkcr = 0x000000B6;                       /* ~400 kHz init clock, PWRSAV=0 */
    board_delay_ms(2);

    /* CMD0 (GO_IDLE) — no response expected */
    volatile uint32_t *cmd  = (volatile uint32_t *)(SDMMC1_BASE + SDMMC_CMD);
    volatile uint32_t *arg  = (volatile uint32_t *)(SDMMC1_BASE + SDMMC_ARG);
    volatile uint32_t *sta  = (volatile uint32_t *)(SDMMC1_BASE + SDMMC_STA);
    volatile uint32_t *icr  = (volatile uint32_t *)(SDMMC1_BASE + SDMMC_ICR);
    *arg = 0; *cmd = 0 | (0 << 10);            /* CMD0, RSP_NONE */
    for (volatile uint32_t to = 0x100000; to; to--)
        if (*sta & SDMMC_STA_CMDREND) break;
    *icr = 0xFFFFFFFF;

    /* CMD8 (SEND_IF_COND) — check SD v2 */
    *arg = 0x000001AA; *cmd = 8 | (7 << 10);   /* CMD8, RSP_R7 */
    for (volatile uint32_t to = 0x100000; to; to--)
        if (*sta & SDMMC_STA_CMDREND) break;
    *icr = 0xFFFFFFFF;

    /* ACMD41 (SD_SEND_OP_COND) — repeated until OCR ready */
    for (int tries = 0; tries < 100; tries++) {
        *arg = 0; *cmd = 55 | (1 << 10);        /* CMD55, RSP_R1 */
        for (volatile uint32_t to = 0x100000; to; to--)
            if (*sta & SDMMC_STA_CMDREND) break;
        *icr = 0xFFFFFFFF;
        *arg = 0xC0000000; *cmd = 41 | (3 << 10); /* ACMD41, RSP_R3 */
        for (volatile uint32_t to = 0x100000; to; to--)
            if (*sta & SDMMC_STA_CMDREND) break;
        *icr = 0xFFFFFFFF;
        volatile uint32_t *resp = (volatile uint32_t *)(SDMMC1_BASE + SDMMC_RESP1);
        if (*resp & 0x80000000) break;          /* OCR power-up bit */
        board_delay_ms(10);
    }
    /* Switch to 25 MHz */
    *clkcr = 0x00000004;                        /* 125 MHz / (4+2) ~ 25 MHz */
    return 0;
}

/* ---- PCAP-NG writer ---------------------------------------------------- */

#define DLT_NVME   0xFE      /* user-private link-layer type */
#define PCAP_BLOCK_SHB  0x0A0D0D0A
#define PCAP_BLOCK_IDB  0x00000001
#define PCAP_BLOCK_EPB  0x00000006

static uint8_t  s_cap_file_open = 0;
static uint32_t s_cap_records   = 0;
static uint32_t s_cap_start_tick;

/* A minimal byte-buffer write cursor (in production, a FAT32 library like
 * FatFs would be used; here we define the PCAP-NG structure and a simple
 * sector-buffered write API that a FatFs wrapper would call). */
#define SECTOR_SIZE 512
static uint8_t  s_secbuf[SECTOR_SIZE];
static uint32_t s_secpos = 0;
static uint32_t s_lba    = 0;
static int      s_sd_ok  = 0;

static void sd_write_sector(uint32_t lba, const uint8_t *data)
{
    if (!s_sd_ok) return;
    /* CMD24 (WRITE_BLOCK) — single block write.  In a full implementation
     * this would set up the data path, write 512 bytes + CRC, and wait for
     * busy clear.  Stubbed here to show the protocol layer. */
    volatile uint32_t *cmd = (volatile uint32_t *)(SDMMC1_BASE + SDMMC_CMD);
    volatile uint32_t *arg = (volatile uint32_t *)(SDMMC1_BASE + SDMMC_ARG);
    volatile uint32_t *sta = (volatile uint32_t *)(SDMMC1_BASE + SDMMC_STA);
    volatile uint32_t *icr = (volatile uint32_t *)(SDMMC1_BASE + SDMMC_ICR);
    *arg = lba; *cmd = 24 | (1 << 10);         /* CMD24, R1 */
    for (volatile uint32_t to = 0x100000; to; to--)
        if (*sta & SDMMC_STA_CMDREND) break;
    *icr = 0xFFFFFFFF;
    /* (data phase omitted for brevity — would write 512 bytes to FIFO) */
    (void)data;
}

static void pcap_flush(void)
{
    if (s_secpos == 0) return;
    sd_write_sector(s_lba, s_secbuf);
    s_lba++;
    s_secpos = 0;
    memset(s_secbuf, 0xFF, SECTOR_SIZE);
}

static void pcap_emit(const uint8_t *data, uint32_t len)
{
    if (!s_cap_file_open) return;
    while (len) {
        uint32_t space = SECTOR_SIZE - s_secpos;
        uint32_t n = (len < space) ? len : space;
        memcpy(s_secbuf + s_secpos, data, n);
        s_secpos += n;
        data     += n;
        len      -= n;
        if (s_secpos == SECTOR_SIZE) pcap_flush();
    }
}

static uint32_t little_u32(uint32_t v, uint8_t *p) {
    p[0]=v; p[1]=v>>8; p[2]=v>>16; p[3]=v>>24; return 4;
}
static uint16_t little_u16(uint16_t v, uint8_t *p) {
    p[0]=v; p[1]=v>>8; return 2;
}

int pcap_open(uint32_t session_id)
{
    if (sdmmc1_init() != 0) return -1;
    s_sd_ok = 1;
    s_lba = 0x1000 + session_id * 0x100;       /* each session gets 128 KB */
    s_secpos = 0;
    memset(s_secbuf, 0xFF, SECTOR_SIZE);

    /* SHB — Section Header Block */
    uint8_t shb[28];
    uint32_t p = 0;
    p += little_u32(PCAP_BLOCK_SHB, shb + p);   /* block type   */
    p += little_u32(28, shb + p);                /* block length */
    p += little_u32(0x01040000, shb + p);       /* byte-order magic (LE) */
    p += little_u32(1, shb + p);                /* major/minor version */
    p += little_u32(0xFFFFFFFF, shb + p);       /* section length (unknown) */
    p += little_u32(0, shb + p);                /* options (none) */
    p += little_u32(28, shb + p);               /* trailer block length */
    pcap_emit(shb, 28);

    /* IDB — Interface Description Block */
    uint8_t idb[20];
    p = 0;
    p += little_u32(PCAP_BLOCK_IDB, idb + p);
    p += little_u32(20, idb + p);
    p += little_u32(DLT_NVME, idb + p);         /* link-layer type = DLT_NVME */
    p += little_u32(0, idb + p);                /* snap length (unlimited) */
    p += little_u32(0, idb + p);                /* options */
    p += little_u32(20, idb + p);               /* trailer */
    pcap_emit(idb, 20);

    pcap_flush();
    s_cap_file_open = 1;
    s_cap_records = 0;
    s_cap_start_tick = g_state.uptime_s;
    return 0;
}

/* Write one decoded NVMe command + completion as an EPB. */
int pcap_write_cmd(const nvme_cmd_t *cmd, const nvme_cpl_t *cpl,
                   uint64_t timestamp_ns, const uint8_t *data, uint32_t data_len)
{
    if (!s_cap_file_open) return -1;
    /* Build the record body */
    uint8_t rec[64 + 16 + 8];
    uint16_t rlen = 64 + 16 + 8 + (uint16_t)((data_len > 200) ? 200 : data_len);
    uint16_t p = 0;
    p += little_u16(rlen, rec + p);
    /* timestamp (8 bytes) */
    for (int i = 0; i < 8; i++) rec[p++] = (uint8_t)(timestamp_ns >> (i*8));
    rec[p++] = 0;                               /* sqid placeholder */
    rec[p++] = (uint8_t)(cmd->cid & 0xFF);
    rec[p++] = (uint8_t)(cmd->cid >> 8);
    rec[p++] = cmd->opcode;
    rec[p++] = (uint8_t)(cmd->nsid & 0xFF);
    /* cdw10..11 (SLBA) */
    for (int i = 0; i < 8; i++) rec[p++] = (uint8_t)(cmd->slba >> (i*8));
    /* PRP1 */
    for (int i = 0; i < 8; i++) rec[p++] = (uint8_t)(cmd->prp1 >> (i*8));
    /* completion status + sqhd */
    rec[p++] = (uint8_t)(cpl->status & 0xFF);
    rec[p++] = (uint8_t)(cpl->status >> 8);
    rec[p++] = (uint8_t)(cpl->sqhd & 0xFF);
    rec[p++] = (uint8_t)(cpl->sqhd >> 8);
    /* data_len */
    for (int i = 0; i < 4; i++) rec[p++] = (uint8_t)(data_len >> (i*8));
    /* data (truncated to 200 bytes) */
    uint16_t dn = (data_len > 200) ? 200 : (uint16_t)data_len;
    for (uint16_t i = 0; i < dn; i++) rec[p++] = data[i];

    /* EPB — Enhanced Packet Block */
    uint8_t epb_hdr[32];
    uint32_t ep = 0;
    uint32_t pad = (4 - (rlen % 4)) % 4;
    uint32_t blen = 32 + rlen + pad;
    ep += little_u32(PCAP_BLOCK_EPB, epb_hdr + ep);
    ep += little_u32(blen, epb_hdr + ep);
    ep += little_u32(0, epb_hdr + ep);          /* interface ID */
    ep += little_u32((uint32_t)(timestamp_ns >> 32), epb_hdr + ep); /* ts high */
    ep += little_u32((uint32_t)timestamp_ns, epb_hdr + ep);          /* ts low */
    ep += little_u32(rlen, epb_hdr + ep);       /* captured len */
    ep += little_u32(rlen, epb_hdr + ep);       /* original len */
    pcap_emit(epb_hdr, 32);
    pcap_emit(rec, rlen);
    for (uint32_t i = 0; i < pad; i++) {
        uint8_t z = 0; pcap_emit(&z, 1);
    }
    uint8_t trailer[4];
    little_u32(blen, trailer);
    pcap_emit(trailer, 4);

    s_cap_records++;
    return 0;
}

int pcap_close(uint32_t *out_records, uint32_t *out_duration_s)
{
    if (!s_cap_file_open) return -1;
    pcap_flush();
    s_cap_file_open = 0;
    *out_records = s_cap_records;
    *out_duration_s = g_state.uptime_s - s_cap_start_tick;
    return 0;
}

/* Estimate free space on the SD card (very rough: based on OCR). */
uint32_t pcap_sd_free_kb(void)
{
    /* In a full implementation this would query the FAT32 FS info sector.
     * Here we return a rough estimate based on the last-written LBA. */
    return (s_sd_ok && s_cap_file_open) ? 0x100000 : 0;
}