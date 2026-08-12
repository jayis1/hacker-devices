/**
 * @file protocol_handler.h
 * @brief Binary protocol handler for app ↔ device communication
 *
 * Author: jayis1
 * @copyright Copyright (c) 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef PROTOCOL_HANDLER_H
#define PROTOCOL_HANDLER_H

#include <stdint.h>
#include <stdbool.h>
#include "registers.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Protocol parser state */
typedef struct {
    uint8_t rx_buffer[256];
    uint16_t rx_index;
    bool command_pending;
    uint8_t pending_command;
    uint8_t pending_params[248];
    uint16_t pending_param_len;
} protocol_state_t;

void protocol_handler_init(protocol_state_t *state);
void protocol_handler_receive_data(protocol_state_t *state, const uint8_t *data, uint16_t length);
bool protocol_handler_has_pending_command(const protocol_state_t *state);
void protocol_handler_process_command(protocol_state_t *state);
void protocol_handler_send_status(const protocol_state_t *state, uint8_t msg_type, uint8_t sys_state);
void protocol_handler_send_capture_stats(const protocol_state_t *state, 
    const void *stats, uint16_t bat_mv, int8_t temp, uint8_t sys_state);
void protocol_handler_send_device_info(const protocol_state_t *state,
    const uint8_t *device_id, display_protocol_t protocol,
    uint16_t width, uint16_t height, uint8_t depth,
    uint8_t fps, trigger_mode_t trigger);

#ifdef __cplusplus
}
#endif

#endif /* PROTOCOL_HANDLER_H */