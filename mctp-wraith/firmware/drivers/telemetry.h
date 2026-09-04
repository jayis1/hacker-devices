/*
 * telemetry.h
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef MCTP_WRAITH_TELEMETRY_H
#define MCTP_WRAITH_TELEMETRY_H

#include <stdint.h>

void telemetry_init(void);
void telemetry_tick(void);
void telemetry_note_capture(void);
void telemetry_note_mutation(void);
void telemetry_note_drop(void);
void telemetry_note_alert(void);
void telemetry_note_signed_bundle(void);
void telemetry_print(void);

#endif
