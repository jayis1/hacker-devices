/*
 * SmartPack Phantom export/radio interface
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef SMARTPACK_RADIO_H
#define SMARTPACK_RADIO_H

#include <stddef.h>
#include "../board.h"

void radio_init(sp_radio_state_t *radio);
void radio_emit_status(sp_runtime_t *runtime, char *buffer, size_t buffer_len);
void radio_emit_event_bundle(const sp_event_t *events,
                             size_t event_count,
                             sp_runtime_t *runtime,
                             char *buffer,
                             size_t buffer_len);

#endif
