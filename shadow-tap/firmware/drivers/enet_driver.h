/*
 * enet_driver.h — ShadowTap Ethernet driver for i.MX RT1062 dual ENET
 *
 * Provides:
 *  - Zero-copy DMA ring buffer management
 *  - Cut-through forwarding between two ports
 *  - Promiscuous transparent bridge mode
 *  - Frame matching and MITM integration hooks
 */

#ifndef SHADOWTAP_ENET_DRIVER_H
#define SHADOWTAP_ENET_DRIVER_H

#include "board.h"
#include "registers.h"

/* ENET driver context */
typedef struct {
    ENET_Type      *base;          /* ENET1 or ENET2 base */
    enet_bd_t      *rx_bd;         /* RX buffer descriptor ring */
    enet_bd_t      *tx_bd;         /* TX buffer descriptor ring */
    uint8_t        (*rx_buf)[ENET_RX_BUFFER_SIZE];
    uint8_t        (*tx_buf)[ENET_RX_BUFFER_SIZE];
    volatile uint32_t rx_idx;     /* Current RX BD index */
    volatile uint32_t tx_idx;     /* Current TX BD index */
    volatile uint32_t tx_done_idx;/* Last completed TX BD */
    uint32_t       rx_count;      /* Frames received */
    uint32_t       tx_count;      /* Frames transmitted */
    uint32_t       drop_count;    /* Frames dropped (no TX BD available) */
    uint32_t       error_count;   /* RX errors */
    uint8_t        port_id;       /* 0=uplink, 1=target */
} enet_driver_t;

/* Port identifiers */
#define ENET_PORT_UPLINK    0U
#define ENET_PORT_TARGET    1U

/* Function prototypes */

/**
 * Initialize ENET driver context for a given port.
 * @param drv       Driver context to initialize
 * @param port_id   ENET_PORT_UPLINK or ENET_PORT_TARGET
 * @param base      ENET1 or ENET2 register base
 * @param rx_bd     RX buffer descriptor ring (64-byte aligned)
 * @param tx_bd     TX buffer descriptor ring (64-byte aligned)
 * @param rx_buf    RX buffer pool
 * @param tx_buf    TX buffer pool
 */
void enet_driver_init(enet_driver_t *drv, uint8_t port_id, ENET_Type *base,
                      enet_bd_t *rx_bd, enet_bd_t *tx_bd,
                      uint8_t (*rx_buf)[ENET_RX_BUFFER_SIZE],
                      uint8_t (*tx_buf)[ENET_RX_BUFFER_SIZE]);

/**
 * Poll for received frames and forward to opposite port.
 * Called from ISR or main loop.
 * @param drv      This port's driver
 * @param dest     Destination port's driver (for forwarding)
 * @return Number of frames processed
 */
uint32_t enet_driver_rx_poll(enet_driver_t *drv, enet_driver_t *dest);

/**
 * Transmit a frame on this port.
 * @param drv      Driver context
 * @param frame    Frame data
 * @param len      Frame length
 * @return 0 on success, -1 if no TX BD available
 */
int8_t enet_driver_tx(enet_driver_t *drv, const uint8_t *frame, uint16_t len);

/**
 * Get link status from PHY via MDIO.
 * @param drv      Driver context
 * @return 1 if link up, 0 if link down
 */
uint8_t enet_driver_get_link(enet_driver_t *drv);

/**
 * Get driver statistics.
 * @param drv      Driver context
 * @param rx_count Frames received (output)
 * @param tx_count Frames transmitted (output)
 * @param drops    Frames dropped (output)
 * @param errors   RX errors (output)
 */
void enet_driver_get_stats(enet_driver_t *drv,
                           uint32_t *rx_count, uint32_t *tx_count,
                           uint32_t *drops, uint32_t *errors);

#endif /* SHADOWTAP_ENET_DRIVER_H */