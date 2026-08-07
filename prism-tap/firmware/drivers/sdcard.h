/*
 * drivers/sdcard.h — SD Card Interface for Prism-Tap
 *
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef PRISM_TAP_SDCARD_H
#define PRISM_TAP_SDCARD_H

#include <stdint.h>

int  sdcard_init(void);
int  sdcard_present(void);

/* File operations (simplified FAT32 interface) */
int  sdcard_open_write(const char *path);
int  sdcard_open_read(const char *path);
int  sdcard_close(int fd);
int  sdcard_write(int fd, const uint8_t *data, uint32_t len);
int  sdcard_read(int fd, uint8_t *data, uint32_t len);

/* Directory and file management */
int  sdcard_mkdir(const char *path);
int  sdcard_file_exists(const char *path);
int  sdcard_delete(const char *path);
int  sdcard_list_dir(const char *path, char *out, uint32_t max_len);

/* Card info */
uint64_t sdcard_capacity_bytes(void);
uint64_t sdcard_free_bytes(void);

/* Low-level SPI (for init sequence) */
int  sdcard_spi_init(void);
int  sdcard_send_cmd(uint8_t cmd, uint32_t arg, uint8_t *response, uint8_t resp_len);

#endif /* PRISM_TAP_SDCARD_H */
/* Author: jayis1 */