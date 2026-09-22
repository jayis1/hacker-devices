/* USB control protocol
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef PTP_PROTOCOL_H
#define PTP_PROTOCOL_H
#include "analyzer.h"
#include "../registers.h"
typedef struct { uint32_t magic; uint32_t sequence; uint16_t length; uint8_t version; uint8_t command; uint8_t payload[PTP_FRAME_MAX-12u]; } ptp_frame_t;
typedef struct { uint32_t last_sequence; uint32_t nonce; uint32_t lease_deadline_ms; bool armed; bool capture_active; } ptp_session_t;
void protocol_init(ptp_session_t *session);
uint32_t protocol_response(uint32_t nonce);
ptp_status_t protocol_process(ptp_session_t *session,ptp_capture_t *capture,ptp_analyzer_t *analyzer,const ptp_frame_t *frame,uint32_t now_ms);
#endif
