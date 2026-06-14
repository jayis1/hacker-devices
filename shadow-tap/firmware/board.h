/*
 * board.h — ShadowTap hardware definitions
 * Target: i.MX RT1062 Cortex-M7 @ 600 MHz
 */

#ifndef SHADOWTAP_BOARD_H
#define SHADOWTAP_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* Clock frequencies */
#define BOARD_XTAL_FREQ_HZ      24000000U
#define BOARD_ARM_PLL_HZ        600000000U
#define BOARD_IPG_CLK_HZ        150000000U
#define BOARD_ENET_CLK_HZ       125000000U
#define BOARD_ENET_MDC_HZ       50000000U
#define BOARD_UART_BAUDRATE     115200U

/* ENET1 (Uplink) base address */
#define ENET1_BASE              0x402D8000U
/* ENET2 (Target) base address */
#define ENET2_BASE              0x402D8400U

/* PHY addresses (MDIO) */
#define PHY1_ADDR               0x00U
#define PHY2_ADDR               0x01U

/* PHY register offsets (88E1510 Clause 22) */
#define PHY_REG_BMCR            0x00U
#define PHY_REG_BMSR            0x01U
#define PHY_REG_PHYID1          0x02U
#define PHY_REG_PHYID2          0x03U
#define PHY_REG_ANAR            0x04U
#define PHY_REG_ANLPAR           0x05U
#define PHY_REG_1000BT          0x09U
#define PHY_REG_SPEC_CTRL       0x10U
#define PHY_REG_SPEC_STAT       0x11U

/* PHY BMCR bits */
#define PHY_BMCR_RESET          (1U << 15)
#define PHY_BMCR_LOOPBACK       (1U << 14)
#define PHY_BMCR_AN_ENABLE      (1U << 12)
#define PHY_BMCR_AN_RESTART     (1U << 9)
#define PHY_BMCR_SPEED_1000     (1U << 6)

/* PHY SPEC_STAT bits */
#define PHY_SPEC_STAT_LINK      (1U << 11)
#define PHY_SPEC_STAT_SPEED_MASK 0x0C00U
#define PHY_SPEC_STAT_DUPLEX    (1U << 13)

/* LPUART1 base address */
#define LPUART1_BASE            0x40185000U

/* FlexSPI base address (boot flash) */
#define FLEXSPI_BASE            0x402A8000U

/* uSDHC1 base address */
#define USDHC1_BASE             0x402C0000U

/* LPSPI1 base address */
#define LPSPI1_BASE             0x40394000U

/* LPI2C1 base address */
#define LPI2C1_BASE             0x401A6000U
/* LPI2C2 base address */
#define LPI2C2_BASE             0x401A8000U

/* CCM base address */
#define CCM_BASE                0x400FC000U
/* CCM_ANALOG base address */
#define CCM_ANALOG_BASE         0x400D8000U

/* IOMUXC base address */
#define IOMUXC_BASE             0x401F8000U

/* GPIO1 base address */
#define GPIO1_BASE              0x401B8000U

/* ENET DMA ring sizes */
#define ENET_RX_RING_SIZE       128U
#define ENET_TX_RING_SIZE        64U
#define ENET_RX_BUFFER_SIZE     2048U

/* PCAP constants */
#define PCAP_MAGIC              0xA1B2C3D4U
#define PCAP_SNAPLEN            65535U
#define PCAP_LINKTYPE_ETHERNET  1U

/* BLE C2 protocol */
#define BLE_C2_MAX_CMD_LEN     512U
#define BLE_C2_SLIP_END        0xC0U
#define BLE_C2_SLIP_ESC        0xDBU
#define BLE_C2_SLIP_ESC_END   0xDCU
#define BLE_C2_SLIP_ESC_ESC   0xDDU

/* MITM rule types */
#define MITM_RULE_ARP_POISON   0x01U
#define MITM_RULE_DNS_SPOOF   0x02U
#define MITM_RULE_DHCP_ROGUE  0x03U
#define MITM_RULE_FRAME_INJECT 0x04U
#define MITM_RULE_DROP         0x05U
#define MITM_RULE_MODIFY       0x06U
#define MITM_RULE_MAX          32U

/* LED colors (WS2812B) */
#define LED_COLOR_OFF          0x000000U
#define LED_COLOR_GREEN        0x00FF00U
#define LED_COLOR_RED          0xFF0000U
#define LED_COLOR_BLUE         0x0000FFU
#define LED_COLOR_YELLOW       0xFFFF00U
#define LED_COLOR_CYAN         0x00FFFFU
#define LED_COLOR_MAGENTA      0xFF00FFU
#define LED_COUNT              4U

/* Pin definitions */
#define PIN_LED_DIN            4U   /* GPIO1[4] */
#define PIN_BTN_IN             5U   /* GPIO1[5] */
#define PIN_SD_CD              6U   /* uSDHC1_CD */

#endif /* SHADOWTAP_BOARD_H */