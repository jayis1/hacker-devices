/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef QC_ENGINE_H
#define QC_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define QC_QUALIFY 3u
#define QC_EARLY 1u
#define QC_ORPHAN 2u
#define QC_OVERLAP 4u
#define QC_GAP 8u
#define QC_EXHAUSTED 16u
#define QC_WATCHDOG 32u
#define QC_WAIT 0u
#define QC_READY 1u
#define QC_ACTIVE 2u
#define QC_DRAIN 3u
/* READER_A, READER_B, RETIRE, RECLAIM; all target pins input-only. */
typedef struct {
    uint32_t now, levels, valid, halted, faults, phase, cohort, owed;
    uint32_t observed, baselines, rounds, accepted, early, orphan;
    uint32_t abandoned, reclaims, overlap, captured, released, discarded;
    uint32_t gaps, missed, tx_fail;
    uint8_t candidates, runs[4];
    bool started, exhausted;
} Ledger;
void ledger_init(Ledger *s);
void ledger_sample(Ledger *s, uint32_t now, uint8_t raw);
void ledger_halt(Ledger *s, uint32_t reason);
void ledger_tx_failure(Ledger *s);
bool ledger_valid(const Ledger *s);
#endif
