/*
 * sdcard.h — SDIO SD card driver for STM32F407
 */

#ifndef SDCARD_DRIVER_H
#define SDCARD_DRIVER_H

#include <stdint.h>
#include "can.h"

#define SDCARD_BLOCK_SIZE  512

typedef enum {
    SD_OK = 0,
    SD_ERROR_CMD,
    SD_ERROR_TIMEOUT,
    SD_ERROR_CRC,
    SD_ERROR_NO_CARD,
    SD_ERROR_WRITE
} sd_result_t;

typedef struct {
    uint32_t card_type;
    uint32_t block_count;
    uint32_t block_size;
    uint8_t  cid[16];
    uint8_t  csd[16];
} sd_card_info_t;

sd_result_t sd_init(void);
sd_result_t sd_read_blocks(uint32_t block_addr, uint8_t *buf, uint32_t count);
sd_result_t sd_write_blocks(uint32_t block_addr, const uint8_t *buf, uint32_t count);
sd_result_t sd_get_info(sd_card_info_t *info);

/* CAN-specific logging functions */
sd_result_t sd_start_recording(const char *filename);
sd_result_t sd_stop_recording(void);
sd_result_t sd_write_can_frame(uint8_t channel, const can_frame_t *frame);
int sd_read_next_can_frame(can_frame_t *frame, uint32_t *timestamp);

/* Replay functions */
sd_result_t sd_start_replay(const char *filename);
sd_result_t sd_stop_replay(void);

#endif /* SDCARD_DRIVER_H */