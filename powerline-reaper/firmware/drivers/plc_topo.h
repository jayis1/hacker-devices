/*
 * plc_topo.h — PLC topology discovery interface
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef PLC_TOPO_H
#define PLC_TOPO_H

#include <stdint.h>
#include "qca7420.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PLC_MAX_STATIONS 32

typedef enum {
    PLC_ROLE_NONE = 0,
    PLC_ROLE_CCO,
    PLC_ROLE_PROXY,
    PLC_ROLE_LEAF
} plc_role_t;

typedef struct {
    uint8_t     mac[6];
    uint8_t     tei;
    uint8_t     snr_db;
    uint8_t     role;       /* plc_role_t */
    uint8_t     nmk_match;
    uint32_t    last_seen_ms;
} plc_station_t;

void     plc_topo_init(void);
void     plc_topo_observe(const qca7420_frame_t *f);
void     plc_topo_refresh(void);
uint32_t plc_topo_get_stations(plc_station_t *out, uint32_t max);
int      plc_topo_get_cco(uint8_t mac[6]);

#ifdef __cplusplus
}
#endif

#endif /* PLC_TOPO_H */