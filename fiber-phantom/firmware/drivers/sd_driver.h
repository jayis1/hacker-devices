/*
 * sd_driver.h — MicroSD card driver and PCAP file writer
 * Author: jayis1
 * Date:   2026-06-17
 */

#ifndef FIBER_PHANTOM_SD_DRIVER_H
#define FIBER_PHANTOM_SD_DRIVER_H

#include <stdint.h>

/* Initialize SD card over SPI1 */
int sd_init(void);

/* Check if SD card is physically present */
uint8_t sd_is_present(void);

/* Get SD card capacity in MB */
uint32_t sd_get_capacity_mb(void);

/* Get free space in MB (from FAT32 FSInfo, approximate) */
uint32_t sd_get_free_mb(void);

/* Open a new PCAP file for writing. Filename: FP_XXXXX.pcap (sequence) */
int sd_pcap_open(uint32_t link_type);

/* Write a packet record to the open PCAP file */
int sd_pcap_write_packet(const uint8_t *data, uint32_t len,
                         uint32_t ts_sec, uint32_t ts_usec,
                         uint32_t orig_len);

/* Close current PCAP file */
int sd_pcap_close(void);

/* Get current PCAP file size in bytes */
uint32_t sd_pcap_get_filesize(void);

/* List capture files on SD card (callback per file) */
typedef void (*sd_file_callback_t)(const char *filename, uint32_t size);
int sd_list_files(sd_file_callback_t callback);

/* Delete a file by name */
int sd_delete_file(const char *filename);

/* Read data from a file (for PCAP download over BLE) */
int sd_read_file_chunk(const char *filename, uint32_t offset,
                       uint8_t *buf, uint32_t chunk_size, uint32_t *bytes_read);

/* Check SD card health: returns 0 if OK */
int sd_health_check(void);

#endif /* FIBER_PHANTOM_SD_DRIVER_H */