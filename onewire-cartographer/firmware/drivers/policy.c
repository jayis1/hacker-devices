/*
 * Physical-consent, timeout, fault and rate-limit policy engine
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#include "policy.h"
#include "onewire_phy.h"
#include <string.h>

#define SESSION_LIFETIME_MS  300000u
#define RATE_WINDOW_MS       1000u
#define RATE_ACTION_LIMIT    24u
#define RESPONSE_SALT        0x4A415931u /* JAY1 */

static bool is_passive(owc_command_t command)
{
    switch (command) {
    case OWC_CMD_GET_INFO:
    case OWC_CMD_GET_STATUS:
    case OWC_CMD_CAPTURE_START:
    case OWC_CMD_CAPTURE_STOP:
    case OWC_CMD_CAPTURE_READ:
    case OWC_CMD_SCAN_ROMS:
        return true;
    default:
        return false;
    }
}

uint32_t owc_policy_expected_response(uint32_t challenge)
{
    uint32_t value = challenge ^ RESPONSE_SALT;
    value ^= value << 13u;
    value ^= value >> 17u;
    value ^= value << 5u;
    return value ^ 0xA5C35A7Du;
}

void owc_policy_init(owc_policy_t *policy)
{
    if (policy == NULL) {
        return;
    }
    memset(policy, 0, sizeof(*policy));
    policy->mode = OWC_MODE_FAIL_OPEN;
}

void owc_policy_tick(owc_policy_t *policy, uint32_t now_ms)
{
    if (policy == NULL) {
        return;
    }
    policy->physical_arm = board_gpio_read(PIN_ARM_SWITCH);
    if ((int32_t)(now_ms - policy->unlock_deadline_ms) >= 0) {
        policy->session_unlocked = false;
    }
    if (now_ms - policy->window_started_ms >= RATE_WINDOW_MS) {
        policy->window_started_ms = now_ms;
        policy->actions_in_window = 0u;
    }
    if (!policy->physical_arm && policy->mode == OWC_MODE_LAB_DRIVE) {
        policy->mode = OWC_MODE_MONITOR;
        owc_phy_release_all();
    }
}

bool owc_policy_unlock(owc_policy_t *policy,
                       uint32_t challenge,
                       uint32_t response)
{
    if (policy == NULL || !board_gpio_read(PIN_ARM_SWITCH)) {
        return false;
    }
    if (response != owc_policy_expected_response(challenge)) {
        policy->denied_actions++;
        return false;
    }
    policy->session_unlocked = true;
    policy->physical_arm = true;
    policy->unlock_deadline_ms = board_millis() + SESSION_LIFETIME_MS;
    return true;
}

bool owc_policy_authorize(owc_policy_t *policy,
                          owc_command_t command,
                          uint32_t now_ms)
{
    if (policy == NULL) {
        return false;
    }
    owc_policy_tick(policy, now_ms);
    if (is_passive(command)) {
        return true;
    }
    if (policy->fault_latched || !policy->physical_arm ||
        !policy->session_unlocked) {
        policy->denied_actions++;
        return false;
    }
    if (policy->actions_in_window >= RATE_ACTION_LIMIT) {
        policy->denied_actions++;
        return false;
    }
    policy->actions_in_window++;
    return true;
}

bool owc_policy_set_mode(owc_policy_t *policy,
                         owc_bridge_mode_t mode,
                         uint32_t now_ms)
{
    if (policy == NULL || mode > OWC_MODE_LAB_DRIVE) {
        return false;
    }
    if (mode == OWC_MODE_LAB_DRIVE &&
        !owc_policy_authorize(policy, OWC_CMD_BRIDGE_MODE, now_ms)) {
        return false;
    }
    if (policy->fault_latched && mode != OWC_MODE_FAIL_OPEN) {
        return false;
    }
    policy->mode = mode;
    board_gpio_write(PIN_BRIDGE_ENABLE,
                     mode == OWC_MODE_MONITOR || mode == OWC_MODE_FAIL_OPEN);
    if (mode != OWC_MODE_LAB_DRIVE) {
        owc_phy_release_all();
    }
    return true;
}

void owc_policy_latch_fault(owc_policy_t *policy)
{
    if (policy == NULL) {
        return;
    }
    policy->fault_latched = true;
    policy->session_unlocked = false;
    policy->mode = OWC_MODE_FAIL_OPEN;
    owc_phy_release_all();
    board_gpio_write(PIN_BRIDGE_ENABLE, true);
    board_gpio_write(PIN_FAULT_LED, true);
}

bool owc_policy_clear_fault(owc_policy_t *policy)
{
    if (policy == NULL || !board_gpio_read(PIN_ARM_SWITCH)) {
        return false;
    }
    if (!owc_phy_bus_safe(OWC_PORT_UPSTREAM) ||
        !owc_phy_bus_safe(OWC_PORT_DOWNSTREAM)) {
        return false;
    }
    policy->fault_latched = false;
    policy->mode = OWC_MODE_MONITOR;
    board_gpio_write(PIN_FAULT_LED, false);
    board_gpio_write(PIN_BRIDGE_ENABLE, true);
    return true;
}
