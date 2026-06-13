/*
 * WFP-100 — AX210 PCIe WiFi Driver Header
 * Intel AX210NGW monitor mode + injection driver for WFP-100.
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef WFP100_AX210_PCIE_H
#define WFP100_AX210_PCIE_H

#include <linux/module.h>
#include <linux/netdevice.h>

/* PCIe IDs */
#define AX210_PCIE_VENDOR_ID     0x8086
#define AX210_PCIE_DEVICE_ID     0x2725
#define AX210_PCI_BAR_NUM        0

/* DMA Ring Configuration */
#define AX210_RX_RING_SIZE       256
#define AX210_TX_RING_SIZE       64
#define AX210_FRAME_BUF_SIZE     4096

/* AX210 MMIO Register Map */
#define AX210_REG_HW_REV            0x0000
#define AX210_REG_FH_CSR            0x0100
#define AX210_REG_FH_RX_BASE        0x0104
#define AX210_REG_FH_RX_SIZE        0x0108
#define AX210_REG_FH_RX_WR_PTR      0x010C
#define AX210_REG_FH_RX_RD_PTR      0x0110
#define AX210_REG_FH_TX_BASE        0x0140
#define AX210_REG_FH_TX_SIZE        0x0144
#define AX210_REG_FH_TX_WR_PTR      0x0148
#define AX210_REG_FH_TX_RD_PTR      0x014C
#define AX210_REG_MAC_CFG           0x0200
#define AX210_REG_MAC_ADDR0         0x0204
#define AX210_REG_MAC_ADDR1         0x0208
#define AX210_REG_CHANNEL           0x0300
#define AX210_REG_BAND              0x0304
#define AX210_REG_MONITOR_CTRL       0x0310
#define AX210_REG_MONITOR_FILTER     0x0314
#define AX210_REG_MONITOR_FLAGS      0x0318
#define AX210_REG_INTA               0x0400
#define AX210_REG_INT_MASK           0x0408
#define AX210_REG_INT_CLEAR          0x040C

/* Interrupt bits */
#define AX210_INTA_RX_COMPLETE       (1U << 0)
#define AX210_INTA_TX_COMPLETE       (1U << 1)
#define AX210_INTA_CHANNEL_SWITCH    (1U << 3)
#define AX210_INTA_FW_READY          (1U << 4)
#define AX210_INTA_PMKID_FOUND       (1U << 7)
#define AX210_INTA_HANDSHAKE         (1U << 8)

/* RX/TX frame descriptors */
struct ax210_rx_desc {
    uint32_t flags;
    uint32_t len;
    uint32_t channel;
    uint32_t freq;
    int8_t    rssi;
    int8_t    noise;
    uint16_t rate;
    uint64_t timestamp;
    uint32_t reserved[3];
    uint8_t   data[];
} __packed;

struct ax210_tx_desc {
    uint32_t flags;
    uint32_t len;
    uint32_t channel;
    uint32_t rate;
    uint32_t retry_limit;
    uint32_t reserved[3];
    uint8_t   data[];
} __packed;

/* Driver private data */
struct ax210_priv {
    struct pci_dev         *pdev;
    struct net_device      *ndev;
    void __iomem           *mmio;
    struct work_struct      irq_work;
    dma_addr_t              rx_dma;
    struct ax210_rx_desc   *rx_ring;
    uint32_t                rx_wr_ptr;
    uint32_t                rx_rd_ptr;
    dma_addr_t              tx_dma;
    struct ax210_tx_desc   *tx_ring;
    uint32_t                tx_wr_ptr;
    uint32_t                tx_rd_ptr;
    uint8_t                 mac_addr[ETH_ALEN];
    uint16_t                channel;
    uint8_t                 band;
    bool                    monitor_enabled;
    bool                    injecting;
    bool                    fw_ready;
    uint64_t                rx_frames;
    uint64_t                tx_frames;
    uint64_t                rx_bytes;
    uint64_t                tx_bytes;
    uint32_t                rx_errors;
    uint32_t                tx_errors;
};

/* Function prototypes */
int ax210_set_channel(struct ax210_priv *priv, uint16_t channel, uint8_t band);
int ax210_enable_monitor(struct ax210_priv *priv);
int ax210_disable_monitor(struct ax210_priv *priv);
netdev_tx_t ax210_inject_frame(struct sk_buff *skb, struct net_device *ndev);

#endif /* WFP100_AX210_PCIE_H */