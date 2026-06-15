/*
 * usb_descriptors.h — USB CDC ACM device descriptors for BLE Phantom
 */

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

/* ============================================================
 * USB Vendor/Product IDs
 * ============================================================ */
#define USB_VID             0x1783  /* Shenzhen Zoro (research) */
#define USB_PID             0xF040  /* BLE Phantom */

/* ============================================================
 * Device descriptor (18 bytes)
 * ============================================================ */
static const uint8_t device_descriptor[] = {
    18,                     /* bLength */
    0x01,                   /* bDescriptorType: DEVICE */
    0x00, 0x02,             /* bcdUSB: 2.00 */
    0x02,                   /* bDeviceClass: CDC */
    0x00,                   /* bDeviceSubClass */
    0x00,                   /* bDeviceProtocol */
    64,                     /* bMaxPacketSize0 */
    (USB_VID & 0xFF), (USB_VID >> 8),     /* idVendor */
    (USB_PID & 0xFF), (USB_PID >> 8),     /* idProduct */
    0x01, 0x00,             /* bcdDevice: 1.00 */
    1,                      /* iManufacturer */
    2,                      /* iProduct */
    3,                      /* iSerialNumber */
    1                       /* bNumConfigurations */
};

/* ============================================================
 * Configuration descriptor (67 bytes total)
 * ============================================================ */

/* Configuration descriptor (9 bytes) */
static const uint8_t config_descriptor[] = {
    /* Configuration descriptor */
    9,                      /* bLength */
    0x02,                   /* bDescriptorType: CONFIGURATION */
    0x43, 0x00,             /* wTotalLength: 67 bytes */
    2,                      /* bNumInterfaces: 2 */
    1,                      /* bConfigurationValue */
    0,                      /* iConfiguration */
    0x80,                   /* bmAttributes: bus-powered */
    250,                    /* bMaxPower: 500 mA */

    /* Interface 0: CDC Communications (9 bytes) */
    9,                      /* bLength */
    0x04,                   /* bDescriptorType: INTERFACE */
    0,                      /* bInterfaceNumber: 0 */
    0,                      /* bAlternateSetting */
    1,                      /* bNumEndpoints: 1 (INT) */
    0x02,                   /* bInterfaceClass: CDC */
    0x02,                   /* bInterfaceSubClass: ACM */
    0x01,                   /* bInterfaceProtocol: V.250 */
    0,                      /* iInterface */

    /* CDC Header Functional Descriptor (5 bytes) */
    5, 0x24, 0x00, 0x10, 0x01,

    /* CDC ACM Functional Descriptor (4 bytes) */
    4, 0x24, 0x02, 0x02,

    /* CDC Union Functional Descriptor (5 bytes) */
    5, 0x24, 0x06, 0x00, 0x01,

    /* CDC Call Management Functional Descriptor (5 bytes) */
    5, 0x24, 0x01, 0x00, 0x01,

    /* Endpoint 2: INT IN (7 bytes) */
    7,                      /* bLength */
    0x05,                   /* bDescriptorType: ENDPOINT */
    0x82,                   /* bEndpointAddress: EP2 IN */
    0x03,                   /* bmAttributes: Interrupt */
    8, 0x00,                /* wMaxPacketSize: 8 */
    255,                    /* bInterval: 255 ms */

    /* Interface 1: CDC Data (9 bytes) */
    9,                      /* bLength */
    0x04,                   /* bDescriptorType: INTERFACE */
    1,                      /* bInterfaceNumber: 1 */
    0,                      /* bAlternateSetting */
    2,                      /* bNumEndpoints: 2 (BULK IN/OUT) */
    0x0A,                   /* bInterfaceClass: CDC Data */
    0x00,                   /* bInterfaceSubClass */
    0x00,                   /* bInterfaceProtocol */
    0,                      /* iInterface */

    /* Endpoint 1: BULK OUT (7 bytes) */
    7,                      /* bLength */
    0x05,                   /* bDescriptorType: ENDPOINT */
    0x01,                   /* bEndpointAddress: EP1 OUT */
    0x02,                   /* bmAttributes: Bulk */
    64, 0x00,               /* wMaxPacketSize: 64 */
    0,                      /* bInterval */

    /* Endpoint 3: BULK IN (7 bytes) */
    7,                      /* bLength */
    0x05,                   /* bDescriptorType: ENDPOINT */
    0x83,                   /* bEndpointAddress: EP3 IN */
    0x02,                   /* bmAttributes: Bulk */
    64, 0x00,               /* wMaxPacketSize: 64 */
    0                       /* bInterval */
};

/* ============================================================
 * String descriptors
 * ============================================================ */

/* Language ID descriptor */
static const uint8_t string_lang[] = {
    4, 0x03, 0x09, 0x04    /* English (US) */
};

/* Manufacturer string */
static const uint8_t string_manufacturer[] = {
    28, 0x03,
    'N', 0, 'o', 0, 'u', 0, 's', 0, ' ', 0,
    'R', 0, 'e', 0, 's', 0, 'e', 0, 'a', 0,
    'r', 0, 'c', 0, 'h', 0
};

/* Product string */
static const uint8_t string_product[] = {
    24, 0x03,
    'B', 0, 'L', 0, 'E', 0, ' ', 0,
    'P', 0, 'h', 0, 'a', 0, 'n', 0,
    't', 0, 'o', 0, 'm', 0
};

/* Serial number string */
static const uint8_t string_serial[] = {
    18, 0x03,
    '0', 0, '0', 0, '0', 0, '0', 0,
    '0', 0, '0', 0, '0', 0, '1', 0
};

/* ============================================================
 * Descriptor sizes
 * ============================================================ */
#define DEVICE_DESCRIPTOR_SIZE     18
#define CONFIG_DESCRIPTOR_SIZE     67
#define STRING_LANG_SIZE           4
#define STRING_MANUFACTURER_SIZE   28
#define STRING_PRODUCT_SIZE        24
#define STRING_SERIAL_SIZE         18

/* ============================================================
 * USB CDC line coding defaults
 * ============================================================ */
#define CDC_LINE_CODING_RATE       115200
#define CDC_LINE_CODING_STOP       0   /* 1 stop bit */
#define CDC_LINE_CODING_PARITY     0   /* No parity */
#define CDC_LINE_CODING_DATA       8   /* 8 data bits */

/* ============================================================
 * Endpoint addresses
 * ============================================================ */
#define CDC_EP_NOTIFY_ADDR    0x82    /* EP2 IN */
#define CDC_EP_NOTIFY_SIZE   8
#define CDC_EP_DATA_OUT_ADDR 0x01    /* EP1 OUT */
#define CDC_EP_DATA_IN_ADDR  0x83    /* EP3 IN */
#define CDC_EP_DATA_SIZE     64

#endif /* USB_DESCRIPTORS_H */