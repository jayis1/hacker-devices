/*
 * capture subsystem
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef NAC_MIRAGE_CAPTURE_H
#define NAC_MIRAGE_CAPTURE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../board.h"

typedef struct {
    uint32_t timestamp_ms;
    uint8_t port;
    uint8_t reason;
    uint16_t ethertype;
    uint16_t length;
    char summary[NM_TEXT_128];
} nm_capture_event_t;

void capture_init(void);
void capture_record(uint32_t timestamp_ms,
                    uint8_t port,
                    uint8_t reason,
                    uint16_t ethertype,
                    uint16_t length,
                    const char *summary);
size_t capture_count(void);
const nm_capture_event_t *capture_get(size_t index);
void capture_dump(void);
uint32_t capture_reason_count(uint8_t reason);

#endif
