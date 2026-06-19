/*
 * plc_topo.c — PLC network topology discovery & station table
 *
 * Snoops beacons and association/disassociation management frames to build
 * a live table of stations on the segment: MAC, TEI, role (CCo/proxy/leaf),
 * SNR, and NMK-match status. Queries the QCA7420 CCo GetStationList primitive
 * to refresh.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <stdint.h>
#include <string.h>
#include "plc_topo.h"
#include "qca7420.h"

static plc_station_t stations[PLC_MAX_STATIONS];
static uint32_t station_count = 0;
static uint8_t  cco_mac[6];
static int      have_cco = 0;

void plc_topo_init(void) {
    memset(stations, 0, sizeof(stations));
    station_count = 0;
    have_cco = 0;
}

static plc_station_t *find_or_add(const uint8_t mac[6]) {
    for (uint32_t i = 0; i < station_count; i++) {
        if (memcmp(stations[i].mac, mac, 6) == 0) return &stations[i];
    }
    if (station_count >= PLC_MAX_STATIONS) {
        /* evict oldest */
        memmove(&stations[0], &stations[1], sizeof(plc_station_t) * (PLC_MAX_STATIONS - 1));
        station_count = PLC_MAX_STATIONS - 1;
    }
    plc_station_t *s = &stations[station_count++];
    memset(s, 0, sizeof(*s));
    memcpy(s->mac, mac, 6);
    s->last_seen_ms = 0;
    return s;
}

void plc_topo_observe(const qca7420_frame_t *f) {
    switch (f->opcode) {
    case 0x01: { /* Beacon */
        plc_station_t *s = find_or_add(f->src);
        s->last_seen_ms = f->ts_ms;
        s->snr_db = f->snr_db;
        s->tei = f->tei;
        /* If beacon's CCO_PRI byte == 0, sender is CCo */
        if (f->len >= 16 && f->data[14] == 0x00) {
            s->role = PLC_ROLE_CCO;
            memcpy(cco_mac, f->src, 6);
            have_cco = 1;
        } else {
            s->role = PLC_ROLE_LEAF;
        }
        s->nmk_match = (f->data[15] & 0x80) ? 1 : 0;
        break;
    }
    case 0x04: { /* Assoc confirm */
        plc_station_t *s = find_or_add(f->src);
        s->last_seen_ms = f->ts_ms;
        s->tei = f->tei;
        s->role = PLC_ROLE_LEAF;
        break;
    }
    case 0x07: { /* Disassoc */
        for (uint32_t i = 0; i < station_count; i++) {
            if (stations[i].tei == f->tei) {
                stations[i].last_seen_ms = f->ts_ms;
                stations[i].role = PLC_ROLE_NONE;
                break;
            }
        }
        break;
    }
    default:
        break;
    }
}

void plc_topo_refresh(void) {
    /* Query QCA7420 GetStationList (opcode 0x0E) — synthesized as a TX mgmt
     * frame; the response comes back as a normal RX frame observed above.
     */
    uint8_t req[8] = {0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    qca7420_tx_frame(req, sizeof(req), 0x20 /* mgmt */);
    /* Age out stations not seen in 30 s */
    uint32_t now = stations[0].last_seen_ms; /* approximate; real impl uses millis() */
    for (uint32_t i = 0; i < station_count; ) {
        if ((now - stations[i].last_seen_ms) > 30000 && stations[i].last_seen_ms != 0) {
            stations[i] = stations[station_count - 1];
            station_count--;
        } else {
            i++;
        }
    }
}

uint32_t plc_topo_get_stations(plc_station_t *out, uint32_t max) {
    uint32_t n = (station_count < max) ? station_count : max;
    memcpy(out, stations, n * sizeof(plc_station_t));
    return n;
}

int plc_topo_get_cco(uint8_t mac[6]) {
    if (!have_cco) return -1;
    memcpy(mac, cco_mac, 6);
    return 0;
}