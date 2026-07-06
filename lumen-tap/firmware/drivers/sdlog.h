/*
 * lumen-tap/firmware/drivers/sdlog.h
 * SD/MMC logging for LumenTap — raw + demodulated audio ring buffer.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

#ifndef LUMEN_TAP_SDLOG_H
#define LUMEN_TAP_SDLOG_H

#include <stdint.h>

/* Session header written at the start of each capture. */
typedef struct {
    char     magic[8];        /* "LTMLOG01" */
    uint32_t header_size;
    uint32_t adc_rate;        /* 192000 */
    uint32_t out_rate;        /* 48000 */
    uint16_t channels;        /* 1 */
    uint16_t bits_per_sample; /* 16 (raw) */
    uint32_t start_tick_ms;
    uint32_t reserved[8];
    /* operator metadata */
    char     operator[32];
    char     target_id[32];
    char     note[128];
} ltm_session_hdr_t;

typedef enum {
    SDLOG_IDLE = 0,
    SDLOG_ARMED,
    SDLOG_RECORDING,
    SDLOG_FAULT
} ltm_sdlog_state_t;

void sdlog_init(void);
int  sdlog_begin_session(const ltm_session_hdr_t *hdr);
int  sdlog_write_block(const uint8_t *raw_adc, int raw_len,
                       const uint8_t *demod, int demod_len);
void sdlog_end_session(void);
ltm_sdlog_state_t sdlog_state(void);

/* Stats */
typedef struct {
    uint32_t blocks_written;
    uint32_t bytes_written;
    uint32_t write_errors;
} ltm_sdlog_stats_t;
void sdlog_get_stats(ltm_sdlog_stats_t *st);

#endif /* LUMEN_TAP_SDLOG_H */