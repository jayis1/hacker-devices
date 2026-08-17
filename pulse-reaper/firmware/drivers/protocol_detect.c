/*
 * protocol_detect.c — auto-detect fieldbus protocol
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Tries each registered protocol's detect() function in turn. The first
 * one that returns non-zero wins. This is cheap because each detector
 * only inspects the first few bytes (address + function code patterns).
 */

#include "protocol_detect.h"
#include "modbus_rtu.h"
#include "profibus_dp.h"
#include "hart_fsk.h"
#include "can_native.h"
#include "pots_dtmf.h"

static const pr_protocol_t *const s_protos[] = {
    &pr_proto_modbus_rtu,
    &pr_proto_profibus_dp,
    &pr_proto_hart_fsk,
    &pr_proto_can,
    &pr_proto_pots_dtmf,
};

#define N_PROTOS (int)(sizeof(s_protos)/sizeof(s_protos[0]))

void protocol_detect_init(void) {
    /* Each protocol registers itself via its own init(). */
}

const pr_protocol_t *protocol_detect(const uint8_t *frame, uint16_t len) {
    if (!frame || len < 1u) return NULL;

    /* Score each protocol's detector and pick the best. */
    int best_score = 0;
    const pr_protocol_t *best = NULL;
    for (int i = 0; i < N_PROTOS; i++) {
        int score = s_protos[i]->detect(frame, len);
        if (score > best_score) {
            best_score = score;
            best = s_protos[i];
        }
    }
    return best;
}