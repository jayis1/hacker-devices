/**
 * @file policy_engine.c
 * @author jayis1
 * @brief High-level policy engine for SATA Phantom
 *
 * Implements complex match patterns not feasible in FPGA hardware alone.
 * Manages stateful rules (e.g., "match 3 reads to same LBA, then inject"),
 * rule persistence in NVS, and coordination with the FPGA control task.
 *
 * Copyright © 2025 jayis1. All rights reserved.
 * Authorized security research use only.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "board.h"
#include "registers.h"

static const char *TAG = "policy";

/* Maximum stateful rules tracked in software */
#define MAX_STATEFUL_RULES     32

/* NVS namespace for rule storage */
#define NVS_RULE_NAMESPACE     "sata_rules"

/* ===================================================================
 * Stateful Rule Structure
 * =================================================================== */

typedef enum {
    STATE_IDLE = 0,
    STATE_WATCHING,       /* Tracking hit count */
    STATE_TRIGGERED,      /* Rule condition satisfied */
    STATE_INJECTED,       /* Injection completed */
    STATE_EXPIRED         /* Timed out */
} stateful_rule_state_t;

typedef struct {
    fpga_rule_t hw_rule;                /* Base hardware rule (programmed to FPGA) */
    stateful_rule_state_t state;         /* Current state */
    uint32_t hit_count;                  /* Number of times this rule has matched */
    uint32_t trigger_threshold;          /* Hit count to trigger action */
    uint32_t cooldown_ms;               /* Cooldown after trigger (0 = one-shot) */
    TickType_t last_trigger_tick;       /* Last trigger timestamp */
    uint8_t inject_payload[512];         /* Payload to inject (if action=inject) */
    uint16_t inject_len;                 /* Inject payload length */
    char name[32];                       /* Human-readable rule name */
    bool persistent;                     /* Saved to NVS */
} stateful_rule_t;

/* Stateful rule table */
static stateful_rule_t stateful_rules[MAX_STATEFUL_RULES];
static int num_stateful_rules = 0;

/* Forward declarations */
static void apply_rule_set(void);
static void load_rules_from_nvs(void);
static void save_rules_to_nvs(void);

/* ===================================================================
 * NVS Rule Persistence
 * =================================================================== */

/**
 * @brief Serialize a rule into a binary blob for NVS storage.
 */
static esp_err_t rule_to_nvs(nvs_handle_t nvs, int index, const stateful_rule_t *rule)
{
    char key[16];
    snprintf(key, sizeof(key), "rule_%d", index);
    return nvs_set_blob(nvs, key, rule, sizeof(stateful_rule_t));
}

/**
 * @brief Deserialize a rule from NVS binary blob.
 */
static esp_err_t rule_from_nvs(nvs_handle_t nvs, int index, stateful_rule_t *rule)
{
    char key[16];
    snprintf(key, sizeof(key), "rule_%d", index);
    size_t len = sizeof(stateful_rule_t);
    return nvs_get_blob(nvs, key, rule, &len);
}

/**
 * @brief Load all persistent rules from NVS flash.
 */
static void load_rules_from_nvs(void)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_RULE_NAMESPACE, NVS_READONLY, &nvs);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "No saved rules found (NVS open: %d)", err);
        return;
    }

    num_stateful_rules = 0;
    for (int i = 0; i < MAX_STATEFUL_RULES; i++) {
        stateful_rule_t rule;
        err = rule_from_nvs(nvs, i, &rule);
        if (err == ESP_OK && rule.persistent) {
            rule.state = STATE_IDLE;
            rule.hit_count = 0;
            rule.last_trigger_tick = 0;
            stateful_rules[num_stateful_rules++] = rule;
            ESP_LOGI(TAG, "Loaded rule '%s' from NVS (slot %d)", rule.name, i);
        } else if (err != ESP_ERR_NVS_NOT_FOUND) {
            break;  /* Reached end of stored rules */
        }
    }

    nvs_close(nvs);
    ESP_LOGI(TAG, "Loaded %d rules from NVS", num_stateful_rules);

    /* Program them into FPGA */
    apply_rule_set();
}

/**
 * @brief Save all persistent rules to NVS flash.
 */
static void save_rules_to_nvs(void)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_RULE_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS for writing: %d", err);
        return;
    }

    /* Erase all existing keys first */
    nvs_erase_all(nvs);

    int saved = 0;
    for (int i = 0; i < num_stateful_rules; i++) {
        if (stateful_rules[i].persistent) {
            err = rule_to_nvs(nvs, i, &stateful_rules[i]);
            if (err == ESP_OK) saved++;
        }
    }

    nvs_commit(nvs);
    nvs_close(nvs);
    ESP_LOGI(TAG, "Saved %d rules to NVS", saved);
}

/* ===================================================================
 * Rule Management API
 * =================================================================== */

/**
 * @brief Add a new stateful rule.
 * @param rule  The rule to add (copied internally)
 * @return Rule index on success, -1 on failure.
 */
int policy_add_rule(const stateful_rule_t *rule)
{
    if (num_stateful_rules >= MAX_STATEFUL_RULES) {
        ESP_LOGE(TAG, "Max stateful rules reached (%d)", MAX_STATEFUL_RULES);
        return -1;
    }

    int idx = num_stateful_rules++;
    stateful_rules[idx] = *rule;
    stateful_rules[idx].state = STATE_IDLE;
    stateful_rules[idx].hit_count = 0;

    /* Program the hardware portion into FPGA */
    ctrl_cmd_t cmd = {
        .cmd_type = CTRL_CMD_WRITE_RULE,
        .rule.slot = idx,
        .rule.rule = rule->hw_rule
    };
    xQueueSend(ctrl_cmd_queue, &cmd, pdMS_TO_TICKS(50));

    if (rule->persistent) {
        save_rules_to_nvs();
    }

    ESP_LOGI(TAG, "Added rule #%d: '%s'", idx, rule->name);
    return idx;
}

/**
 * @brief Remove a rule by index.
 */
bool policy_remove_rule(int index)
{
    if (index < 0 || index >= num_stateful_rules) {
        return false;
    }

    /* Shift remaining rules */
    for (int i = index; i < num_stateful_rules - 1; i++) {
        stateful_rules[i] = stateful_rules[i + 1];
    }
    num_stateful_rules--;

    /* Re-program all FPGA rules */
    apply_rule_set();
    save_rules_to_nvs();

    ESP_LOGI(TAG, "Removed rule #%d", index);
    return true;
}

/**
 * @brief Apply the current rule set to the FPGA.
 */
static void apply_rule_set(void)
{
    uint16_t enable_mask = 0;

    for (int i = 0; i < num_stateful_rules; i++) {
        ctrl_cmd_t cmd = {
            .cmd_type = CTRL_CMD_WRITE_RULE,
            .rule.slot = i,
            .rule.rule = stateful_rules[i].hw_rule
        };
        xQueueSend(ctrl_cmd_queue, &cmd, pdMS_TO_TICKS(50));

        if (stateful_rules[i].hw_rule.enabled) {
            enable_mask |= (1 << i);
        }
    }

    /* Disable unused slots */
    for (int i = num_stateful_rules; i < FPGA_MAX_RULES; i++) {
        ctrl_cmd_t cmd = {
            .cmd_type = CTRL_CMD_WRITE_REG,
            .reg.addr = RULE_ADDR(i, RULE_OFFS_CTRL),
            .reg.data = 0  /* Disabled */
        };
        xQueueSend(ctrl_cmd_queue, &cmd, pdMS_TO_TICKS(50));
    }

    /* Set enable mask */
    ctrl_cmd_t cmd = {
        .cmd_type = CTRL_CMD_WRITE_REG,
        .reg.addr = REG_RULE_GLOBAL_ENABLE,
        .reg.data = enable_mask
    };
    xQueueSend(ctrl_cmd_queue, &cmd, pdMS_TO_TICKS(50));

    ESP_LOGI(TAG, "Applied %d rules to FPGA (enable mask: 0x%04X)",
             num_stateful_rules, enable_mask);
}

/* ===================================================================
 * Built-in Preconfigured Rule Templates
 * =================================================================== */

/**
 * @brief Create a rule to monitor all reads to the boot sector (LBA 0).
 */
static void add_boot_sector_monitor_rule(void)
{
    stateful_rule_t rule = {0};
    rule.hw_rule.lba_start = 0;
    rule.hw_rule.lba_mask  = 0;   /* Exact match LBA 0 */
    rule.hw_rule.opcode    = ATA_CMD_READ_DMA_EXT;
    rule.hw_rule.opcode_mask = 0;
    rule.hw_rule.direction = 0;    /* Read */
    rule.hw_rule.action    = 0;    /* Monitor */
    rule.hw_rule.enabled   = 1;
    rule.hw_rule.priority  = 10;
    rule.trigger_threshold = 1;
    rule.cooldown_ms = 1000;
    strncpy(rule.name, "Boot Sector Read Monitor", sizeof(rule.name) - 1);
    rule.persistent = true;

    policy_add_rule(&rule);
}

/**
 * @brief Create a rule to intercept secure erase commands.
 */
static void add_secure_erase_intercept_rule(void)
{
    stateful_rule_t rule = {0};
    rule.hw_rule.lba_start = 0;
    rule.hw_rule.lba_mask  = 0xFFFFFFFFFFFFFFFFULL;  /* Match any LBA */
    rule.hw_rule.opcode    = ATA_CMD_SECURITY_ERASE_UNIT;
    rule.hw_rule.opcode_mask = 0;  /* Exact opcode match */
    rule.hw_rule.direction = 0xFF; /* Both directions */
    rule.hw_rule.action    = 1;    /* Drop (block the erase) */
    rule.hw_rule.enabled   = 1;
    rule.hw_rule.priority  = 0;    /* Highest priority */
    rule.trigger_threshold = 1;
    rule.cooldown_ms = 0;          /* Always block */
    strncpy(rule.name, "Block Secure Erase", sizeof(rule.name) - 1);
    rule.persistent = true;

    policy_add_rule(&rule);
}

/**
 * @brief Create a rule to capture writes to the EFI System Partition.
 */
static void add_esp_write_capture_rule(void)
{
    stateful_rule_t rule = {0};
    /* Typical ESP starts at LBA 2048 and is ~100 MB (204800 sectors) */
    rule.hw_rule.lba_start = 2048;
    rule.hw_rule.lba_mask  = 0x00000000000FFFFFULL;  /* First ~204800 sectors from start */
    rule.hw_rule.opcode    = 0;     /* Any opcode */
    rule.hw_rule.opcode_mask = 0xFF; /* Wildcard */
    rule.hw_rule.direction = 1;     /* Write */
    rule.hw_rule.action    = 3;     /* Capture to scratch + notify ESP32 */
    rule.hw_rule.enabled   = 1;
    rule.hw_rule.priority  = 5;
    rule.trigger_threshold = 1;
    rule.cooldown_ms = 5000;        /* Don't spam captures */
    strncpy(rule.name, "ESP Write Capture", sizeof(rule.name) - 1);
    rule.persistent = true;

    policy_add_rule(&rule);
}

/**
 * @brief Load all default rules for first-time setup.
 */
void policy_load_defaults(void)
{
    ESP_LOGI(TAG, "Loading default rule set");
    num_stateful_rules = 0;
    add_boot_sector_monitor_rule();
    add_secure_erase_intercept_rule();
    add_esp_write_capture_rule();
    ESP_LOGI(TAG, "Default rules loaded: %d rules active", num_stateful_rules);
}

/* ===================================================================
 * Rule Hit Processing
 * =================================================================== */

/**
 * @brief Process a rule match event from the FPGA.
 */
static void process_rule_match(uint8_t rule_index)
{
    if (rule_index >= num_stateful_rules) {
        ESP_LOGW(TAG, "Rule match for unknown index %d", rule_index);
        return;
    }

    stateful_rule_t *rule = &stateful_rules[rule_index];
    rule->hit_count++;

    ESP_LOGD(TAG, "Rule '%s' hit #%d (threshold: %d)",
             rule->name, rule->hit_count, rule->trigger_threshold);

    /* Check if we should trigger */
    if (rule->hit_count >= rule->trigger_threshold) {
        TickType_t now = xTaskGetTickCount();
        TickType_t elapsed = now - rule->last_trigger_tick;

        if (elapsed >= pdMS_TO_TICKS(rule->cooldown_ms)) {
            rule->state = STATE_TRIGGERED;
            rule->last_trigger_tick = now;
            ESP_LOGI(TAG, "Rule '%s' TRIGGERED (hit #%d)", rule->name, rule->hit_count);

            /* Handle action types beyond FPGA's simple actions */
            switch (rule->hw_rule.action) {
                case 3: /* Inject after match */
                    if (rule->inject_len > 0) {
                        ctrl_cmd_t cmd = {
                            .cmd_type = CTRL_CMD_INJECT_FRAME,
                            .frame.data = malloc(rule->inject_len),
                            .frame.len = rule->inject_len
                        };
                        if (cmd.frame.data) {
                            memcpy(cmd.frame.data, rule->inject_payload, rule->inject_len);
                            xQueueSend(ctrl_cmd_queue, &cmd, pdMS_TO_TICKS(50));
                        }
                    }
                    break;

                case 2: /* Corrupt — handled by FPGA hardware */
                default:
                    break;
            }

            /* If cooldown_ms is 0, this is one-shot — disable the rule */
            if (rule->cooldown_ms == 0) {
                rule->hw_rule.enabled = 0;
                apply_rule_set();
            }
        }
    }
}

/* ===================================================================
 * Policy Engine Task
 * =================================================================== */

/**
 * @brief Main policy engine task loop.
 *
 * Listens for frame match events from the FPGA (via the ctrl_task's IRQ
 * handler which sets EVENT_BIT_FRAME_MATCH), processes them, and
 * implements stateful logic.
 */
void policy_engine_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Policy engine task started");

    /* Wait for FPGA to be ready */
    xEventGroupWaitBits(system_events, EVENT_BIT_FPGA_READY,
                        pdFALSE, pdTRUE, portMAX_DELAY);

    /* Load rules from NVS, or install defaults if first boot */
    load_rules_from_nvs();
    if (num_stateful_rules == 0) {
        ESP_LOGI(TAG, "No rules found — installing defaults");
        policy_load_defaults();
        save_rules_to_nvs();
    }

    /* Register with the capture data queue to receive frame match info */
    uint32_t capture_fis_type = 0;
    uint32_t capture_lba = 0;
    uint8_t  capture_direction = 0;

    while (1) {
        EventBits_t bits = xEventGroupWaitBits(
            system_events,
            EVENT_BIT_FRAME_MATCH | EVENT_BIT_CONFIG_CHANGE | EVENT_BIT_CAPTURE_AVAIL,
            pdTRUE,   /* Clear on read */
            pdFALSE,  /* Any bit */
            pdMS_TO_TICKS(500)
        );

        if (bits & EVENT_BIT_FRAME_MATCH) {
            /* Read the last matching rule index from FPGA */
            ctrl_cmd_t cmd = {
                .cmd_type = CTRL_CMD_READ_REG,
                .reg.addr = REG_RULE_LAST_MATCH
            };
            xQueueSend(ctrl_cmd_queue, &cmd, pdMS_TO_TICKS(50));
            /* In real impl: wait for response via a response queue */
            uint8_t match_idx = 0; /* Simplified */
            process_rule_match(match_idx);
        }

        if (bits & EVENT_BIT_CONFIG_CHANGE) {
            ESP_LOGI(TAG, "Configuration change detected — reloading rules");
            load_rules_from_nvs();
        }

        if (bits & EVENT_BIT_CAPTURE_AVAIL) {
            /* A captured frame is available for analysis */
            uint8_t cap_buf[1024];
            uint16_t cap_len = 0;
            ctrl_cmd_t cmd = {
                .cmd_type = CTRL_CMD_CAPTURE_READ,
                .capture.buffer = cap_buf,
                .capture.max_len = sizeof(cap_buf),
                .capture.actual_len = &cap_len
            };
            xQueueSend(ctrl_cmd_queue, &cmd, pdMS_TO_TICKS(50));
            /* Place captured data into proto_parser queue */
            /* Simplified: just log the event */
            if (cap_len > 0) {
                ESP_LOGD(TAG, "Captured %d bytes for further analysis", cap_len);
            }
        }

        /* Periodic maintenance */
        static int maint_counter = 0;
        if (++maint_counter >= 60) {  /* Every ~30 seconds */
            /* Check for expired rules */
            TickType_t now = xTaskGetTickCount();
            for (int i = 0; i < num_stateful_rules; i++) {
                if (stateful_rules[i].state == STATE_TRIGGERED) {
                    TickType_t elapsed = now - stateful_rules[i].last_trigger_tick;
                    if (elapsed > pdMS_TO_TICKS(60000)) {  /* 60 second timeout */
                        stateful_rules[i].state = STATE_EXPIRED;
                        ESP_LOGD(TAG, "Rule '%s' expired", stateful_rules[i].name);
                    }
                }
            }
            maint_counter = 0;
        }
    }
}
