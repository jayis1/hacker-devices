/* Credential Canary command channel
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef CREDENTIAL_CANARY_COMMAND_H
#define CREDENTIAL_CANARY_COMMAND_H
#include "board.h"
void command_init(void);
void command_rx(const uint8_t *data, uint16_t length);
void command_poll(void);
void command_stream_event(const capture_event_t *event);
#endif
