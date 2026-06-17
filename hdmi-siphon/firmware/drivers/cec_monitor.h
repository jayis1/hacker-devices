/**
 * @file    cec_monitor.h
 * @brief   HDMI-CEC bus monitor — header
 * @author  jayis1
 */

#ifndef CEC_MONITOR_H
#define CEC_MONITOR_H

#include <stdint.h>
#include <stdbool.h>

/** CEC log entry structure (public) */
typedef struct {
    uint64_t timestamp_us;
    uint16_t source_addr;
    uint16_t dest_addr;
    uint8_t  opcode;
    uint8_t  data[16];
    uint8_t  data_len;
    bool     is_broadcast;
} cec_log_entry_t;

/**
 * @brief Initialize CEC monitor hardware
 */
void cec_monitor_init(void);

/**
 * @brief Poll CEC bus for activity (call at ~200 Hz)
 */
void cec_monitor_poll(void);

/**
 * @brief Send a CEC message
 * @param destination Destination logical address (0-15)
 * @param opcode CEC opcode
 * @param data Payload data
 * @param data_len Payload length in bytes
 * @return 0 on success, negative on error
 */
int cec_monitor_send(uint8_t destination, uint8_t opcode,
                     const uint8_t *data, uint8_t data_len);

/**
 * @brief Send CEC Standby command to TV
 * @return 0 on success
 */
int cec_monitor_send_standby(void);

/**
 * @brief Send CEC Active Source report
 * @param physical_addr 16-bit physical address
 * @return 0 on success
 */
int cec_monitor_send_active_source(uint16_t physical_addr);

/**
 * @brief Send a remote control keypress via CEC
 * @param destination Target device address
 * @param uc_code User control code (e.g., CEC_UC_POWER, CEC_UC_VOLUME_UP)
 * @return 0 on success
 */
int cec_monitor_send_user_control(uint8_t destination, uint8_t uc_code);

/**
 * @brief Check if CEC bus is currently active
 */
bool cec_monitor_is_bus_active(void);

/**
 * @brief Get total CEC message count
 */
uint32_t cec_monitor_get_message_count(void);

/**
 * @brief Get CEC log entries
 * @param entries Output buffer
 * @param max_entries Max entries to read
 * @return Number of entries returned
 */
int cec_monitor_get_log(cec_log_entry_t *entries, int max_entries);

/**
 * @brief Enable or disable CEC monitoring
 */
void cec_monitor_enable(bool enable);

/**
 * @brief FreeRTOS task for CEC monitoring
 */
void cec_task(void *pvParameters);

#endif /* CEC_MONITOR_H */
