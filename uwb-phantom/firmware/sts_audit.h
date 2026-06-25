/*
 * sts_audit.h — STS auditor: probe a verifier's STS enforcement.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UWB_PHANTOM_STS_AUDIT_H
#define UWB_PHANTOM_STS_AUDIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    AUDIT_NO_STS        = 0,  /* prover pretends to be legacy 4a, no STS */
    AUDIT_STATIC_STS    = 1,  /* use a captured static STS key/IV */
    AUDIT_DOWNGRADE     = 2,  /* advertise 4a then negotiate 4z */
    AUDIT_COUNTER_REPLAY = 3,  /* replay captured frame counters */
} audit_suite_t;

typedef struct {
    bool    accepted_no_sts;
    bool    accepted_static_sts;
    bool    accepted_downgrade;
    bool    accepted_replay;
    uint8_t captured_sts_quality;
    uint32_t frames_examined;
} audit_result_t;

int  sts_audit_run(audit_suite_t suite, const uint8_t target_eui[8],
                   audit_result_t *out, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* UWB_PHANTOM_STS_AUDIT_H */