/*
 * SmartPack Phantom auth harness
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include "auth_chip.h"

static uint32_t mix32(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

void auth_chip_init(sp_auth_state_t *state) {
    if (state == 0) {
        return;
    }
    state->challenge_count = 0u;
    state->replay_count = 0u;
    state->failure_count = 0u;
    state->synthetic_count = 0u;
    state->last_challenge = 0u;
    state->last_response = 0u;
    state->mode = SP_AUTH_PASSTHROUGH;
}

const char *auth_chip_mode_name(sp_auth_mode_t mode) {
    switch (mode) {
        case SP_AUTH_PASSTHROUGH: return "passthrough";
        case SP_AUTH_SYNTHETIC: return "synthetic";
        case SP_AUTH_REPLAY_LAST: return "replay-last";
        case SP_AUTH_STALE_NONCE: return "stale-nonce";
        case SP_AUTH_FORCE_FAIL: return "force-fail";
        default: return "unknown";
    }
}

void auth_chip_set_mode(sp_auth_state_t *state, sp_auth_mode_t mode) {
    if (state == 0) {
        return;
    }
    state->mode = mode;
}

uint32_t auth_chip_issue_challenge(sp_auth_state_t *state, uint32_t seed) {
    uint32_t challenge;
    if (state == 0) {
        return 0u;
    }
    state->challenge_count++;
    if (state->mode == SP_AUTH_STALE_NONCE && state->last_challenge != 0u) {
        challenge = state->last_challenge;
    } else {
        challenge = mix32(seed ^ (0x13579bdfU + state->challenge_count * 0x1021U));
        state->last_challenge = challenge;
    }
    return challenge;
}

uint32_t auth_chip_respond(sp_auth_state_t *state, uint32_t challenge, bool *accepted) {
    uint32_t response;
    bool ok = true;
    if (state == 0) {
        if (accepted != 0) {
            *accepted = false;
        }
        return 0u;
    }

    switch (state->mode) {
        case SP_AUTH_PASSTHROUGH:
            response = mix32(challenge ^ 0xa5a55a5aU ^ state->challenge_count);
            break;
        case SP_AUTH_SYNTHETIC:
            response = mix32(challenge ^ 0x5aa5f00dU ^ (state->synthetic_count + 7u));
            state->synthetic_count++;
            break;
        case SP_AUTH_REPLAY_LAST:
            response = state->last_response != 0u ? state->last_response : mix32(challenge ^ 0xdeadbeefU);
            state->replay_count++;
            break;
        case SP_AUTH_STALE_NONCE:
            response = mix32(state->last_challenge ^ 0x2468ace0U);
            break;
        case SP_AUTH_FORCE_FAIL:
            response = 0xffffffffU;
            ok = false;
            state->failure_count++;
            break;
        default:
            response = 0u;
            ok = false;
            state->failure_count++;
            break;
    }

    if (state->mode != SP_AUTH_FORCE_FAIL && response == 0u) {
        ok = false;
        state->failure_count++;
    }

    state->last_response = response;
    if (accepted != 0) {
        *accepted = ok;
    }
    return response;
}
