/*
 * sd_pcap.c — MicroSD pcap writer via FatFs (DLT_USER0 custom link layer)
 *
 * Writes captured PLC frames to a FAT32 ring-buffer pcap file. When the file
 * exceeds PCAP_MAX_BYTES it rotates (closes current, opens new with incrementing
 * suffix). crypto_erase() overwrites the FAT and reinitializes.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <stdint.h>
#include <string.h>
#include "sd_pcap.h"
#include "fatfs.h"
#include "../board.h"

static FATFS g_fs;
static FIL   g_file;
static int   g_mounted = 0;
static int   g_open = 0;
static uint32_t g_written = 0;
static uint32_t g_seq = 0;
static uint8_t g_card_present = 0;

/* pcap global header (DLT_USER0 = 147) */
static const uint8_t PCAP_MAGIC[24] = {
    0xD4, 0xC3, 0xB2, 0xA1, /* magic */
    0x02, 0x00, 0x04, 0x00, /* version 2.4 */
    0x00, 0x00, 0x00, 0x00, /* thiszone */
    0x00, 0x00, 0x00, 0x00, /* sigfigs */
    0xFF, 0xFF, 0x00, 0x00, /* snaplen 65535 */
    0x93, 0x00, 0x00, 0x00  /* DLT_USER0 = 147 */
};

int sd_pcap_init(void) {
    /* Check card detect */
    if ((GPIOC->IDR) & (1U << 5)) {
        g_card_present = 0;
        return -1; /* no card */
    }
    g_card_present = 1;
    if (fatfs_mount(&g_fs) != 0) return -1;
    g_mounted = 1;
    return sd_pcap_open_next();
}

int sd_pcap_open_next(void) {
    if (!g_mounted) return -1;
    if (g_open) { fatfs_close(&g_file); g_open = 0; }
    char name[16];
    /* name = REAP_XXXX.pcap */
    name[0]='R'; name[1]='E'; name[2]='A'; name[3]='P'; name[4]='_';
    uint32_t s = g_seq;
    for (int i = 0; i < 4; i++) {
        name[5+i] = "0123456789ABCDEF"[(s >> ((3-i)*4)) & 0xF];
    }
    name[9]='.'; name[10]='p'; name[11]='c'; name[12]='a'; name[13]='p'; name[14]=0;
    if (fatfs_open(&g_file, name, FA_WRITE | FA_CREATE_ALWAYS) != 0) return -1;
    /* Write pcap global header */
    UINT bw;
    fatfs_write(&g_file, PCAP_MAGIC, 24, &bw);
    g_written = 24;
    g_open = 1;
    g_seq++;
    return 0;
}

int sd_pcap_write(const uint8_t *data, uint32_t len) {
    if (!g_open) return -1;
    UINT bw;
    if (fatfs_write(&g_file, data, len, &bw) != 0) return -1;
    g_written += bw;
    if (g_written >= PCAP_MAX_BYTES) {
        sd_pcap_open_next();
    }
    return 0;
}

void sd_pcap_flush_tick(uint32_t now_ms) {
    (void)now_ms;
    if (g_open) fatfs_sync(&g_file);
}

void sd_pcap_crypto_erase(void) {
    if (g_open) { fatfs_close(&g_file); g_open = 0; }
    /* Overwrite FAT with pseudo-random pattern */
    static uint8_t buf[512];
    for (uint32_t i = 0; i < sizeof(buf); i++) buf[i] = (uint8_t)(i * 31 + 7);
    /* In a real impl: open the file in raw mode and overwrite sectors.
     * Simplified: just unmount + reformat FAT32.
     */
    fatfs_format(&g_fs);
    g_mounted = 0;
    g_written = 0;
    g_seq = 0;
}

int sd_pcap_present(void) { return g_card_present; }
uint32_t sd_pcap_written(void) { return g_written; }