/*
 * Dual-segment 1-Wire physical-layer capture and controlled drive
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#include "onewire_phy.h"
#include <string.h>

static unsigned sense_pin(owc_port_t port)
{
    return port == OWC_PORT_UPSTREAM ? PIN_UPSTREAM_SENSE : PIN_DOWNSTREAM_SENSE;
}

static unsigned pull_pin(owc_port_t port)
{
    return port == OWC_PORT_UPSTREAM ? PIN_UPSTREAM_PULL : PIN_DOWNSTREAM_PULL;
}

static uint16_t ring_next(uint16_t index)
{
    return (uint16_t)((index + 1u) % OWC_CAPTURE_DEPTH);
}

void owc_phy_init(owc_phy_t *phy)
{
    if (phy == NULL) {
        return;
    }
    memset(phy, 0, sizeof(*phy));
    phy->low_threshold_mv = OWC_ADC_LOW_MV;
    phy->high_threshold_mv = OWC_ADC_HIGH_MV;
    owc_phy_release_all();
}

void owc_phy_start(owc_phy_t *phy)
{
    if (phy == NULL || phy->faulted) {
        return;
    }
    phy->read_index = 0u;
    phy->write_index = 0u;
    phy->dropped = 0u;
    phy->capturing = true;
}

void owc_phy_stop(owc_phy_t *phy)
{
    if (phy != NULL) {
        phy->capturing = false;
    }
}

void owc_phy_record_edge(owc_phy_t *phy,
                         owc_port_t port,
                         owc_edge_kind_t kind,
                         uint16_t mv,
                         uint32_t timestamp)
{
    uint16_t next;
    owc_edge_t *destination;

    if (phy == NULL || !phy->capturing || port > OWC_PORT_DOWNSTREAM) {
        return;
    }
    next = ring_next(phy->write_index);
    if (next == phy->read_index) {
        phy->dropped++;
        return;
    }
    destination = &phy->edges[phy->write_index];
    destination->timestamp_us = timestamp;
    destination->voltage_mv = mv;
    destination->port = (uint8_t)port;
    destination->kind = (uint8_t)kind;
    phy->write_index = next;
}

bool owc_phy_next_edge(owc_phy_t *phy, owc_edge_t *edge)
{
    if (phy == NULL || edge == NULL || phy->read_index == phy->write_index) {
        return false;
    }
    *edge = phy->edges[phy->read_index];
    phy->read_index = ring_next(phy->read_index);
    return true;
}

bool owc_phy_set_thresholds(owc_phy_t *phy, uint16_t low_mv, uint16_t high_mv)
{
    if (phy == NULL || low_mv < 200u || high_mv > 5500u) {
        return false;
    }
    if ((uint16_t)(low_mv + 400u) >= high_mv) {
        return false;
    }
    phy->low_threshold_mv = low_mv;
    phy->high_threshold_mv = high_mv;
    return true;
}

bool owc_phy_bus_safe(owc_port_t port)
{
    uint16_t first;
    uint16_t second;

    if (port > OWC_PORT_DOWNSTREAM) {
        return false;
    }
    first = board_adc_mv(sense_pin(port));
    board_delay_us(10u);
    second = board_adc_mv(sense_pin(port));
    if (first < OWC_SHORT_MV || second < OWC_SHORT_MV) {
        return false;
    }
    if (first > OWC_OVER_VOLT_MV || second > OWC_OVER_VOLT_MV) {
        return false;
    }
    return true;
}

static bool pull_low_checked(owc_phy_t *phy, owc_port_t port, uint32_t duration_us)
{
    uint16_t observed;

    if (phy == NULL || phy->faulted || !owc_phy_bus_safe(port)) {
        return false;
    }
    board_gpio_write(pull_pin(port), true);
    board_delay_us(3u);
    observed = board_adc_mv(sense_pin(port));
    if (observed > OWC_ADC_LOW_MV) {
        board_gpio_write(pull_pin(port), false);
        phy->faulted = true;
        return false;
    }
    if (duration_us > 3u) {
        board_delay_us(duration_us - 3u);
    }
    board_gpio_write(pull_pin(port), false);
    return true;
}

bool owc_phy_drive_reset(owc_phy_t *phy, owc_port_t port)
{
    uint32_t deadline;
    bool presence = false;

    if (!pull_low_checked(phy, port, 480u)) {
        return false;
    }
    board_delay_us(15u);
    deadline = board_micros() + 240u;
    while ((int32_t)(deadline - board_micros()) > 0) {
        if (board_adc_mv(sense_pin(port)) < OWC_ADC_LOW_MV) {
            presence = true;
        }
        board_delay_us(4u);
    }
    return presence;
}

bool owc_phy_write_bit(owc_phy_t *phy, owc_port_t port, bool bit)
{
    uint32_t low_time = bit ? 6u : 60u;
    uint32_t recovery = bit ? 64u : 10u;

    if (!pull_low_checked(phy, port, low_time)) {
        return false;
    }
    board_delay_us(recovery);
    return owc_phy_bus_safe(port);
}

bool owc_phy_write_byte(owc_phy_t *phy, owc_port_t port, uint8_t value)
{
    unsigned bit;

    for (bit = 0u; bit < 8u; ++bit) {
        if (!owc_phy_write_bit(phy, port, ((value >> bit) & 1u) != 0u)) {
            return false;
        }
    }
    return true;
}

void owc_phy_release_all(void)
{
    board_gpio_write(PIN_UPSTREAM_PULL, false);
    board_gpio_write(PIN_DOWNSTREAM_PULL, false);
    board_gpio_write(PIN_STRONG_PULLUP, false);
}

uint8_t owc_crc8(const uint8_t *data, size_t length)
{
    uint8_t crc = 0u;
    size_t index;
    unsigned bit;

    if (data == NULL) {
        return 0u;
    }
    for (index = 0u; index < length; ++index) {
        uint8_t input = data[index];
        for (bit = 0u; bit < 8u; ++bit) {
            uint8_t mix = (uint8_t)((crc ^ input) & 1u);
            crc >>= 1u;
            if (mix != 0u) {
                crc ^= 0x8Cu;
            }
            input >>= 1u;
        }
    }
    return crc;
}
