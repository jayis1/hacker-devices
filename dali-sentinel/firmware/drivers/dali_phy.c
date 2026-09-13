/* DALI Sentinel physical/link layer
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include "dali_phy.h"
#include <string.h>

typedef struct {
    dali_edge_t edge[EDGE_RING_CAPACITY];
    volatile uint16_t write;
    volatile uint16_t read;
} edge_ring_t;

typedef struct {
    dali_frame_t frame[FRAME_RING_CAPACITY];
    uint8_t write;
    uint8_t read;
} frame_ring_t;

typedef struct {
    bool active;
    bool last_level;
    uint32_t start_tick;
    uint32_t last_tick;
    uint32_t raw;
    uint16_t minimum_mv;
    uint8_t bits;
    uint16_t quality;
    uint8_t collision;
    uint8_t half_phase;
} decoder_t;

typedef struct {
    bool pending;
    bus_side_t side;
    uint32_t raw;
    uint32_t expiry_ms;
    uint32_t next_tick;
    uint8_t bits;
    uint8_t repeats_left;
    uint8_t half_index;
    bool high_impact;
} transmitter_t;

static edge_ring_t edges;
static frame_ring_t frames;
static decoder_t decoders[2];
static transmitter_t tx;
static dali_safety_status_t safety;
static uint32_t last_arm_epoch;

static uint16_t next_edge(uint16_t value)
{
    return (uint16_t)((value + 1u) % EDGE_RING_CAPACITY);
}

static uint8_t next_frame(uint8_t value)
{
    return (uint8_t)((value + 1u) % FRAME_RING_CAPACITY);
}

static bool elapsed(uint32_t now, uint32_t deadline)
{
    return (int32_t)(now - deadline) >= 0;
}

static void push_frame(const dali_frame_t *frame)
{
    uint8_t next = next_frame(frames.write);
    if (next == frames.read) {
        safety.frame_overflows++;
        return;
    }
    frames.frame[frames.write] = *frame;
    frames.write = next;
}

static void decoder_reset(decoder_t *decoder)
{
    memset(decoder, 0, sizeof(*decoder));
    decoder->minimum_mv = UINT16_MAX;
}

static bool is_half(uint32_t ticks)
{
    return ticks >= DALI_HALF_MIN_TICKS && ticks <= DALI_HALF_MAX_TICKS;
}

static bool is_full(uint32_t ticks)
{
    return ticks >= DALI_FULL_MIN_TICKS && ticks <= DALI_FULL_MAX_TICKS;
}

static uint8_t timing_quality(uint32_t ticks, uint32_t expected)
{
    uint32_t error = ticks > expected ? ticks - expected : expected - ticks;
    uint32_t percentage = (error * 100u) / expected;
    if (percentage <= 5u) return 100u;
    if (percentage >= 35u) return 0u;
    return (uint8_t)(100u - (percentage * 2u));
}

static void finish_frame(decoder_t *decoder, bus_side_t side, uint32_t end_tick)
{
    if (!decoder->active) return;
    dali_frame_t frame;
    frame.start_tick = decoder->start_tick;
    frame.duration_ticks = end_tick - decoder->start_tick;
    frame.raw = decoder->raw;
    frame.minimum_mv = decoder->minimum_mv;
    frame.bit_count = decoder->bits;
    frame.quality = decoder->bits ? (uint8_t)(decoder->quality / decoder->bits) : 0u;
    frame.collision_score = decoder->collision;
    frame.side = side;
    frame.framing_ok = (decoder->bits == 8u || decoder->bits == 16u || decoder->bits == 24u) &&
                       frame.quality >= 55u && decoder->collision < 4u;
    if (decoder->bits > 0u) push_frame(&frame);
    decoder_reset(decoder);
}

static void append_bit(decoder_t *decoder, bool bit, uint8_t quality)
{
    if (decoder->bits >= 24u) {
        decoder->collision++;
        return;
    }
    decoder->raw = (decoder->raw << 1u) | (bit ? 1u : 0u);
    decoder->bits++;
    decoder->quality = (uint16_t)(decoder->quality + quality);
}

static void consume_edge(const dali_edge_t *edge)
{
    if (edge->side != SIDE_CONTROLLER && edge->side != SIDE_GEAR) return;
    decoder_t *decoder = &decoders[(unsigned)edge->side];
    if (!decoder->active) {
        if (!edge->level_high) {
            decoder->active = true;
            decoder->start_tick = edge->tick;
            decoder->last_tick = edge->tick;
            decoder->last_level = edge->level_high;
            decoder->minimum_mv = edge->bus_mv;
            decoder->half_phase = 0u;
        }
        return;
    }
    uint32_t delta = edge->tick - decoder->last_tick;
    if (edge->bus_mv < decoder->minimum_mv) decoder->minimum_mv = edge->bus_mv;
    if (delta > DALI_FULL_MAX_TICKS * 3u) {
        finish_frame(decoder, edge->side, decoder->last_tick);
        if (!edge->level_high) consume_edge(edge);
        return;
    }
    if (is_half(delta)) {
        uint8_t q = timing_quality(delta, DALI_HALF_BIT_TICKS);
        decoder->half_phase ^= 1u;
        if (decoder->half_phase == 0u) append_bit(decoder, edge->level_high, q);
    } else if (is_full(delta)) {
        uint8_t q = timing_quality(delta, DALI_HALF_BIT_TICKS * 2u);
        append_bit(decoder, edge->level_high, q);
        decoder->half_phase = 0u;
    } else {
        decoder->collision++;
        if (delta < DALI_HALF_MIN_TICKS / 2u) {
            decoder->quality = decoder->quality > 20u ? (uint16_t)(decoder->quality - 20u) : 0u;
        }
    }
    decoder->last_tick = edge->tick;
    decoder->last_level = edge->level_high;
}

void dali_phy_init(void)
{
    memset(&edges, 0, sizeof(edges));
    memset(&frames, 0, sizeof(frames));
    memset(&tx, 0, sizeof(tx));
    memset(&safety, 0, sizeof(safety));
    decoder_reset(&decoders[0]);
    decoder_reset(&decoders[1]);
    safety.mode = MODE_OBSERVE;
    safety.bypass_commanded_closed = true;
    board_set_sink(SIDE_CONTROLLER, false);
    board_set_sink(SIDE_GEAR, false);
    board_set_bypass(true);
}

void dali_phy_capture_edge(bus_side_t side, bool high, uint32_t tick, uint16_t bus_mv)
{
    uint16_t write = edges.write;
    uint16_t next = next_edge(write);
    if (next == edges.read) {
        safety.edge_overflows++;
        return;
    }
    edges.edge[write].tick = tick;
    edges.edge[write].side = side;
    edges.edge[write].level_high = high;
    edges.edge[write].bus_mv = bus_mv;
    edges.write = next;
}

void dali_phy_poll(uint32_t now_tick)
{
    while (edges.read != edges.write) {
        dali_edge_t edge = edges.edge[edges.read];
        edges.read = next_edge(edges.read);
        consume_edge(&edge);
    }
    for (unsigned side = 0; side < 2u; ++side) {
        decoder_t *decoder = &decoders[side];
        if (decoder->active && now_tick - decoder->last_tick > DALI_FULL_MAX_TICKS * 3u) {
            finish_frame(decoder, (bus_side_t)side, now_tick);
        }
    }
}

phy_result_t dali_phy_next_frame(dali_frame_t *frame)
{
    if (frame == NULL) return PHY_INVALID;
    if (frames.read == frames.write) return PHY_EMPTY;
    *frame = frames.frame[frames.read];
    frames.read = next_frame(frames.read);
    return PHY_OK;
}

static bool electrical_inputs_safe(const board_inputs_t *inputs)
{
    if (inputs == NULL) return false;
    if (inputs->temperature_c > BOARD_TEMP_MAX_C) return false;
    for (unsigned side = 0; side < 2u; ++side) {
        if (inputs->bus_mv[side] > BUS_OVERVOLT_MV) return false;
        if (inputs->sink_ma[side] > SINK_CURRENT_MAX_MA) return false;
    }
    return true;
}

phy_result_t dali_phy_request_mode(operating_mode_t mode, const board_inputs_t *inputs)
{
    if (!electrical_inputs_safe(inputs)) {
        dali_phy_emergency_stop("unsafe electrical state");
        return PHY_DENIED;
    }
    if (mode == MODE_OBSERVE || mode == MODE_INLINE) {
        board_set_sink(SIDE_CONTROLLER, false);
        board_set_sink(SIDE_GEAR, false);
        safety.sink_enabled[0] = false;
        safety.sink_enabled[1] = false;
        board_set_bypass(true);
        safety.bypass_commanded_closed = true;
        safety.mode = mode;
        tx.pending = false;
        return PHY_OK;
    }
    if (mode != MODE_LAB_ISOLATE) return PHY_INVALID;
    if (!inputs->authorization_jumper || !inputs->arm_pressed) return PHY_DENIED;
    if (!inputs->bypass_closed_feedback) return PHY_DENIED;
    if (!inputs->isolated_power_good[0] || !inputs->isolated_power_good[1]) return PHY_DENIED;
    if (inputs->bus_mv[0] < BUS_HIGH_MIN_MV || inputs->bus_mv[1] < BUS_HIGH_MIN_MV) return PHY_DENIED;
    if (inputs->now_ms == last_arm_epoch) return PHY_DENIED;
    last_arm_epoch = inputs->now_ms;
    safety.arm_epoch = inputs->now_ms;
    safety.armed_latched = true;
    safety.active_deadline_ms = inputs->now_ms + ACTIVE_SESSION_MAX_MS;
    board_set_bypass(false);
    safety.bypass_commanded_closed = false;
    safety.mode = MODE_LAB_ISOLATE;
    return PHY_OK;
}

phy_result_t dali_phy_schedule(bus_side_t side, uint32_t raw, uint8_t bits,
                               uint8_t repeats, uint32_t expiry_ms, bool high_impact)
{
    if (safety.mode != MODE_LAB_ISOLATE || !safety.armed_latched) return PHY_DENIED;
    if (side != SIDE_CONTROLLER && side != SIDE_GEAR) return PHY_INVALID;
    if (bits != 8u && bits != 16u && bits != 24u) return PHY_INVALID;
    if (repeats == 0u || repeats > TX_REPEAT_MAX) return PHY_INVALID;
    if (tx.pending) return PHY_BUSY;
    if (high_impact && repeats > 2u) return PHY_DENIED;
    tx.pending = true;
    tx.side = side;
    tx.raw = raw;
    tx.bits = bits;
    tx.repeats_left = repeats;
    tx.expiry_ms = expiry_ms;
    tx.next_tick = board_capture_ticks();
    tx.half_index = 0u;
    tx.high_impact = high_impact;
    return PHY_OK;
}

static bool tx_half_level(const transmitter_t *item)
{
    if (item->half_index < 2u) return item->half_index == 1u;
    unsigned data_half = (unsigned)item->half_index - 2u;
    unsigned bit_index = data_half / 2u;
    if (bit_index >= item->bits) return true;
    bool bit = ((item->raw >> (item->bits - 1u - bit_index)) & 1u) != 0u;
    bool second_half = (data_half & 1u) != 0u;
    return second_half ? bit : !bit;
}

void dali_phy_service_tx(uint32_t now_tick, uint32_t now_ms)
{
    board_inputs_t inputs = board_inputs();
    if (safety.mode == MODE_LAB_ISOLATE) {
        if (elapsed(now_ms, safety.active_deadline_ms) || !electrical_inputs_safe(&inputs) ||
            !inputs.authorization_jumper || inputs.bypass_closed_feedback) {
            dali_phy_emergency_stop("active safety condition");
            return;
        }
    }
    if (!tx.pending) return;
    if (elapsed(now_ms, tx.expiry_ms)) {
        tx.pending = false;
        board_set_sink(tx.side, false);
        safety.sink_enabled[(unsigned)tx.side] = false;
        return;
    }
    if (!elapsed(now_tick, tx.next_tick)) return;
    bool line_high = tx_half_level(&tx);
    board_set_sink(tx.side, !line_high);
    safety.sink_enabled[(unsigned)tx.side] = !line_high;
    tx.half_index++;
    tx.next_tick += DALI_HALF_BIT_TICKS;
    uint8_t total_halves = (uint8_t)(2u + tx.bits * 2u + 4u);
    if (tx.half_index >= total_halves) {
        board_set_sink(tx.side, false);
        safety.sink_enabled[(unsigned)tx.side] = false;
        tx.half_index = 0u;
        if (--tx.repeats_left == 0u) tx.pending = false;
        else tx.next_tick += DALI_HALF_BIT_TICKS * 20u;
    }
}

void dali_phy_emergency_stop(const char *reason)
{
    board_set_sink(SIDE_CONTROLLER, false);
    board_set_sink(SIDE_GEAR, false);
    safety.sink_enabled[0] = false;
    safety.sink_enabled[1] = false;
    tx.pending = false;
    board_set_bypass(true);
    safety.bypass_commanded_closed = true;
    safety.armed_latched = false;
    safety.mode = MODE_FAULT;
    board_set_status(255u, 0u, 0u);
    board_log(reason);
}

dali_safety_status_t dali_phy_status(void)
{
    return safety;
}

bool dali_phy_run_self_test(void)
{
    dali_phy_init();
    board_inputs_t safe = {0};
    safe.now_ms = 100u;
    safe.bus_mv[0] = 16000u;
    safe.bus_mv[1] = 16000u;
    safe.temperature_c = 25;
    safe.arm_pressed = true;
    safe.authorization_jumper = true;
    safe.bypass_closed_feedback = true;
    safe.isolated_power_good[0] = true;
    safe.isolated_power_good[1] = true;
    if (dali_phy_request_mode(MODE_LAB_ISOLATE, &safe) != PHY_OK) return false;
    if (dali_phy_schedule(SIDE_GEAR, 0xFF00u, 16u, 1u, 1000u, true) != PHY_OK) return false;
    safe.temperature_c = 90;
    if (dali_phy_request_mode(MODE_LAB_ISOLATE, &safe) != PHY_DENIED) return false;
    if (dali_phy_status().mode != MODE_FAULT) return false;
    dali_phy_init();
    if (dali_phy_request_mode(MODE_OBSERVE, &safe) != PHY_DENIED) return false;
    return true;
}
