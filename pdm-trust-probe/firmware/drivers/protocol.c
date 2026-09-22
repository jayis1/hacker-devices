/* Bounded, replay-resistant USB command parser
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#include "protocol.h"
#include <string.h>

static uint32_t get_u32(const uint8_t *pointer)
{
    return (uint32_t)pointer[0] |
           ((uint32_t)pointer[1] << 8u) |
           ((uint32_t)pointer[2] << 16u) |
           ((uint32_t)pointer[3] << 24u);
}

static uint16_t get_u16(const uint8_t *pointer)
{
    return (uint16_t)((uint16_t)pointer[0] |
                      ((uint16_t)pointer[1] << 8u));
}

void protocol_init(ptp_session_t *session)
{
    if (session != NULL) {
        memset(session, 0, sizeof(*session));
        session->nonce = 0xA5C31F27u;
    }
}

uint32_t protocol_response(uint32_t nonce)
{
    uint32_t value = nonce ^ 0x6D2B79F5u;

    value ^= value << 13u;
    value ^= value >> 17u;
    value ^= value << 5u;
    return value;
}

static bool session_is_armed(ptp_session_t *session, uint32_t now_ms)
{
    if (!session->armed) {
        return false;
    }
    if ((int32_t)(session->lease_deadline_ms - now_ms) <= 0) {
        session->armed = false;
        return false;
    }
    return true;
}

static ptp_status_t set_policy(ptp_analyzer_t *analyzer,
                               const ptp_frame_t *frame)
{
    ptp_policy_t policy;

    if (frame->length != 17u) {
        return ST_BAD_FRAME;
    }
    memset(&policy, 0, sizeof(policy));
    policy.density_min_permille = get_u16(frame->payload);
    policy.density_max_permille = get_u16(frame->payload + 2u);
    policy.transition_min_permille = get_u16(frame->payload + 4u);
    policy.transition_max_permille = get_u16(frame->payload + 6u);
    policy.clock_min_hz = get_u32(frame->payload + 8u);
    policy.clock_max_hz = get_u32(frame->payload + 12u);
    policy.enabled_channels = frame->payload[16];
    return analyzer_set_policy(analyzer, &policy) ? ST_OK : ST_RANGE;
}

static ptp_status_t arm_session(ptp_session_t *session,
                                const ptp_frame_t *frame,
                                uint32_t now_ms)
{
    uint32_t nonce;
    uint32_t response;

    if (frame->length != 8u || !board_gpio_read(PIN_ARM)) {
        return ST_DENIED;
    }
    nonce = get_u32(frame->payload);
    response = get_u32(frame->payload + 4u);
    if (nonce != session->nonce || response != protocol_response(nonce)) {
        return ST_DENIED;
    }
    session->armed = true;
    session->lease_deadline_ms = now_ms + PTP_ARM_LEASE_MS;
    session->nonce = protocol_response(response);
    return ST_OK;
}

static ptp_status_t run_pattern(ptp_session_t *session,
                                const ptp_frame_t *frame,
                                uint32_t now_ms)
{
    uint32_t duration_ms;
    uint8_t pattern;
    uint8_t channel_mask;

    if (frame->length != 6u ||
        !session_is_armed(session, now_ms) ||
        !board_gpio_read(PIN_ARM)) {
        return ST_DENIED;
    }
    pattern = frame->payload[0];
    channel_mask = (uint8_t)(frame->payload[1] & 0x0Fu);
    duration_ms = get_u32(frame->payload + 2u);
    if (pattern > 3u || channel_mask == 0u) {
        return ST_RANGE;
    }
    if (duration_ms == 0u || duration_ms > PTP_MAX_INJECT_MS) {
        return ST_RANGE;
    }
    return board_run_fixed_pattern(pattern, channel_mask, duration_ms)
             ? ST_OK
             : ST_HW_FAULT;
}

static ptp_status_t dispatch(ptp_session_t *session,
                             ptp_capture_t *capture,
                             ptp_analyzer_t *analyzer,
                             const ptp_frame_t *frame,
                             uint32_t now_ms)
{
    switch ((ptp_command_t)frame->command) {
    case CMD_INFO:
    case CMD_STATUS:
    case CMD_EVENT_READ:
        return frame->length == 0u ? ST_OK : ST_BAD_FRAME;
    case CMD_CAPTURE_START:
        if (frame->length != 0u) {
            return ST_BAD_FRAME;
        }
        capture_start(capture);
        session->capture_active = true;
        return ST_OK;
    case CMD_CAPTURE_STOP:
        if (frame->length != 0u) {
            return ST_BAD_FRAME;
        }
        capture_stop(capture);
        session->capture_active = false;
        return ST_OK;
    case CMD_POLICY_SET:
        return set_policy(analyzer, frame);
    case CMD_ARM:
        return arm_session(session, frame, now_ms);
    case CMD_PATTERN_RUN:
        return run_pattern(session, frame, now_ms);
    case CMD_RESET:
        if (frame->length != 0u) {
            return ST_BAD_FRAME;
        }
        session->armed = false;
        capture_stop(capture);
        session->capture_active = false;
        return ST_OK;
    default:
        return ST_BAD_FRAME;
    }
}

ptp_status_t protocol_process(ptp_session_t *session,
                              ptp_capture_t *capture,
                              ptp_analyzer_t *analyzer,
                              const ptp_frame_t *frame,
                              uint32_t now_ms)
{
    ptp_status_t status;

    if (session == NULL || capture == NULL || analyzer == NULL ||
        frame == NULL || frame->magic != PTP_USB_MAGIC ||
        frame->version != PTP_PROTOCOL_VERSION ||
        frame->length > sizeof(frame->payload) ||
        session->last_sequence == UINT32_MAX ||
        frame->sequence == 0u ||
        frame->sequence != session->last_sequence + 1u) {
        return ST_BAD_FRAME;
    }
    status = dispatch(session, capture, analyzer, frame, now_ms);
    if (status != ST_BAD_FRAME) {
        session->last_sequence = frame->sequence;
    }
    return status;
}