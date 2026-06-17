/**
 * @file    edid_manager.h
 * @brief   EDID management — header
 * @author  jayis1
 */

#ifndef EDID_MANAGER_H
#define EDID_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

/** EDID source type */
typedef enum {
    EDID_SOURCE_PASSTHROUGH = 0,  /**< EDID passed through from display */
    EDID_SOURCE_LOCAL,            /**< EDID from local EEPROM */
    EDID_SOURCE_INJECTED          /**< Custom EDID injected by user */
} edid_source_t;

/**
 * @brief Initialize I²C master bus
 */
void i2c_init(void);

/**
 * @brief Initialize EDID manager
 */
esp_err_t edid_manager_init(void);

/**
 * @brief Read EDID from connected display via HDMI OUT DDC
 */
esp_err_t edid_read_from_display(void);

/**
 * @brief Read EDID from local 24LC02 EEPROM
 */
esp_err_t edid_read_from_local(void);

/**
 * @brief Inject custom EDID into local EEPROM
 * @param edid_data EDID binary data (128 bytes minimum)
 * @param len Data length
 */
esp_err_t edid_inject(const uint8_t *edid_data, size_t len);

/**
 * @brief Disable HDCP support flag in current EDID
 */
esp_err_t edid_disable_hdcp(void);

/**
 * @brief Restore passthrough EDID mode
 */
esp_err_t edid_restore_passthrough(void);

/**
 * @brief Get current active EDID
 * @param len Output: EDID length in bytes
 * @return Pointer to EDID data
 */
const uint8_t *edid_get_current(size_t *len);

/**
 * @brief Get current EDID source
 */
edid_source_t edid_get_source(void);

/**
 * @brief Check if current EDID is valid
 */
bool edid_is_valid(void);

#endif /* EDID_MANAGER_H */
