/*
 * usb_cdc.c — USB 2.0 Full-Speed CDC virtual serial port on STM32G474
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Implements a minimal CDC-ACM device using the STM32 USB FS peripheral.
 * Endpoints:
 *   EP0  — control (enumeration, SET_LINE_CODING, etc.)
 *   EP1  — bulk OUT (host → device data)
 *   EP2  — bulk IN  (device → host data)
 *   EP3  — interrupt IN (notification, SendBreak)
 *
 * Uses the USB packet memory area (PMA) at 0x40006000 for buffer descriptors
 * and data. Ring buffers decouple the application from USB transfer timing.
 */

#include "usb_cdc.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ---- PMA buffer descriptor table ----
 * Each entry is 8 bytes: ADDR_TX, COUNT_TX, ADDR_RX, COUNT_RX (16-bit each,
 * but COUNT_RX is split into two 16-bit halfwords in G4). We keep it simple.
 */
#define BTABLE_ADDR       0x0000
#define PMA_EP0_TX_ADDR   0x0040
#define PMA_EP0_RX_ADDR   0x0080
#define PMA_EP1_RX_ADDR   0x00C0   /* bulk OUT */
#define PMA_EP2_TX_ADDR   0x0100   /* bulk IN  */
#define PMA_EP3_TX_ADDR   0x0180   /* interrupt IN */

#define EP0_MAXPKT         64
#define EP_BULK_MAXPKT     64
#define EP_INT_MAXPKT      8

/* CDC device descriptor */
static const uint8_t dev_desc[] = {
    18, 0x01,                               /* bLength, bDescriptorType */
    0x00, 0x02,                             /* bcdUSB 2.00 */
    0x02, 0x00, 0x00,                       /* class/subclass/proto (CDC) */
    EP0_MAXPKT, 0x00,                       /* bMaxPacketSize0 */
    0x83, 0x04,                             /* idVendor  (0x0483 ST) */
    0x50, 0x57,                             /* idProduct (0x5750) */
    0x00, 0x02,                             /* bcdDevice 2.00 */
    0x01, 0x02, 0x03,                       /* iManufacturer, iProduct, iSerial */
    0x01                                    /* bNumConfigurations */
};

/* Configuration descriptor (CDC-ACM) — 67 bytes total */
static const uint8_t cfg_desc[] = {
    /* Config descriptor */
    9, 0x02,
    67, 0x00,                               /* wTotalLength */
    0x02, 0x01, 0x00,                       /* bNumInterfaces, bConfigurationValue, iConfiguration */
    0xC0, 0x32,                             /* bmAttributes (self-powered), bMaxPower (100 mA) */

    /* Interface 0 — CDC Control */
    9, 0x04, 0x00, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,
    /* CDC Header functional */
    5, 0x24, 0x00, 0x10, 0x01,
    /* CDC Call Management */
    5, 0x24, 0x01, 0x00, 0x01,
    /* CDC ACM */
    4, 0x24, 0x02, 0x02,
    /* CDC Union */
    5, 0x24, 0x06, 0x00, 0x01,
    /* Notification endpoint (EP3 IN interrupt) */
    7, 0x05, 0x83, 0x03, EP_INT_MAXPKT, 0x00, 0x10,

    /* Interface 1 — CDC Data */
    9, 0x04, 0x01, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,
    /* Bulk OUT endpoint (EP1) */
    7, 0x05, 0x01, 0x02, EP_BULK_MAXPKT, 0x00, 0x00,
    /* Bulk IN endpoint (EP2) */
    7, 0x05, 0x82, 0x02, EP_BULK_MAXPKT, 0x00, 0x00
};

/* String descriptors */
static const uint8_t str_lang[]   = { 4, 0x03, 0x09, 0x04 };
static const uint8_t str_mfr[]    = { 22, 0x03, 'j','\0','a','\0','y','\0','i','\0','s','\0','1','\0','\0','\0','\0','\0','\0','\0' };
static const uint8_t str_prod[]   = { 44, 0x03,
    'E','\0','m','\0','b','\0','e','\0','r','\0','-','\0',
    'T','\0','a','\0','p','\0',' ','\0','P','\0','D','\0',
    ' ','\0','F','\0','u','\0','z','\0','z','\0','e','\0','r','\0','\0','\0'};
static const uint8_t str_serial[] = { 18, 0x03, '0','\0','0','\0','0','\0','0','\0','1','\0','\0','\0','\0','\0','\0','\0' };

/* ---- Ring buffers ---- */
static volatile char cdc_tx_buf[CDC_TX_BUF_SIZE];
static volatile uint16_t cdc_tx_head = 0, cdc_tx_tail = 0;
static volatile char cdc_rx_buf[CDC_RX_BUF_SIZE];
static volatile uint16_t cdc_rx_head = 0, cdc_rx_tail = 0;

static volatile int cdc_dtr = 0;   /* host port open */
static volatile int tx_busy = 0;
static volatile uint8_t usb_addr = 0;
static volatile uint8_t usb_configured = 0;

/* ---- PMA access helpers ----
 * The STM32G4 USB PMA is 16-bit wide. We write 16-bit words.
 */
static void pma_write16(uint16_t pma_off, const uint8_t *src, int len) {
    volatile uint16_t *pma = (volatile uint16_t *)(USB_PMA_BASE + pma_off);
    for (int i = 0; i < len; i += 2) {
        uint16_t w = src[i];
        if (i + 1 < len) w |= (src[i+1] << 8);
        *pma++ = w;
    }
}

static void pma_read16(uint16_t pma_off, uint8_t *dst, int len) {
    volatile uint16_t *pma = (volatile uint16_t *)(USB_PMA_BASE + pma_off);
    for (int i = 0; i < len; i += 2) {
        uint16_t w = *pma++;
        dst[i] = w & 0xFF;
        if (i + 1 < len) dst[i+1] = (w >> 8) & 0xFF;
    }
}

/* ---- Endpoint setup ---- */
static void ep_set_tx(uint8_t ep, uint16_t pma_off, uint16_t maxpkt) {
    /* For G4, BTABLE is 16-bit; we write the TX_ADDR and TX_COUNT words. */
    volatile uint16_t *bd = (volatile uint16_t *)(USB_PMA_BASE + (ep * 8));
    bd[0] = pma_off;       /* ADDR_TX */
    /* bd[1] = COUNT_TX — set before each TX */
    (void)maxpkt;
}

static void ep_set_rx(uint16_t ep, uint16_t pma_off, uint16_t maxpkt) {
    volatile uint16_t *bd = (volatile uint16_t *)(USB_PMA_BASE + (ep * 8));
    bd[2] = pma_off;       /* ADDR_RX */
    bd[3] = (1 << 15) | maxpkt;  /* COUNT_RX: block size = maxpkt (g4 uses bit15+count) */
}

/* ---- Handle EP0 control requests ---- */
static void handle_ep0_setup(void) {
    uint8_t setup[8];
    pma_read16(PMA_EP0_RX_ADDR, setup, 8);

    uint8_t bmReqType = setup[0];
    uint8_t bRequest  = setup[1];
    uint16_t wValue   = setup[2] | (setup[3] << 8);
    uint16_t wIndex   = setup[4] | (setup[5] << 8);
    uint16_t wLength  = setup[6] | (setup[7] << 8);

    (void)wIndex;

    if ((bmReqType & 0x60) == 0x00) {
        /* Standard device request */
        switch (bRequest) {
            case 0x05: { /* SET_ADDRESS */
                usb_addr = wValue & 0x7F;
                /* Send zero-length status, then set address */
                USB->EP0R = (USB->EP0R & 0x0F0F) | USB_EP_CTR_TX;
                USB->DADDR = (1U << 7) | usb_addr;
                return;
            }
            case 0x09: /* SET_CONFIGURATION */
                usb_configured = 1;
                break;
            case 0x06: { /* GET_DESCRIPTOR */
                uint8_t dtype = wValue >> 8;
                uint8_t didx  = wValue & 0xFF;
                const uint8_t *d = NULL;
                int dlen = 0;
                switch (dtype) {
                    case 0x01: d = dev_desc;  dlen = sizeof(dev_desc); break;
                    case 0x02: d = cfg_desc;  dlen = sizeof(cfg_desc); break;
                    case 0x03:
                        if (didx == 0)      { d = str_lang;   dlen = str_lang[0]; }
                        else if (didx == 1) { d = str_mfr;    dlen = str_mfr[0]; }
                        else if (didx == 2) { d = str_prod;   dlen = str_prod[0]; }
                        else if (didx == 3) { d = str_serial; dlen = str_serial[0]; }
                        break;
                }
                if (d && dlen) {
                    if (dlen > wLength) dlen = wLength;
                    pma_write16(PMA_EP0_TX_ADDR, d, dlen);
                    volatile uint16_t *bd = (volatile uint16_t *)(USB_PMA_BASE + 0);
                    bd[1] = dlen;  /* TX_COUNT */
                    USB->EP0R = (USB->EP0R & 0x0F0F) | USB_EP_CTR_TX;
                    return;
                }
                break;
            }
            default:
                break;
        }
    } else if ((bmReqType & 0x60) == 0x20) {
        /* CDC class request */
        if (bRequest == 0x22) { /* SET_CONTROL_LINE_STATE */
            cdc_dtr = wValue & 0x01;
        }
        /* Send zero-length status */
        volatile uint16_t *bd = (volatile uint16_t *)(USB_PMA_BASE + 0);
        bd[1] = 0;
        USB->EP0R = (USB->EP0R & 0x0F0F) | USB_EP_CTR_TX;
        return;
    }

    /* Stall on unsupported request */
    USB->EP0R = (USB->EP0R & 0x0F0F) | USB_EP_CTR_TX | (1U << 4); /* DTOG_TX stall */
}

/* ---- USB interrupt handler ---- */
void USB_FS_IRQHandler(void) {
    uint16_t istr = USB->ISTR;

    if (istr & USB_ISTR_RESET) {
        /* Reset — reinit endpoints, set address 0 */
        USB->ISTR = ~(USB_ISTR_RESET);
        usb_addr = 0;
        usb_configured = 0;
        USB->DADDR = (1U << 7);
        USB->BTABLE = BTABLE_ADDR;

        /* Configure EP0 (control) */
        USB->EP0R = 0x0000;
        ep_set_tx(0, PMA_EP0_TX_ADDR, EP0_MAXPKT);
        ep_set_rx(0, PMA_EP0_RX_ADDR, EP0_MAXPKT);
        USB->EP0R = (1U << 9) | (1U << 8) | (1U << 12); /* RX Valid, TX NAK, Status OUT */

        /* Configure EP1 (bulk OUT) */
        USB->EP1R = (1U << 4);  /* type=bulk */
        ep_set_rx(1, PMA_EP1_RX_ADDR, EP_BULK_MAXPKT);
        USB->EP1R = (1U << 12);  /* RX Valid */

        /* Configure EP2 (bulk IN) */
        USB->EP2R = (1U << 9);  /* type=bulk IN */
        ep_set_tx(2, PMA_EP2_TX_ADDR, EP_BULK_MAXPKT);

        /* Configure EP3 (interrupt IN) */
        USB->EP3R = (1U << 9) | (1U << 10);  /* interrupt IN */
        ep_set_tx(3, PMA_EP3_TX_ADDR, EP_INT_MAXPKT);
    }

    if (istr & USB_ISTR_CTR) {
        uint8_t ep = istr & USB_ISTR_EP_ID_MSK;
        if (ep == 0) {
            if (istr & USB_ISTR_DIR) {
                /* RX setup/data on EP0 */
                handle_ep0_setup();
                USB->EP0R = (USB->EP0R & 0x0F0F) | USB_EP_CTR_RX;
            } else {
                USB->EP0R = (USB->EP0R & 0x0F0F) | USB_EP_CTR_TX;
            }
        } else if (ep == 1) {
            /* Bulk OUT — host data arrived */
            volatile uint16_t *bd = (volatile uint16_t *)(USB_PMA_BASE + (1 * 8));
            uint16_t cnt = bd[3] & 0x3FF;
            uint8_t tmp[64];
            pma_read16(PMA_EP1_RX_ADDR, tmp, cnt);
            for (int i = 0; i < cnt; i++) {
                uint16_t nh = (cdc_rx_head + 1) % CDC_RX_BUF_SIZE;
                if (nh != cdc_rx_tail) {
                    cdc_rx_buf[cdc_rx_head] = tmp[i];
                    cdc_rx_head = nh;
                }
            }
            USB->EP1R = (USB->EP1R & 0x0F0F) | USB_EP_CTR_RX;
        } else if (ep == 2) {
            /* Bulk IN TX complete */
            tx_busy = 0;
            USB->EP2R = (USB->EP2R & 0x0F0F) | USB_EP_CTR_TX;
        }
    }

    if (istr & USB_ISTR_SUSP) {
        USB->ISTR = ~(USB_ISTR_SUSP);
        cdc_dtr = 0;
    }
}

/* ---- Init ---- */
void usb_cdc_init(void) {
    /* Enable USB FS clock */
    RCC->APB1ENR1 |= RCC_APB1ENR1_USBFSEN;
    for (volatile int i = 0; i < 100; i++);

    /* Configure PA11 (DM) / PA12 (DP) as AF10 */
    GPIOA->MODER &= ~(0x3U << (USB_DM_PIN * 2));
    GPIOA->MODER |=  (GPIO_MODE_AF << (USB_DM_PIN * 2));
    GPIOA->AFRH  &= ~(0xFU << ((USB_DM_PIN - 8) * 4));
    GPIOA->AFRH  |=  (0xAU << ((USB_DM_PIN - 8) * 4));

    GPIOA->MODER &= ~(0x3U << (USB_DP_PIN * 2));
    GPIOA->MODER |=  (GPIO_MODE_AF << (USB_DP_PIN * 2));
    GPIOA->AFRH  &= ~(0xFU << ((USB_DP_PIN - 8) * 4));
    GPIOA->AFRH  |=  (0xAU << ((USB_DP_PIN - 8) * 4));

    /* Reset USB peripheral */
    USB->CNTR = USB_CNTR_RESETM | USB_CNTR_CTRM | USB_CNTR_SUSPM;
    USB->BTABLE = BTABLE_ADDR;
    USB->ISTR = 0;
    USB->DADDR = 0;

    nvic_enable(USB_FS_IRQn);

    /* Enable USB */
    USB->CNTR = USB_CNTR_CTRM | USB_CNTR_RESETM | USB_CNTR_SUSPM;
}

int usb_cdc_connected(void) {
    return cdc_dtr && usb_configured;
}

int usb_cdc_write(const char *buf, int len) {
    int queued = 0;
    for (int i = 0; i < len; i++) {
        uint16_t nh = (cdc_tx_head + 1) % CDC_TX_BUF_SIZE;
        if (nh == cdc_tx_tail) break;
        cdc_tx_buf[cdc_tx_head] = buf[i];
        cdc_tx_head = nh;
        queued++;
    }
    return queued;
}

int usb_cdc_puts(const char *s) {
    int n = 0;
    while (*s) { n += usb_cdc_write(s, 1); s++; }
    return n;
}

int usb_cdc_getchar(void) {
    if (cdc_rx_head == cdc_rx_tail) return -1;
    char c = cdc_rx_buf[cdc_rx_tail];
    cdc_rx_tail = (cdc_rx_tail + 1) % CDC_RX_BUF_SIZE;
    return (unsigned char)c;
}

void usb_cdc_poll(void) {
    if (!usb_configured) return;
    if (tx_busy) return;
    if (cdc_tx_head == cdc_tx_tail) return;

    /* Pump up to 64 bytes into EP2 PMA */
    uint8_t tmp[64];
    int n = 0;
    while (n < 64 && cdc_tx_head != cdc_tx_tail) {
        tmp[n++] = cdc_tx_buf[cdc_tx_tail];
        cdc_tx_tail = (cdc_tx_tail + 1) % CDC_TX_BUF_SIZE;
    }
    if (n > 0) {
        pma_write16(PMA_EP2_TX_ADDR, tmp, n);
        volatile uint16_t *bd = (volatile uint16_t *)(USB_PMA_BASE + (2 * 8));
        bd[1] = n;
        tx_busy = 1;
        /* Toggle TX status to VALID */
        USB->EP2R = (USB->EP2R & 0x0F0F) | (1U << 4) | (1U << 7); /* DTOG_TX, STAT_TX=0 */
    }
}

/* end of file — author: jayis1 */