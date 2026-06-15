/*
 * usb_cdc.c — USB CDC ACM device driver for STM32F401 (OTG FS)
 *
 * Minimal implementation of USB CDC ACM virtual serial port
 * using the STM32F401's OTG Full-Speed peripheral.
 */

#include "usb_cdc.h"
#include "registers.h"
#include "board.h"
#include "usb_descriptors.h"
#include <string.h>

/* ============================================================
 * Internal state
 * ============================================================ */
static usb_rx_callback_t rx_callback = NULL;
static volatile int usb_configured = 0;
static volatile int usb_suspended = 0;
static uint8_t rx_buf[USB_CDC_BUF_SIZE];
static volatile uint16_t rx_len = 0;

/* ============================================================
 * USB device state machine
 * ============================================================ */
typedef enum {
    USB_STATE_DEFAULT,
    USB_STATE_ADDRESSED,
    USB_STATE_CONFIGURED
} usb_state_t;

static volatile usb_state_t usb_state = USB_STATE_DEFAULT;
static volatile uint8_t usb_config_value = 0;
static volatile uint16_t usb_device_status = 0x0001; /* Self-powered */

/* ============================================================
 * Control transfer state
 * ============================================================ */
static volatile uint8_t ctrl_buf[128];
static volatile uint16_t ctrl_len = 0;
static volatile uint16_t ctrl_remaining = 0;

/* ============================================================
 * Helper: Stall endpoint
 * ============================================================ */
static void usb_stall_ep0(void) {
    OTG_FS_DEVICE->DIEPCTL0 |= (1U << 21); /* Stall */
}

/* ============================================================
 * Handle standard USB requests on EP0
 * ============================================================ */
static void usb_handle_setup(void) {
    /* Read setup packet from RX FIFO */
    volatile uint32_t *rx_fifo = (volatile uint32_t *)0x50002000U;
    uint32_t pkt0 = *rx_fifo;  /* First word: bmRequestType, bRequest, wValue, wIndex */
    uint32_t pkt1 = *rx_fifo;  /* Second word: wLength */

    uint8_t bmRequestType = pkt0 & 0xFF;
    uint8_t bRequest = (pkt0 >> 8) & 0xFF;
    uint16_t wValue = (pkt0 >> 16) & 0xFFFF;
    uint16_t wIndex = pkt1 & 0xFFFF;
    uint16_t wLength = (pkt1 >> 16) & 0xFFFF;

    (void)wIndex;

    /* Standard requests */
    if ((bmRequestType & 0x60) == 0x00) { /* Standard */
        switch (bRequest) {
            case 0x00: /* GET_STATUS */
                ctrl_buf[0] = usb_device_status & 0xFF;
                ctrl_buf[1] = (usb_device_status >> 8) & 0xFF;
                ctrl_len = 2;
                break;

            case 0x05: /* SET_ADDRESS */
                usb_state = USB_STATE_ADDRESSED;
                OTG_FS_DEVICE->DCFG = (OTG_FS_DEVICE->DCFG & ~0x3FU) | (wValue & 0x3FU);
                /* Send zero-length status */
                return;

            case 0x06: /* GET_DESCRIPTOR */
                switch ((wValue >> 8) & 0xFF) {
                    case 0x01: /* Device */
                        memcpy((void *)ctrl_buf, device_descriptor, DEVICE_DESCRIPTOR_SIZE);
                        ctrl_len = (wLength < DEVICE_DESCRIPTOR_SIZE) ? wLength : DEVICE_DESCRIPTOR_SIZE;
                        break;
                    case 0x02: /* Configuration */
                        memcpy((void *)ctrl_buf, config_descriptor, CONFIG_DESCRIPTOR_SIZE);
                        ctrl_len = (wLength < CONFIG_DESCRIPTOR_SIZE) ? wLength : CONFIG_DESCRIPTOR_SIZE;
                        break;
                    case 0x03: /* String */
                        switch (wValue & 0xFF) {
                            case 0: memcpy((void *)ctrl_buf, string_lang, STRING_LANG_SIZE); ctrl_len = STRING_LANG_SIZE; break;
                            case 1: memcpy((void *)ctrl_buf, string_manufacturer, STRING_MANUFACTURER_SIZE); ctrl_len = STRING_MANUFACTURER_SIZE; break;
                            case 2: memcpy((void *)ctrl_buf, string_product, STRING_PRODUCT_SIZE); ctrl_len = STRING_PRODUCT_SIZE; break;
                            case 3: memcpy((void *)ctrl_buf, string_serial, STRING_SERIAL_SIZE); ctrl_len = STRING_SERIAL_SIZE; break;
                            default: usb_stall_ep0(); return;
                        }
                        break;
                    default:
                        usb_stall_ep0();
                        return;
                }
                break;

            case 0x09: /* SET_CONFIGURATION */
                usb_config_value = wValue;
                if (wValue == 1) {
                    usb_state = USB_STATE_CONFIGURED;
                    usb_configured = 1;
                    /* Enable EP1 OUT (data), EP2 IN (notify), EP3 IN (data) */
                    OTG_FS_DEVICE->DOEPCTL1 = (1U << 15);  /* EPENA */
                    OTG_FS_DEVICE->DIEPCTL2 = (1U << 15) | (2U << 18) | (1U << 27); /* EP2 IN, INT, CNAK */
                    OTG_FS_DEVICE->DIEPCTL3 = (1U << 15) | (2U << 18); /* EP3 IN, BULK */
                } else {
                    usb_state = USB_STATE_ADDRESSED;
                    usb_configured = 0;
                }
                ctrl_len = 0;
                break;

            default:
                usb_stall_ep0();
                return;
        }
    }
    /* CDC class requests */
    else if ((bmRequestType & 0x60) == 0x20) { /* Class */
        switch (bRequest) {
            case 0x20: /* SET_LINE_CODING */
                ctrl_len = 0; /* Accept, no action needed */
                break;
            case 0x21: /* GET_LINE_CODING */
                ctrl_buf[0] = (CDC_LINE_CODING_RATE >> 0) & 0xFF;
                ctrl_buf[1] = (CDC_LINE_CODING_RATE >> 8) & 0xFF;
                ctrl_buf[2] = (CDC_LINE_CODING_RATE >> 16) & 0xFF;
                ctrl_buf[3] = (CDC_LINE_CODING_RATE >> 24) & 0xFF;
                ctrl_buf[4] = CDC_LINE_CODING_STOP;
                ctrl_buf[5] = CDC_LINE_CODING_PARITY;
                ctrl_buf[6] = CDC_LINE_CODING_DATA;
                ctrl_len = 7;
                break;
            case 0x22: /* SET_CONTROL_LINE_STATE */
                ctrl_len = 0;
                break;
            default:
                usb_stall_ep0();
                return;
        }
    } else {
        usb_stall_ep0();
        return;
    }

    /* Send response on EP0 IN */
    if (ctrl_len > 0) {
        volatile uint32_t *ep0_fifo = (volatile uint32_t *)0x50002000U; /* EP0 FIFO */
        uint16_t send_len = (ctrl_len > 64) ? 64 : ctrl_len;
        for (uint16_t i = 0; i < send_len; i += 4) {
            uint32_t word = 0;
            for (int b = 0; (b < 4) && ((i + b) < send_len); b++) {
                word |= (ctrl_buf[i + b] << (b * 8));
            }
            *ep0_fifo = word;
        }
        OTG_FS_DEVICE->DIEPTSIZ0 = (1U << 19) | send_len; /* 1 packet */
        OTG_FS_DEVICE->DIEPCTL0 |= (1U << 15); /* EPENA */
    }
}

/* ============================================================
 * Initialize USB CDC device
 * ============================================================ */
int usb_cdc_init(void) {
    /* Enable OTG FS clock */
    RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;

    /* Delay after clock enable */
    for (volatile int i = 0; i < 1000; i++);

    /* Reset OTG FS */
    OTG_FS_GLOBAL->GRSTCTL = OTG_GRSTCTL_CSRST;
    while (OTG_FS_GLOBAL->GRSTCTL & OTG_GRSTCTL_CSRST);

    /* Configure for device mode, Full Speed */
    OTG_FS_GLOBAL->GUSBCFG = OTG_GUSBCFG_FDMOD
                            | OTG_GUSBCFG_TRDT(5)
                            | OTG_GUSBCFG_PHYSEL;

    /* Device configuration: Full Speed, non-zero-length status */
    OTG_FS_DEVICE->DCFG = OTG_DCFG_DSPD_FULL | OTG_DCFG_NZLSOHSK;

    /* Set RX FIFO size: 128 words (512 bytes) */
    OTG_FS_GLOBAL->GRXFSIZ = 128;

    /* Set TX FIFO sizes (in 32-bit words) */
    OTG_FS_GLOBAL->DIEPTXF0 = (16U << 16) | 128U;     /* EP0: 16 words, start at 128 */
    /* EP1 TX not used (OUT only) */
    OTG_FS_GLOBAL->DIEPTXF1 = (16U << 16) | (128U + 16U); /* EP2 IN: 16 words (notify) */
    /* Note: DIEPTXF2 and DIEPTXF3 share register space — using shared EP3 for data IN */

    /* Unmask device interrupts */
    OTG_FS_GLOBAL->GINTMSK = OTG_GINTMSK_USBRST
                            | OTG_GINTMSK_ENUMDNE
                            | OTG_GINTMSK_RXFLVL
                            | OTG_GINTMSK_IEPINT;

    /* Enable global interrupt */
    OTG_FS_GLOBAL->GAHBCFG = OTG_GAHBCFG_GINT;

    /* Soft disconnect disabled = device connects */
    OTG_FS_DEVICE->DCTL &= ~OTG_DCTL_SDIS;

    /* Enable USB IRQ in NVIC */
    NVIC->ISER[OTG_FS_IRQn >> 5] |= (1U << (OTG_FS_IRQn & 0x1F));

    return 0;
}

/* ============================================================
 * Send data over CDC
 * ============================================================ */
int usb_cdc_send(const uint8_t *data, uint16_t len) {
    if (!usb_configured) return -1;
    if (len > USB_CDC_EP_SIZE) len = USB_CDC_EP_SIZE;

    /* Write data to EP3 TX FIFO */
    volatile uint32_t *ep3_fifo = (volatile uint32_t *)0x50002000U + (3U * 0x400U) / 4U;

    /* Wait for FIFO space */
    while (!(OTG_FS_DEVICE->DIEPINT3 & (1U << 7))); /* TXFE */

    uint16_t i;
    for (i = 0; i + 3 < len; i += 4) {
        uint32_t word = data[i] | (data[i + 1] << 8)
                      | (data[i + 2] << 16) | (data[i + 3] << 24);
        *ep3_fifo = word;
    }
    /* Handle remaining bytes */
    if (i < len) {
        uint32_t word = 0;
        for (uint16_t j = 0; i + j < len; j++) {
            word |= ((uint32_t)data[i + j] << (j * 8));
        }
        *ep3_fifo = word;
    }

    /* Set transfer size and enable endpoint */
    OTG_FS_DEVICE->DIEPTSIZ3 = (1U << 19) | len; /* 1 packet, len bytes */
    OTG_FS_DEVICE->DIEPCTL3 |= (1U << 15); /* EPENA */

    return len;
}

/* ============================================================
 * Register RX callback
 * ============================================================ */
void usb_cdc_set_rx_callback(usb_rx_callback_t cb) {
    rx_callback = cb;
}

/* ============================================================
 * Check if USB is configured
 * ============================================================ */
int usb_cdc_is_configured(void) {
    return usb_configured;
}

/* ============================================================
 * USB ISR handler
 * ============================================================ */
void usb_cdc_isr(void) {
    uint32_t gintsts = OTG_FS_GLOBAL->GINTSTS;

    /* USB Reset */
    if (gintsts & OTG_GINTSTS_USBRST) {
        OTG_FS_GLOBAL->GINTSTS = OTG_GINTSTS_USBRST;
        usb_configured = 0;
        usb_state = USB_STATE_DEFAULT;
        rx_len = 0;
        ctrl_len = 0;
    }

    /* Enumeration Done */
    if (gintsts & OTG_GINTSTS_ENUMDNE) {
        OTG_FS_GLOBAL->GINTSTS = OTG_GINTSTS_ENUMDNE;
        /* Read enumeration speed from DSTS */
    }

    /* RX FIFO Not Empty */
    if (gintsts & OTG_GINTSTS_RXFLVL) {
        uint32_t grxstsp = OTG_FS_GLOBAL->GRXSTSP;
        uint8_t ep_num = grxstsp & 0xFU;
        uint8_t pkt_status = (grxstsp >> 17) & 0xFU;
        uint16_t pkt_len = (grxstsp >> 4) & 0x7FFU;

        if (pkt_status == 0x06U && ep_num == 0) {
            /* Setup packet on EP0 */
            usb_handle_setup();
        } else if (pkt_status == 0x02U && ep_num == 1) {
            /* OUT data on EP1 (CDC data) */
            volatile uint32_t *rx_fifo = (volatile uint32_t *)0x50002000U;
            uint16_t words = (pkt_len + 3U) / 4U;
            uint16_t idx = 0;
            for (uint16_t w = 0; w < words; w++) {
                uint32_t word = *rx_fifo;
                for (int b = 0; b < 4 && idx < pkt_len && idx < USB_CDC_BUF_SIZE; b++) {
                    rx_buf[idx++] = (word >> (b * 8)) & 0xFF;
                }
            }
            rx_len = idx;

            if (rx_callback != NULL && rx_len > 0) {
                rx_callback(rx_buf, rx_len);
            }
        } else if (pkt_status == 0x04U) {
            /* Setup stage complete — discard */
        } else {
            /* Read and discard */
            volatile uint32_t *rx_fifo = (volatile uint32_t *)0x50002000U;
            uint16_t words = (pkt_len + 3U) / 4U;
            for (uint16_t w = 0; w < words; w++) {
                (void)*rx_fifo;
            }
        }
    }
}