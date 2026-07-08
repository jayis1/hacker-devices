/*
 * tpm-phantom — drivers/sd_capture.h
 * MicroSD capture storage interface.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef TPM_PHANTOM_SD_CAPTURE_H
#define TPM_PHANTOM_SD_CAPTURE_H

#include <stdint.h>

#define SD_OK             0
#define SD_ERR_NO_CARD    1
#define SD_ERR_INIT       2
#define SD_ERR_WRITE      3

uint8_t  sd_capture_init(void);
uint8_t  sd_capture_append(const uint8_t *data, uint16_t len);
uint8_t  sd_capture_flush(void);
uint8_t  sd_capture_ready(void);
uint32_t sd_capture_blocks_written(void);
uint32_t sd_capture_errors(void);
void     sd_capture_reset(void);

extern volatile uint32_t sd_total_bytes;
extern volatile uint32_t sd_card_capacity_mb;

#endif /* TPM_PHANTOM_SD_CAPTURE_H */