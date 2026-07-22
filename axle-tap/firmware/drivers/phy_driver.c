/*
 * phy_driver.c — Marvell 88Q2112 100/1000BASE-T1 PHY driver for AxleTap
 * Author: jayis1
 * License: GPL-2.0
 * Date:   2026-07-22
 *
 * Drives the two automotive Ethernet PHYs (link A and link B). Both are
 * Marvell 88Q2112-class, accessed via the STM32H7's built-in Ethernet
 * MDIO interface. Provides:
 *   - Hardware reset via GPIO
 *   - Link mode selection (100BASE-T1 / 1000BASE-T1 / auto)
 *   - Master/slave role control (auto / forced master / forced slave)
 *   - SQI (signal quality) readout
 *   - Force-downgrade attack (force a 1000BASE-T1 link down to 100BASE-T1)
 *   - Raw MDIO + MMD register access
 */

#include "phy_driver.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/* MDIO primitive — bit-banged over GPIO because the STM32H7 MAC MDIO
 * is busy driving the link. For AxleTap we bit-bang MDC/MDIO on
 * dedicated GPIO pins (PC2 / PA10 alternate mapping) for both PHYs and
 * the L2 switch.
 *
 * MDIO clock: 2.5 MHz max. We use a relaxed 1 MHz cycle (1 us high, 1 us
 * low) for robustness over long inline harnesses.
 */
/* ------------------------------------------------------------------ */

#define MDIO_GPIO_MDC_PORT   GPIOC
#define MDIO_GPIO_MDC_PIN    2
#define MDIO_GPIO_MDIO_PORT  GPIOA
#define MDIO_GPIO_MDIO_PIN   10

static void mdio_delay_us(uint32_t us)
{
    /* Approximate 1 us at 550 MHz with a few cycles per iteration */
    volatile uint32_t n = us * 200UL;
    while (n--) { __asm__ volatile("nop"); }
}

static void mdio_clock(void)
{
    /* Rising edge on MDC samples/shifts MDIO */
    MDIO_GPIO_MDC_PORT->BSRR = BIT(MDIO_GPIO_MDC_PIN);    /* high */
    mdio_delay_us(1);
    MDIO_GPIO_MDC_PORT->BSRR = BIT(MDIO_GPIO_MDC_PIN) << 16; /* low */
    mdio_delay_us(1);
}

static void mdio_set_output(void)
{
    uint32_t moder = MDIO_GPIO_MDIO_PORT->MODER;
    moder &= ~(0x3 << (MDIO_GPIO_MDIO_PIN * 2));
    moder |= (GPIO_MODE_OUT << (MDIO_GPIO_MDIO_PIN * 2));
    MDIO_GPIO_MDIO_PORT->MODER = moder;
    /* Open-drain-ish: drives 0, releases 1 via pull-up */
    MDIO_GPIO_MDIO_PORT->OTYPER &= ~(1 << MDIO_GPIO_MDIO_PIN);
}

static void mdio_set_input(void)
{
    uint32_t moder = MDIO_GPIO_MDIO_PORT->MODER;
    moder &= ~(0x3 << (MDIO_GPIO_MDIO_PIN * 2));
    moder |= (GPIO_MODE_IN << (MDIO_GPIO_MDIO_PIN * 2));
    MDIO_GPIO_MDIO_PORT->MODER = moder;
}

static void mdio_write_bit(int v)
{
    if (v) MDIO_GPIO_MDIO_PORT->BSRR = BIT(MDIO_GPIO_MDIO_PIN);
    else   MDIO_GPIO_MDIO_PORT->BSRR = BIT(MDIO_GPIO_MDIO_PIN) << 16;
    mdio_clock();
}

static int mdio_read_bit(void)
{
    int v = (MDIO_GPIO_MDIO_PORT->IDR >> MDIO_GPIO_MDIO_PIN) & 1;
    mdio_clock();
    return v;
}

uint16_t phy_mdio_read(uint8_t addr, uint8_t reg)
{
    uint16_t val = 0;
    int i;

    mdio_set_output();
    /* 32-bit preamble */
    for (i = 0; i < 32; i++) mdio_write_bit(1);
    /* ST = 01, OP = 10 (read) */
    mdio_write_bit(0); mdio_write_bit(1);
    mdio_write_bit(1); mdio_write_bit(0);
    /* PHY addr 5 bits */
    for (i = 4; i >= 0; i--) mdio_write_bit((addr >> i) & 1);
    /* Reg 5 bits */
    for (i = 4; i >= 0; i--) mdio_write_bit((reg >> i) & 1);
    /* Turnaround: 2 clocks high-Z (we read) */
    mdio_set_input();
    mdio_clock();   /* TA bit 1 - should read 0 */
    mdio_clock();   /* TA bit 0 */
    /* 16-bit data, MSB first */
    for (i = 15; i >= 0; i--) {
        val |= (uint16_t)mdio_read_bit() << i;
    }
    /* Leave MDIO high */
    mdio_set_output();
    mdio_write_bit(1);
    return val;
}

void phy_mdio_write(uint8_t addr, uint8_t reg, uint16_t val)
{
    int i;
    mdio_set_output();
    for (i = 0; i < 32; i++) mdio_write_bit(1);   /* preamble */
    /* ST = 01, OP = 01 (write) */
    mdio_write_bit(0); mdio_write_bit(1);
    mdio_write_bit(0); mdio_write_bit(1);
    for (i = 4; i >= 0; i--) mdio_write_bit((addr >> i) & 1);
    for (i = 4; i >= 0; i--) mdio_write_bit((reg >> i) & 1);
    /* Turnaround: 1,0 */
    mdio_write_bit(1); mdio_write_bit(0);
    for (i = 15; i >= 0; i--) mdio_write_bit((val >> i) & 1);
    /* Leave MDIO high */
    mdio_write_bit(1);
}

uint16_t phy_mmd_read(uint8_t addr, uint8_t dev, uint16_t reg)
{
    phy_mdio_write(addr, PHY_MMD_CTRL, 0x0000 | dev);    /* address */
    phy_mdio_write(addr, PHY_MMD_DATA, reg);
    phy_mdio_write(addr, PHY_MMD_CTRL, 0x4000 | dev);    /* data */
    return phy_mdio_read(addr, PHY_MMD_DATA);
}

void phy_mmd_write(uint8_t addr, uint8_t dev, uint16_t reg, uint16_t val)
{
    phy_mdio_write(addr, PHY_MMD_CTRL, 0x0000 | dev);
    phy_mdio_write(addr, PHY_MMD_DATA, reg);
    phy_mdio_write(addr, PHY_MMD_CTRL, 0x4000 | dev);
    phy_mdio_write(addr, PHY_MMD_DATA, val);
}

/* ------------------------------------------------------------------ */
/* Hardware reset                                                      */
/* ------------------------------------------------------------------ */
void phy_hw_reset(uint8_t addr)
{
    GPIO_TypeDef *port;
    int pin;
    if (addr == PHYA_MDIO_ADDR) { port = PHYA_RESET_PORT; pin = PHYA_RESET_PIN; }
    else if (addr == PHYB_MDIO_ADDR) { port = PHYB_RESET_PORT; pin = PHYB_RESET_PIN; }
    else return;

    /* Assert reset (low) for 50 ms, release, wait 100 ms for link-up */
    port->BSRR = BIT(pin) << 16;     /* low */
    mdio_delay_us(50000);
    port->BSRR = BIT(pin);            /* high */
    mdio_delay_us(100000);
}

/* ------------------------------------------------------------------ */
/* Initialization                                                       */
/* ------------------------------------------------------------------ */
int phy_init(uint8_t addr, phy_mode_t mode, phy_role_t role)
{
    uint16_t id1, id2, bmcr;

    phy_hw_reset(addr);

    id1 = phy_mdio_read(addr, PHY_ID1);
    id2 = phy_mdio_read(addr, PHY_ID2);
    if (id1 != MV_88Q2112_ID1 || (id2 & 0xFFF0) != (MV_88Q2112_ID2 & 0xFFF0)) {
        /* PHY not responding or wrong model */
        return -1;
    }

    /* Soft reset */
    phy_mdio_write(addr, PHY_BMCR, BMCR_RESET);
    do { bmcr = phy_mdio_read(addr, PHY_BMCR); } while (bmcr & BMCR_RESET);

    /* Disable auto-negotiate for forced modes; configure for T1 */
    uint16_t ctrl = 0;
    if (mode == PHY_MODE_AUTO) {
        ctrl |= BMCR_ANEG_EN;
    } else {
        ctrl |= BMCR_DUPLEX;       /* full duplex */
    }
    if (mode == PHY_MODE_1000T1) {
        ctrl |= BMCR_SPEED_MSB;    /* 1000 */
    } else if (mode == PHY_MODE_100T1) {
        ctrl |= BMCR_SPEED_LSB;    /* 100 */
    }

    /* Set master/slave preference via MMD */
    uint16_t role_val = 0;
    if (role == PHY_ROLE_MASTER)      role_val = 0x4000;  /* force master */
    else if (role == PHY_ROLE_SLAVE)  role_val = 0x0000;   /* force slave */
    else                              role_val = 0x2000;   /* auto */
    phy_mmd_write(addr, 0x1, 0x0834, role_val);

    /* For AxleTap inline bridge: PHY A = master, PHY B = slave so that
     * the two sides of the broken link both see a valid link partner.
     * The ECU side expects to be master on its 1000BASE-T1 link, so
     * AxleTap's PHY A (toward ECU) is slave; the compute side expects
     * to be slave, so AxleTap's PHY B (toward compute) is master.
     */
    phy_mdio_write(addr, PHY_BMCR, ctrl);
    if (mode != PHY_MODE_AUTO) {
        /* Restart ANEG to apply */
        phy_mdio_write(addr, PHY_BMCR, ctrl | BMCR_RESTART_ANEG);
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Status                                                              */
/* ------------------------------------------------------------------ */
void phy_get_status(uint8_t addr, phy_status_t *st)
{
    uint16_t bmsr, stat;
    memset(st, 0, sizeof(*st));
    bmsr = phy_mdio_read(addr, PHY_BMSR);
    st->link_up = (bmsr & BMSR_LINK) ? 1 : 0;
    st->duplex  = 1;  /* T1 always full-duplex */
    if (bmsr & BMSR_1000T1)      st->speed = 2;
    else if (bmsr & BMSR_100T1)  st->speed = 1;
    /* Role readout via MMD */
    uint16_t r = phy_mmd_read(addr, 0x1, 0x0834);
    st->master = (r & 0x4000) ? 1 : 0;
    /* SQI from extended status register */
    stat = phy_mdio_read(addr, PHY_EXT_STATUS);
    st->sqi = (int8_t)((stat >> 8) & 0x7F);
    if (st->sqi > 100) st->sqi = 100;
}

/* ------------------------------------------------------------------ */
/* Force-downgrade attack                                              */
/* ------------------------------------------------------------------ */
int phy_force_downgrade(uint8_t addr)
{
    /* Force a 1000BASE-T1 link to re-link as 100BASE-T1 by clearing the
     * 1000BASE-T1 advertisement and advertising only 100BASE-T1, then
     * restarting auto-negotiation. The link partner will fall back.
     */
    uint16_t anar = phy_mdio_read(addr, PHY_ANAR);
    anar &= ~0x0080;   /* drop 1000BASE-T1 adv bit */
    phy_mdio_write(addr, PHY_ANAR, anar);
    /* Advertise 100BASE-T1 in PHY_100T1_CTRL */
    phy_mdio_write(addr, PHY_100T1_CTRL, 0x0001);  /* 100BASE-T1 adv */
    /* Restart ANEG */
    uint16_t bmcr = phy_mdio_read(addr, PHY_BMCR);
    phy_mdio_write(addr, PHY_BMCR, bmcr | BMCR_RESTART_ANEG | BMCR_ANEG_EN);
    return 0;
}

int phy_set_role(uint8_t addr, phy_role_t role)
{
    uint16_t v = 0;
    if (role == PHY_ROLE_MASTER) v = 0x4000;
    else if (role == PHY_ROLE_SLAVE) v = 0x0000;
    else v = 0x2000;
    phy_mmd_write(addr, 0x1, 0x0834, v);
    return 0;
}