/*
 * hart-bleeder — storage.h
 * microSD + QSPI flash logging for captured frames, attack logs,
 * and device profiles for the HART Fieldbus Covert In-Line Attack
 * Implant.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef HART_BLEEDER_STORAGE_H
#define HART_BLEEDER_STORAGE_H

#include <stdint.h>
#include "hart_stack.h"

int  storage_init(void);
int  storage_log_frame(const void *frame, uint16_t len);
int  storage_log_text(const char *text, uint32_t len);
int  storage_log_attack(const char *opname, uint8_t addr, int rc);
int  storage_flush(void);
int  storage_format(void);
uint32_t storage_bytes_used(void);
uint32_t storage_bytes_free(void);

/* QSPI external flash for large captures */
int  qspi_init(void);
int  qspi_read(uint32_t addr, void *buf, uint32_t len);
int  qspi_write(uint32_t addr, const void *buf, uint32_t len);
int  qspi_erase_sector(uint32_t addr);
void qspi_memory_mapped_enable(void);

/* Device profile persistence */
typedef struct {
    uint8_t  poll_addr;
    uint8_t  manufacturer;
    uint8_t  dev_type;
    uint8_t  long_addr[5];
    char     tag[6];
    char     descriptor[12];
    float    pv_min;
    float    pv_max;
    uint8_t  units_code;
} storage_profile_t;

int  storage_save_profile(const storage_profile_t *p);
int  storage_load_profile(uint8_t index, storage_profile_t *p);
int  storage_list_profiles(void);

#endif /* HART_BLEEDER_STORAGE_H */