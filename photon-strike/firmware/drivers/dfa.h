/**
 * drivers/dfa.h — Piret-Quisquater AES-128 Differential Fault Analysis
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef PHOTONSTRIKE_DFA_H
#define PHOTONSTRIKE_DFA_H

#include <stdint.h>

#define PS_DFA_MAX_FAULTS  16   /* max faulted pairs to keep */

typedef struct {
    uint8_t  correct_ct[16];    /* expected (correct) ciphertext      */
    uint8_t  faulted_ct[16];    /* last faulted ciphertext            */
    uint8_t  fault_count;       /* number of faulted pairs fed        */
    uint8_t  key_candidates[16];/* recovered key bytes (0xFF = unknown)*/
    uint8_t  key_unique[16];    /* 1 if that byte is uniquely known   */
} ps_dfa_state_t;

void dfa_init(ps_dfa_state_t *s);
void dfa_feed(ps_dfa_state_t *s, const uint8_t *correct,
              const uint8_t *faulted, uint16_t len);
int  dfa_solve(ps_dfa_state_t *s, uint8_t out_key[16]);  /* returns #unique bytes */

#endif