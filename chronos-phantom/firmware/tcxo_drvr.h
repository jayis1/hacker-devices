/*
 * tcxo_drvr.h — VC-TCXO frequency control and 1-PPS disciplining
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef CHRONOS_PHANTOM_TCXO_DRVR_H
#define CHRONOS_PHANTOM_TCXO_DRVR_H

#include <stdint.h>

#define TCXO_DAC_MAX      4095
#define TCXO_DAC_CENTER   2048
#define TCXO_DRIFT_MAX_PPB 500  /* max controlled drift in ppb */

typedef struct {
    uint16_t dac_value;
    int32_t  drift_ppb;     /* current applied drift in ppb */
    int32_t  target_ppb;    /* target drift */
    uint32_t pps_count;     /* 1-PPS pulses seen */
    int64_t  pps_offset_ns; /* offset between PPS and TCXO */
    uint8_t  disciplined;    /* GNSS disciplining active */
    /* PI controller state */
    int32_t  integ;
    int32_t  prop;
} tcxo_state_t;

void tcxo_init(tcxo_state_t *st);
void tcxo_set_drift(tcxo_state_t *st, int32_t target_ppb);
void tcxo_apply(tcxo_state_t *st);   /* write DAC */
void tcxo_pps_handler(tcxo_state_t *st, int64_t offset_ns);
void tcxo_discipline_tick(tcxo_state_t *st, uint32_t now_ms);
int32_t tcxo_get_drift(const tcxo_state_t *st);

#endif /* CHRONOS_PHANTOM_TCXO_DRVR_H */