/*
 * log.c - EDID Phantom event log implementation
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static ep_event_t g_events[EP_MAX_EVENTS];
static size_t g_count;
static uint32_t g_last_timestamp_ms;

void log_init(void) {
    memset(g_events, 0, sizeof(g_events));
    g_count = 0;
    g_last_timestamp_ms = 0;
}

static void log_store(uint32_t timestamp_ms, ep_event_code_t code, ep_risk_t risk, const char *message) {
    size_t index = g_count;
    if (index >= EP_MAX_EVENTS) {
        memmove(&g_events[0], &g_events[1], sizeof(g_events[0]) * (EP_MAX_EVENTS - 1u));
        index = EP_MAX_EVENTS - 1u;
    } else {
        g_count++;
    }

    g_events[index].timestamp_ms = timestamp_ms;
    g_events[index].code = code;
    g_events[index].risk = risk;
    snprintf(g_events[index].message, sizeof(g_events[index].message), "%s", message);
}

void log_event(ep_event_code_t code, ep_risk_t risk, const char *fmt, ...) {
    char message[EP_MAX_EVENT_MESSAGE];
    va_list args;
    g_last_timestamp_ms += EP_TICK_MS;

    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    log_store(g_last_timestamp_ms, code, risk, message);
}

size_t log_count(void) {
    return g_count;
}

const ep_event_t *log_get(size_t index) {
    if (index >= g_count) {
        return NULL;
    }
    return &g_events[index];
}

void log_dump(void) {
    size_t index;
    printf("---- Event Log (%zu events) ----\n", g_count);
    for (index = 0; index < g_count; ++index) {
        printf("[%06u] code=%u risk=%u %s\n",
               g_events[index].timestamp_ms,
               (unsigned)g_events[index].code,
               (unsigned)g_events[index].risk,
               g_events[index].message);
    }
}
