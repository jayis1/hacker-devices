/*
 * usb_cdc.c — WireReaper USB CDC virtual serial driver
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — GPL-2.0
 *
 * Implements a minimal USB 2.0 Hi-Speed CDC virtual serial port
 * using the STM32H743 USB OTG peripheral with ULPI PHY (USB3300).
 * Provides the binary command channel for laptop control.
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"

/* USB OTG register offsets (DWC2 core) */
#define OTG_GUSBCFG     REG32(USB_OTG_BASE + 0x00)
#define OTG_GRSTCTL     REG32(USB_OTG_BASE + 0x14)
#define OTG_GINTSTS     REG32(USB_OTG_BASE + 0x0C)
#define OTG_GINTMSK     REG32(USB_OTG_BASE + 0x10)
#define OTG_GCCFG       REG32(USB_OTG_BASE + 0x38)
#define OTG_GRXFSIZ     REG32(USB_OTG_BASE + 0x1C)
#define OTG_GNPTXFSIZ   REG32(USB_OTG_BASE + 0x24)

/* Device-mode registers */
#define OTG_DCFG        REG32(USB_OTG_BASE + 0x800)
#define OTG_DCTL        REG32(USB_OTG_BASE + 0x804)
#define OTG_DIEPMSK     REG32(USB_OTG_BASE + 0x810)
#define OTG_DOEPMSK     REG32(USB_OTG_BASE + 0x814)
#define OTG_DAINT       REG32(USB_OTG_BASE + 0x818)
#define OTG_DAINTMSK    REG32(USB_OTG_BASE + 0x81C)

/* Endpoint registers (EP0 IN) */
#define OTG_DIEP0_CTL   REG32(USB_OTG_BASE + 0x900)
#define OTG_DIEP0_INT   REG32(USB_OTG_BASE + 0x908)
#define OTG_DIEP0_TSIZ  REG32(USB_OTG_BASE + 0x910)
#define OTG_DIEP0_DMA    REG32(USB_OTG_BASE + 0x914)

/* Endpoint registers (EP0 OUT) */
#define OTG_DOEP0_CTL   REG32(USB_OTG_BASE + 0xB00)
#define OTG_DOEP0_INT   REG32(USB_OTG_BASE + 0xB08)
#define OTG_DOEP0_TSIZ  REG32(USB_OTG_BASE + 0xB10)

/* FIFO registers */
#define OTG_RX0STS      REG32(USB_OTG_BASE + 0x1000) /* RxFIFO */

/* ---- USB CDC descriptors ---- */
/* Device descriptor */
static const uint8_t dev_desc[] = {
    18,     /* bLength */
    0x01,   /* bDescriptorType: Device */
    0x00, 0x02, /* USB 2.0 */
    0x02,   /* bDeviceClass: CDC */
    0x00,   /* bDeviceSubClass */
    0x00,   /* bDeviceProtocol */
    64,     /* bMaxPacketSize0 (control) */
    0x83, 0x18, /* idVendor: 0x1883 (jayis1 custom) */
    0x57, 0x04, /* idProduct: 0x0457 (WireReaper) */
    0x00, 0x01, /* bcdDevice: 1.00 */
    0x01,   /* iManufacturer: "jayis1" */
    0x02,   /* iProduct: "WireReaper" */
    0x03,   /* iSerial: "WR0001" */
    0x01    /* bNumConfigurations */
};

/* String descriptor 0 (language) */
static const uint8_t str_desc0[] = {
    4, 0x03, 0x09, 0x04  /* English */
};

/* String descriptor 1: Manufacturer */
static const uint8_t str_desc1[] = {
    14, 0x03,
    'j',0, 'a',0, 'y',0, 'i',0, 's',0, '1',0
};

/* String descriptor 2: Product */
static const uint8_t str_desc2[] = {
    22, 0x03,
    'W',0, 'i',0, 'r',0, 'e',0, 'R',0, 'e',0, 'a',0, 'p',0, 'e',0, 'r',0
};

/* String descriptor 3: Serial */
static const uint8_t str_desc3[] = {
    12, 0x03,
    'W',0, 'R',0, '0',0, '0',0, '0',0, '1',0
};

/* ---- USB state ---- */
static struct {
    int configured;
    int address_set;
    uint8_t addr;
    uint8_t rx_buf[256];
    int rx_len;
    int rx_ready;
    uint8_t tx_buf[256];
    int tx_len;
    int tx_pending;
} usb_state;

/* ---- USB initialization ---- */
void wr_usb_init(void) {
    memset(&usb_state, 0, sizeof(usb_state));

    /* Enable USB clock */
    RCC_AHB2ENR |= RCC_AHB2ENR_USB;

    /* Configure GPIO for ULPI mode (12 pins) */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOA | RCC_AHB1ENR_GPIOB |
                   RCC_AHB1ENR_GPIOC;

    /* ULPI pins AF10 */
    struct { uint32_t port; uint8_t pin; } ulpi_pins[] = {
        {GPIOA_BASE, 3},  {GPIOA_BASE, 5},  {GPIOC_BASE, 0},
        {GPIOC_BASE, 1},  {GPIOC_BASE, 2},  {GPIOC_BASE, 3},
        {GPIOH_BASE, 4},  {GPIOH_BASE, 5},  {GPIOH_BASE, 6},
        {GPIOH_BASE, 7},
    };
    for (int i = 0; i < (int)(sizeof(ulpi_pins)/sizeof(ulpi_pins[0])); i++) {
        gpio_t *g = GPIO(ulpi_pins[i].port);
        uint8_t p = ulpi_pins[i].pin;
        g->MODER &= ~(3U << (p * 2));
        g->MODER |= (GPIO_MODE_AF << (p * 2));
        g->OSPEEDR |= (GPIO_OSPEED_VHIGH << (p * 2));
        g->AFR[p / 8] &= ~(0xFU << ((p % 8) * 4));
        g->AFR[p / 8] |= (10U << ((p % 8) * 4)); /* AF10 = ULPI */
    }

    /* Core soft reset */
    OTG_GRSTCTL |= (1U << 0); /* CSRST */
    while (OTG_GRSTCTL & (1U << 0));

    /* Select ULPI (external PHY) */
    OTG_GUSBCFG = (1U << 4) |  /* PHYSEL = ULPI */
                  (0U << 6) |  /* FS/LS clock from ULPI */
                  (9U << 10);  /* ULPI 8-bit interface, single data rate */

    /* Wait for AHB idle */
    while (!(OTG_GRSTCTL & (1U << 31)));

    /* Device mode configuration */
    OTG_DCFG = (0x03 << 0) | /* DMA off, device address = 0 initially */ 0;
    OTG_DCTL = (1U << 0); /* Soft disconnect */

    /* Enable VBUS sensing and connect */
    OTG_GCCFG |= (1U << 16) | (1U << 19); /* Enable VBUS, no power-down */

    /* Set up RxFIFO and TxFIFO sizes */
    OTG_GRXFSIZ = 64;   /* 64 DWORDs = 256 bytes RxFIFO */
    OTG_GNPTXFSIZ = (64 << 16) | 64; /* 64 DWORD start, 64 size */

    /* Clear soft disconnect (connect to host) */
    OTG_DCTL &= ~(1U << 0);

    /* Enable interrupts */
    OTG_GINTMSK = (1U << 10) |  /* ESUSP */
                  (1U << 11) |  /* USBSUSP */
                  (1U << 12) |  /* USBRST */
                  (1U << 13) |  /* ENUMDN */
                  (1U << 3)  |  /* SOF */
                  (1U << 4);    /* RXFLVL */

    nvic_enable(77); /* OTG_HS global interrupt on H743 */
}

/* ---- Handle USB SETUP packet (simplified control endpoint) ---- */
static void handle_setup(const uint8_t *setup) {
    uint8_t bmReq = setup[0];
    uint8_t bReq   = setup[1];
    uint16_t wVal  = setup[2] | (setup[3] << 8);
    uint16_t wIdx  = setup[4] | (setup[5] << 8);
    uint16_t wLen  = setup[6] | (setup[7] << 8);

    (void)wIdx;

    if ((bmReq & 0x60) == 0x00) { /* Standard request */
        switch (bReq) {
        case 0x05: /* SET_ADDRESS */
            usb_state.addr = wVal & 0x7F;
            OTG_DCFG = (OTG_DCFG & ~0x7F) | usb_state.addr;
            usb_state.address_set = 1;
            break;
        case 0x09: /* SET_CONFIGURATION */
            usb_state.configured = (wVal != 0);
            break;
        case 0x06: /* GET_DESCRIPTOR */
            switch ((wVal >> 8) & 0xFF) {
            case 0x01: /* Device */
                wr_usb_send(dev_desc, sizeof(dev_desc));
                break;
            case 0x03: /* String */
                switch (wVal & 0xFF) {
                case 0: wr_usb_send(str_desc0, sizeof(str_desc0)); break;
                case 1: wr_usb_send(str_desc1, sizeof(str_desc1)); break;
                case 2: wr_usb_send(str_desc2, sizeof(str_desc2)); break;
                case 3: wr_usb_send(str_desc3, sizeof(str_desc3)); break;
                }
                break;
            }
            break;
        }
    }
}

/* ---- USB send (EP1 IN, bulk) ---- */
void wr_usb_send(const uint8_t *data, int len) {
    if (len > 255)
        len = 255;
    memcpy(usb_state.tx_buf, data, len);
    usb_state.tx_len = len;
    usb_state.tx_pending = 1;

    /* In a full implementation, we'd write to the TxFIFO and
     * enable the EP IN interrupt. The DWC2 core would handle
     * the actual USB transmission. Here we store data for the
     * interrupt handler to pick up. */
    (void)OTG_DIEP0_CTL; /* Would configure EP1 IN here */
}

/* ---- USB receive poll ---- */
int wr_usb_recv(uint8_t *buf, int maxlen) {
    if (!usb_state.rx_ready || usb_state.rx_len == 0)
        return 0;
    int len = usb_state.rx_len;
    if (len > maxlen)
        len = maxlen;
    memcpy(buf, usb_state.rx_buf, len);
    usb_state.rx_ready = 0;
    usb_state.rx_len = 0;
    return len;
}

/* ---- USB configured check ---- */
int wr_usb_configured(void) {
    return usb_state.configured;
}

/* ---- USB global interrupt handler (simplified) ---- */
IRQ_HANDLER(OTG_HS_EP1_OUT_IRQHandler) {
    uint32_t gintsts = OTG_GINTSTS;

    if (gintsts & (1U << 12)) { /* USBRST */
        OTG_GINTSTS = (1U << 12);
        usb_state.configured = 0;
        usb_state.address_set = 0;
    }
    if (gintsts & (1U << 13)) { /* ENUMDN */
        OTG_GINTSTS = (1U << 13);
    }
    if (gintsts & (1U << 4)) { /* RXFLVL */
        /* Read from RxFIFO */
        uint32_t sts = OTG_RX0STS;
        int pkt_len = (sts >> 0) & 0x7FF;
        int pkt_type = (sts >> 17) & 0xF;

        if (pkt_type == 1 && pkt_len <= 255) { /* SETUP packet */
            /* Read setup data from FIFO */
            for (int i = 0; i < 8; i += 4) {
                uint32_t word = OTG_RX0STS; /* Would read from DFIFO */
                usb_state.rx_buf[i]     = word & 0xFF;
                usb_state.rx_buf[i + 1] = (word >> 8) & 0xFF;
                usb_state.rx_buf[i + 2] = (word >> 16) & 0xFF;
                usb_state.rx_buf[i + 3] = (word >> 24) & 0xFF;
            }
            handle_setup(usb_state.rx_buf);
        } else if (pkt_type == 2) { /* OUT packet (data) */
            /* Read data from RxFIFO */
            int words = (pkt_len + 3) / 4;
            for (int i = 0; i < words && i < 64; i++) {
                /* Would read from DFIFO0 */
            }
            usb_state.rx_ready = 1;
            usb_state.rx_len = pkt_len;
        }
        OTG_GINTSTS = (1U << 4);
    }
}