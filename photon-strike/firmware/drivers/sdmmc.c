/**
 * drivers/sdmmc.c — MicroSD (SDMMC1) logging for PhotonStrike
 * Author: jayis1
 * License: GPL-2.0
 *
 * Writes a CSV log of every shot to a file on the SD card. The file is
 * opened at scan start and closed at scan end. Each shot is one line:
 *
 *   seq,x_um,y_um,delay_ps,width_ns,energy,trig_src,fault_class,
 *   output_hash,output_len,target_clk_khz,laser_temp_c,flags
 *
 * The SDMMC1 peripheral is used in 1-bit mode at 25 MHz (default speed)
 * for maximum compatibility with cheap cards. A FAT32 driver (ported
 * FatFs — not included in this snippet for brevity but linked in by
 * the Makefile as third_party/fatfs/) provides f_open/f_write/f_close.
 * This driver wraps the FatFs calls and formats the CSV lines.
 */

#include "sdmmc.h"
#include "gpio.h"
#include "../registers.h"
#include <stdio.h>
#include <string.h>

/* FatFs declarations (linked externally). */
typedef void FIL;
extern int  f_open(FIL *fp, const char *path, uint8_t mode);
extern int  f_write(FIL *fp, const void *buf, uint32_t btw, uint32_t *bw);
extern int  f_close(FIL *fp);
extern int  f_mount(void *fs, const char *path, uint8_t opt);

#define FA_WRITE          0x02u
#define FA_CREATE_ALWAYS  0x08u
#define FA_OPEN_APPEND    0x30u

static FIL     s_log_file;
static char    s_line[160];
static bool    s_open;

bool sd_init(void)
{
    /* Init SDMMC1 peripheral: 1-bit, default speed, clock from PLL1Q.
     * The full init sequence (CMD0 reset, CMD8, ACMD41, CMD2, CMD3,
     * CMD7, CMD9 CSD) is handled by the ported FatFs disk_io layer
     * (third_party/fatfs/diskio.c). Here we just mount the filesystem. */
    volatile uint32_t *rcc = (volatile uint32_t *)RCC_BASE;
    rcc[RCC_AHB3ENR / 4u] |= RCC_AHB3ENR_SDMMC1EN;

    int ok = f_mount(NULL, "0:", 1);
    s_open = false;
    return ok == 0;
}

bool sd_open_log(const char *name)
{
    char path[24];
    snprintf(path, sizeof(path), "0:/%s.csv", name);
    if (f_open(&s_log_file, path, FA_WRITE | FA_CREATE_ALWAYS) != 0) {
        s_open = false;
        return false;
    }
    s_open = true;

    /* CSV header. */
    const char *hdr =
        "seq,x_um,y_um,delay_ps,width_ns,energy,trig_src,fault_class,"
        "output_hash,output_len,target_clk_khz,laser_temp_c,flags\n";
    uint32_t bw;
    f_write(&s_log_file, hdr, (uint32_t)strlen(hdr), &bw);
    return true;
}

bool sd_log_shot(const ps_shot_t *shot)
{
    if (!s_open) return false;
    int n = snprintf(s_line, sizeof(s_line),
        "%lu,%u,%u,%lu,%u,%u,%u,%u,%lu,%u,%d,%u,%u\n",
        (unsigned long)shot->seq,
        shot->x_um, shot->y_um,
        (unsigned long)shot->delay_ps,
        shot->width_ns, shot->energy,
        shot->trig_src, shot->fault_class,
        (unsigned long)shot->output_hash,
        shot->output_len,
        (int)shot->target_clk_khz,
        shot->laser_temp_c, shot->flags);
    if (n <= 0) return false;
    uint32_t bw;
    f_write(&s_log_file, s_line, (uint32_t)n, &bw);
    return bw == (uint32_t)n;
}

bool sd_close_log(void)
{
    if (!s_open) return false;
    s_open = false;
    return f_close(&s_log_file) == 0;
}