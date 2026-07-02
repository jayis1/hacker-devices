/*
 * storage.h — SD-card raw-event + frame + log storage
 * Author: jayis1   License: GPL-2.0
 */
#ifndef AURORA_STORAGE_H
#define AURORA_STORAGE_H
#include <stdint.h>
#include <stdbool.h>

int  storage_init(void);
bool storage_ready(void);

/* Session lifecycle */
int  storage_session_open(const char *name, bool stream_events);
void storage_session_close(void);
int  storage_session_write_frame(const uint16_t *words, uint32_t n);

/* Browsing / exfil (rendezvous) */
uint16_t storage_list_frames(void);
int      storage_read_frame(uint16_t idx, uint16_t *out, uint32_t max);

/* Misc */
int      storage_read_file(const char *path, uint8_t *buf, uint32_t max);
int      storage_append_log(const char *line, uint32_t len);

#endif