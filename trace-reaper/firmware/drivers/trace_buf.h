/*
 * TRACE-REAPER — trace buffer driver header
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef TRACE_BUF_H
#define TRACE_BUF_H

#include <stdint.h>
#include "../board.h"

void trace_buf_init(const uint8_t session_id[SESSION_ID_LEN],
                     const uint8_t passkey[16]);
int  trace_buf_push(const int16_t *samples, uint16_t nsamp, uint32_t ts_ms);
int  trace_buf_get(uint32_t idx, int16_t *out, uint16_t *nsamp, uint32_t *ts_ms);
uint32_t trace_buf_count(void);
void trace_buf_wipe(void);

#endif /* TRACE_BUF_H */