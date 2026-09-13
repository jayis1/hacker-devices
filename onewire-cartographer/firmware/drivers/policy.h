/* OneWire Cartographer safety policy. Author: jayis1. MIT. */
#ifndef OWC_POLICY_H
#define OWC_POLICY_H
#include "../board.h"
#include "../registers.h"

typedef struct {
    bool session_unlocked;
    bool physical_arm;
    bool fault_latched;
    uint32_t unlock_deadline_ms;
    uint32_t window_started_ms;
    uint16_t actions_in_window;
    uint16_t denied_actions;
    owc_bridge_mode_t mode;
} owc_policy_t;

void owc_policy_init(owc_policy_t *policy);
void owc_policy_tick(owc_policy_t *policy, uint32_t now_ms);
bool owc_policy_unlock(owc_policy_t *policy, uint32_t challenge, uint32_t response);
bool owc_policy_authorize(owc_policy_t *policy, owc_command_t command, uint32_t now_ms);
bool owc_policy_set_mode(owc_policy_t *policy, owc_bridge_mode_t mode, uint32_t now_ms);
void owc_policy_latch_fault(owc_policy_t *policy);
bool owc_policy_clear_fault(owc_policy_t *policy);
uint32_t owc_policy_expected_response(uint32_t challenge);

#endif
