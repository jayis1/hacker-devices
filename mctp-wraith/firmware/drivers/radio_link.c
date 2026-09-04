/*
 * radio_link.c
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <stdio.h>
#include <string.h>
#include "radio_link.h"

static char g_topics[12][24];
static char g_payloads[12][72];
static uint32_t g_log_count;

void radio_link_init(void) {
    memset(g_topics, 0, sizeof(g_topics));
    memset(g_payloads, 0, sizeof(g_payloads));
    g_log_count = 0U;
}

void radio_link_send_status(const char *topic, const char *payload) {
    if (g_log_count >= 12U) {
        return;
    }
    snprintf(g_topics[g_log_count], sizeof(g_topics[g_log_count]), "%s", topic != NULL ? topic : "status");
    snprintf(g_payloads[g_log_count], sizeof(g_payloads[g_log_count]), "%s", payload != NULL ? payload : "");
    g_log_count++;
}

void radio_link_print_log(void) {
    printf("[radio] pending_frames=%u\n", g_log_count);
    for (uint32_t i = 0U; i < g_log_count; ++i) {
        printf("  [%02u] topic=%s payload=%s\n", i, g_topics[i], g_payloads[i]);
    }
}
