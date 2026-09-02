/*
 * radio.c - I3C Poltergeist operator-link helpers
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <string.h>

#include "radio.h"
#include "i3c_bus.h"

void ip_radio_init(ip_system_t *sys)
{
    if (sys == NULL) {
        return;
    }

    sys->status.link_state[IP_LINK_USB] = 1u;
    sys->status.link_state[IP_LINK_BLE] = 1u;
    sys->status.link_state[IP_LINK_WIFI] = 0u;
}

void ip_radio_heartbeat(const ip_system_t *sys, char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0u) {
        return;
    }

    if (sys == NULL) {
        (void)snprintf(buffer, buffer_size, "heartbeat=offline");
        return;
    }

    (void)snprintf(buffer,
                   buffer_size,
                   "heartbeat device=%s author=%s usb=%u ble=%u wifi=%u mode=%u captures=%u anomalies=%u",
                   IP_DEVICE_NAME,
                   IP_AUTHOR,
                   (unsigned int)sys->status.link_state[IP_LINK_USB],
                   (unsigned int)sys->status.link_state[IP_LINK_BLE],
                   (unsigned int)sys->status.link_state[IP_LINK_WIFI],
                   (unsigned int)sys->status.mode,
                   (unsigned int)sys->capture_count,
                   (unsigned int)sys->status.anomalies);
}

void ip_radio_export_frame(const ip_system_t *sys, char *buffer, size_t buffer_size, size_t event_limit)
{
    size_t i;
    size_t used;

    if (buffer == NULL || buffer_size == 0u) {
        return;
    }

    if (sys == NULL) {
        (void)snprintf(buffer, buffer_size, "{}");
        return;
    }

    used = (size_t)snprintf(buffer,
                            buffer_size,
                            "{\"device\":\"%s\",\"author\":\"%s\",\"events\":[",
                            IP_DEVICE_NAME,
                            IP_AUTHOR);

    for (i = 0u; i < sys->event_count && i < event_limit && used + 16u < buffer_size; ++i) {
        int written = snprintf(buffer + used,
                               buffer_size - used,
                               "%s{\"t\":%u,\"code\":%u,\"msg\":\"%s\"}",
                               i == 0u ? "" : ",",
                               (unsigned int)sys->events[i].timestamp_ms,
                               (unsigned int)sys->events[i].code,
                               sys->events[i].message);
        if (written < 0) {
            break;
        }
        used += (size_t)written;
    }

    if (used + 128u < buffer_size) {
        char capture_summary[256];
        ip_i3c_describe_capture(sys, capture_summary, sizeof(capture_summary));
        (void)snprintf(buffer + used,
                       buffer_size - used,
                       "],\"capture_summary\":\"%s\",\"target_count\":%u}",
                       capture_summary,
                       (unsigned int)sys->target_count);
    } else {
        buffer[buffer_size - 1u] = '\0';
    }
}
