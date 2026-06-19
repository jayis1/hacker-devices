/*
 * nmk_attack.h — NMK recovery attack interface
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef NMK_ATTACK_H
#define NMK_ATTACK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void     nmk_attack_init(void);
void     nmk_attack_abort(void);
int      nmk_attack_is_running(void);
int      nmk_attack_recovered(uint8_t nmk[16]);

int      nmk_attack_start_offline(const uint8_t *wordlist, uint32_t len,
                                    const uint8_t *salt, uint8_t salt_len,
                                    const uint8_t nek_enc[16]);
int      nmk_attack_start_online(const uint8_t *wordlist, uint32_t len,
                                   const uint8_t *salt, uint8_t salt_len);
void     nmk_attack_step(void);
uint32_t nmk_attack_progress(uint32_t *tried, uint32_t *rate_hps);

#ifdef __cplusplus
}
#endif

#endif /* NMK_ATTACK_H */