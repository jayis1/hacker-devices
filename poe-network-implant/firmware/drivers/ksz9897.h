/*
 * ksz9897.h — KSZ9897R Gigabit Managed Switch Driver
 * PhantomBridge PoE Network Implant
 *
 * Provides initialization, port mirroring, VLAN config,
 * and MIB counter access via MDIO/SMI interface.
 */

#ifndef KSZ9897_H
#define KSZ9897_H

#include <stdint.h>

/* KSZ9897 Register Addresses (indirect via MDIO) */
#define KSZ_CHIP_ID0         0x0000
#define KSZ_CHIP_ID1         0x0001
#define KSZ_CHIP_ID2         0x0002
#define KSZ_CHIP_ID3         0x0003
#define KSZ_P1_CTRL          0x0100
#define KSZ_P1_STAT          0x0102
#define KSZ_P1_LINK_MD       0x0104
#define KSZ_P2_CTRL          0x0200
#define KSZ_P2_STAT          0x0202
#define KSZ_P2_LINK_MD       0x0204
#define KSZ_P6_CTRL          0x0600  /* RGMII CPU port */
#define KSZ_GLOBAL_CTRL      0x0300
#define KSZ_GLOBAL_STAT      0x0302
#define KSZ_MIRROR_CTRL      0x0380
#define KSZ_IGMP_CTRL        0x0390
#define KSZ_VLAN_CTRL        0x0400
#define KSZ_VLAN_TABLE       0x0404
#define KSZ_ACL_CTRL         0x0500
#define KSZ_P1_MIB_BASE      0x1100
#define KSZ_P2_MIB_BASE      0x1200
#define KSZ_P6_MIB_BASE      0x1600

/* MIB Counter Offsets (relative to port base) */
#define MIB_RX_LO            0x00  /* RX Lo priority frames */
#define MIB_RX_HI            0x04  /* RX Hi priority frames */
#define MIB_RX_UNDERSIZE     0x20  /* RX undersize frames */
#define MIB_RX_FRAGMENT      0x24  /* RX fragments */
#define MIB_RX_OVERSIZE      0x28  /* RX oversize frames */
#define MIB_RX_JABBER        0x2C  /* RX jabber frames */
#define MIB_RX_CRC_ERR       0x30  /* RX CRC errors */
#define MIB_TX_LO            0x80  /* TX Lo priority frames */
#define MIB_TX_HI            0x84  /* TX Hi priority frames */
#define MIB_TX_COLLISION     0xA0  /* TX collisions */
#define MIB_TX_DROP          0xA4  /* TX dropped frames */

/* Port Control bits */
#define PORT_CTRL_SPEED_10   (0 << 0)
#define PORT_CTRL_SPEED_100  (1 << 0)
#define PORT_CTRL_SPEED_1000 (2 << 0)
#define PORT_CTRL_DUPLEX     (1 << 2)
#define PORT_CTRL_ENABLE     (1 << 3)
#define PORT_CTRL_FLOW_CTRL  (1 << 4)
#define PORT_CTRL_FORCE      (1 << 7)

/* Mirror Control bits */
#define MIRROR_EN             (1 << 0)
#define MIRROR_P1_RX          (1 << 1)
#define MIRROR_P1_TX          (1 << 2)
#define MIRROR_P2_RX          (1 << 3)
#define MIRROR_P2_TX          (1 << 4)
#define MIRROR_P3_RX          (1 << 5)
#define MIRROR_P3_TX          (1 << 6)
#define MIRROR_P4_RX          (1 << 7)
#define MIRROR_P4_TX          (1 << 8)
#define MIRROR_TO_P5          (0 << 9)
#define MIRROR_TO_P6          (1 << 9)  /* CPU RGMII port */
#define MIRROR_SNIFFER        (1 << 10)

/* VLAN Table Entry */
typedef struct __attribute__((packed)) {
    uint16_t vid;
    uint8_t  valid    : 1;
    uint8_t  priority : 3;
    uint8_t  fid      : 4;
    uint8_t  member   : 7;
    uint8_t  untag    : 7;
    uint8_t  reserved : 1;
} ksz_vlan_entry_t;

/* Port MIB Statistics */
typedef struct {
    uint32_t rx_lo_frames;
    uint32_t rx_hi_frames;
    uint32_t rx_undersize;
    uint32_t rx_fragment;
    uint32_t rx_oversize;
    uint32_t rx_jabber;
    uint32_t rx_crc_err;
    uint32_t tx_lo_frames;
    uint32_t tx_hi_frames;
    uint32_t tx_collision;
    uint32_t tx_drop;
} ksz_mib_stats_t;

/* ===== Function Prototypes ===== */

/**
 * ksz9897_init — Initialize the KSZ9897R switch
 * Configures ports, disables unused ports, enables RGMII CPU port
 * Returns 0 on success, -1 on chip ID mismatch
 */
int ksz9897_init(void);

/**
 * ksz9897_read_reg — Read a 16-bit switch register via MDIO
 * @reg: Register address
 * Returns: Register value or -1 on error
 */
int16_t ksz9897_read_reg(uint16_t reg);

/**
 * ksz9897_write_reg — Write a 16-bit switch register via MDIO
 * @reg: Register address
 * @val: Value to write
 * Returns: 0 on success, -1 on error
 */
int ksz9897_write_reg(uint16_t reg, uint16_t val);

/**
 * ksz9897_set_mirror — Configure port mirroring
 * @flags: Combination of MIRROR_* flags
 * Returns: 0 on success
 */
int ksz9897_set_mirror(uint16_t flags);

/**
 * ksz9897_set_vlan — Configure VLAN table entry
 * @entry: VLAN entry configuration
 * Returns: 0 on success
 */
int ksz9897_set_vlan(const ksz_vlan_entry_t *entry);

/**
 * ksz9897_get_mib — Read MIB statistics for a port
 * @port: Port number (1-2 or 6 for CPU)
 * @stats: Pointer to stats structure to fill
 * Returns: 0 on success
 */
int ksz9897_get_mib(uint8_t port, ksz_mib_stats_t *stats);

/**
 * ksz9897_set_port_speed — Force port speed and duplex
 * @port: Port number
 * @speed: 0=10M, 1=100M, 2=1G
 * @duplex: 0=half, 1=full
 * Returns: 0 on success
 */
int ksz9897_set_port_speed(uint8_t port, uint8_t speed, uint8_t duplex);

/**
 * ksz9897_enable_port — Enable/disable a switch port
 * @port: Port number
 * @enable: 1=enable, 0=disable
 * Returns: 0 on success
 */
int ksz9897_enable_port(uint8_t port, uint8_t enable);

/**
 * ksz9897_set_forwarding — Configure port forwarding mask
 * @port: Source port
 * @mask: Bitmask of destination ports (bit 0=P1, bit 1=P2, bit 5=P6)
 * Returns: 0 on success
 */
int ksz9897_set_forwarding(uint8_t port, uint8_t mask);

#endif /* KSZ9897_H */