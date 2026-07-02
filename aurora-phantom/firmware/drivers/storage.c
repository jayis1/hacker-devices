/*
 * storage.c — SD-card raw-event + frame + log storage
 * Device: Aurora-Phantom   Author: jayis1   License: GPL-2.0
 *
 * Uses the ESP32-S3 SDMMC peripheral (4-bit, 40 MHz) via FatFs. Manages:
 *   /aurora/log.txt         - append-only text log
 *   /aurora/events/<sess>   - raw photon-event files (optional)
 *   /aurora/frames/<sess>   - reconstructed frame magnitude files
 *   /aurora/missions/*.json - mission files
 *
 * For host test builds the FatFs calls are weak-stubbed.
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "../board.h"
#include "storage.h"

__attribute__((weak)) int sd_mount(void) { return 0; }
__attribute__((weak)) int sd_open(const char *p, int mode) { (void)p; (void)mode; return 1; }
__attribute__((weak)) int sd_write(int fd, const void *buf, uint32_t len) { (void)fd; (void)buf; return (int)len; }
__attribute__((weak)) int sd_read(int fd, void *buf, uint32_t len) { (void)fd; (void)buf; return (int)len; }
__attribute__((weak)) void sd_close(int fd) { (void)fd; }
__attribute__((weak)) int sd_size(const char *p) { (void)p; return 0; }
__attribute__((weak)) int sd_read_file(const char *p, uint8_t *b, uint32_t n) { (void)p; (void)b; (void)n; return -1; }
__attribute__((weak)) int sd_append(const char *p, const char *s, uint32_t n) { (void)p; (void)s; (void)n; return 0; }
__attribute__((weak)) int sd_count_dir(const char *p) { (void)p; return 0; }
__attribute__((weak)) int sd_open_idx(const char *dir, uint16_t idx) { (void)dir; (void)idx; return 1; }

static bool g_ready = false;
static int  g_frame_fd = -1;
static int  g_event_fd = -1;
static char g_session[64];
static uint16_t g_frame_count = 0;

int storage_init(void)
{
    if (sd_mount() == 0) {
        g_ready = true;
        return 0;
    }
    g_ready = false;
    return -1;
}

bool storage_ready(void) { return g_ready; }

int storage_session_open(const char *name, bool stream_events)
{
    if (!g_ready) return -1;
    strncpy(g_session, name, sizeof(g_session) - 1);
    g_session[sizeof(g_session) - 1] = 0;
    g_frame_count = 0;

    char path[96];
    snprintf(path, sizeof(path), "%s.dat", name);
    g_frame_fd = sd_open(path, 2 /* write/create */);
    if (g_frame_fd < 0) return -1;

    if (stream_events) {
        snprintf(path, sizeof(path), "%s.evt", name);
        g_event_fd = sd_open(path, 2);
    }
    return 0;
}

void storage_session_close(void)
{
    if (g_frame_fd >= 0) { sd_close(g_frame_fd); g_frame_fd = -1; }
    if (g_event_fd >= 0) { sd_close(g_event_fd); g_event_fd = -1; }
}

int storage_session_write_frame(const uint16_t *words, uint32_t n)
{
    if (g_frame_fd < 0) return -1;
    int w = sd_write(g_frame_fd, words, n * sizeof(uint16_t));
    if (w > 0) g_frame_count++;
    return w;
}

uint16_t storage_list_frames(void)
{
    if (!g_ready) return 0;
    return (uint16_t)sd_count_dir(SD_FRAME_DIR);
}

int storage_read_frame(uint16_t idx, uint16_t *out, uint32_t max)
{
    if (!g_ready) return -1;
    int fd = sd_open_idx(SD_FRAME_DIR, idx);
    if (fd < 0) return -1;
    int n = sd_read(fd, out, max * sizeof(uint16_t));
    sd_close(fd);
    return n / (int)sizeof(uint16_t);
}

int storage_read_file(const char *path, uint8_t *buf, uint32_t max)
{
    return sd_read_file(path, buf, max);
}

int storage_append_log(const char *line, uint32_t len)
{
    if (!g_ready) return -1;
    return sd_append("/aurora/log.txt", line, len);
}