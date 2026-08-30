/*
 * main.c - EDID Phantom firmware simulator
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include "board.h"
#include "registers.h"
#include "drivers/cec.h"
#include "drivers/ddc.h"
#include "drivers/log.h"
#include "drivers/profile.h"
#include "drivers/radio.h"
#include "drivers/safety.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t g_registers[0x0700u / 4u + 1u];

uint32_t reg_read(uint16_t reg) {
    return g_registers[reg / 4u];
}

void reg_write(uint16_t reg, uint32_t value) {
    g_registers[reg / 4u] = value;
}

void reg_set_bits(uint16_t reg, uint32_t mask) {
    g_registers[reg / 4u] |= mask;
}

void reg_clear_bits(uint16_t reg, uint32_t mask) {
    g_registers[reg / 4u] &= ~mask;
}

void reg_reset_all(void) {
    memset(g_registers, 0, sizeof(g_registers));
}

static void status_init(ep_status_t *status, const ep_profile_t *profile) {
    memset(status, 0, sizeof(*status));
    status->mode = profile->mode;
    status->ddc_mode = profile->ddc_mode;
    status->target_connected = 1u;
    status->sink_present = 1u;
    status->hpd_asserted = 1u;
    status->mutation_enabled = profile->allow_edid_mutation;
    status->cec_guard_enabled = profile->allow_cec_injection;
    snprintf(status->active_profile, sizeof(status->active_profile), "%s", profile->name);
    reg_set_bits(REG_SYS_STATUS, SYS_STATUS_TARGET_CONNECTED | SYS_STATUS_SINK_PRESENT);
    reg_set_bits(REG_HPD_STATUS, HPD_STATUS_ASSERTED);
}

static void print_profile_catalog(void) {
    size_t index;
    puts("Available profiles:");
    for (index = 0; index < profile_count(); ++index) {
        const ep_profile_t *profile = profile_at(index);
        if (profile != NULL) {
            printf("  - %s (mode=%u ddc=%u hpd=%u cec=%u mutate=%u)\n",
                   profile->name,
                   (unsigned)profile->mode,
                   (unsigned)profile->ddc_mode,
                   (unsigned)profile->allow_hpd_glitch,
                   (unsigned)profile->allow_cec_injection,
                   (unsigned)profile->allow_edid_mutation);
        }
    }
}

static void print_edid_summary(void) {
    const ep_edid_image_t *edid = ddc_active_edid();
    size_t index;
    printf("EDID summary: len=%zu checksum_ok=%u name=%s\n",
           edid->length,
           (unsigned)edid->checksum_valid,
           edid->monitor_name);
    printf("EDID first 32 bytes:");
    for (index = 0; index < 32u && index < edid->length; ++index) {
        printf(" %02X", edid->bytes[index]);
    }
    putchar('\n');
}

static void print_ddc_log(void) {
    size_t count = ddc_transaction_count();
    size_t index;
    printf("---- DDC Transactions (%zu) ----\n", count);
    for (index = 0; index < count; ++index) {
        const ep_ddc_txn_t *txn = ddc_transaction(index);
        if (txn == NULL) {
            continue;
        }
        printf("txn[%zu] t=%u addr=0x%02X off=%u len=%u ack=%u sum=0x%02X data=",
               index,
               (unsigned)txn->start_ms,
               (unsigned)txn->address,
               (unsigned)txn->offset,
               (unsigned)txn->length,
               (unsigned)txn->acked,
               (unsigned)txn->checksum);
        for (size_t j = 0; j < txn->length && j < sizeof(txn->data); ++j) {
            printf("%02X", txn->data[j]);
            if (j + 1u < txn->length && j + 1u < sizeof(txn->data)) {
                putchar(':');
            }
        }
        putchar('\n');
    }
}

static void print_cec_log(void) {
    size_t count = cec_frame_count();
    size_t index;
    printf("---- CEC Frames (%zu) ----\n", count);
    for (index = 0; index < count; ++index) {
        const ep_cec_frame_t *frame = cec_frame(index);
        size_t j;
        if (frame == NULL) {
            continue;
        }
        printf("cec[%zu] t=%u %X->%X op=0x%02X len=%u payload=",
               index,
               (unsigned)frame->timestamp_ms,
               frame->initiator,
               frame->destination,
               frame->opcode,
               frame->length);
        for (j = 0; j < frame->length; ++j) {
            printf("%02X", frame->payload[j]);
            if (j + 1u < frame->length) {
                putchar(':');
            }
        }
        putchar('\n');
    }
}

static void print_register_snapshot(void) {
    printf("Registers: SYS=0x%08X DDC=0x%08X HPD=0x%08X CEC=0x%08X RADIO=0x%08X POLICY=0x%08X\n",
           reg_read(REG_SYS_STATUS),
           reg_read(REG_DDC_STATUS),
           reg_read(REG_HPD_STATUS),
           reg_read(REG_CEC_STATUS),
           reg_read(REG_RADIO_STATUS),
           reg_read(REG_POLICY_STATUS));
}

static void print_reports(const ep_status_t *status) {
    char ddc_report[160];
    char cec_report[160];
    char safety_report[160];

    ddc_build_report(ddc_report, sizeof(ddc_report));
    cec_build_report(cec_report, sizeof(cec_report));
    safety_build_report(safety_report, sizeof(safety_report), status);

    printf("DDC REPORT: %s\n", ddc_report);
    printf("CEC REPORT: %s\n", cec_report);
    printf("SAFETY REPORT: %s\n", safety_report);
}

int main(int argc, char **argv) {
    const char *profile_name = argc > 1 ? argv[1] : NULL;
    const ep_profile_t *profile;
    ep_status_t status;
    uint32_t tick;

    reg_reset_all();
    log_init();
    profile_init();
    ddc_init();
    cec_init();
    radio_init();
    safety_init();

    profile = profile_find(profile_name);
    status_init(&status, profile);
    ddc_set_profile(profile);
    cec_set_profile(profile);
    radio_set_profile(profile);
    safety_apply_profile(profile);

    log_event(EP_EVENT_BOOT, EP_RISK_INFO, "%s booted by %s with profile=%s",
              EP_DEVICE_NAME,
              EP_AUTHOR,
              profile->name);

    print_profile_catalog();

    for (tick = 10u; tick <= 240u; tick += EP_TICK_MS) {
        status.uptime_ms = tick;
        safety_tick(tick, &status);
        ddc_capture_cycle(tick, &status);
        cec_service(tick, &status);
        radio_service(tick, &status);

        if ((tick % 60u) == 0u) {
            safety_pulse_hpd(&status);
        }

        if (tick == 150u) {
            ddc_force_mutation((uint16_t)(profile->mutate_seed ^ 0x0F0Fu), &status);
        }
    }

    puts("\n=== EDID Phantom Simulation Summary ===");
    printf("Device: %s\nAuthor: %s\nProfile: %s\n",
           EP_DEVICE_NAME,
           EP_AUTHOR,
           status.active_profile);
    printf("Uptime=%ums DDC=%u CEC=%u HPD=%u PolicyBlocks=%u Temp=%.1fC Current=%.1fmA\n",
           (unsigned)status.uptime_ms,
           (unsigned)status.ddc_captures,
           (unsigned)status.cec_frames,
           (unsigned)status.hpd_pulses_sent,
           (unsigned)status.policy_blocks,
           status.board_temp_c,
           status.current_draw_ma);

    print_edid_summary();
    print_reports(&status);
    print_register_snapshot();
    print_ddc_log();
    print_cec_log();
    radio_emit_status(&status);
    radio_emit_transcript();
    log_dump();

    return EXIT_SUCCESS;
}
