/* Transmission safety policy. Author: jayis1. */
#ifndef LIN_MIMIC_POLICY_H
#define LIN_MIMIC_POLICY_H
#include "lin.h"
typedef enum { MODE_OBSERVE, MODE_INLINE, MODE_EMULATE, MODE_FUZZ } operation_mode_t;
typedef struct {
 operation_mode_t mode; bool physically_armed; bool session_unlocked;
 uint64_t allowed_ids; uint32_t unlock_deadline_us; uint32_t window_started_us;
 uint16_t sent_in_window; uint16_t max_per_second; uint8_t max_mutations;
 uint16_t min_target_mv; uint16_t max_target_mv;
} safety_policy_t;
void policy_init(safety_policy_t *policy);
void policy_update_arm(safety_policy_t *policy,bool pressed,uint32_t now);
bool policy_unlock(safety_policy_t *policy,uint32_t token,uint32_t now);
bool policy_set_mode(safety_policy_t *policy,operation_mode_t mode,uint32_t now);
bool policy_allow_tx(safety_policy_t *policy,const lin_frame_t *frame,uint32_t now,uint16_t mv);
void policy_lock(safety_policy_t *policy);
const char *policy_mode_name(operation_mode_t mode);
#endif
