/*
 * espi_bus.c - channel model for eSPI Revenant
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <string.h>

#include "espi_bus.h"
#include "../registers.h"

static void fill_frame(er_trace_frame_t *frame,
                       uint32_t ts,
                       er_channel_t channel,
                       uint8_t opcode,
                       uint8_t tag,
                       const uint8_t *data,
                       uint8_t len)
{
    frame->timestamp_ms = ts;
    frame->channel = channel;
    frame->opcode = opcode;
    frame->tag = tag;
    frame->length = len;
    memset(frame->data, 0, sizeof(frame->data));
    if (data != NULL && len > 0u) {
        if (len > sizeof(frame->data)) {
            len = (uint8_t)sizeof(frame->data);
            frame->length = len;
        }
        memcpy(frame->data, data, len);
    }
}

void er_bus_init(er_bus_t *bus)
{
    memset(bus, 0, sizeof(*bus));
    strcpy(bus->profile.name, "baseline-passive");
    bus->profile.passive_prearm = 1u;
    bus->target_awake = 1u;
}

void er_bus_load_profile(er_bus_t *bus, const er_profile_t *profile)
{
    bus->profile = *profile;
    bus->flash_delay_ns = profile->flash_delay_ns;
    bus->profile_step = 0u;
    bus->maintenance_hint = 0u;
}

void er_bus_force_resume_window(er_bus_t *bus)
{
    bus->vwire_state = ER_VWIRE_SLP_S3;
    bus->target_awake = 0u;
    bus->profile_step = 0u;
}

void er_bus_apply_vwire_pulse(er_bus_t *bus, uint8_t mask)
{
    bus->vwire_state ^= mask;
}

void er_bus_set_flash_delay(er_bus_t *bus, uint32_t delay_ns)
{
    if (delay_ns > ER_FLASH_DELAY_MAX_NS) {
        delay_ns = ER_FLASH_DELAY_MAX_NS;
    }
    bus->flash_delay_ns = delay_ns;
}

static void build_vwire_frame(er_bus_t *bus, er_trace_frame_t *frame)
{
    uint8_t data[3];
    if (bus->target_awake == 0u) {
        bus->vwire_state &= (uint8_t)~ER_VWIRE_SLP_S3;
        bus->target_awake = 1u;
    } else if ((bus->tick_ms % 40u) == 0u) {
        bus->vwire_state ^= ER_VWIRE_SUS_WARN;
    }

    data[0] = bus->vwire_state;
    data[1] = (uint8_t)(bus->target_awake ? 1u : 0u);
    data[2] = (uint8_t)bus->profile_step;
    fill_frame(frame, bus->tick_ms, ER_CH_VWIRE, 0x10u, 0x01u, data, 3u);
}

static void build_periph_frame(er_bus_t *bus, er_trace_frame_t *frame)
{
    uint8_t data[5];
    data[0] = (uint8_t)(0x60u + (bus->profile_step % 4u));
    data[1] = (uint8_t)(bus->maintenance_hint ? 0xA5u : 0x55u);
    data[2] = (uint8_t)(0x10u + bus->profile_step);
    data[3] = (uint8_t)(bus->target_awake ? 0x01u : 0x00u);
    data[4] = (uint8_t)(bus->flash_delay_ns & 0xFFu);
    fill_frame(frame, bus->tick_ms, ER_CH_PERIPHERAL, 0x22u, 0x02u, data, 5u);
}

static void build_oob_frame(er_bus_t *bus, er_trace_frame_t *frame)
{
    uint8_t data[4];
    data[0] = 0xEEu;
    data[1] = (uint8_t)(bus->profile.allow_vwire_inject ? 1u : 0u);
    data[2] = (uint8_t)(bus->profile.allow_flash_delay ? 1u : 0u);
    data[3] = (uint8_t)(bus->profile.allow_peripheral_replay ? 1u : 0u);
    fill_frame(frame, bus->tick_ms, ER_CH_OOB, 0x31u, 0x03u, data, 4u);
}

static void build_flash_frame(er_bus_t *bus, er_trace_frame_t *frame)
{
    uint8_t data[6];
    bus->flash_window_open = (uint8_t)(((bus->tick_ms / 5u) % 2u) ? 1u : 0u);
    data[0] = 0x9Fu;
    data[1] = (uint8_t)(bus->flash_delay_ns & 0xFFu);
    data[2] = (uint8_t)((bus->flash_delay_ns >> 8u) & 0xFFu);
    data[3] = bus->flash_window_open;
    data[4] = (uint8_t)(0x40u + (bus->profile_step & 0x0Fu));
    data[5] = (uint8_t)(bus->maintenance_hint ? 0x1u : 0x0u);
    fill_frame(frame, bus->tick_ms, ER_CH_FLASH, 0x40u, 0x04u, data, 6u);
}

void er_bus_step(er_bus_t *bus, er_trace_frame_t *frame_out)
{
    bus->tick_ms += ER_TICK_MS;
    switch ((bus->tick_ms / ER_TICK_MS) % 4u) {
        case 0u:
            build_vwire_frame(bus, frame_out);
            break;
        case 1u:
            build_periph_frame(bus, frame_out);
            break;
        case 2u:
            build_oob_frame(bus, frame_out);
            break;
        default:
            build_flash_frame(bus, frame_out);
            break;
    }

    if ((bus->tick_ms % 25u) == 0u) {
        bus->profile_step++;
    }
    if (bus->profile.trigger_on_resume && bus->profile_step == 3u) {
        bus->maintenance_hint = 1u;
    }
}

const char *er_bus_channel_name(er_channel_t channel)
{
    switch (channel) {
        case ER_CH_VWIRE:
            return "vwire";
        case ER_CH_PERIPHERAL:
            return "peripheral";
        case ER_CH_OOB:
            return "oob";
        case ER_CH_FLASH:
            return "flash";
        case ER_CH_GPIO:
            return "gpio";
        default:
            return "unknown";
    }
}

void er_bus_describe_frame(const er_trace_frame_t *frame, char *buffer, size_t buffer_len)
{
    if (buffer_len == 0u) {
        return;
    }

    switch (frame->channel) {
        case ER_CH_VWIRE:
            snprintf(buffer,
                     buffer_len,
                     "VWIRE opcode=0x%02X state=0x%02X awake=%u step=%u",
                     frame->opcode,
                     frame->data[0],
                     frame->data[1],
                     frame->data[2]);
            break;
        case ER_CH_PERIPHERAL:
            snprintf(buffer,
                     buffer_len,
                     "PERIPH port=0x%02X val=0x%02X seq=%u awake=%u",
                     frame->data[0],
                     frame->data[1],
                     frame->data[2],
                     frame->data[3]);
            break;
        case ER_CH_OOB:
            snprintf(buffer,
                     buffer_len,
                     "OOB caps[v=%u f=%u p=%u] tag=%u",
                     frame->data[1],
                     frame->data[2],
                     frame->data[3],
                     frame->tag);
            break;
        case ER_CH_FLASH:
            snprintf(buffer,
                     buffer_len,
                     "FLASH cmd=0x%02X delay=%u ns window=%u phase=%u",
                     frame->data[0],
                     (unsigned)(frame->data[1] | ((uint16_t)frame->data[2] << 8u)),
                     frame->data[3],
                     frame->data[4]);
            break;
        default:
            snprintf(buffer, buffer_len, "GPIO/CTRL len=%u", frame->length);
            break;
    }
}
