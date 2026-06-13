/*
 * WFP-100 — Intel AX210 PCIe WiFi Driver (Monitor Mode)
 * Linux kernel module for AX210NGW on StarFive JH7110 via PCIe Gen3 x1.
 *
 * See phase4_software_stack.md section 4.4 for the full implementation.
 * This file contains the module skeleton — the complete driver with
 * DMA ring buffers, monitor mode, channel hopping, and frame injection
 * is documented in the Phase 4 markdown.
 *
 * Build: make -C /path/to/linux M=$(pwd) modules
 * Load:  insmod ax210_pcie.ko
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/ieee80211.h>
#include <linux/workqueue.h>
#include <linux/firmware.h>
#include <linux/delay.h>
#include "ax210_pcie.h"

MODULE_AUTHOR("WFP-100 Project");
MODULE_DESCRIPTION("Intel AX210 WiFi 6E PCIe Driver — Monitor Mode for Pentesting");
MODULE_LICENSE("GPL");

/* PCI device ID table */
static const struct pci_device_id ax210_pci_ids[] = {
    { PCI_DEVICE(AX210_PCIE_VENDOR_ID, AX210_PCIE_DEVICE_ID) },
    { }
};
MODULE_DEVICE_TABLE(pci, ax210_pci_ids);

/* Open the network device — allocate DMA rings, enable monitor mode */
static int ax210_open(struct net_device *ndev)
{
    struct ax210_priv *priv = netdev_priv(ndev);
    int ret;

    ret = request_irq(priv->pdev->irq, ax210_irq_handler,
                      IRQF_SHARED, "ax210", priv);
    if (ret)
        return ret;

    priv->rx_ring = dma_alloc_coherent(&priv->pdev->dev,
                                        AX210_RX_RING_SIZE * AX210_FRAME_BUF_SIZE,
                                        &priv->rx_dma, GFP_KERNEL);
    if (!priv->rx_ring) {
        free_irq(priv->pdev->irq, priv);
        return -ENOMEM;
    }

    priv->tx_ring = dma_alloc_coherent(&priv->pdev->dev,
                                         AX210_TX_RING_SIZE * AX210_FRAME_BUF_SIZE,
                                         &priv->tx_dma, GFP_KERNEL);
    if (!priv->tx_ring) {
        dma_free_coherent(&priv->pdev->dev,
                          AX210_RX_RING_SIZE * AX210_FRAME_BUF_SIZE,
                          priv->rx_ring, priv->rx_dma);
        free_irq(priv->pdev->irq, priv);
        return -ENOMEM;
    }

    priv->rx_wr_ptr = 0;
    priv->rx_rd_ptr = 0;
    priv->tx_wr_ptr = 0;
    priv->tx_rd_ptr = 0;

    /* Program DMA ring addresses into hardware */
    mmio_write32(priv->mmio + AX210_REG_FH_RX_BASE, priv->rx_dma);
    mmio_write32(priv->mmio + AX210_REG_FH_RX_SIZE, AX210_RX_RING_SIZE);
    mmio_write32(priv->mmio + AX210_REG_FH_TX_BASE, priv->tx_dma);
    mmio_write32(priv->mmio + AX210_REG_FH_TX_SIZE, AX210_TX_RING_SIZE);

    ret = ax210_enable_monitor(priv);
    if (ret) {
        dev_err(&priv->pdev->dev, "Failed to enable monitor mode: %d\n", ret);
        dma_free_coherent(&priv->pdev->dev,
                          AX210_RX_RING_SIZE * AX210_FRAME_BUF_SIZE,
                          priv->rx_ring, priv->rx_dma);
        dma_free_coherent(&priv->pdev->dev,
                          AX210_TX_RING_SIZE * AX210_FRAME_BUF_SIZE,
                          priv->tx_ring, priv->tx_dma);
        free_irq(priv->pdev->irq, priv);
        return ret;
    }

    netif_start_queue(ndev);
    return 0;
}

/* Stop the network device */
static int ax210_stop(struct net_device *ndev)
{
    struct ax210_priv *priv = netdev_priv(ndev);

    netif_stop_queue(ndev);
    mmio_write32(priv->mmio + AX210_REG_MONITOR_CTRL, 0);

    dma_free_coherent(&priv->pdev->dev,
                      AX210_RX_RING_SIZE * AX210_FRAME_BUF_SIZE,
                      priv->rx_ring, priv->rx_dma);
    dma_free_coherent(&priv->pdev->dev,
                      AX210_TX_RING_SIZE * AX210_FRAME_BUF_SIZE,
                      priv->tx_ring, priv->tx_dma);

    free_irq(priv->pdev->irq, priv);
    priv->monitor_enabled = false;
    return 0;
}

/* Inject a raw 802.11 frame */
static netdev_tx_t ax210_inject_frame(struct sk_buff *skb,
                                       struct net_device *ndev)
{
    struct ax210_priv *priv = netdev_priv(ndev);
    struct ax210_tx_desc *tx_desc;
    uint32_t tx_next;

    if (!priv->monitor_enabled || !priv->fw_ready) {
        ndev->stats.tx_dropped++;
        dev_kfree_skb(skb);
        return NETDEV_TX_BUSY;
    }

    tx_next = priv->tx_wr_ptr % AX210_TX_RING_SIZE;

    if ((priv->tx_wr_ptr - priv->tx_rd_ptr) >= AX210_TX_RING_SIZE - 1) {
        ndev->stats.tx_dropped++;
        dev_kfree_skb(skb);
        return NETDEV_TX_BUSY;
    }

    tx_desc = &priv->tx_ring[tx_next];
    tx_desc->flags = (1U << 0);
    tx_desc->len = skb->len;
    tx_desc->channel = priv->channel;
    tx_desc->rate = 0;
    tx_desc->retry_limit = 7;

    memcpy(tx_desc->data, skb->data, skb->len);

    priv->tx_wr_ptr++;
    mmio_write32(priv->mmio + AX210_REG_FH_TX_WR_PTR, priv->tx_wr_ptr);

    /* Kick TX engine */
    mmio_write32(priv->mmio + AX210_REG_FH_CSR,
                 mmio_read32(priv->mmio + AX210_REG_FH_CSR) | (1U << 8));

    priv->tx_frames++;
    priv->tx_bytes += skb->len;
    ndev->stats.tx_packets++;
    ndev->stats.tx_bytes += skb->len;

    dev_kfree_skb(skb);
    return NETDEV_TX_OK;
}

/* IRQ handler (top half) */
static irqreturn_t ax210_irq_handler(int irq, void *dev_id)
{
    struct ax210_priv *priv = dev_id;
    uint32_t inta = mmio_read32(priv->mmio + AX210_REG_INTA);

    if (!inta)
        return IRQ_NONE;

    mmio_write32(priv->mmio + AX210_REG_INT_CLEAR, inta);
    schedule_work(&priv->irq_work);
    return IRQ_HANDLED;
}

/* Bottom-half IRQ processing */
static void ax210_irq_work(struct work_struct *work)
{
    struct ax210_priv *priv = container_of(work, struct ax210_priv, irq_work);
    uint32_t inta = mmio_read32(priv->mmio + AX210_REG_INTA);

    if (inta & AX210_INTA_RX_COMPLETE) {
        while (priv->rx_rd_ptr != priv->rx_wr_ptr) {
            struct ax210_rx_desc *rx_desc =
                &priv->rx_ring[priv->rx_rd_ptr % AX210_RX_RING_SIZE];
            struct sk_buff *skb;
            uint32_t frame_len = rx_desc->len;

            if (frame_len > AX210_FRAME_BUF_SIZE || frame_len == 0) {
                priv->rx_errors++;
                goto rx_next;
            }

            skb = netdev_alloc_skb(priv->ndev, frame_len + 2);
            if (!skb) {
                priv->rx_errors++;
                goto rx_next;
            }

            skb_put(skb, frame_len);
            memcpy(skb->data, rx_desc->data, frame_len);
            skb->protocol = eth_type_trans(skb, priv->ndev);
            netif_rx(skb);

            priv->rx_frames++;
            priv->rx_bytes += frame_len;
rx_next:
            priv->rx_rd_ptr++;
        }
        mmio_write32(priv->mmio + AX210_REG_FH_RX_RD_PTR, priv->rx_rd_ptr);
    }

    if (inta & AX210_INTA_FW_READY) {
        priv->fw_ready = true;
        dev_info(&priv->pdev->dev, "AX210 firmware ready\n");
    }
}

/* Netdev operations */
static const struct net_device_ops ax210_netdev_ops = {
    .ndo_open       = ax210_open,
    .ndo_stop        = ax210_stop,
    .ndo_start_xmit  = ax210_inject_frame,
};

/* PCI probe */
static int ax210_pci_probe(struct pci_dev *pdev,
                             const struct pci_device_id *id)
{
    struct net_device *ndev;
    struct ax210_priv *priv;
    int ret;

    ret = pci_enable_device(pdev);
    if (ret)
        return ret;

    ret = pci_request_regions(pdev, KBUILD_MODNAME);
    if (ret)
        goto err_disable;

    ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(64));
    if (ret) {
        ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
        if (ret)
            goto err_regions;
    }

    ndev = alloc_etherdev(sizeof(struct ax210_priv));
    if (!ndev) {
        ret = -ENOMEM;
        goto err_regions;
    }

    priv = netdev_priv(ndev);
    priv->pdev = pdev;
    priv->ndev = ndev;
    priv->monitor_enabled = false;
    priv->injecting = false;
    priv->fw_ready = false;

    priv->mmio = pci_iomap(pdev, AX210_PCI_BAR_NUM, 0);
    if (!priv->mmio) {
        ret = -EIO;
        goto err_free_netdev;
    }

    INIT_WORK(&priv->irq_work, ax210_irq_work);
    pci_set_drvdata(pdev, priv);
    pci_set_master(pdev);

    /* Read MAC address */
    priv->mac_addr[0] = mmio_read32(priv->mmio + AX210_REG_MAC_ADDR0) & 0xFF;
    priv->mac_addr[1] = (mmio_read32(priv->mmio + AX210_REG_MAC_ADDR0) >> 8) & 0xFF;
    priv->mac_addr[2] = (mmio_read32(priv->mmio + AX210_REG_MAC_ADDR0) >> 16) & 0xFF;
    priv->mac_addr[3] = (mmio_read32(priv->mmio + AX210_REG_MAC_ADDR0) >> 24) & 0xFF;
    priv->mac_addr[4] = mmio_read32(priv->mmio + AX210_REG_MAC_ADDR1) & 0xFF;
    priv->mac_addr[5] = (mmio_read32(priv->mmio + AX210_REG_MAC_ADDR1) >> 8) & 0xFF;
    eth_hw_addr_set(ndev, priv->mac_addr);

    ndev->netdev_ops = &ax210_netdev_ops;
    ndev->flags |= IFF_PROMISC | IFF_NOARP;

    ret = register_netdev(ndev);
    if (ret)
        goto err_free_netdev;

    dev_info(&pdev->dev, "AX210 WiFi driver registered: %s (%pM)\n",
             ndev->name, priv->mac_addr);
    return 0;

err_free_netdev:
    free_netdev(ndev);
err_regions:
    pci_release_regions(pdev);
err_disable:
    pci_disable_device(pdev);
    return ret;
}

static void ax210_pci_remove(struct pci_dev *pdev)
{
    struct ax210_priv *priv = pci_get_drvdata(pdev);
    unregister_netdev(priv->ndev);
    pci_iounmap(pdev, priv->mmio);
    pci_release_regions(pdev);
    pci_disable_device(pdev);
    free_netdev(priv->ndev);
}

static struct pci_driver ax210_pci_driver = {
    .name = KBUILD_MODNAME,
    .id_table = ax210_pci_ids,
    .probe = ax210_pci_probe,
    .remove = ax210_pci_remove,
};

module_pci_driver(ax210_pci_driver);