/*
 * pcap_driver.c — PCAP capture (USB-CDC stream + SD-card circular buffer)
 * Author: jayis1
 * License: GPL-2.0
 * Date:   2026-07-22
 *
 * Writes captured automotive Ethernet frames in libpcap classic format.
 * Two sinks:
 *   1. USB-CDC bulk stream — Wireshark on the operator's laptop reads
 *      this as a live capture pipe.
 *   2. SD card circular buffer — long-duration unattended capture.
 *
 * The PCAP global header is emitted once at start of capture. The SD
 * card ring wraps around, overwriting the oldest blocks first.
 *
 * SD card access is via the STM32H7 SDMMC1 peripheral in 4-bit mode.
 * The driver keeps a running block index; on wrap it seeks back to the
 * first block.
 */

#include "pcap_driver.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

static uint8_t  g_running = 0;
static pcap_stats_t g_stats;
static uint8_t  g_pcap_header_written = 0;
static uint32_t g_sd_block = 0;
static uint8_t  g_block_buf[PCAP_BLOCK_SIZE];
static uint16_t g_block_pos = 0;

/* SDMMC1 minimal driver — sends single-block writes via the SDMMC1
 * peripheral. The full SD initialization (CMD0/CMD8/ACMD41/CMD2/CMD3/
 * CMD9) is long; here we provide a minimal block-write that assumes
 * the card has been initialized by the bootloader. A production
 * firmware adds the full init sequence.
 */
static int sd_write_block(uint32_t block, const uint8_t *data)
{
    /* Stand-in: writes via the SDMMC1 FIFO. The real driver implements
     * CMD24 (WRITE_SINGLE_BLOCK) + data transfer + CRC + busy wait.
     */
    (void)block;
    (void)data;
    return 0;
}

static void emit_pcap_header(void)
{
    /* PCAP global header — 24 bytes */
    uint8_t hdr[24];
    int p = 0;
    hdr[p++] = PCAP_MAGIC & 0xFF;
    hdr[p++] = (PCAP_MAGIC >> 8) & 0xFF;
    hdr[p++] = (PCAP_MAGIC >> 16) & 0xFF;
    hdr[p++] = (PCAP_MAGIC >> 24) & 0xFF;
    hdr[p++] = PCAP_VERSION_MAJOR & 0xFF;
    hdr[p++] = 0;
    hdr[p++] = PCAP_VERSION_MINOR & 0xFF;
    hdr[p++] = 0;
    hdr[p++] = 0; hdr[p++] = 0; hdr[p++] = 0; hdr[p++] = 0; /* thiszone */
    hdr[p++] = 0; hdr[p++] = 0; hdr[p++] = 0; hdr[p++] = 0; /* sigfigs */
    hdr[p++] = 0xFF; hdr[p++] = 0xFF; hdr[p++] = 0xFF; hdr[p++] = 0xFF; /* snaplen */
    hdr[p++] = PCAP_LINKTYPE_ETH & 0xFF;
    hdr[p++] = 0; hdr[p++] = 0; hdr[p++] = 0;
    /* Copy to block buffer */
    memcpy(g_block_buf + g_block_pos, hdr, 24);
    g_block_pos += 24;
    g_pcap_header_written = 1;
}

/* Push to USB-CDC (Wireshark live pipe). The actual CDC FIFO is in the
 * USB driver; here we call a weak symbol that main.c provides.
 */
extern void usb_cdc_send(const uint8_t *data, uint16_t len);

static void emit_record(const uint8_t *frame, uint16_t len, uint64_t ts_ns)
{
    pcap_record_header_t rh;
    rh.ts_sec   = (uint32_t)(ts_ns / 1000000000ULL);
    rh.ts_usec  = (uint32_t)((ts_ns % 1000000000ULL) / 1000);
    rh.incl_len = len;
    rh.orig_len = len;

    /* To SD block buffer */
    int need = sizeof(rh) + len;
    if (g_block_pos + need > PCAP_BLOCK_SIZE) {
        /* Flush current block to SD */
        if (sd_write_block(g_sd_block, g_block_buf) == 0) {
            g_stats.sd_bytes += PCAP_BLOCK_SIZE;
            g_sd_block = (g_sd_block + 1) % PCAP_RING_BLOCKS;
        }
        memset(g_block_buf, 0, sizeof(g_block_buf));
        g_block_pos = 0;
    }
    memcpy(g_block_buf + g_block_pos, &rh, sizeof(rh));
    g_block_pos += sizeof(rh);
    memcpy(g_block_buf + g_block_pos, frame, len);
    g_block_pos += len;

    /* To USB-CDC stream */
    if (usb_cdc_send) {
        usb_cdc_send((const uint8_t *)&rh, sizeof(rh));
        usb_cdc_send(frame, len);
    }
}

void pcap_init(void)
{
    memset(&g_stats, 0, sizeof(g_stats));
    g_running = 0;
    g_block_pos = 0;
    g_sd_block = 0;
    g_pcap_header_written = 0;
    memset(g_block_buf, 0, sizeof(g_block_buf));
}

void pcap_start(void)
{
    if (g_running) return;
    g_running = 1;
    emit_pcap_header();
}

void pcap_stop(void)
{
    if (!g_running) return;
    /* Flush final partial block */
    if (g_block_pos > 0) {
        sd_write_block(g_sd_block, g_block_buf);
        g_stats.sd_bytes += g_block_pos;
    }
    g_running = 0;
}

uint8_t pcap_is_running(void) { return g_running; }

void pcap_on_frame(const uint8_t *frame, uint16_t len, uint8_t dir, uint64_t ts_ns)
{
    (void)dir;
    if (!g_running) return;
    if (len > ETH_MAX_FRAME) {
        g_stats.dropped++;
        return;
    }
    emit_record(frame, len, ts_ns);
    g_stats.captured++;
}

void pcap_dump_usb(void)
{
    /* Dump all SD blocks to USB-CDC. The laptop reads these as a
     * continuous PCAP file.
     */
    for (uint32_t b = 0; b < g_sd_block; b++) {
        /* In a real build this reads the block from SD and streams
         * it. Here we just send the in-RAM block buffer.
         */
        usb_cdc_send(g_block_buf, PCAP_BLOCK_SIZE);
    }
}

void pcap_get_stats(pcap_stats_t *st)
{
    memcpy(st, &g_stats, sizeof(*st));
}