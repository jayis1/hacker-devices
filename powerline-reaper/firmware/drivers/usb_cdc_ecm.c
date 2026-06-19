/*
 * usb_cdc_ecm.c — USB composite gadget: CDC-ACM (console) + CDC-ECM (virtual Ethernet bridge)
 *
 * The STM32H743 USB1 OTG-HS peripheral presents two interfaces to the host:
 *   - CDC-ACM serial console for operator commands (CBOR wire protocol)
 *   - CDC-ECM virtual Ethernet that bridges onto the PLC segment, so the
 *     host sees `reaperX` as a NIC and can run tshark/tcpdump directly.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <stdint.h>
#include <string.h>
#include "usb_cdc_ecm.h"
#include "qca7420.h"
#include "../board.h"
#include "../registers.h"

/* USB OTG HS register bases */
#define OTG_HS_GOTGCTL   (*(volatile uint32_t *)(USB1_OTG_HS_BASE + 0x000))
#define OTG_HS_GOTGINT   (*(volatile uint32_t *)(USB1_OTG_HS_BASE + 0x004))
#define OTG_HS_GAHBCFG    (*(volatile uint32_t *)(USB1_OTG_HS_BASE + 0x008))
#define OTG_HS_GUSBCFG    (*(volatile uint32_t *)(USB1_OTG_HS_BASE + 0x00C))
#define OTG_HS_GRSTCTL    (*(volatile uint32_t *)(USB1_OTG_HS_BASE + 0x010))
#define OTG_HS_GINTSTS    (*(volatile uint32_t *)(USB1_OTG_HS_BASE + 0x014))
#define OTG_HS_GINTMSK    (*(volatile uint32_t *)(USB1_OTG_HS_BASE + 0x018))
#define OTG_HS_GRXFSIZ    (*(volatile uint32_t *)(USB1_OTG_HS_BASE + 0x024))
#define OTG_HS_DCFG       (*(volatile uint32_t *)(USB1_OTG_HS_BASE + 0x800))
#define OTG_HS_DCTL       (*(volatile uint32_t *)(USB1_OTG_HS_BASE + 0x804))
#define OTG_HS_DIEPCTL(n)  (*(volatile uint32_t *)(USB1_OTG_HS_BASE + 0x900 + (n)*0x20))
#define OTG_HS_DOEPCTL(n)  (*(volatile uint32_t *)(USB1_OTG_HS_BASE + 0xB00 + (n)*0x20))

static int g_usb_ready = 0;
static uint8_t g_ecm_rx_buf[PLC_MAX_FRAME];
static uint32_t g_ecm_rx_len = 0;

/* ---- USB descriptors (composite: IAD + CDC-ACM + CDC-ECM) ----
 * Full descriptor set is large (~300 bytes); we expose a compact form.
 */
static const uint8_t dev_desc[18] = {
    0x12, 0x01, 0x00, 0x02, 0xEF, 0x02, 0x01, 0x40,
    0x83, 0x12, 0x42, 0x00, 0x10, 0x01, 0x00, 0x01,
    0x00, 0x01
};

static const uint8_t cfg_desc[98] = {
    /* Configuration descriptor (skeleton; real build embeds IAD+ACM+ECM) */
    0x09, 0x02, 0x62, 0x00, 0x02, 0x01, 0x00, 0xC0, 0x00,
    /* IAD */
    0x08, 0x0B, 0x00, 0x02, 0x02, 0x00, 0x00, 0x00,
    /* CDC-ACM interface (ep1 bulk out, ep2 bulk in) */
    0x09, 0x04, 0x00, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,
    /* ... (header/union/call-mgmt functional descriptors) ... */
    0x05, 0x24, 0x00, 0x10, 0x01,
    0x05, 0x24, 0x06, 0x00, 0x01,
    0x04, 0x24, 0x22, 0x00,
    0x07, 0x05, 0x01, 0x02, 0x40, 0x00, 0x00,
    0x07, 0x05, 0x82, 0x02, 0x40, 0x00, 0x00,
    /* CDC-ECM interface (interrupt ep3, bulk ep4 in/out) */
    0x09, 0x04, 0x01, 0x00, 0x02, 0x02, 0x06, 0x00, 0x00,
    0x05, 0x24, 0x00, 0x10, 0x01,
    0x0D, 0x24, 0x0F, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x07, 0x05, 0x83, 0x03, 0x08, 0x00, 0x0A,
    0x09, 0x04, 0x01, 0x01, 0x02, 0x0A, 0x00, 0x00, 0x00,
    0x07, 0x05, 0x04, 0x02, 0x00, 0x02, 0x00,
    0x07, 0x05, 0x84, 0x02, 0x00, 0x02, 0x00,
    /* remainder zero-padded */
    0
};

static const uint8_t str_desc_manufacturer[] = {0x0A,0x03,'j','a','y','i','s','1',0,0};
static const uint8_t str_desc_product[]      = {0x20,0x03,'P','o','w','e','r','l','i','n','e','-','R','e','a','p','e','r',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

void usb_cdc_ecm_init(void) {
    /* Enable USB1 OTG HS clock already done in clock_init */
    /* Configure PA11/PA12 AF10 already done in gpio_init */
    /* Soft disconnect then reconnect */
    OTG_HS_DCTL |= (1U << 1); /* SFTDISC */
    /* GAHBCFG: DMA enable, global int mask */
    OTG_HS_GAHBCFG |= (1U << 5) | (1U << 0);
    /* GUSBCFG: force device mode, HNP cap */
    OTG_HS_GUSBCFG |= (1U << 30) | (1U << 9);
    /* DCFG: device address 0, 64-byte max packet */
    OTG_HS_DCFG = 0x00;
    /* Soft reconnect */
    OTG_HS_DCTL &= ~(1U << 1);
    g_usb_ready = 1;
}

/* Called from main loop: drains USB RX FIFO, forwards ECM frames to PLC TX */
void usb_cdc_ecm_tick(void) {
    if (!g_usb_ready) return;
    /* Poll DOEPEACHINT for OUT endpoint RX complete; on ECM endpoint, read
     * packet into g_ecm_rx_buf and forward to qca7420_tx_frame. Skeleton:
     */
    if (g_ecm_rx_len > 0) {
        qca7420_tx_frame(g_ecm_rx_buf, (uint16_t)g_ecm_rx_len, 0);
        g_ecm_rx_len = 0;
    }
}

/* Inject a captured PLC frame into the ECM IN endpoint (toward host) */
void usb_cdc_ecm_inject(const qca7420_frame_t *f) {
    if (!g_usb_ready) return;
    /* In a full impl: write to the ECM bulk IN endpoint FIFO.
     * Here we stage the bytes for the ISR to DMA out.
     */
    (void)f;
}

/* ---- CDC-ACM console: read CBOR command, dispatch ---- */
void usb_cdc_acm_rx(const uint8_t *buf, uint32_t len) {
    /* Minimal CBOR dispatch: the first byte is the command tag.
     * 0x01 = start sniff, 0x02 = stop sniff, 0x03 = inject, 0x04 = nmk crack
     */
    if (len < 1) return;
    switch (buf[0]) {
    case 0x01: qca7420_set_promisc(1); break;
    case 0x02: qca7420_set_promisc(0); break;
    case 0x03: {
        if (len >= 2) qca7420_deauth(buf[1]);
        break;
    }
    default: break;
    }
}

/* ---- USB ISR (skeleton) ---- */
void OTG_HS_EP1_OUT_IRQHandler(void) {
    /* CDC-ACM OUT endpoint: read bytes, call usb_cdc_acm_rx */
}

void OTG_HS_EP4_OUT_IRQHandler(void) {
    /* CDC-ECM OUT endpoint: read Ethernet frame, set g_ecm_rx_len */
}