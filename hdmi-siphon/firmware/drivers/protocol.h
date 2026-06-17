/**
 * @file    protocol.h
 * @brief   JSON wire protocol — header
 * @author  jayis1
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"
#include "sd_card.h"

/**
 * @brief Build full device status JSON string
 * @param buf Output buffer
 * @param buf_size Buffer size
 * @return String length (excluding null terminator)
 */
size_t protocol_build_status(char *buf, size_t buf_size);

/**
 * @brief Build gallery listing JSON string
 * @param buf Output buffer
 * @param buf_size Buffer size
 * @param files Array of file info
 * @param count Number of files
 * @return String length
 */
size_t protocol_build_gallery_json(char *buf, size_t buf_size,
                                   const sd_card_file_info_t *files, int count);

/**
 * @brief Parse JSON command string and execute
 * @param json_str Incoming command
 * @param result_out Output buffer for response
 * @param result_size Response buffer size
 * @return ESP_OK on success
 */
esp_err_t protocol_parse_and_execute(const char *json_str,
                                      char *result_out, size_t result_size);

/**
 * @brief Handle EDID injection command (simplified for HTTP handlers)
 */
esp_err_t protocol_handle_edid_command(const char *json_str);

/**
 * @brief Handle CEC command (simplified for HTTP handlers)
 */
esp_err_t protocol_handle_cec_command(const char *json_str);

/**
 * @brief Handle configuration command
 */
esp_err_t protocol_handle_config_command(const char *json_str);

#endif /* PROTOCOL_H */
