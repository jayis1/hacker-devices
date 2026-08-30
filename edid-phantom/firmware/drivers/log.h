/*
 * log.h - EDID Phantom event log interface
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef EDID_PHANTOM_LOG_H
#define EDID_PHANTOM_LOG_H

#include "../board.h"

void log_init(void);
void log_event(ep_event_code_t code, ep_risk_t risk, const char *fmt, ...);
size_t log_count(void);
const ep_event_t *log_get(size_t index);
void log_dump(void);

#endif
