/*
 * DP AUX Phantom capture subsystem
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "capture.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    uint8_t type;
    char message[DPA_TEXT_128];
    dpa_aux_transaction_t tx;
    bool has_tx;
} dpa_capture_event_t;

static dpa_capture_event_t g_events[DPA_MAX_EVENTS];
static size_t g_event_count = 0U;

static const char *type_name(uint8_t type)
{
    switch (type) {
    case DPA_EVENT_INFO:
        return "info";
    case DPA_EVENT_RULE_HIT:
        return "rule";
    case DPA_EVENT_ALERT:
        return "alert";
    case DPA_EVENT_CAPTURE:
        return "capture";
    default:
        return "unknown";
    }
}

void capture_init(void)
{
    memset(g_events, 0, sizeof(g_events));
    g_event_count = 0U;
}

void capture_log(uint8_t type, const char *message, const dpa_aux_transaction_t *tx)
{
    dpa_capture_event_t *event = NULL;

    if (g_event_count >= DPA_MAX_EVENTS) {
        return;
    }

    event = &g_events[g_event_count++];
    event->type = type;
    if (message != NULL) {
        snprintf(event->message, sizeof(event->message), "%s", message);
    }
    if (tx != NULL) {
        event->tx = *tx;
        event->has_tx = true;
    }
}

size_t capture_count(void)
{
    return g_event_count;
}

size_t capture_type_count(uint8_t type)
{
    size_t i = 0U;
    size_t count = 0U;

    for (i = 0U; i < g_event_count; ++i) {
        if (g_events[i].type == type) {
            ++count;
        }
    }
    return count;
}

static void print_tx(const dpa_aux_transaction_t *tx)
{
    size_t i = 0U;
    printf("addr=0x%05X op=0x%X len=%u injected=%s error=%s data=",
           tx->address,
           tx->op,
           tx->length,
           tx->injected ? "yes" : "no",
           tx->error ? "yes" : "no");
    for (i = 0U; i < tx->length; ++i) {
        printf("%02X", tx->data[i]);
        if ((i + 1U) < tx->length) {
            putchar(':');
        }
    }
    printf(" note=%s", tx->note);
}

void capture_dump(void)
{
    size_t i = 0U;

    puts("-- DP AUX Phantom capture log --");
    for (i = 0U; i < g_event_count; ++i) {
        printf("[%03zu] type=%s message=%s",
               i,
               type_name(g_events[i].type),
               g_events[i].message);
        if (g_events[i].has_tx) {
            putchar(' ');
            print_tx(&g_events[i].tx);
        }
        putchar('\n');
    }
}
