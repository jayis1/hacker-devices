/*
 * nac-mirage capture subsystem
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "capture.h"

#include <stdio.h>
#include <string.h>

static nm_capture_event_t g_events[NM_MAX_EVENTS];
static size_t g_event_count = 0U;
static uint32_t g_reason_histogram[4] = {0U, 0U, 0U, 0U};

void capture_init(void)
{
    memset(g_events, 0, sizeof(g_events));
    g_event_count = 0U;
    memset(g_reason_histogram, 0, sizeof(g_reason_histogram));
}

void capture_record(uint32_t timestamp_ms,
                    uint8_t port,
                    uint8_t reason,
                    uint16_t ethertype,
                    uint16_t length,
                    const char *summary)
{
    nm_capture_event_t *slot = NULL;

    if (g_event_count < NM_MAX_EVENTS) {
        slot = &g_events[g_event_count++];
    } else {
        memmove(&g_events[0], &g_events[1], sizeof(g_events[0]) * (NM_MAX_EVENTS - 1U));
        slot = &g_events[NM_MAX_EVENTS - 1U];
    }

    slot->timestamp_ms = timestamp_ms;
    slot->port = port;
    slot->reason = reason;
    slot->ethertype = ethertype;
    slot->length = length;
    (void)snprintf(slot->summary, sizeof(slot->summary), "%s", (summary != NULL) ? summary : "no-summary");

    if (reason < 4U) {
        g_reason_histogram[reason]++;
    }
}

size_t capture_count(void)
{
    return g_event_count;
}

const nm_capture_event_t *capture_get(size_t index)
{
    if (index >= g_event_count) {
        return NULL;
    }
    return &g_events[index];
}

uint32_t capture_reason_count(uint8_t reason)
{
    if (reason >= 4U) {
        return 0U;
    }
    return g_reason_histogram[reason];
}

void capture_dump(void)
{
    size_t index = 0U;
    puts("-- NAC Mirage capture log --");
    for (index = 0U; index < g_event_count; ++index) {
        const nm_capture_event_t *event = &g_events[index];
        printf("[%06u ms] port=%u reason=%u ethertype=0x%04X len=%u %s\n",
               event->timestamp_ms,
               event->port,
               event->reason,
               event->ethertype,
               event->length,
               event->summary);
    }
}
