/**
 * @file    sd_card.h
 * @brief   microSD card driver — header
 * @author  jayis1
 */

#ifndef SD_CARD_H
#define SD_CARD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include "esp_err.h"

/** File info structure for directory listing */
typedef struct {
    char     name[64];    /**< Filename (null-terminated) */
    size_t   size;        /**< File size in bytes */
    time_t   mod_time;    /**< Last modification time */
} sd_card_file_info_t;

/**
 * @brief Initialize and mount microSD card
 * @return ESP_OK on success
 */
esp_err_t sd_card_init(void);

/**
 * @brief Write a captured frame to SD card
 * @param filename Output filename (e.g., "frame_0001.jpg")
 * @param data Frame data buffer
 * @param len Data length in bytes
 * @return ESP_OK on success
 */
esp_err_t sd_card_write_frame(const char *filename, const uint8_t *data, size_t len);

/**
 * @brief Append log entry to a CEC log file
 * @param filename Log filename
 * @param data Data to append
 * @param len Data length
 * @return ESP_OK on success
 */
esp_err_t sd_card_write_log(const char *filename, const char *data, size_t len);

/**
 * @brief Write EDID dump to file
 * @param edid_data EDID binary data
 * @param len Data length
 * @return ESP_OK on success
 */
esp_err_t sd_card_write_edid(const uint8_t *edid_data, size_t len);

/**
 * @brief Get free space on SD card in bytes
 * @return Free bytes
 */
uint64_t sd_card_get_free_space(void);

/**
 * @brief Get total space on SD card in bytes
 * @return Total bytes
 */
uint64_t sd_card_get_total_space(void);

/**
 * @brief Check if SD card has enough space
 * @param required_bytes Bytes needed
 * @return true if space available
 */
bool sd_card_has_space_for(size_t required_bytes);

/**
 * @brief List captured frame files
 * @param files Output array of file info
 * @param max_files Maximum entries
 * @return Number of files found, or -1 on error
 */
int sd_card_list_captures(sd_card_file_info_t *files, int max_files);

/**
 * @brief Check if SD card is mounted
 * @return true if mounted
 */
bool sd_card_is_mounted(void);

/**
 * @brief Unmount SD card safely
 * @return ESP_OK on success
 */
esp_err_t sd_card_unmount(void);

#endif /* SD_CARD_H */
