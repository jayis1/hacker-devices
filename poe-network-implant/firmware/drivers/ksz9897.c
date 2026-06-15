/*
 * ksz9897.c — KSZ9897R Gigabit Managed Switch Driver
 * PhantomBridge PoE Network Implant
 *
 * MDIO (Clause 22) interface for register access.
 * The MCU's ETH_MDC/ETH_MDIO pins drive the KSZ9897 management port.
 */

#include "ksz9897.h"
#include "../board.h"
#include "../registers.h"

/* MDIO timing: MDC max 2.5MHz → 400ns per bit at 120MHz APB1 = 48 ticks */
/* We use bit-banging for simplicity since we need precise timing control */

#define MDIO_READ_CMD   0x06  /* Preamble(32) + ST(01) + OP(10) + PHY(5) + REG(5) */
#define MDIO_WRITE_CMD  0x05  /* Preamble(32) + ST(01) + OP(01) + PHY(5) + REG(5) */

/* MDIO PHY address for KSZ9897 — port registers mapped as PHY addresses */
#define MDIO_ADDR_GLOBAL 0x00
#define MDIO_ADDR_PORT1   0x01
#define MDIO_ADDR_PORT2   0x02
#define MDIO_ADDR_PORT6   0x06

/* ===== MDIO Bit-Bang Interface ===== */

static void mdio_delay(void) {
    for (volatile int i = 0; i < 20; i++); /* ~160ns at 480MHz */
}

/* Write one bit on MDIO */
static void mdio_write_bit(uint8_t bit) {
    if (bit)
        GPIO_SET(GPIOA_BASE, ETH_MDIO_PIN);   /* MDIO high */
    else
        GPIO_CLR(GPIOA_BASE, ETH_MDIO_PIN);   /* MDIO low */
    mdio_delay();
    GPIO_SET(GPIOA_BASE, ETH_MDC_PIN);        /* MCK rising edge */
    mdio_delay();
    GPIO_CLR(GPIOA_BASE, ETH_MDC_PIN);        /* MCK falling edge */
}

/* Read one bit from MDIO */
static uint8_t mdio_read_bit(void) {
    uint8_t val = GPIO_READ(GPIOA_BASE, ETH_MDIO_PIN);
    mdio_delay();
    GPIO_SET(GPIOA_BASE, ETH_MDC_PIN);
    mdio_delay();
    GPIO_CLR(GPIOA_BASE, ETH_MDC_PIN);
    return val;
}

/* Send 32-bit preamble */
static void mdio_preamble(void) {
    for (int i = 0; i < 32; i++) {
        mdio_write_bit(1);
    }
}

/* Send MDIO read transaction */
static uint16_t mdio_read(uint8_t phy_addr, uint8_t reg_addr) {
    uint16_t data = 0;

    /* Switch MDIO to output */
    GPIO_PIN_MODE(GPIOA_BASE, ETH_MDIO_PIN, GPIO_MODE_OUTPUT);
    GPIO_PIN_MODE(GPIOA_BASE, ETH_MDC_PIN, GPIO_MODE_OUTPUT);

    mdio_preamble();

    /* ST: 01 */
    mdio_write_bit(0);
    mdio_write_bit(1);

    /* OP: 10 (read) */
    mdio_write_bit(1);
    mdio_write_bit(0);

    /* PHY address (5 bits) */
    for (int i = 4; i >= 0; i--) {
        mdio_write_bit((phy_addr >> i) & 1);
    }

    /* Register address (5 bits) */
    for (int i = 4; i >= 0; i--) {
        mdio_write_bit((reg_addr >> i) & 1);
    }

    /* Switch MDIO to input (high-Z) for turnaround + data */
    GPIO_PIN_MODE(GPIOA_BASE, ETH_MDIO_PIN, GPIO_MODE_INPUT);
    GPIO_PIN_PULL(GPIOA_BASE, ETH_MDIO_PIN, GPIO_PULL_UP);

    /* Turnaround: 2 bits (Z0) */
    mdio_read_bit(); /* Z - should be high */
    mdio_read_bit(); /* 0 - should be low */

    /* Read 16-bit data (MSB first) */
    for (int i = 15; i >= 0; i--) {
        data |= (mdio_read_bit() << i);
    }

    /* Switch MDIO back to output */
    GPIO_PIN_MODE(GPIOA_BASE, ETH_MDIO_PIN, GPIO_MODE_AF);

    return data;
}

/* Send MDIO write transaction */
static void mdio_write(uint8_t phy_addr, uint8_t reg_addr, uint16_t data) {
    /* Switch MDIO to output */
    GPIO_PIN_MODE(GPIOA_BASE, ETH_MDIO_PIN, GPIO_MODE_OUTPUT);
    GPIO_PIN_MODE(GPIOA_BASE, ETH_MDC_PIN, GPIO_MODE_OUTPUT);

    mdio_preamble();

    /* ST: 01 */
    mdio_write_bit(0);
    mdio_write_bit(1);

    /* OP: 01 (write) */
    mdio_write_bit(0);
    mdio_write_bit(1);

    /* PHY address (5 bits) */
    for (int i = 4; i >= 0; i--) {
        mdio_write_bit((phy_addr >> i) & 1);
    }

    /* Register address (5 bits) */
    for (int i = 4; i >= 0; i--) {
        mdio_write_bit((reg_addr >> i) & 1);
    }

    /* Turnaround: 2 bits (10) */
    mdio_write_bit(1);
    mdio_write_bit(0);

    /* Write 16-bit data (MSB first) */
    for (int i = 15; i >= 0; i--) {
        mdio_write_bit((data >> i) & 1);
    }

    /* Switch MDIO back to AF */
    GPIO_PIN_MODE(GPIOA_BASE, ETH_MDIO_PIN, GPIO_MODE_AF);
}

/* ===== Driver Implementation ===== */

int ksz9897_init(void) {
    /* Read chip ID to verify communication */
    uint16_t id0 = mdio_read(MDIO_ADDR_GLOBAL, 0x00);
    uint16_t id1 = mdio_read(MDIO_ADDR_GLOBAL, 0x01);

    /* KSZ9897 chip ID: 0x0022 for family, specific in ID1 */
    if ((id0 & 0xFFF0) != 0x0020) {
        return -1; /* Chip ID mismatch */
    }

    /* Disable all ports except P1, P2, and P6 (CPU) */
    for (int port = 3; port <= 5; port++) {
        ksz9897_enable_port(port, 0);
    }

    /* Enable Port 1 (IN) - auto-negotiate, gigabit */
    ksz9897_write_reg(KSZ_P1_CTRL, PORT_CTRL_ENABLE | PORT_CTRL_SPEED_1000 | PORT_CTRL_DUPLEX);

    /* Enable Port 2 (OUT) - auto-negotiate, gigabit */
    ksz9897_write_reg(KSZ_P2_CTRL, PORT_CTRL_ENABLE | PORT_CTRL_SPEED_1000 | PORT_CTRL_DUPLEX);

    /* Enable Port 6 (RGMII CPU) - forced 1Gbps full duplex */
    ksz9897_write_reg(KSZ_P6_CTRL, PORT_CTRL_ENABLE | PORT_CTRL_SPEED_1000 | PORT_CTRL_DUPLEX | PORT_CTRL_FORCE);

    /* Set default forwarding: P1↔P2 and P1,P2→P6 (mirror) */
    ksz9897_set_forwarding(1, 0x22); /* P1 → P2 + P6 */
    ksz9897_set_forwarding(2, 0x11); /* P2 → P1 + P6 */
    ksz9897_set_forwarding(6, 0x03); /* P6 (CPU) → P1 + P2 */

    return 0;
}

int16_t ksz9897_read_reg(uint16_t reg) {
    uint8_t phy = (reg >> 5) & 0x1F;
    uint8_t addr = reg & 0x1F;
    return (int16_t)mdio_read(phy, addr);
}

int ksz9897_write_reg(uint16_t reg, uint16_t val) {
    uint8_t phy = (reg >> 5) & 0x1F;
    uint8_t addr = reg & 0x1F;
    mdio_write(phy, addr, val);
    return 0;
}

int ksz9897_set_mirror(uint16_t flags) {
    return ksz9897_write_reg(KSZ_MIRROR_CTRL, flags);
}

int ksz9897_set_vlan(const ksz_vlan_entry_t *entry) {
    /* Write VLAN table entry */
    uint16_t vlan_lo = (entry->valid & 1) | ((entry->fid & 0xF) << 1)
                     | ((entry->member & 0x7F) << 4) | ((entry->untag & 0x7F) << 11);
    uint16_t vlan_hi = entry->vid & 0xFFF;

    ksz9897_write_reg(KSZ_VLAN_TABLE, vlan_lo);
    ksz9897_write_reg(KSZ_VLAN_TABLE + 1, vlan_hi);

    /* Trigger VLAN table write */
    ksz9897_write_reg(KSZ_VLAN_CTRL, (1 << 0) | (1 << 7)); /* Write + Valid */

    return 0;
}

int ksz9897_get_mib(uint8_t port, ksz_mib_stats_t *stats) {
    uint16_t base;
    switch (port) {
        case 1: base = KSZ_P1_MIB_BASE; break;
        case 2: base = KSZ_P2_MIB_BASE; break;
        case 6: base = KSZ_P6_MIB_BASE; break;
        default: return -1;
    }

    /* Read MIB counters (each is 32-bit, read as two 16-bit MDIO reads) */
    stats->rx_lo_frames = ((uint32_t)ksz9897_read_reg(base + MIB_RX_LO) << 16)
                        | ksz9897_read_reg(base + MIB_RX_LO + 1);
    stats->rx_hi_frames = ((uint32_t)ksz9897_read_reg(base + MIB_RX_HI) << 16)
                        | ksz9897_read_reg(base + MIB_RX_HI + 1);
    stats->rx_crc_err   = ((uint32_t)ksz9897_read_reg(base + MIB_RX_CRC_ERR) << 16)
                        | ksz9897_read_reg(base + MIB_RX_CRC_ERR + 1);
    stats->tx_lo_frames = ((uint32_t)ksz9897_read_reg(base + MIB_TX_LO) << 16)
                        | ksz9897_read_reg(base + MIB_TX_LO + 1);
    stats->tx_drop      = ((uint32_t)ksz9897_read_reg(base + MIB_TX_DROP) << 16)
                        | ksz9897_read_reg(base + MIB_TX_DROP + 1);

    return 0;
}

int ksz9897_set_port_speed(uint8_t port, uint8_t speed, uint8_t duplex) {
    uint16_t reg;
    switch (port) {
        case 1: reg = KSZ_P1_CTRL; break;
        case 2: reg = KSZ_P2_CTRL; break;
        case 6: reg = KSZ_P6_CTRL; break;
        default: return -1;
    }

    uint16_t val = PORT_CTRL_ENABLE | PORT_CTRL_FORCE;
    val |= (speed & 3) << 0;
    if (duplex) val |= PORT_CTRL_DUPLEX;

    return ksz9897_write_reg(reg, val);
}

int ksz9897_enable_port(uint8_t port, uint8_t enable) {
    uint16_t reg;
    switch (port) {
        case 1: reg = KSZ_P1_CTRL; break;
        case 2: reg = KSZ_P2_CTRL; break;
        case 3: reg = 0x0300; break;
        case 4: reg = 0x0400; break;
        case 5: reg = 0x0500; break;
        case 6: reg = KSZ_P6_CTRL; break;
        default: return -1;
    }

    uint16_t val = ksz9897_read_reg(reg);
    if (enable)
        val |= PORT_CTRL_ENABLE;
    else
        val &= ~PORT_CTRL_ENABLE;

    return ksz9897_write_reg(reg, val);
}

int ksz9897_set_forwarding(uint8_t port, uint8_t mask) {
    /* Port forwarding mask register: offset varies by port */
    uint16_t reg = 0x0100 + (port * 0x20) + 0x0C; /* Approximate */
    return ksz9897_write_reg(reg, mask);
}