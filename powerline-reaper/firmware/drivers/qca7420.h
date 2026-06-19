/*
 * qca7420.h — QCA7420 PLC SoC driver interface
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef QCA7420_H
#define QCA7420_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PLC_MAX_FRAME        1600
#define PLC_RX_RING_LEN      32
#define PLC_TX_RING_LEN       16

typedef struct {
    uint32_t ts_ms;
    uint8_t  src[6];
    uint8_t  dst[6];
    uint8_t  opcode;
    uint8_t  tei;
    uint8_t  snr_db;
    uint16_t len;
    uint8_t  data[PLC_MAX_FRAME];
} qca7420_frame_t;

typedef void (*qca7420_frame_cb_t)(const qca7420_frame_t *f);

/* Lifecycle */
int  qca7420_init(qca7420_frame_cb_t cb);
void qca7420_reset(void);
int  qca7420_sync(uint32_t timeout_ms);

/* Mode */
void qca7420_set_promisc(int on);
void qca7420_wipe_keys(void);
int  qca7420_set_nmk(const uint8_t nmk[16]);

/* TX */
int  qca7420_tx_frame(const uint8_t *buf, uint16_t len, uint8_t flags);
int  qca7420_rogue_cco(const uint8_t ccobeacon[64]);
int  qca7420_deauth(uint8_t tei);

/* RX (ISR + poll) */
void qca7420_irq_handler(void);
void qca7420_poll(void);

/* Info */
void qca7420_get_mac(uint8_t mac[6]);

#ifdef __cplusplus
}
#endif

#endif /* QCA7420_H */