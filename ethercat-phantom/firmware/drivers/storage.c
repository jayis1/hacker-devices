/*
 * Simulated persistent storage and capture logging
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "storage.h"
#include "ethercat.h"

#include <stdio.h>
#include <string.h>

static eph_runtime_config_t g_saved_config;
static bool g_has_config;
static eph_capture_t g_log[EPH_STORAGE_SECTORS * 2u];
static size_t g_log_count;

void eph_storage_init(void) {
    memset(&g_saved_config, 0, sizeof(g_saved_config));
    memset(g_log, 0, sizeof(g_log));
    g_has_config = false;
    g_log_count = 0u;
}

bool eph_storage_store_config(const eph_runtime_config_t *config) {
    if (config == NULL) {
        return false;
    }
    g_saved_config = *config;
    g_has_config = true;
    return true;
}

bool eph_storage_load_config(eph_runtime_config_t *config) {
    if (!g_has_config || config == NULL) {
        return false;
    }
    *config = g_saved_config;
    return true;
}

size_t eph_storage_append_captures(const eph_capture_t *captures, size_t count) {
    size_t appended = 0u;
    while (appended < count && g_log_count < (sizeof(g_log) / sizeof(g_log[0]))) {
        g_log[g_log_count++] = captures[appended++];
    }
    return appended;
}

size_t eph_storage_log_count(void) {
    return g_log_count;
}

void eph_storage_dump_recent(size_t limit) {
    size_t start;
    size_t i;
    if (g_log_count == 0u) {
        printf("[storage] no captures logged\n");
        return;
    }

    start = (g_log_count > limit) ? (g_log_count - limit) : 0u;
    printf("[storage] recent captures (%zu of %zu):\n", g_log_count - start, g_log_count);
    for (i = start; i < g_log_count; ++i) {
        printf("  cycle=%lu slave=%u object=0x%04X:%u before=%ld after=%ld modified=%s wc=%u\n",
               (unsigned long)g_log[i].cycle_counter,
               g_log[i].slave_address,
               g_log[i].index,
               g_log[i].subindex,
               (long)g_log[i].value_before,
               (long)g_log[i].value_after,
               g_log[i].modified ? "yes" : "no",
               g_log[i].working_counter);
    }
}
