/* OneWire Cartographer protocol analyzer API. Author: jayis1. MIT. */
#ifndef OWC_ANALYZER_H
#define OWC_ANALYZER_H
#include "onewire_phy.h"
#include "../registers.h"

typedef struct {
    owc_event_type_t type;
    uint32_t timestamp_us;
    uint32_t duration_us;
    uint16_t voltage_mv;
    uint8_t port;
    uint8_t value;
    uint8_t flags;
} owc_event_t;

typedef struct {
    uint8_t rom[OWC_ROM_BYTES];
    uint32_t first_seen_ms;
    uint32_t last_seen_ms;
    uint32_t observations;
    uint16_t mean_presence_us;
    uint16_t jitter_score;
    bool crc_valid;
} owc_device_t;

typedef struct {
    owc_event_t events[OWC_EVENT_QUEUE_DEPTH];
    owc_device_t devices[OWC_MAX_DEVICES];
    uint8_t event_head;
    uint8_t event_tail;
    uint8_t device_count;
    uint8_t bit_count;
    uint8_t byte_accumulator;
    uint32_t falling_at[2];
    uint32_t resets;
    uint32_t slots;
    uint32_t anomalies;
} owc_analyzer_t;

void owc_analyzer_init(owc_analyzer_t *analyzer);
void owc_analyzer_consume(owc_analyzer_t *analyzer, const owc_edge_t *edge);
bool owc_analyzer_next_event(owc_analyzer_t *analyzer, owc_event_t *event);
bool owc_analyzer_observe_rom(owc_analyzer_t *analyzer, const uint8_t rom[8], uint16_t presence_us);
const owc_device_t *owc_analyzer_device(const owc_analyzer_t *analyzer, uint8_t index);
uint16_t owc_timing_fingerprint(const owc_analyzer_t *analyzer);

#endif
