/*
 * phy_driver.h — Marvell 88Q2112 100/1000BASE-T1 PHY driver
 * Author: jayis1
 * License: GPL-2.0
 * Date:   2026-07-22
 */
#ifndef AXLETAP_PHY_DRIVER_H
#define AXLETAP_PHY_DRIVER_H

#include <stdint.h>

/* PHY link mode */
typedef enum {
    PHY_MODE_PASSIVE = 0,    /* Link up, no transmit */
    PHY_MODE_100T1   = 1,    /* 100BASE-T1 forced */
    PHY_MODE_1000T1  = 2,    /* 1000BASE-T1 forced */
    PHY_MODE_AUTO    = 3     /* Auto-negotiate T1 */
} phy_mode_t;

/* PHY role */
typedef enum {
    PHY_ROLE_MASTER = 0,
    PHY_ROLE_SLAVE  = 1,
    PHY_ROLE_AUTO   = 2
} phy_role_t;

/* PHY status snapshot */
typedef struct {
    uint8_t  link_up;
    uint8_t  speed;       /* 0 = down, 1 = 100T1, 2 = 1000T1 */
    uint8_t  duplex;      /* 0 = half, 1 = full */
    uint8_t  master;      /* 1 = master, 0 = slave */
    uint32_t link_time_ms;
    int8_t   sqi;         /* Signal Quality Indicator 0..100 (best) */
} phy_status_t;

/* Initialize a PHY. addr = MDIO address. mode sets link negotiation. */
int phy_init(uint8_t addr, phy_mode_t mode, phy_role_t role);

/* Reset a PHY via hardware reset GPIO (PHY A or B). */
void phy_hw_reset(uint8_t addr);

/* Read the PHY status snapshot. */
void phy_get_status(uint8_t addr, phy_status_t *st);

/* Force a downgrade from 1000BASE-T1 to 100BASE-T1 (link attack).
 * Returns 0 on success, negative on error.
 */
int phy_force_downgrade(uint8_t addr);

/* Set the master/slave preference for the next link-up. */
int phy_set_role(uint8_t addr, phy_role_t role);

/* Read/ write a PHY register via MDIO. */
uint16_t phy_mdio_read(uint8_t addr, uint8_t reg);
void     phy_mdio_write(uint8_t addr, uint8_t reg, uint16_t val);

/* MMD register access (extended register space). */
uint16_t phy_mmd_read(uint8_t addr, uint8_t dev, uint16_t reg);
void     phy_mmd_write(uint8_t addr, uint8_t dev, uint16_t reg, uint16_t val);

/* Expected PHY ID for Marvell 88Q2112 (OUI 0x5043, model 0x22). */
#define MV_88Q2112_ID1   0x0141
#define MV_88Q2112_ID2   0x0F20

#endif /* AXLETAP_PHY_DRIVER_H */