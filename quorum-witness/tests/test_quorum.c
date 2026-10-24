/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "quorum.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void hold(struct quorum *q, uint8_t value, unsigned count)
{
    while (count--) qw_sample(q, q->time + 1u, value);
}

int main(void)
{
    struct quorum q;
    qw_init(&q, 0);
    assert(!q.valid);
    hold(&q, 8, 2);
    assert(!q.valid && !q.unsafe);
    hold(&q, 8, 1);
    assert(q.valid && q.levels == 8 && q.majority == 0);
    hold(&q, 8, 49);
    assert(q.unsafe == 0);
    hold(&q, 8, 1);
    assert(q.unsafe == 1 && (q.alarms & 1));
    hold(&q, 8, 500);
    assert(q.unsafe == 1);
    hold(&q, 0, 3);
    assert(q.alarms == 0 && q.mismatch_ms == 0);
    hold(&q, 8, 53);
    assert(q.unsafe == 2);

    /* Majority high, output low: deny episode and minority C dissent. */
    qw_init(&q, 0);
    hold(&q, 3, 3);
    assert(q.majority == 1 && q.dissent == 4);
    hold(&q, 3, 50);
    assert(q.denied == 1 && q.dissent_count[2] == 0);
    hold(&q, 3, 50);
    assert(q.dissent_count[2] == 1 && (q.alarms & 16));
    hold(&q, 11, 3);
    assert(q.mismatch_ms == 0 && q.dissent_count[2] == 1);
    hold(&q, 15, 3);
    assert(q.dissent == 0 && q.alarms == 0);
    hold(&q, 13, 103);
    assert(q.dissent_count[1] == 1);
    hold(&q, 14, 103);
    assert(q.dissent_count[0] == 1);

    /* Every truth-table entry matches majority logic without output faults. */
    for (unsigned votes = 0; votes < 8; ++votes) {
        unsigned ones = (votes & 1) + ((votes >> 1) & 1) + ((votes >> 2) & 1);
        unsigned maj = ones >= 2;
        qw_init(&q, 0);
        hold(&q, votes | (maj << 3), 203);
        assert(q.majority == maj && !q.unsafe && !q.denied);
        assert(q.dissent == ((maj ? ~votes : votes) & 7));
    }

    /* A sample gap invalidates continuity, never fabricates elapsed evidence. */
    qw_init(&q, 0);
    hold(&q, 8, 52);
    assert(q.mismatch_ms == 49);
    qw_sample(&q, q.time + 1000, 0);
    assert(q.gaps == 1 && q.missed == 999 && !q.valid);
    assert(q.levels == 8 && q.unsafe == 0);
    hold(&q, 8, 3);
    assert(q.valid && q.unsafe == 0 && q.mismatch_ms == 0);
    hold(&q, 8, 50);
    assert(q.unsafe == 1);

    /* Duplicate ticks cannot qualify, glitches cannot erase held violations. */
    qw_init(&q, 0);
    for (unsigned i = 0; i < 100; ++i) qw_sample(&q, 0, 8);
    assert(!q.valid);
    hold(&q, 8, 40);
    hold(&q, 0, 1);
    hold(&q, 8, 12);
    assert(q.unsafe == 1);

    /* Wrap and saturation are deliberate, no ancient duration aliases. */
    qw_init(&q, UINT32_MAX - 2);
    hold(&q, 8, 53);
    assert(q.unsafe == 1 && q.gaps == 0);
    q.mismatch_ms = UINT32_MAX - 1;
    hold(&q, 8, 4);
    assert(q.mismatch_ms == UINT32_MAX && q.max_mismatch == UINT32_MAX);
    q.tx_fail = UINT32_MAX;
    qw_tx_fail(&q);
    assert(q.tx_fail == UINT32_MAX);

    /* Queue pressure is counted; pop is FIFO and empty-safe. */
    qw_init(&q, 0);
    for (unsigned i = 0; i < 100; ++i) hold(&q, (i & 1) ? 15 : 0, 3);
    assert(q.dropped > 0);
    struct qw_event e;
    unsigned n = 0;
    uint32_t prev = 0;
    while (qw_pop(&q, &e)) {
        assert(!n || e.sequence > prev);
        prev = e.sequence;
        ++n;
    }
    assert(n == QW_QUEUE && !qw_pop(&q, &e));

    char packet[384];
    assert(qw_crc("123456789", 9) == 0x29b1);
    assert(qw_status(packet, sizeof packet, &q) > 0);
    assert(strncmp(packet, "QW1,S,", 6) == 0);
    assert(!qw_status(packet, 8, &q));
    assert(!qw_status(NULL, sizeof packet, &q));
    assert(!qw_status(packet, sizeof packet, NULL));
    assert(qw_policy(packet, sizeof packet));
    assert(strncmp(packet, "QW1,P,50,100,3*", 14) == 0);
    puts("PASS: truth table, thresholds, dissent identities, gaps, glitches, wrap, saturation, queue, encoder");
}
