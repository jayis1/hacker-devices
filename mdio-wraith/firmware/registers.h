/*
 * registers.h - MDIO Wraith register map and constants
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef MDIO_WRAITH_REGISTERS_H
#define MDIO_WRAITH_REGISTERS_H

#define MW_REG_BMCR              0x00u
#define MW_REG_BMSR              0x01u
#define MW_REG_PHYIDR1           0x02u
#define MW_REG_PHYIDR2           0x03u
#define MW_REG_ANAR              0x04u
#define MW_REG_ANLPAR            0x05u
#define MW_REG_ANER              0x06u
#define MW_REG_ANNPTR            0x07u
#define MW_REG_GBCR              0x09u
#define MW_REG_GBSR              0x0Au
#define MW_REG_MMD_CTRL          0x0Du
#define MW_REG_MMD_DATA          0x0Eu
#define MW_REG_EXT_STATUS        0x0Fu
#define MW_REG_VENDOR_PAGE       0x1Fu

#define MW_BMCR_RESET            0x8000u
#define MW_BMCR_LOOPBACK         0x4000u
#define MW_BMCR_SPEED_SEL_LSB    0x2000u
#define MW_BMCR_AUTONEG_EN       0x1000u
#define MW_BMCR_POWER_DOWN       0x0800u
#define MW_BMCR_ISOLATE          0x0400u
#define MW_BMCR_RESTART_AUTONEG  0x0200u
#define MW_BMCR_DUPLEX           0x0100u
#define MW_BMCR_COLLISION_TEST   0x0080u
#define MW_BMCR_SPEED_SEL_MSB    0x0040u

#define MW_BMSR_100BASE_T4       0x8000u
#define MW_BMSR_100BASE_X_FD     0x4000u
#define MW_BMSR_100BASE_X_HD     0x2000u
#define MW_BMSR_10_FD            0x1000u
#define MW_BMSR_10_HD            0x0800u
#define MW_BMSR_100BASE_T2_FD    0x0400u
#define MW_BMSR_100BASE_T2_HD    0x0200u
#define MW_BMSR_EXT_STATUS       0x0100u
#define MW_BMSR_MF_PREAMBLE      0x0040u
#define MW_BMSR_AUTONEG_COMPLETE 0x0020u
#define MW_BMSR_REMOTE_FAULT     0x0010u
#define MW_BMSR_AUTONEG_ABLE     0x0008u
#define MW_BMSR_LINK_STATUS      0x0004u
#define MW_BMSR_JABBER_DETECT    0x0002u
#define MW_BMSR_EXT_CAPABILITY   0x0001u

#define MW_DEVAD_PMA             1u
#define MW_DEVAD_WIS             2u
#define MW_DEVAD_PCS             3u
#define MW_DEVAD_PHY_XS          4u
#define MW_DEVAD_DTE_XS          5u
#define MW_DEVAD_TC              6u
#define MW_DEVAD_AUTONEG         7u
#define MW_DEVAD_VENDOR1         30u
#define MW_DEVAD_VENDOR2         31u

#define MW_TRIGGER_PHY_ANY       0xFFFFu
#define MW_TRIGGER_REG_ANY       0xFFFFu

#endif
