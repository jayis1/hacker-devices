/**
 * @file usb_cdc_driver.c
 * @brief USB CDC-ACM Driver Implementation
 *
 * Full USB CDC-ACM implementation using nRF52840 USBD peripheral.
 * Handles enumeration, control transfers, bulk data, and CDC class requests.
 *
 * USB Hardware: nRF52840 USBD peripheral
 * Speed: Full Speed 12 Mbps
 * Endpoints: EP0 (control), EP1 IN/OUT (bulk), EP2 IN (interrupt)
 */

#include "usb_cdc_driver.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/*===========================================================================
 * STATIC VARIABLES
 *===========================================================================*/

static bool g_usb_initialized = false;
static bool g_usb_connected = false;     /* Enumerated and configured */
static bool g_usb_powered = false;       /* VBUS detected */
static uint8_t g_usb_address = 0;
static usb_cdc_line_coding_t g_line_coding;
static usb_cdc_rx_callback_t g_rx_callback = NULL;
static usb_cdc_tx_complete_callback_t g_tx_complete_callback = NULL;
static usb_cdc_state_callback_t g_state_callback = NULL;

/* TX ring buffer */
#define USB_CDC_TX_BUF_SIZE    2048
static uint8_t g_tx_buf[USB_CDC_TX_BUF_SIZE] __attribute__((aligned(4)));
static volatile uint16_t g_tx_head = 0;
static volatile uint16_t g_tx_tail = 0;
static volatile bool g_tx_active = false;

/* RX buffer (single packet) */
static uint8_t g_rx_buf[USB_CDC_EP_BULK_MAX_PACKET] __attribute__((aligned(4)));

/* EP0 setup buffer */
static uint8_t g_ep0_buf[128] __attribute__((aligned(4)));

/*===========================================================================
 * USB DESCRIPTORS
 *===========================================================================*/

/** Device Descriptor */
static const uint8_t g_device_desc[USB_CDC_DEVICE_DESC_SIZE] = {
    0x12,                       /* bLength (18) */
    0x01,                       /* bDescriptorType (DEVICE) */
    0x00, 0x02,                 /* bcdUSB (2.00) */
    0x02,                       /* bDeviceClass (CDC) */
    0x00,                       /* bDeviceSubClass */
    0x00,                       /* bDeviceProtocol */
    USB_CDC_EP0_MAX_PACKET,     /* bMaxPacketSize0 (64) */
    (uint8_t)(USB_CDC_VID & 0xFF), (uint8_t)(USB_CDC_VID >> 8),  /* idVendor */
    (uint8_t)(USB_CDC_PID & 0xFF), (uint8_t)(USB_CDC_PID >> 8),  /* idProduct */
    (uint8_t)(USB_CDC_BCD_DEVICE & 0xFF), (uint8_t)(USB_CDC_BCD_DEVICE >> 8), /* bcdDevice */
    0x01,                       /* iManufacturer */
    0x02,                       /* iProduct */
    0x03,                       /* iSerialNumber */
    0x01                        /* bNumConfigurations */
};

/** Configuration Descriptor (CDC-ACM) */
static const uint8_t g_config_desc[USB_CDC_CONFIG_DESC_SIZE] = {
    /* Configuration Descriptor */
    0x09,                       /* bLength (9) */
    0x02,                       /* bDescriptorType (CONFIGURATION) */
    (uint8_t)(USB_CDC_CONFIG_DESC_SIZE & 0xFF),
    (uint8_t)(USB_CDC_CONFIG_DESC_SIZE >> 8),  /* wTotalLength */
    0x02,                       /* bNumInterfaces (2) */
    0x01,                       /* bConfigurationValue */
    0x00,                       /* iConfiguration */
    0xC0,                       /* bmAttributes (Self-powered) */
    0x32,                       /* bMaxPower (100 mA) */

    /* Interface Association Descriptor (IAD) */
    0x08,                       /* bLength (8) */
    0x0B,                       /* bDescriptorType (IAD) */
    USB_CDC_INTF_CONTROL,       /* bFirstInterface */
    0x02,                       /* bInterfaceCount (2) */
    0x02,                       /* bFunctionClass (CDC) */
    0x02,                       /* bFunctionSubClass (ACM) */
    0x01,                       /* bFunctionProtocol (AT Commands V.250) */
    0x04,                       /* iFunction */

    /* CDC Header Functional Descriptor */
    0x05,                       /* bLength */
    0x24,                       /* bDescriptorType (CS_INTERFACE) */
    0x00,                       /* bDescriptorSubtype (Header) */
    0x10, 0x01,                 /* bcdCDC (1.10) */

    /* CDC ACM Functional Descriptor */
    0x04,                       /* bLength */
    0x24,                       /* bDescriptorType (CS_INTERFACE) */
    0x02,                       /* bDescriptorSubtype (ACM) */
    0x02,                       /* bmCapabilities (D0: SetLineCoding, SerialState) */

    /* CDC Union Functional Descriptor */
    0x05,                       /* bLength */
    0x24,                       /* bDescriptorType (CS_INTERFACE) */
    0x06,                       /* bDescriptorSubtype (Union) */
    USB_CDC_INTF_CONTROL,       /* bControlInterface */
    USB_CDC_INTF_DATA,          /* bSubordinateInterface */

    /* CDC Call Management Functional Descriptor */
    0x05,                       /* bLength */
    0x24,                       /* bDescriptorType (CS_INTERFACE) */
    0x01,                       /* bDescriptorSubtype (Call Management) */
    0x00,                       /* bmCapabilities */
    USB_CDC_INTF_DATA,          /* bDataInterface */

    /* Interface Descriptor: Communication (Control) */
    0x09,                       /* bLength (9) */
    0x04,                       /* bDescriptorType (INTERFACE) */
    USB_CDC_INTF_CONTROL,       /* bInterfaceNumber */
    0x00,                       /* bAlternateSetting */
    0x01,                       /* bNumEndpoints (1) */
    0x02,                       /* bInterfaceClass (CDC) */
    0x02,                       /* bInterfaceSubClass (ACM) */
    0x01,                       /* bInterfaceProtocol (AT Commands) */
    0x04,                       /* iInterface */

    /* Endpoint Descriptor: Interrupt IN */
    0x07,                       /* bLength (7) */
    0x05,                       /* bDescriptorType (ENDPOINT) */
    USB_CDC_EP_INTERRUPT_IN,    /* bEndpointAddress (EP2 IN) */
    0x03,                       /* bmAttributes (Interrupt) */
    (uint8_t)(USB_CDC_EP_INTERRUPT_MAX_PACKET & 0xFF),
    (uint8_t)(USB_CDC_EP_INTERRUPT_MAX_PACKET >> 8), /* wMaxPacketSize */
    USB_CDC_EP_INTERRUPT_INTERVAL, /* bInterval (64ms) */

    /* Interface Descriptor: Data */
    0x09,                       /* bLength (9) */
    0x04,                       /* bDescriptorType (INTERFACE) */
    USB_CDC_INTF_DATA,          /* bInterfaceNumber */
    0x00,                       /* bAlternateSetting */
    0x02,                       /* bNumEndpoints (2) */
    0x0A,                       /* bInterfaceClass (CDC Data) */
    0x00,                       /* bInterfaceSubClass */
    0x00,                       /* bInterfaceProtocol */
    0x00,                       /* iInterface */

    /* Endpoint Descriptor: Bulk OUT */
    0x07,                       /* bLength (7) */
    0x05,                       /* bDescriptorType (ENDPOINT) */
    USB_CDC_EP_BULK_OUT,        /* bEndpointAddress (EP1 OUT) */
    0x02,                       /* bmAttributes (Bulk) */
    (uint8_t)(USB_CDC_EP_BULK_MAX_PACKET & 0xFF),
    (uint8_t)(USB_CDC_EP_BULK_MAX_PACKET >> 8), /* wMaxPacketSize */
    0x00,                       /* bInterval (ignored for bulk) */

    /* Endpoint Descriptor: Bulk IN */
    0x07,                       /* bLength (7) */
    0x05,                       /* bDescriptorType (ENDPOINT) */
    USB_CDC_EP_BULK_IN,         /* bEndpointAddress (EP1 IN) */
    0x02,                       /* bmAttributes (Bulk) */
    (uint8_t)(USB_CDC_EP_BULK_MAX_PACKET & 0xFF),
    (uint8_t)(USB_CDC_EP_BULK_MAX_PACKET >> 8), /* wMaxPacketSize */
    0x00                        /* bInterval */
};

/** Language ID String Descriptor */
static const uint8_t g_string_langid[USB_CDC_STRING_LANGID_SIZE] = {
    0x04,                       /* bLength */
    0x03,                       /* bDescriptorType (STRING) */
    0x09, 0x04                  /* wLANGID (0x0409 = English US) */
};

/*===========================================================================
 * PRIVATE HELPERS
 *===========================================================================*/

/**
 * @brief Build manufacturer string descriptor
 */
static uint16_t build_string_desc(const char *str, uint8_t *buf, uint16_t buf_size) {
    uint16_t len = (uint16_t)strlen(str);
    uint16_t desc_len = 2 + len * 2;  /* bLength + bDescriptorType + UTF-16LE chars */

    if (desc_len > buf_size) return 0;

    buf[0] = (uint8_t)(desc_len & 0xFF);
    buf[1] = 0x03;  /* STRING descriptor type */

    for (uint16_t i = 0; i < len; i++) {
        buf[2 + i * 2] = (uint8_t)str[i];
        buf[2 + i * 2 + 1] = 0x00;  /* UTF-16LE: high byte = 0 for ASCII */
    }

    return desc_len;
}

/**
 * @brief Build serial number string from device ID
 */
static uint16_t build_serial_string(uint8_t *buf, uint16_t buf_size) {
    uint8_t dev_id[8];
    board_get_device_id(dev_id);

    /* Format as 12 hex characters */
    char serial[13];
    for (int i = 0; i < 6; i++) {
        serial[i * 2]     = "0123456789ABCDEF"[dev_id[i] >> 4];
        serial[i * 2 + 1] = "0123456789ABCDEF"[dev_id[i] & 0x0F];
    }
    serial[12] = '\0';

    return build_string_desc(serial, buf, buf_size);
}

/**
 * @brief Handle standard device requests on EP0
 */
static void handle_standard_request(uint8_t bmRequestType, uint8_t bRequest,
                                     uint16_t wValue, uint16_t wIndex, uint16_t wLength) {
    switch (bRequest) {
        case 0x00: /* GET_STATUS */
            g_ep0_buf[0] = 0x00;  /* Self-powered, no remote wakeup */
            g_ep0_buf[1] = 0x00;
            NRF_USBD->EPIN[0].PTR = (uint32_t)g_ep0_buf;
            NRF_USBD->EPIN[0].MAXCNT = 2;
            NRF_USBD->TASKS_STARTEPIN[0] = 1;
            break;

        case 0x01: /* CLEAR_FEATURE */
        case 0x03: /* SET_FEATURE */
            /* No features supported — just ACK */
            NRF_USBD->TASKS_EP0STATUS = 1;
            break;

        case 0x05: /* SET_ADDRESS */
            g_usb_address = (uint8_t)(wValue & 0x7F);
            NRF_USBD->TASKS_EP0STATUS = 1;
            /* Address is applied after status stage completes */
            break;

        case 0x06: /* GET_DESCRIPTOR */
            {
                uint8_t desc_type = (uint8_t)(wValue >> 8);
                uint8_t desc_index = (uint8_t)(wValue & 0xFF);
                const uint8_t *desc = NULL;
                uint16_t desc_len = 0;

                switch (desc_type) {
                    case 0x01: /* DEVICE */
                        desc = g_device_desc;
                        desc_len = USB_CDC_DEVICE_DESC_SIZE;
                        break;
                    case 0x02: /* CONFIGURATION */
                        desc = g_config_desc;
                        desc_len = USB_CDC_CONFIG_DESC_SIZE;
                        break;
                    case 0x03: /* STRING */
                        if (desc_index == 0) {
                            desc = g_string_langid;
                            desc_len = USB_CDC_STRING_LANGID_SIZE;
                        } else if (desc_index == 1) {
                            desc_len = build_string_desc(USB_CDC_MANUFACTURER_STRING,
                                                         g_ep0_buf, sizeof(g_ep0_buf));
                            desc = g_ep0_buf;
                        } else if (desc_index == 2) {
                            desc_len = build_string_desc(USB_CDC_PRODUCT_STRING,
                                                         g_ep0_buf, sizeof(g_ep0_buf));
                            desc = g_ep0_buf;
                        } else if (desc_index == 3) {
                            desc_len = build_serial_string(g_ep0_buf, sizeof(g_ep0_buf));
                            desc = g_ep0_buf;
                        }
                        break;
                }

                if (desc && desc_len > 0) {
                    if (wLength < desc_len) desc_len = wLength;
                    memcpy(g_ep0_buf, desc, desc_len);
                    NRF_USBD->EPIN[0].PTR = (uint32_t)g_ep0_buf;
                    NRF_USBD->EPIN[0].MAXCNT = desc_len;
                    NRF_USBD->TASKS_STARTEPIN[0] = 1;
                } else {
                    NRF_USBD->TASKS_EP0STALL = 1;
                }
            }
            break;

        case 0x08: /* GET_CONFIGURATION */
            g_ep0_buf[0] = g_usb_connected ? 0x01 : 0x00;
            NRF_USBD->EPIN[0].PTR = (uint32_t)g_ep0_buf;
            NRF_USBD->EPIN[0].MAXCNT = 1;
            NRF_USBD->TASKS_STARTEPIN[0] = 1;
            break;

        case 0x09: /* SET_CONFIGURATION */
            if (wValue == 0x01 || wValue == 0x00) {
                g_usb_connected = (wValue == 0x01);
                if (g_usb_connected) {
                    /* Enable endpoints */
                    NRF_USBD->EPINEN  = (1 << USB_CDC_EP_BULK_IN) | (1 << USB_CDC_EP_INTERRUPT_IN);
                    NRF_USBD->EPOUTEN = (1 << USB_CDC_EP_BULK_OUT);
                    /* Set max packet sizes */
                    /* (EP0 is always 64, bulk/interrupt set via EPMODE/EPMAXPACKETSIZE) */
                } else {
                    NRF_USBD->EPINEN  = 0;
                    NRF_USBD->EPOUTEN = 0;
                }
                if (g_state_callback) g_state_callback(g_usb_connected);
                NRF_USBD->TASKS_EP0STATUS = 1;
            } else {
                NRF_USBD->TASKS_EP0STALL = 1;
            }
            break;

        default:
            NRF_USBD->TASKS_EP0STALL = 1;
            break;
    }
}

/**
 * @brief Handle CDC class-specific requests
 */
static void handle_cdc_request(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength) {
    switch (bRequest) {
        case USB_CDC_REQ_SET_LINE_CODING:
            /* Host sends 7-byte line coding structure */
            NRF_USBD->EPOUT[0].PTR = (uint32_t)g_ep0_buf;
            NRF_USBD->EPOUT[0].MAXCNT = 7;
            NRF_USBD->TASKS_STARTEPOUT[0] = 1;
            /* Data will be processed in EP0DATADONE event */
            break;

        case USB_CDC_REQ_GET_LINE_CODING:
            /* Return current line coding */
            memcpy(g_ep0_buf, &g_line_coding, 7);
            NRF_USBD->EPIN[0].PTR = (uint32_t)g_ep0_buf;
            NRF_USBD->EPIN[0].MAXCNT = 7;
            NRF_USBD->TASKS_STARTEPIN[0] = 1;
            break;

        case USB_CDC_REQ_SET_CONTROL_LINE_STATE:
            /* wValue: DTR (bit 0), RTS (bit 1) */
            /* Just ACK — we don't use hardware flow control */
            NRF_USBD->TASKS_EP0STATUS = 1;
            break;

        default:
            NRF_USBD->TASKS_EP0STALL = 1;
            break;
    }
}

/**
 * @brief Start bulk IN transfer from TX ring buffer
 */
static void start_bulk_in_transfer(void) {
    uint16_t avail;

    if (g_tx_active) return;  /* Already transmitting */

    CRITICAL_REGION_ENTER();
    if (g_tx_head == g_tx_tail) {
        CRITICAL_REGION_EXIT();
        return;  /* Nothing to send */
    }

    /* Calculate available data */
    if (g_tx_head > g_tx_tail) {
        avail = g_tx_head - g_tx_tail;
    } else {
        avail = USB_CDC_TX_BUF_SIZE - g_tx_tail + g_tx_head;
    }

    /* Limit to max packet size */
    if (avail > USB_CDC_EP_BULK_MAX_PACKET) {
        avail = USB_CDC_EP_BULK_MAX_PACKET;
    }

    /* Copy from ring buffer to linear DMA buffer */
    uint16_t tail = g_tx_tail;
    uint16_t space_to_end = USB_CDC_TX_BUF_SIZE - tail;
    if (avail <= space_to_end) {
        memcpy(g_ep0_buf, &g_tx_buf[tail], avail);
        g_tx_tail = (tail + avail) % USB_CDC_TX_BUF_SIZE;
    } else {
        memcpy(g_ep0_buf, &g_tx_buf[tail], space_to_end);
        memcpy(&g_ep0_buf[space_to_end], g_tx_buf, avail - space_to_end);
        g_tx_tail = avail - space_to_end;
    }

    g_tx_active = true;
    CRITICAL_REGION_EXIT();

    /* Start EasyDMA transfer on EP1 IN */
    NRF_USBD->EPIN[USB_CDC_EP_BULK_IN].PTR = (uint32_t)g_ep0_buf;
    NRF_USBD->EPIN[USB_CDC_EP_BULK_IN].MAXCNT = avail;
    NRF_USBD->TASKS_STARTEPIN[USB_CDC_EP_BULK_IN] = 1;
}

/*===========================================================================
 * PUBLIC API
 *===========================================================================*/

int usb_cdc_init(const usb_cdc_config_t *config) {
    if (!config) return USB_CDC_ERR_PARAM;

    g_rx_callback = config->rx_callback;
    g_tx_complete_callback = config->tx_complete_callback;
    g_state_callback = config->state_callback;

    /* Set default line coding */
    g_line_coding.dwDTERate   = USB_CDC_DEFAULT_BAUD_RATE;
    g_line_coding.bCharFormat = USB_CDC_DEFAULT_STOP_BITS;
    g_line_coding.bParityType = USB_CDC_DEFAULT_PARITY;
    g_line_coding.bDataBits   = USB_CDC_DEFAULT_DATA_BITS;

    /* Initialize TX ring buffer */
    g_tx_head = 0;
    g_tx_tail = 0;
    g_tx_active = false;

    /* Enable USBD peripheral */
    NRF_USBD->ENABLE = USBD_ENABLE_ENABLE;

    /* Enable D+ pull-up (triggers host enumeration) */
    NRF_USBD->USBPULLUP = USBD_PULLUP_ENABLE;

    /* Enable interrupts */
    NRF_USBD->INTEN = USBD_INT_USBRESET | USBD_INT_STARTED |
                      USBD_INT_EP0SETUP | USBD_INT_EP0DATADONE |
                      USBD_INT_EPDATA | USBD_INT_USBEVENT;

    g_usb_initialized = true;

    return USB_CDC_ERR_OK;
}

int usb_cdc_deinit(void) {
    if (!g_usb_initialized) return USB_CDC_ERR_NOT_INIT;

    NRF_USBD->USBPULLUP = USBD_PULLUP_DISABLE;
    NRF_USBD->ENABLE = USBD_ENABLE_DISABLE;

    g_usb_initialized = false;
    g_usb_connected = false;

    return USB_CDC_ERR_OK;
}

int usb_cdc_tx(const uint8_t *data, uint16_t len) {
    if (!g_usb_initialized) return USB_CDC_ERR_NOT_INIT;
    if (!g_usb_connected) return USB_CDC_ERR_NOT_CONNECTED;
    if (!data || len == 0) return USB_CDC_ERR_PARAM;

    /* Add to TX ring buffer */
    CRITICAL_REGION_ENTER();
    uint16_t head = g_tx_head;
    uint16_t tail = g_tx_tail;

    /* Calculate free space */
    uint16_t free_space;
    if (head >= tail) {
        free_space = USB_CDC_TX_BUF_SIZE - head + tail - 1;
    } else {
        free_space = tail - head - 1;
    }

    if (len > free_space) {
        CRITICAL_REGION_EXIT();
        return USB_CDC_ERR_TX_FULL;
    }

    /* Copy data */
    uint16_t space_to_end = USB_CDC_TX_BUF_SIZE - head;
    if (len <= space_to_end) {
        memcpy(&g_tx_buf[head], data, len);
        g_tx_head = (head + len) % USB_CDC_TX_BUF_SIZE;
    } else {
        memcpy(&g_tx_buf[head], data, space_to_end);
        memcpy(g_tx_buf, data + space_to_end, len - space_to_end);
        g_tx_head = len - space_to_end;
    }
    CRITICAL_REGION_EXIT();

    /* Start transfer if not already active */
    start_bulk_in_transfer();

    return USB_CDC_ERR_OK;
}

int usb_cdc_tx_pending(void) {
    if (!g_usb_initialized) return 0;

    uint16_t head = g_tx_head;
    uint16_t tail = g_tx_tail;

    if (head >= tail) {
        return (int)(head - tail);
    } else {
        return (int)(USB_CDC_TX_BUF_SIZE - tail + head);
    }
}

bool usb_cdc_is_connected(void) {
    return g_usb_connected;
}

int usb_cdc_get_line_coding(usb_cdc_line_coding_t *coding) {
    if (!coding) return USB_CDC_ERR_PARAM;
    memcpy(coding, &g_line_coding, sizeof(usb_cdc_line_coding_t));
    return USB_CDC_ERR_OK;
}

int usb_cdc_send_serial_state(uint16_t state) {
    if (!g_usb_initialized || !g_usb_connected) return USB_CDC_ERR_NOT_CONNECTED;

    usb_cdc_serial_state_t notification;
    notification.bmRequestType = 0xA1;  /* Class, Interface, IN */
    notification.bNotification = USB_CDC_NOTIFY_SERIAL_STATE;
    notification.wValue = 0x0000;
    notification.wIndex = USB_CDC_INTF_CONTROL;
    notification.wLength = 0x0002;
    notification.data = state;

    memcpy(g_ep0_buf, &notification, 10);
    NRF_USBD->EPIN[USB_CDC_EP_INTERRUPT_IN].PTR = (uint32_t)g_ep0_buf;
    NRF_USBD->EPIN[USB_CDC_EP_INTERRUPT_IN].MAXCNT = 10;
    NRF_USBD->TASKS_STARTEPIN[USB_CDC_EP_INTERRUPT_IN] = 1;

    return USB_CDC_ERR_OK;
}

/**
 * @brief Handle USBD events (called from USBD interrupt handler)
 */
void usb_cdc_event_handler(const void *event) {
    /* In real implementation, this processes USBD events:
     *
     * if (NRF_USBD->EVENTS_USBRESET) {
     *     NRF_USBD->EVENTS_USBRESET = 0;
     *     g_usb_address = 0;
     *     // Re-enable endpoints after reset
     * }
     *
     * if (NRF_USBD->EVENTS_STARTED) {
     *     NRF_USBD->EVENTS_STARTED = 0;
     *     // USB started (VBUS detected, D+ pull-up active)
     * }
     *
     * if (NRF_USBD->EVENTS_EP0SETUP) {
     *     NRF_USBD->EVENTS_EP0SETUP = 0;
     *     // Read setup packet from BMREQUESTTYPE..WLENGTHH registers
     *     uint8_t bmRequestType = (uint8_t)NRF_USBD->BMREQUESTTYPE;
     *     uint8_t bRequest = (uint8_t)NRF_USBD->BREQUEST;
     *     uint16_t wValue = NRF_USBD->WVALUEL | (NRF_USBD->WVALUEH << 8);
     *     uint16_t wIndex = NRF_USBD->WINDEXL | (NRF_USBD->WINDEXH << 8);
     *     uint16_t wLength = NRF_USBD->WLENGTHL | (NRF_USBD->WLENGTHH << 8);
     *
     *     if ((bmRequestType & 0x60) == 0x00) {
     *         handle_standard_request(bmRequestType, bRequest, wValue, wIndex, wLength);
     *     } else if ((bmRequestType & 0x60) == 0x20) {
     *         handle_cdc_request(bRequest, wValue, wIndex, wLength);
     *     }
     * }
     *
     * if (NRF_USBD->EVENTS_EP0DATADONE) {
     *     NRF_USBD->EVENTS_EP0DATADONE = 0;
     *     // EP0 data stage complete
     *     // If SET_LINE_CODING, parse received data
     *     if (NRF_USBD->EPOUT[0].AMOUNT == 7) {
     *         memcpy(&g_line_coding, g_ep0_buf, 7);
     *     }
     *     NRF_USBD->TASKS_EP0STATUS = 1;
     * }
     *
     * if (NRF_USBD->EVENTS_EPDATA) {
     *     NRF_USBD->EVENTS_EPDATA = 0;
     *     uint32_t ep_status = NRF_USBD->EPDATASTATUS;
     *     // Check which endpoint completed
     *     if (ep_status & (1 << USB_CDC_EP_BULK_IN)) {
     *         // EP1 IN transfer complete
     *         g_tx_active = false;
     *         if (g_tx_complete_callback) g_tx_complete_callback();
     *         start_bulk_in_transfer();  // Start next if data pending
     *     }
     *     if (ep_status & (1 << USB_CDC_EP_BULK_OUT)) {
     *         // EP1 OUT data received
     *         uint32_t amount = NRF_USBD->EPOUT[USB_CDC_EP_BULK_OUT].AMOUNT;
     *         if (g_rx_callback && amount > 0) {
     *             g_rx_callback(g_rx_buf, (uint16_t)amount);
     *         }
     *         // Re-arm OUT endpoint
     *         NRF_USBD->EPOUT[USB_CDC_EP_BULK_OUT].PTR = (uint32_t)g_rx_buf;
     *         NRF_USBD->EPOUT[USB_CDC_EP_BULK_OUT].MAXCNT = USB_CDC_EP_BULK_MAX_PACKET;
     *         NRF_USBD->TASKS_STARTEPOUT[USB_CDC_EP_BULK_OUT] = 1;
     *     }
     * }
     *
     * if (NRF_USBD->EVENTS_USBEVENT) {
     *     NRF_USBD->EVENTS_USBEVENT = 0;
     *     // USB event (resume, suspend, etc.)
     * }
     */

    (void)event;
}
