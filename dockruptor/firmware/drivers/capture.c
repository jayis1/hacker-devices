/*
 * Dockruptor Capture Store
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <stdio.h>
#include <string.h>

#include "capture.h"

void dr_capture_init(dr_context_t *ctx)
{
    ctx->event_count = 0;
    memset(ctx->events, 0, sizeof(ctx->events));
}

void dr_capture_event(dr_context_t *ctx, dr_event_type_t type, dr_direction_t direction, const char *summary)
{
    dr_event_t *event = NULL;

    if (ctx->event_count < DR_MAX_EVENTS) {
        event = &ctx->events[ctx->event_count++];
    } else {
        memmove(&ctx->events[0], &ctx->events[1], sizeof(ctx->events[0]) * (DR_MAX_EVENTS - 1));
        event = &ctx->events[DR_MAX_EVENTS - 1];
    }

    event->tick = ctx->tick;
    event->type = type;
    event->direction = direction;
    snprintf(event->summary, sizeof(event->summary), "%s", summary);
}

static const char *event_type_name(dr_event_type_t type)
{
    switch (type) {
    case DR_EVENT_INFO:
        return "info";
    case DR_EVENT_PD_MESSAGE:
        return "pd";
    case DR_EVENT_POLICY:
        return "policy";
    case DR_EVENT_POWER:
        return "power";
    case DR_EVENT_SAFETY:
        return "safety";
    case DR_EVENT_COMMAND:
        return "command";
    default:
        return "unknown";
    }
}

static const char *direction_name(dr_direction_t direction)
{
    switch (direction) {
    case DR_DIR_HOST_TO_DOCK:
        return "host>dck";
    case DR_DIR_DOCK_TO_HOST:
        return "dock>hst";
    case DR_DIR_INTERNAL:
        return "internal";
    default:
        return "?";
    }
}

void dr_capture_render_recent(const dr_context_t *ctx, char *buffer, size_t buffer_len, size_t max_items)
{
    size_t start = 0;
    size_t i = 0;
    size_t used = 0;

    if (buffer_len == 0U) {
        return;
    }

    buffer[0] = '\0';

    if (ctx->event_count > max_items) {
        start = ctx->event_count - max_items;
    }

    for (i = start; i < ctx->event_count; ++i) {
        const dr_event_t *event = &ctx->events[i];
        int written = snprintf(
            buffer + used,
            buffer_len - used,
            "[%04u] %-7s %-8s %s\n",
            (unsigned)event->tick,
            event_type_name(event->type),
            direction_name(event->direction),
            event->summary);

        if (written < 0 || (size_t)written >= buffer_len - used) {
            break;
        }
        used += (size_t)written;
    }
}
