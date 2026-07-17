/*
 * pd_phy.h — USB-C Power Delivery PHY (FUSB302B) driver interface for CC-Stiletto.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * Two FUSB302B devices are used: one toward the source (U2, I2C1) and one toward
 * the sink (U3, I2C2). Each is driven through this same interface, parameterized
 * by a pd_phy_t struct that holds the I2C peripheral and interrupt pin.
 */

#ifndef CC_STILETTO_PD_PHY_H
#define CC_STILETTO_PD_PHY_H

#include <stdint.h>
#include <stdbool.h>
#include "registers.h"

/* FUSB302B I2C slave address (7-bit, shifted left by driver) */
#define FUSB302_I2C_ADDR_7BIT    0x22u

/* ---- FUSB302B register map (excerpt used by driver) ------------------------ */
#define FUSB_DEVICE_ID           0x01u
#define FUSB_SWITCHES0           0x02u
#define FUSB_SWITCHES1           0x03u
#define FUSB_MEASURE             0x04u
#define FUSB_SLICE0              0x08u
#define FUSB_SLICE1              0x09u
#define FUSB_CONTROL0           0x06u
#define FUSB_CONTROL1            0x07u
#define FUSB_CONTROL2            0x08u
#define FUSB_CONTROL3            0x09u
#define FUSB_CONTROL4            0x0Cu
#define FUSB_FIFOS                0x3Du   /* TX/RX FIFO access */
#define FUSB_STATUS0              0x40u
#define FUSB_STATUS1              0x41u
#define FUSB_INTERRUPTA          0x42u
#define FUSB_INTERRUPTB          0x43u
#define FUSB_INTERRUPTC          0x44u

/* SOP token prefixes written to TX FIFO before a PD message body */
#define SOP_SOP                   0x12u    /* SOP */
#define SOP_SOPP                  0x13u    /* SOP'  (cable / e-marker) */
#define FUSB_TXON                  0x26u   /* control byte: transmit on */
#define FUSB_RXORDERSET           0x40u    /* RX FIFO: order set marker */
#define FUSB_RXSOPX                0x41u
#define FUSB_RXSOPXP              0x42u
#define FUSB_RXSOPXX              0x43u

/* SWITCHES0 bits — CC routing */
#define SW0_CC1_EN                 (1u << 0)
#define SW0_CC1_PD                  (1u << 1)
#define SW0_CC2_EN                 (1u << 2)
#define SW0_CC2_PD                  (1u << 3)
#define SW0_VCONN_CC1              (1u << 4)
#define SW0_VCONN_CC2              (1u << 5)

/* SWITCHES1 bits — auto GoodCRC + role */
#define SW1_SPECREV0              (1u << 0)
#define SW1_SPECREV1              (1u << 1)
#define SW1_AUTO_CRC              (1u << 2)
#define SW1_TXCC1                   (1u << 4)
#define SW1_TXCC2                   (1u << 5)

/* ---- Orientation + CC role ------------------------------------------------ */
typedef enum {
    CC_NONE = 0,
    CC1_ACTIVE,
    CC2_ACTIVE
} cc_orientation_t;

typedef enum {
    PD_ROLE_SINK = 0,    /* UFP — device */
    PD_ROLE_SOURCE = 1,  /* DFP — host */
} pd_data_role_t;

typedef enum {
    PD_POWER_SINK = 0,
    PD_POWER_SOURCE = 1,
} pd_power_role_t;

/* A decoded PD message as it comes out of the FUSB302B RX FIFO */
typedef struct {
    uint8_t  sop_type;     /* 0=SOP, 1=SOP', 2=SOP'' */
    uint16_t header;       /* raw 16-bit header */
    uint8_t  msg_id;
    uint8_t  num_obj;      /* number of 32-bit data objects (0..7) */
    uint32_t obj[7];
} pd_msg_t;

/* Driver handle */
typedef struct {
    i2c_reg_t   *i2c;       /* I2C1 (source) or I2C2 (sink) */
    gpio_reg_t  *int_port;   /* interrupt port */
    uint32_t     int_pin;
    uint8_t      addr_w;     /* 8-bit write addr */
    uint8_t      addr_r;     /* 8-bit read addr */
    cc_orientation_t orient;
    pd_data_role_t   data_role;
    pd_power_role_t  power_role;
} pd_phy_t;

/* ---- API ------------------------------------------------------------------- */

void    pd_phy_init(pd_phy_t *p);
bool    pd_phy_detect(pd_phy_t *p);              /* returns true if FUSB302B found */
int     pd_phy_set_role(pd_phy_t *p, pd_power_role_t pr, pd_data_role_t dr);
int     pd_phy_detect_cc(pd_phy_t *p);            /* returns cc_orientation_t */
int     pd_phy_enable_vconn(pd_phy_t *p, bool en);
int     pd_phy_send(pd_phy_t *p, uint8_t sop, const uint16_t hdr,
                   const uint32_t *obj, uint8_t nobj);
int     pd_phy_recv(pd_phy_t *p, pd_msg_t *out);   /* non-blocking; 1 if msg, 0 if none */
int     pd_phy_send_hard_reset(pd_phy_t *p);
int     pd_phy_send_bist(pd_phy_t *p, uint8_t bist_mode);
void    pd_phy_flush_rx(pd_phy_t *p);

/* Low-level I2C helpers (exposed for testing / console) */
int     fusb_write(pd_phy_t *p, uint8_t reg, uint8_t val);
int     fusb_writes(pd_phy_t *p, uint8_t reg, const uint8_t *buf, uint8_t n);
int     fusb_read(pd_phy_t *p, uint8_t reg, uint8_t *val);
int     fusb_reads(pd_phy_t *p, uint8_t reg, uint8_t *buf, uint8_t n);

#endif /* CC_STILETTO_PD_PHY_H */