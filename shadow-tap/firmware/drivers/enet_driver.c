/*
 * enet_driver.c — ShadowTap dual-ENET driver implementation
 */

#include "drivers/enet_driver.h"
#include <string.h>

/* MDIO access via ENET MMFR (MII Management Frame Register) */
/* For i.MX RT1062, the MMFR register is at ENET base + 0x040 */
/* We use a simplified bit-bang via MDC/MDIO GPIO for reliability */

#define ENET_MMFR_OFFSET   0x040U
#define ENET_MMFR_ST_MASK  (1U << 30)
#define ENET_MMFR_OP_READ  (2U << 28)
#define ENET_MMFR_OP_WRITE (1U << 28)
#define ENET_MMFR_PA_SHIFT  23
#define ENET_MMFR_RA_SHIFT  18
#define ENET_MMFR_TA_MASK   (2U << 16)
#define ENET_MMFR_DATA_MASK 0xFFFFU

static uint16_t mdio_read_reg(ENET_Type *enet, uint8_t phy, uint8_t reg) {
    volatile uint32_t *mmfr = (volatile uint32_t *)((uint32_t)enet + ENET_MMFR_OFFSET);
    uint32_t frame = ENET_MMFR_ST_MASK | ENET_MMFR_OP_READ |
                     ((phy & 0x1FU) << ENET_MMFR_PA_SHIFT) |
                     ((reg & 0x1FU) << ENET_MMFR_RA_SHIFT) |
                     ENET_MMFR_TA_MASK;
    *mmfr = frame;

    /* Wait for MMFR operation to complete (EIR.MII bit) */
    uint32_t timeout = 100000U;
    while (!(enet->EIR & (1U << 23))) {
        if (--timeout == 0) return 0xFFFFU;
    }
    enet->EIR = (1U << 23); /* Clear MII interrupt */

    return (uint16_t)(*mmfr & ENET_MMFR_DATA_MASK);
}

static void mdio_write_reg(ENET_Type *enet, uint8_t phy, uint8_t reg, uint16_t val) {
    volatile uint32_t *mmfr = (volatile uint32_t *)((uint32_t)enet + ENET_MMFR_OFFSET);
    uint32_t frame = ENET_MMFR_ST_MASK | ENET_MMFR_OP_WRITE |
                     ((phy & 0x1FU) << ENET_MMFR_PA_SHIFT) |
                     ((reg & 0x1FU) << ENET_MMFR_RA_SHIFT) |
                     ENET_MMFR_TA_MASK |
                     (val & ENET_MMFR_DATA_MASK);
    *mmfr = frame;

    uint32_t timeout = 100000U;
    while (!(enet->EIR & (1U << 23))) {
        if (--timeout == 0) return;
    }
    enet->EIR = (1U << 23);
}

void enet_driver_init(enet_driver_t *drv, uint8_t port_id, ENET_Type *base,
                      enet_bd_t *rx_bd, enet_bd_t *tx_bd,
                      uint8_t (*rx_buf)[ENET_RX_BUFFER_SIZE],
                      uint8_t (*tx_buf)[ENET_RX_BUFFER_SIZE]) {
    uint32_t i;

    drv->base      = base;
    drv->port_id   = port_id;
    drv->rx_bd     = rx_bd;
    drv->tx_bd     = tx_bd;
    drv->rx_buf    = rx_buf;
    drv->tx_buf    = tx_buf;
    drv->rx_idx    = 0;
    drv->tx_idx    = 0;
    drv->tx_done_idx = 0;
    drv->rx_count  = 0;
    drv->tx_count  = 0;
    drv->drop_count = 0;
    drv->error_count = 0;

    /* Reset the ENET module */
    base->ECR = ENET_ECR_SPEED_MASK;
    for (volatile int d = 0; d < 100000; d++);

    /* Clear interrupt flags */
    base->EIR = 0xFFFFFFFFU;
    base->EIMR = 0;

    /* MSCR: MDC clock = IPG_CLK / 24 = 150/24 ≈ 6.25 MHz (within 2.5 MHz MDIO spec we use /60) */
    base->MSCR = (60U << ENET_MSCR_MII_SPEED_SHIFT);

    /* RCR: Promiscuous mode, RGMII, max frame 1518 */
    base->RCR = ENET_RCR_PROM_MASK |
                ENET_RCR_MII_MODE_MASK |
                (1518U << ENET_RCR_MAX_FL_SHIFT);

    /* TCR: Full duplex, MAC address insert disabled, CRC appended */
    base->TCR = ENET_TCR_FDEN_MASK;

    /* Set zero MAC (transparent mode — we never source frames with our MAC) */
    base->PALR = 0x00000000U;
    base->PAUR = 0x88080000U;

    /* TX FIFO: Store-and-forward for reliability; set to 4 for cut-through */
    base->TFWR = 4U; /* Cut-through after 64 bytes */

    /* Clear hash registers (accept all in promisc mode) */
    base->IALR = 0;
    base->IAUR = 0;
    base->GALR = 0;
    base->GAUR = 0;

    /* Initialize RX BDs */
    for (i = 0; i < ENET_RX_RING_SIZE; i++) {
        rx_bd[i].status = ENET_RX_BD_E_MASK;  /* Empty, owned by DMA */
        rx_bd[i].length = 0;
        rx_bd[i].data   = rx_buf[i];
        if (i == ENET_RX_RING_SIZE - 1) {
            rx_bd[i].status |= ENET_RX_BD_W_MASK;
        }
    }

    /* Initialize TX BDs */
    for (i = 0; i < ENET_TX_RING_SIZE; i++) {
        tx_bd[i].status = (i == ENET_TX_RING_SIZE - 1) ? ENET_TX_BD_W_MASK : 0;
        tx_bd[i].length = 0;
        tx_bd[i].data   = tx_buf[i];
    }

    /* Set ring addresses and max buffer size */
    base->RDSR = (uint32_t)rx_bd;
    base->TDSR = (uint32_t)tx_bd;
    base->MRBR = ENET_RX_BUFFER_SIZE;

    /* Enable RX/TX interrupts */
    base->EIMR = ENET_EIR_RXF_MASK | ENET_EIR_TXF_MASK | ENET_EIR_EBERR_MASK;

    /* Enable ENET with RGMII speed bit */
    base->ECR = ENET_ECR_ETHEREN_MASK | ENET_ECR_SPEED_MASK;

    /* Activate RX DMA */
    base->RDAR = 0x01000000U;

    /* Initialize PHY */
    uint8_t phy_addr = (port_id == ENET_PORT_UPLINK) ? PHY1_ADDR : PHY2_ADDR;

    /* Reset PHY */
    mdio_write_reg(base, phy_addr, PHY_REG_BMCR, 0x8000);
    while (mdio_read_reg(base, phy_addr, PHY_REG_BMCR) & 0x8000);

    /* Configure RGMII mode */
    mdio_write_reg(base, phy_addr, 0x16, 0x0002); /* Page 2 */
    mdio_write_reg(base, phy_addr, 0x14, 0x0F72);  /* RGMII w/ delay */
    mdio_write_reg(base, phy_addr, 0x16, 0x0000); /* Back to page 0 */

    /* Enable auto-negotiation */
    mdio_write_reg(base, phy_addr, PHY_REG_1000BT, 0x0200);
    mdio_write_reg(base, phy_addr, PHY_REG_ANAR, 0x01E1);
    mdio_write_reg(base, phy_addr, PHY_REG_BMCR, 0x1200);
}

uint32_t enet_driver_rx_poll(enet_driver_t *drv, enet_driver_t *dest) {
    uint32_t count = 0;

    while (!(drv->rx_bd[drv->rx_idx].status & ENET_RX_BD_E_MASK)) {
        uint16_t len = drv->rx_bd[drv->rx_idx].length;
        uint8_t *data = drv->rx_bd[drv->rx_idx].data;

        /* Check for errors */
        if (drv->rx_bd[drv->rx_idx].status & (ENET_RX_BD_CR_MASK |
                                               ENET_RX_BD_OV_MASK |
                                               ENET_RX_BD_TR_MASK)) {
            drv->error_count++;
        } else if (len > 0 && len <= ENET_RX_BUFFER_SIZE) {
            /* Forward frame to destination port */
            if (enet_driver_tx(dest, data, len) != 0) {
                drv->drop_count++;
            }
            drv->rx_count++;
            count++;
        }

        /* Return BD to DMA */
        drv->rx_bd[drv->rx_idx].status &= ~ENET_RX_BD_L_MASK;
        drv->rx_bd[drv->rx_idx].status |= ENET_RX_BD_E_MASK;
        drv->rx_idx = (drv->rx_idx + 1) % ENET_RX_RING_SIZE;
    }

    /* Re-activate RX DMA */
    drv->base->RDAR = 0x01000000U;

    return count;
}

int8_t enet_driver_tx(enet_driver_t *drv, const uint8_t *frame, uint16_t len) {
    /* Find available TX BD */
    if (drv->tx_bd[drv->tx_idx].status & ENET_TX_BD_R_MASK) {
        /* No available BD — drop frame */
        return -1;
    }

    /* Copy frame to TX buffer (zero-copy optimization: pointer swap) */
    uint8_t *dst = drv->tx_bd[drv->tx_idx].data;
    for (uint16_t i = 0; i < len && i < ENET_RX_BUFFER_SIZE; i++) {
        dst[i] = frame[i];
    }

    /* Set up BD for transmission */
    drv->tx_bd[drv->tx_idx].length = len;
    uint16_t bd_status = drv->tx_bd[drv->tx_idx].status;
    bd_status |= ENET_TX_BD_R_MASK | ENET_TX_BD_L_MASK | ENET_TX_BD_TC_MASK;
    drv->tx_bd[drv->tx_idx].status = bd_status;

    /* Activate TX DMA */
    drv->base->TDAR = 0x01000000U;

    /* Advance TX index */
    drv->tx_idx = (drv->tx_idx + 1) % ENET_TX_RING_SIZE;
    drv->tx_count++;

    return 0;
}

uint8_t enet_driver_get_link(enet_driver_t *drv) {
    uint8_t phy_addr = (drv->port_id == ENET_PORT_UPLINK) ? PHY1_ADDR : PHY2_ADDR;
    uint16_t stat = mdio_read_reg(drv->base, phy_addr, PHY_REG_SPEC_STAT);
    return (stat & PHY_SPEC_STAT_LINK) ? 1 : 0;
}

void enet_driver_get_stats(enet_driver_t *drv,
                           uint32_t *rx_count, uint32_t *tx_count,
                           uint32_t *drops, uint32_t *errors) {
    if (rx_count) *rx_count = drv->rx_count;
    if (tx_count) *tx_count = drv->tx_count;
    if (drops)    *drops    = drv->drop_count;
    if (errors)   *errors   = drv->error_count;
}