/*
 * storage.h — hit-log persistence (NOR flash + microSD)
 *
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef HARMONIC_REAPER_STORAGE_H
#define HARMONIC_REAPER_STORAGE_H

#include <stdint.h>

typedef struct hit_record {
    uint32_t timestamp_ms;
    int16_t  pitch_deg;
    int16_t  yaw_deg;
    int16_t  p2_dbfs;
    int16_t  p3_dbfs;
    int8_t   ratio_db;
    uint8_t  classify;      /* 0=none 1=semi 2=metal 3=ambiguous */
    int8_t   tx_power_dbm;
} hit_record_t;

void storage_init(void);
void storage_log_hit(const hit_record_t *h);
void storage_flush_pending(void);
uint32_t storage_export_to_sd(void);     /* returns # records exported */
uint32_t storage_export_count(void);
uint32_t storage_get_hit_count(void);

#endif /* HARMONIC_REAPER_STORAGE_H */
/* EOF — storage.h — jayis1 */