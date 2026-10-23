/* Fail-safe transmit policy. Author: jayis1. */
#include "policy.h"
#include "../board.h"
#include <string.h>

#define UNLOCK_MAGIC 0x4c494e31u
#define UNLOCK_LIFETIME_US 30000000u

void policy_init(safety_policy_t *p) {
    memset(p,0,sizeof(*p));
    p->mode=MODE_OBSERVE;
    p->max_per_second=SAFE_MAX_TX_PER_SEC;
    p->max_mutations=2;
    p->min_target_mv=8000;
    p->max_target_mv=18000;
    /* IDs 0x3c/0x3d diagnostic are excluded until explicitly enabled. */
    p->allowed_ids=0x0fffffffffffffffull;
}

void policy_update_arm(safety_policy_t *p,bool pressed,uint32_t now) {
    if(pressed && !p->physically_armed) {
        p->physically_armed=true;
        p->unlock_deadline_us=now+UNLOCK_LIFETIME_US;
    } else if(!pressed && p->physically_armed) {
        p->physically_armed=false;
        p->session_unlocked=false;
        p->mode=MODE_OBSERVE;
        board_set_bypass(true);
    }
}

bool policy_unlock(safety_policy_t *p,uint32_t token,uint32_t now) {
    /* The token is deliberate friction, not cryptographic authentication.
       Physical possession and the ARM button remain mandatory. */
    if(!p->physically_armed || token!=UNLOCK_MAGIC) return false;
    p->session_unlocked=true;
    p->unlock_deadline_us=now+UNLOCK_LIFETIME_US;
    return true;
}

bool policy_set_mode(safety_policy_t *p,operation_mode_t mode,uint32_t now) {
    if(mode==MODE_OBSERVE) { p->mode=mode; board_set_bypass(true); return true; }
    if(!p->physically_armed || !p->session_unlocked) return false;
    if((int32_t)(p->unlock_deadline_us-now)<=0) return false;
    p->mode=mode;
    board_set_bypass(mode==MODE_INLINE);
    return true;
}

bool policy_allow_tx(safety_policy_t *p,const lin_frame_t *f,uint32_t now,uint16_t mv) {
    if(p->mode==MODE_OBSERVE || !p->physically_armed || !p->session_unlocked) return false;
    if((int32_t)(p->unlock_deadline_us-now)<=0) { policy_lock(p); return false; }
    if(mv<p->min_target_mv || mv>p->max_target_mv) return false;
    if(f->id>63u || ((p->allowed_ids>>f->id)&1u)==0u) return false;
    if(!f->parity_ok || f->length==0 || f->length>8) return false;
    if((uint32_t)(now-p->window_started_us)>=1000000u) {
        p->window_started_us=now;
        p->sent_in_window=0;
    }
    if(p->sent_in_window>=p->max_per_second) return false;
    p->sent_in_window++;
    return true;
}

void policy_lock(safety_policy_t *p) {
    p->session_unlocked=false;
    p->mode=MODE_OBSERVE;
    p->sent_in_window=0;
    board_set_bypass(true);
}

const char *policy_mode_name(operation_mode_t mode) {
    switch(mode) {
    case MODE_OBSERVE:return "observe";
    case MODE_INLINE:return "inline";
    case MODE_EMULATE:return "emulate";
    case MODE_FUZZ:return "fuzz";
    default:return "invalid";
    }
}
