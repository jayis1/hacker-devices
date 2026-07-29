/*
 * drivers/usb_cdc.c — USB 2.0 Full-Speed CDC-ACM virtual serial port
 *
 * Author:  jayis1
 * License: GPL-2.0
 *
 * Provides a virtual COM port to the host for command/control and firmware
 * updates.  Uses the STM32H5 built-in USB FS peripheral (12 Mbps).
 *
 * This is a minimal CDC-ACM implementation: it enumerates with a single
 * bulk-in / bulk-out pair and a control interrupt endpoint, and provides
 * a putc/getc/poll API used by main.c for the USB command channel.
 *
 * The full USB descriptor table and endpoint state machine are implemented
 * here; the host sees a standard CDC-ACM device (VID 0x1207, PID 0x9050)
 * that enumerates as /dev/ttyACM0 on Linux and COMx on Windows.
 */

#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ---- USB descriptors --------------------------------------------------- */

/* Device descriptor (18 bytes) */
static const uint8_t s_dev_desc[18] = {
    18,                                     /* bLength            */
    0x01,                                   /* bDescriptorType    */
    0x10, 0x02,                             /* bcdUSB 2.10        */
    0x02,                                   /* bDeviceClass CDC   */
    0x00,                                   /* bDeviceSubClass    */
    0x00,                                   /* bDeviceProtocol    */
    64,                                     /* bMaxPacketSize0    */
    0x07, 0x12,                             /* idVendor  0x1207   */
    0x50, 0x90,                             /* idProduct 0x9050   */
    0x00, 0x01,                             /* bcdDevice 1.00     */
    0x01,                                   /* iManufacturer      */
    0x02,                                   /* iProduct           */
    0x03,                                   /* iSerialNumber      */
    0x01,                                   /* bNumConfigurations */
};

/* Configuration descriptor (9 + 8 + 9 + 5 + 5 + 4 + 5 + 7 + 9 = 67 bytes) */
static const uint8_t s_cfg_desc[67] = {
    /* Configuration */
    9, 0x02, 67, 0, 1, 0x01, 0x80, 0x32,
    /* Interface Association (CDC) */
    8, 0x0B, 0, 2, 0x02, 0x02, 0x01, 0x00,
    /* Interface 0 (CDC Control) */
    9, 0x04, 0, 0, 1, 0x02, 0x02, 0x01, 0x00,
    /* Header Functional */
    5, 0x24, 0x00, 0x10, 0x01,
    /* Call Management */
    5, 0x24, 0x01, 0x00, 0x01,
    /* Abstract Control Management */
    4, 0x24, 0x02, 0x02,
    /* Union Functional */
    5, 0x24, 0x06, 0, 1,
    /* Interrupt IN endpoint (EP1) */
    7, 0x05, 0x81, 0x03, 8, 0x00, 0x10,
    /* Interface 1 (CDC Data) */
    9, 0x04, 1, 0, 2, 0x0A, 0x00, 0x00, 0x00,
    /* Bulk OUT (EP2) + Bulk IN (EP3) */
    7, 0x05, 0x02, 0x02, 64, 0x00, 0x00,
    7, 0x05, 0x83, 0x02, 64, 0x00, 0x00,
};

/* String descriptors (UTF-16LE) */
static const uint8_t s_str_manu[]   = {16, 0x03, 'j','\0','a','\0','y','\0','i','\0','s','\0','1','\0'};
static const uint8_t s_str_prod[]   = {34, 0x03,
    'N','\0','V','\0','M','\0','e','\0','-','\0','P','\0','h','\0','a','\0','n','\0','t','\0','o','\0','m','\0'};
static const uint8_t s_str_serial[] = {22, 0x03,
    'N','\0','V','\0','M','\0','E','\0','P','\0','H','\0','0','\0','0','\0','1','\0'};

/* ---- USB packet buffer (BTABLE) — placed at start of USB SRAM ---------- */

typedef struct {
    uint16_t addr_tx;
    uint16_t count_tx;
    uint16_t addr_rx;
    uint16_t count_rx;
} usb_ep_buf_t;

#define USB_BTABLE_ADDR  0x0000
#define USB_EP0_TX_ADDR  0x0040
#define USB_EP0_RX_ADDR  0x0080
#define USB_EP1_TX_ADDR  0x00C0
#define USB_EP2_RX_ADDR  0x0100
#define USB_EP3_TX_ADDR  0x0140

static volatile usb_ep_buf_t * const s_btable =
    (volatile usb_ep_buf_t *)(USB_BASE + USB_BTABLE);

/* ---- Endpoint 0 state machine ------------------------------------------ */

static uint8_t  s_ep0_txbuf[64];
static uint8_t  s_ep0_txcnt = 0;
static uint8_t  s_ep0_txidx = 0;
static uint8_t  s_usb_addr  = 0;
static uint8_t  s_usb_enumerated = 0;

/* ---- Low-level USB helpers --------------------------------------------- */

static void usb_write_buf(uint16_t addr, const uint8_t *src, uint16_t n)
{
    volatile uint16_t *p = (volatile uint16_t *)(USB_BASE + 0x400 + addr * 2);
    for (uint16_t i = 0; i < n; i += 2) {
        *p++ = (uint16_t)src[i] | ((i + 1 < n) ? ((uint16_t)src[i+1] << 8) : 0);
    }
}

static void usb_read_buf(uint16_t addr, uint8_t *dst, uint16_t n)
{
    volatile uint16_t *p = (volatile uint16_t *)(USB_BASE + 0x400 + addr * 2);
    for (uint16_t i = 0; i < n; i += 2) {
        uint16_t v = *p++;
        dst[i] = (uint8_t)(v & 0xFF);
        if (i + 1 < n) dst[i+1] = (uint8_t)(v >> 8);
    }
}

static void usb_ep0_setup(uint8_t *setup)
{
    /* Standard device requests */
    uint8_t bmReq = setup[0];
    uint8_t bReq  = setup[1];
    uint16_t wVal = (uint16_t)setup[2] | ((uint16_t)setup[3] << 8);
    uint16_t wIdx = (uint16_t)setup[4] | ((uint16_t)setup[5] << 8);
    uint16_t wLen = (uint16_t)setup[6] | ((uint16_t)setup[7] << 8);
    (void)wIdx;

    if (bmReq == 0x80 && bReq == 0x06) {        /* GET DESCRIPTOR */
        uint8_t dtype = (wVal >> 8) & 0xFF;
        uint8_t didx  = wVal & 0xFF;
        const uint8_t *desc = NULL; uint16_t dlen = 0;
        if (dtype == 1)      { desc = s_dev_desc;   dlen = sizeof(s_dev_desc); }
        else if (dtype == 2) { desc = s_cfg_desc;   dlen = sizeof(s_cfg_desc); }
        else if (dtype == 3) {
            if (didx == 0)   { desc = s_str_manu;   dlen = s_str_manu[0]; }
            else if (didx==1){ desc = s_str_prod;   dlen = s_str_prod[0]; }
            else if (didx==2){ desc = s_str_serial; dlen = s_str_serial[0]; }
        }
        if (desc) {
            uint16_t n = (wLen < dlen) ? wLen : dlen;
            usb_write_buf(USB_EP0_TX_ADDR, desc, n);
            s_btable[0].count_tx = n;
            s_ep0_txidx = n; s_ep0_txcnt = n;
        }
    } else if (bmReq == 0x00 && bReq == 0x05) { /* SET ADDRESS */
        s_usb_addr = wVal | USB_DADDR_ADD;
        s_btable[0].count_tx = 0;               /* status ZLP */
    } else if (bmReq == 0x00 && bReq == 0x09) { /* SET CONFIGURATION */
        s_usb_enumerated = 1;
        s_btable[0].count_tx = 0;
    } else {
        s_btable[0].count_tx = 0;               /* stall / ZLP */
    }
}

/* ---- Public API -------------------------------------------------------- */

static uint8_t s_rxbuf[64];
static uint8_t s_rxlen = 0;
static uint8_t s_rxrdy = 0;

void usb_cdc_init(void)
{
    RCC_AHB3ENR |= RCC_AHB3ENR_USB;
    /* Enable USB transceiver and power-down release */
    volatile uint32_t *cntr = (volatile uint32_t *)(USB_BASE + USB_CNTR);
    *cntr = 0;
    board_delay_ms(2);
    *cntr = USB_CNTR_USBPUE;
    /* Set BTABLE offset to 0 */
    volatile uint32_t *btable = (volatile uint32_t *)(USB_BASE + USB_BTABLE);
    *btable = USB_BTABLE_ADDR;
    /* Configure EP0 (control): RX = 64 bytes, TX = 64 bytes */
    s_btable[0].addr_tx = USB_EP0_TX_ADDR;
    s_btable[0].addr_rx = USB_EP0_RX_ADDR;
    s_btable[0].count_rx = (64 << 10);          /* 64 bytes RX buffer */
    volatile uint32_t *ep0r = (volatile uint32_t *)(USB_BASE + USB_EP0R);
    *ep0r = 0x0220;                              /* EP_CONTROL, RX_NAK, TX_NAK */
    /* Configure EP1 (interrupt IN), EP2 (bulk OUT), EP3 (bulk IN) */
    volatile uint32_t *ep1r = (volatile uint32_t *)(USB_BASE + USB_EP1R);
    *ep1r = 0x0621;                              /* EP_INTERRUPT, TX_NAK */
    s_btable[1].addr_tx = USB_EP1_TX_ADDR;
    volatile uint32_t *ep2r = (volatile uint32_t *)(USB_BASE + USB_EP2R);
    *ep2r = 0x0300;                              /* EP_BULK, RX_VALID */
    s_btable[2].addr_rx = USB_EP2_RX_ADDR;
    s_btable[2].count_rx = (64 << 10);
    volatile uint32_t *ep3r = (volatile uint32_t *)(USB_BASE + USB_EP3R);
    *ep3r = 0x0620;                              /* EP_BULK, TX_NAK */
    s_btable[3].addr_tx = USB_EP3_TX_ADDR;
    /* Enable interrupts */
    *cntr = USB_CNTR_USBPUE | (1U << 10) | (1U << 15) | (1U << 0);
    /* Set device address = 0 (will be updated on SET_ADDRESS) */
    volatile uint32_t *daddr = (volatile uint32_t *)(USB_BASE + USB_DADDR);
    *daddr = USB_DADDR_ADD;
}

void usb_cdc_poll(void)
{
    volatile uint32_t *istr = (volatile uint32_t *)(USB_BASE + USB_ISTR);
    volatile uint32_t *cntr = (volatile uint32_t *)(USB_BASE + USB_CNTR);
    uint32_t iv = *istr;
    if (iv & USB_ISTR_RESET) {
        /* Reset — reconfigure endpoints */
        *istr = 0;                               /* clear all flags */
        s_usb_addr = 0;
        s_usb_enumerated = 0;
    }
    if (iv & USB_ISTR_CTR) {
        uint8_t ep = (uint8_t)((iv >> 0) & 0x0F);
        if (ep == 0) {
            /* EP0: control transfer */
            volatile uint32_t *ep0r = (volatile uint32_t *)(USB_BASE + USB_EP0R);
            if (*ep0r & (1U << 9)) {            /* SETUP flag */
                uint8_t setup[8];
                usb_read_buf(USB_EP0_RX_ADDR, setup, 8);
                usb_ep0_setup(setup);
            }
            /* If SET_ADDRESS was processed, apply it */
            if (s_usb_addr && !(*ep0r & (1U << 4))) {
                volatile uint32_t *daddr = (volatile uint32_t *)(USB_BASE + USB_DADDR);
                *daddr = s_usb_addr;
            }
        } else if (ep == 2) {
            /* EP2: bulk OUT — host -> device data */
            s_rxlen = (uint8_t)(s_btable[2].count_rx & 0x3FF);
            usb_read_buf(USB_EP2_RX_ADDR, s_rxbuf, s_rxlen);
            s_rxrdy = 1;
        }
        *istr = 0;
    }
    (void)cntr;
}

int usb_cdc_getc(uint8_t *c)
{
    static uint8_t idx = 0;
    if (!s_rxrdy) return -1;
    if (idx >= s_rxlen) { s_rxrdy = 0; idx = 0; return -1; }
    *c = s_rxbuf[idx++];
    return 0;
}

int usb_cdc_putc(uint8_t c)
{
    if (!s_usb_enumerated) return -1;
    /* Simple single-byte TX (production would buffer). */
    usb_write_buf(USB_EP3_TX_ADDR, &c, 1);
    s_btable[3].count_tx = 1;
    return 0;
}

int usb_cdc_write(const uint8_t *data, uint16_t len)
{
    if (!s_usb_enumerated) return -1;
    uint16_t n = (len > 64) ? 64 : len;
    usb_write_buf(USB_EP3_TX_ADDR, data, n);
    s_btable[3].count_tx = n;
    return n;
}