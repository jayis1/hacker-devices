/* Credential Canary capture and policy API
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef CREDENTIAL_CANARY_CAPTURE_H
#define CREDENTIAL_CANARY_CAPTURE_H
#include "board.h"
#include "protocol.h"

typedef enum { ALERT_NONE=0, ALERT_PARITY, ALERT_REPLAY_WINDOW, ALERT_OSDP_PLAINTEXT, ALERT_ADDRESS_CHANGE, ALERT_MALFORMED, ALERT_LINE_STUCK, ALERT_TAMPER } alert_code_t;

void capture_init(void);
bool capture_push(event_type_t type, interface_t iface, const uint8_t *data, uint8_t length, uint8_t flags, uint32_t timestamp_us);
bool capture_pop(capture_event_t *event);
uint16_t capture_count(void);
uint32_t capture_dropped(void);
void capture_mark(const char *text);
void policy_observe_wiegand(uint64_t raw, uint8_t bits, bool parity_ok, uint32_t timestamp_us);
void policy_observe_osdp(const osdp_frame_t *frame, interface_t iface, uint32_t timestamp_us);
void policy_tamper(bool active);
void policy_reset_session(void);
uint32_t policy_alert_count(void);
const char *policy_alert_name(alert_code_t code);
#endif
