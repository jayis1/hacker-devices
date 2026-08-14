/*
 * usb_cdc.c — STM32G474 USB FS CDC ACM (12 Mbps) minimal driver
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Implements just enough of the USB FS peripheral + CDC ACM to get a
 * /dev/ttyACM0 endpoint on the host for the CLI and capture stream.
 * Uses the standard STM32 USB block registers directly (no HAL).
 *
 * NOTE: For brevity the actual endpoint table & descriptor packet
 * assembly is presented as a compact but real state machine. The
 * buffer-table (BTABLE) layout in USB SRAM at offset 0x4000_6000 is:
 *   EP0 OUT  @ 0x00 (8-byte setup + 64-byte out)
 *   EP0 IN   @ 0x40 (64-byte in)
 *   EP3 IN    @ 0x80 (bulk, 64-byte)
 *   EP3 OUT  @ 0xC0 (bulk, 64-byte)
 */

#include "../board.h"
#include "../registers.h"
#include "usb_cdc.h"
#include <string.h>

/* ---- USB FS packet memory area (PMA) ---- */
#define PMA_BASE   0x40006000u
#define PMA(off)   (*(volatile uint16_t *)(PMA_BASE + (off)*2))   /* 16-bit cells */

/* BTABLE offsets (in 16-bit PMA words) */
#define BTABLE_ADDR 0x00
#define EP0_TX_ADDR 0x40
#define EP0_RX_ADDR 0x08
#define EP3_TX_ADDR 0x80
#define EP3_RX_ADDR 0xC0

/* ---- RX ring (host -> device) ---- */
#define RX_RING_SZ 256
static char rx_ring[RX_RING_SZ];
static volatile uint16_t rx_head = 0, rx_tail = 0;

/* ---- TX staging (device -> host) ---- */
static char tx_buf[64];
static volatile uint8_t tx_len = 0;
static volatile uint8_t tx_busy = 0;

/* ---- Device address pending ---- */
static volatile uint8_t set_addr_pending = 0;
static volatile uint8_t dev_addr = 0;

/* ---- Configured ---- */
static volatile uint8_t configured = 0;

/* ---- CDC line coding (115200 8N1 by default; we ignore on device) ---- */
static uint8_t line_coding[7] = {0x00,0xC2,0x01,0x00, 0x00,0x00,0x08};

/* ---- Standard USB descriptors (compact, single config) ---- */
static const uint8_t dev_desc[] = {
    0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,  /* 18b, USB2, 64-byte pkt */
    0xC0, 0x16, 0x55, 0x15, 0x00, 0x01, 0x01, 0x02,  /* VID/PID/rel */
    0x03                                                  /* idx str */
};
static const uint8_t cfg_desc[] = {
    /* Config descriptor (9 bytes) */
    0x09, 0x02, 0x43, 0x00, 0x02, 0x01, 0x00, 0x80, 0x32,
    /* CDC IAD (8) */
    0x08, 0x0B, 0x00, 0x02, 0x02, 0x01, 0x00, 0x00,
    /* CDC interface (9) */
    0x09, 0x04, 0x00, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,
    /* CDC header functional (5) */
    0x05, 0x24, 0x00, 0x10, 0x01,
    /* CDC call mgmt (5) */
    0x05, 0x24, 0x01, 0x00, 0x01,
    /* CDC ACM functional (4) */
    0x04, 0x24, 0x02, 0x02,
    /* CDC union functional (5) */
    0x05, 0x24, 0x06, 0x00, 0x01,
    /* CDC bulk IN endpoint (7) */
    0x07, 0x05, 0x82, 0x02, 0x40, 0x00, 0x00,
    /* Data interface (9) */
    0x09, 0x04, 0x01, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,
    /* Bulk OUT endpoint (7) */
    0x07, 0x05, 0x03, 0x02, 0x40, 0x00, 0x00,
    /* Bulk IN endpoint (7) */
    0x07, 0x05, 0x83, 0x02, 0x40, 0x00, 0x00,
};
static const uint8_t str0[] = {0x04, 0x03, 0x09, 0x04};
static const char *str_manu = "jayis1";
static const char *str_prod = "1553-Phantom";

/* ---- PMA helpers ---- */
static void pma_write(uint16_t addr, const uint8_t *src, int n) {
    for (int i = 0; i < n; i += 2) {
        uint16_t v = src[i];
        if (i + 1 < n) v |= ((uint16_t)src[i+1] << 8);
        PMA(addr + i/2) = v;
    }
}
static void pma_read(uint16_t addr, uint8_t *dst, int n) {
    for (int i = 0; i < n; i += 2) {
        uint16_t v = PMA(addr + i/2);
        dst[i] = v & 0xFF;
        if (i + 1 < n) dst[i+1] = (v >> 8) & 0xFF;
    }
}

/* ---- Endpoint register helpers ---- */
#define EP_TYPE_CTRL  0x3210
#define EP_TYPE_BULK  0x0800

static void ep_set(int ep, uint16_t type, uint16_t addr, uint16_t sz) {
    /* BTABLE entry: TX_ADDR, TX_CNT, RX_ADDR, RX_CNT (4 × 16-bit) */
    PMA(ep * 8 + 0) = addr;          /* TX_ADDR */
    PMA(ep * 8 + 1) = 0;            /* TX_CNT */
    PMA(ep * 8 + 2) = addr + 64;     /* RX_ADDR */
    PMA(ep * 8 + 3) = ((sz == 64) ? 0x8000 : 0x4000) | sz;  /* RX_CNT/blsiz */
    REG32(USB_BASE + ep * 4) = type | ep;   /* EPnR */
}

void usb_cdc_init(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_USB;

    USB->CNTR = 0;            /* clear */
    for (volatile int i = 0; i < 1000; i++) __asm volatile("nop");
    USB->CNTR |= (1u<<8);    /* PDWN reset */
    for (volatile int i = 0; i < 1000; i++) __asm volatile("nop");
    USB->CNTR &= ~(1u<<8);
    USB->BTABLE = 0;
    USB->DADDR = 0;          /* not addressed yet */

    /* Configure EP0 control */
    ep_set(0, EP_TYPE_CTRL, EP0_TX_ADDR, 64);
    /* EP3 bulk for CDC data */
    ep_set(3, EP_TYPE_BULK, EP3_TX_ADDR, 64);

    /* Enable interrupts */
    USB->CNTR = (1u<<0)    /* CTR */
              | (1u<<1)    /* PMAOUT */
              | (1u<<2)    /* ERR */
              | (1u<<10)  /* RESET */
              | (1u<<11); /* SOF */
}

/* ---- Send on EP0 (control) ---- */
static void ep0_send(const uint8_t *data, int n) {
    if (n > 64) n = 64;
    pma_write(EP0_TX_ADDR, data, n);
    PMA(0*8 + 1) = (uint16_t)n;
    REG32(USB_BASE + 0*4) = EP_TYPE_CTRL | (1u<<4) | (1u<<5);  /* TX | CTR */
}

/* ---- Send on EP3 (bulk IN to host) ---- */
static void ep3_send(const uint8_t *data, int n) {
    if (n > 64) n = 64;
    pma_write(EP3_TX_ADDR, data, n);
    PMA(3*8 + 1) = (uint16_t)n;
    REG32(USB_BASE + 3*4) = EP_TYPE_BULK | (1u<<4) | (1u<<5);
    tx_busy = 1;
}

/* ---- USB ISR (vector points here) ---- */
void USB_HP_IRQHandler(void)  { usb_cdc_isr(); }
void USB_LP_IRQHandler(void)  { usb_cdc_isr(); }

void usb_cdc_isr(void) {
    uint16_t istr = USB->ISTR;
    USB->ISTR = 0;     /* clear all flags */

    if (istr & (1u<<10)) {    /* RESET */
        USB->DADDR = 0;
        ep_set(0, EP_TYPE_CTRL, EP0_TX_ADDR, 64);
        ep_set(3, EP_TYPE_BULK, EP3_TX_ADDR, 64);
        configured = 0;
    }

    if (istr & (1u<<0)) {    /* CTR — endpoint transfer complete */
        uint16_t ep0r = REG32(USB_BASE + 0*4);
        if (ep0r & (1u<<7)) {     /* setup/out */
            uint8_t setup[8];
            pma_read(EP0_RX_ADDR, setup, 8);
            /* Standard requests */
            uint8_t req = setup[0];
            uint8_t typ = setup[1];
            if (req == 0x05) {   /* SET_ADDRESS */
                dev_addr = setup[2];
                set_addr_pending = 1;
                ep0_send(0, 0);
            } else if (req == 0x06 && typ == 0x80) {  /* GET_DESCRIPTOR */
                uint8_t dt = setup[3];
                if (dt == 0x01)      ep0_send(dev_desc, sizeof(dev_desc));
                else if (dt == 0x02) ep0_send(cfg_desc, sizeof(cfg_desc));
                else if (dt == 0x03) ep0_send(str0, sizeof(str0));
                else                 ep0_send(0, 0);
            } else if (req == 0x09) {  /* SET_CONFIGURATION */
                configured = 1;
                ep0_send(0, 0);
            } else if (req == 0x21) {  /* CDC SET_LINE_CODING */
                /* data phase; we just accept */
                ep0_send(0, 0);
            } else {
                ep0_send(0, 0);
            }
            REG32(USB_BASE + 0*4) = EP_TYPE_CTRL | (1u<<4);   /* clear RX */
        }
        if (ep0r & (1u<<4)) {     /* in (TX done) */
            if (set_addr_pending) {
                USB->DADDR = (1u<<7) | dev_addr;   /* enable + addr */
                set_addr_pending = 0;
            }
            REG32(USB_BASE + 0*4) = EP_TYPE_CTRL | (1u<<7);
        }

        /* EP3 bulk */
        uint16_t ep3r = REG32(USB_BASE + 3*4);
        if (ep3r & (1u<<7)) {     /* OUT (host -> device) */
            int n = PMA(3*8 + 3) & 0x3FF;
            uint8_t buf[64];
            pma_read(EP3_RX_ADDR, buf, n);
            for (int i = 0; i < n; i++) {
                rx_ring[rx_head] = (char)buf[i];
                rx_head = (rx_head + 1) % RX_RING_SZ;
            }
            REG32(USB_BASE + 3*4) = EP_TYPE_BULK | (1u<<7);
        }
        if (ep3r & (1u<<4)) {     /* IN (TX done) */
            tx_busy = 0;
        }
    }
}

/* ---- Public API ---- */
int usb_cdc_getc(void) {
    if (rx_tail == rx_head) return -1;
    char c = rx_ring[rx_tail];
    rx_tail = (rx_tail + 1) % RX_RING_SZ;
    return c;
}

void usb_cdc_putc(char c) {
    tx_buf[tx_len++] = c;
    if (tx_len == 64) usb_cdc_flush();
}

void usb_cdc_puts(const char *s) {
    while (*s) usb_cdc_putc(*s++);
}

void usb_cdc_flush(void) {
    if (tx_len == 0) return;
    while (tx_busy) { }    /* wait for previous */
    ep3_send((const uint8_t *)tx_buf, tx_len);
    tx_len = 0;
}

/* ---- Small helpers used by other modules (declared here so all share) ---- */
int atoi_lite(const char *s) {
    int v = 0, sign = 1;
    if (*s == '-') { sign = -1; s++; }
    while (*s >= '0' && *s <= '9') { v = v*10 + (*s - '0'); s++; }
    return v * sign;
}

uint32_t hex32(const char *s) {
    uint32_t v = 0;
    while (*s) {
        char c = *s++;
        if (c >= '0' && c <= '9')      v = (v << 4) | (c - '0');
        else if (c >= 'a' && c <= 'f') v = (v << 4) | (c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') v = (v << 4) | (c - 'A' + 10);
        else break;
    }
    return v;
}

/* ---- va_arg-ish helpers for snprintf_lite (freestanding) ---- */
typedef __builtin_va_list va_list_t;
#define va_start(v,l) __builtin_va_start(v,l)
#define va_end(v)     __builtin_va_end(v)
#define va_arg(v,t)   __builtin_va_arg(v,t)

int itoa_dec(int v, char *buf, int sz) {
    char tmp[12];
    int n = 0, neg = 0;
    if (v < 0) { neg = 1; v = -v; }
    if (v == 0) tmp[n++] = '0';
    while (v > 0 && n < 12) { tmp[n++] = '0' + (v % 10); v /= 10; }
    int out = 0;
    if (neg && out < sz) buf[out++] = '-';
    while (n > 0 && out < sz) buf[out++] = tmp[--n];
    return out;
}

int utoa_dec(unsigned v, char *buf, int sz) {
    char tmp[12];
    int n = 0;
    if (v == 0) tmp[n++] = '0';
    while (v > 0 && n < 12) { tmp[n++] = '0' + (v % 10); v /= 10; }
    int out = 0;
    while (n > 0 && out < sz) buf[out++] = tmp[--n];
    return out;
}

/* ---- snprintf_lite: freestanding minimal %s/%d/%u formatter ---- */
int snprintf_lite(char *buf, int sz, const char *fmt, ...) {
    va_list_t ap;
    va_start(ap, fmt);
    int n = 0;
    while (*fmt && n < sz - 1) {
        if (*fmt == '%' && fmt[1]) {
            fmt++;
            if (*fmt == 's') {
                const char *s = va_arg(ap, const char*);
                while (*s && n < sz - 1) buf[n++] = *s++;
            } else if (*fmt == 'd') {
                int v = va_arg(ap, int);
                n += itoa_dec(v, buf + n, sz - n);
            } else if (*fmt == 'u') {
                unsigned v = va_arg(ap, unsigned);
                n += utoa_dec(v, buf + n, sz - n);
            } else { buf[n++] = '%'; if (n < sz - 1) buf[n++] = *fmt; }
            fmt++;
        } else {
            buf[n++] = *fmt++;
        }
    }
    buf[n] = 0;
    va_end(ap);
    return n;
}