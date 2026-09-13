/*
 * Timing-aware passive 1-Wire decoder and device cartographer
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#include "analyzer.h"
#include <string.h>

#define FLAG_OUT_OF_RANGE 0x01u
#define FLAG_CRC_OK       0x02u

static uint8_t queue_next(uint8_t index)
{
    return (uint8_t)((index + 1u) % OWC_EVENT_QUEUE_DEPTH);
}

static void enqueue(owc_analyzer_t *analyzer,
                    owc_event_type_t type,
                    const owc_edge_t *edge,
                    uint32_t duration,
                    uint8_t value,
                    uint8_t flags)
{
    uint8_t next;
    owc_event_t *event;

    next = queue_next(analyzer->event_head);
    if (next == analyzer->event_tail) {
        analyzer->event_tail = queue_next(analyzer->event_tail);
        analyzer->anomalies++;
    }
    event = &analyzer->events[analyzer->event_head];
    event->type = type;
    event->timestamp_us = edge->timestamp_us;
    event->duration_us = duration;
    event->voltage_mv = edge->voltage_mv;
    event->port = edge->port;
    event->value = value;
    event->flags = flags;
    analyzer->event_head = next;
}

void owc_analyzer_init(owc_analyzer_t *analyzer)
{
    if (analyzer != NULL) {
        memset(analyzer, 0, sizeof(*analyzer));
    }
}

static void complete_byte(owc_analyzer_t *analyzer, const owc_edge_t *edge)
{
    enqueue(analyzer, OWC_EVT_BYTE, edge, 0u,
            analyzer->byte_accumulator, 0u);
    analyzer->bit_count = 0u;
    analyzer->byte_accumulator = 0u;
}

static void consume_pulse(owc_analyzer_t *analyzer,
                          const owc_edge_t *edge,
                          uint32_t duration)
{
    uint8_t flags = 0u;
    uint8_t bit_value;

    if (duration >= OWC_RESET_MIN_US && duration <= OWC_RESET_MAX_US) {
        enqueue(analyzer, OWC_EVT_RESET, edge, duration, 0u, 0u);
        analyzer->resets++;
        analyzer->bit_count = 0u;
        analyzer->byte_accumulator = 0u;
        return;
    }
    if (duration > OWC_RESET_MAX_US) {
        enqueue(analyzer, OWC_EVT_TIMING_ANOMALY, edge, duration, 0u,
                FLAG_OUT_OF_RANGE);
        analyzer->anomalies++;
        analyzer->bit_count = 0u;
        return;
    }
    if (duration < 1u || duration > OWC_SLOT_MAX_US) {
        enqueue(analyzer, OWC_EVT_TIMING_ANOMALY, edge, duration, 0u,
                FLAG_OUT_OF_RANGE);
        analyzer->anomalies++;
        return;
    }
    bit_value = duration < 15u ? 1u : 0u;
    if (duration > 15u && duration < OWC_SLOT_MIN_US) {
        flags |= FLAG_OUT_OF_RANGE;
        analyzer->anomalies++;
    }
    enqueue(analyzer, OWC_EVT_BIT, edge, duration, bit_value, flags);
    analyzer->slots++;
    analyzer->byte_accumulator |= (uint8_t)(bit_value << analyzer->bit_count);
    analyzer->bit_count++;
    if (analyzer->bit_count == 8u) {
        complete_byte(analyzer, edge);
    }
}

void owc_analyzer_consume(owc_analyzer_t *analyzer, const owc_edge_t *edge)
{
    uint32_t started;
    uint32_t duration;

    if (analyzer == NULL || edge == NULL || edge->port > 1u) {
        return;
    }
    if (edge->voltage_mv > OWC_OVER_VOLT_MV) {
        enqueue(analyzer, OWC_EVT_VOLTAGE_ANOMALY, edge, 0u, 0u,
                FLAG_OUT_OF_RANGE);
        analyzer->anomalies++;
    }
    if (edge->kind == OWC_EDGE_FALLING) {
        analyzer->falling_at[edge->port] = edge->timestamp_us;
        return;
    }
    started = analyzer->falling_at[edge->port];
    if (started == 0u || edge->timestamp_us < started) {
        enqueue(analyzer, OWC_EVT_TIMING_ANOMALY, edge, 0u, 0u,
                FLAG_OUT_OF_RANGE);
        analyzer->anomalies++;
        return;
    }
    duration = edge->timestamp_us - started;
    analyzer->falling_at[edge->port] = 0u;
    consume_pulse(analyzer, edge, duration);
}

bool owc_analyzer_next_event(owc_analyzer_t *analyzer, owc_event_t *event)
{
    if (analyzer == NULL || event == NULL ||
        analyzer->event_tail == analyzer->event_head) {
        return false;
    }
    *event = analyzer->events[analyzer->event_tail];
    analyzer->event_tail = queue_next(analyzer->event_tail);
    return true;
}

static int find_rom(const owc_analyzer_t *analyzer, const uint8_t rom[8])
{
    uint8_t index;

    for (index = 0u; index < analyzer->device_count; ++index) {
        if (memcmp(analyzer->devices[index].rom, rom, OWC_ROM_BYTES) == 0) {
            return (int)index;
        }
    }
    return -1;
}

bool owc_analyzer_observe_rom(owc_analyzer_t *analyzer,
                              const uint8_t rom[8],
                              uint16_t presence_us)
{
    int found;
    owc_device_t *device;
    uint32_t weighted;

    if (analyzer == NULL || rom == NULL) {
        return false;
    }
    found = find_rom(analyzer, rom);
    if (found < 0) {
        if (analyzer->device_count >= OWC_MAX_DEVICES) {
            return false;
        }
        found = analyzer->device_count++;
        device = &analyzer->devices[found];
        memset(device, 0, sizeof(*device));
        memcpy(device->rom, rom, OWC_ROM_BYTES);
        device->first_seen_ms = board_millis();
        device->mean_presence_us = presence_us;
    }
    device = &analyzer->devices[found];
    weighted = (uint32_t)device->mean_presence_us * device->observations;
    device->observations++;
    device->mean_presence_us = (uint16_t)((weighted + presence_us) /
                                         device->observations);
    device->last_seen_ms = board_millis();
    device->crc_valid = owc_crc8(rom, 7u) == rom[7];
    if (presence_us > device->mean_presence_us) {
        device->jitter_score = (uint16_t)(presence_us - device->mean_presence_us);
    } else {
        device->jitter_score = (uint16_t)(device->mean_presence_us - presence_us);
    }
    return true;
}

const owc_device_t *owc_analyzer_device(const owc_analyzer_t *analyzer,
                                        uint8_t index)
{
    if (analyzer == NULL || index >= analyzer->device_count) {
        return NULL;
    }
    return &analyzer->devices[index];
}

uint16_t owc_timing_fingerprint(const owc_analyzer_t *analyzer)
{
    uint32_t hash;
    uint8_t index;

    if (analyzer == NULL) {
        return 0u;
    }
    hash = 2166136261u;
    for (index = 0u; index < analyzer->device_count; ++index) {
        const owc_device_t *device = &analyzer->devices[index];
        hash ^= device->mean_presence_us;
        hash *= 16777619u;
        hash ^= device->jitter_score;
        hash *= 16777619u;
        hash ^= device->rom[0];
        hash *= 16777619u;
    }
    return (uint16_t)((hash >> 16u) ^ hash);
}
