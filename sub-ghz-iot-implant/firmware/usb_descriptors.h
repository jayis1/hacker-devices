/*
 * usb_descriptors.h — USB Device Descriptors for Substation
 * CDC-ECM (Ethernet) + CDC-ACM (Debug Console)
 */

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

/* ============================================
 * USB VID/PID
 * ============================================ */
#define USB_VID                 0x1209      /* pid.codes */
#define USB_PID                 0x5UB5      /* "SUBSTN" */

/* ============================================
 * Device Descriptor (18 bytes)
 * ============================================ */
static const uint8_t device_descriptor[] = {
    18,                         /* bLength */
    0x01,                       /* bDescriptorType: DEVICE */
    0x00, 0x02,                 /* bcdUSB: USB 2.0 */
    0xEF,                       /* bDeviceClass: Misc */
    0x02,                       /* bDeviceSubClass: Common Class */
    0x01,                       /* bDeviceProtocol: Interface Association */
    64,                         /* bMaxPacketSize0 */
    0x09, 0x12,                 /* idVendor: 0x1209 */
    0xB5, 0x5B,                 /* idProduct: 0x5UB5 */
    0x00, 0x01,                 /* bcdDevice: 1.0 */
    0x01,                       /* iManufacturer */
    0x02,                       /* iProduct */
    0x03,                       /* iSerialNumber */
    0x01                        /* bNumConfigurations */
};

/* ============================================
 * Configuration Descriptor
 * ============================================ */
/* Configuration descriptor total size:
 *   Config: 9
 *   IAD: 8
 *   CDC-ECM: Iface 9 + 5+5+5+4+5+7+9 = 36+9=46
 *   CDC-ACM: Iface 9 + 5+5+5+4+7 = 26+9=35
 *   Total: 9 + 8 + 46 + 35 = 98 bytes
 */

static const uint8_t config_descriptor[] = {
    /* Configuration Descriptor (9 bytes) */
    9,                          /* bLength */
    0x02,                       /* bDescriptorType: CONFIGURATION */
    0x62, 0x00,                 /* wTotalLength: 98 bytes */
    0x03,                       /* bNumInterfaces: 3 (ECM uses 2, ACM uses 1) */
    0x01,                       /* bConfigurationValue */
    0x00,                       /* iConfiguration */
    0x80,                       /* bmAttributes: Bus powered */
    250,                        /* bMaxPower: 500 mA */

    /* Interface Association Descriptor (8 bytes) */
    8,                          /* bLength */
    0x0B,                       /* bDescriptorType: IAD */
    0x00,                       /* bFirstInterface: 0 */
    0x02,                       /* bInterfaceCount: 2 */
    0x02,                       /* bFunctionClass: CDC */
    0x06,                       /* bFunctionSubClass: ECM */
    0x00,                       /* bFunctionProtocol */
    0x04,                       /* iFunction */

    /* ===== CDC-ECM Interface (MAC bridge) ===== */

    /* Interface 0: CDC-ECM Communication (9 bytes) */
    9,                          /* bLength */
    0x04,                       /* bDescriptorType: INTERFACE */
    0x00,                       /* bInterfaceNumber: 0 */
    0x00,                       /* bAlternateSetting */
    0x01,                       /* bNumEndpoints: 1 (interrupt) */
    0x02,                       /* bInterfaceClass: CDC */
    0x06,                       /* bInterfaceSubClass: ECM */
    0x00,                       /* bInterfaceProtocol */
    0x00,                       /* iInterface */

    /* CDC Header Functional Descriptor (5 bytes) */
    5, 0x24, 0x00, 0x10, 0x01,

    /* CDC ECM Functional Descriptor (13 bytes) */
    13, 0x24, 0x0F,
    0x00,                       /* iMACAddress: string index */
    0x00, 0x00, 0x00, 0x00,     /* bmEthernetStatistics: none */
    0xEA, 0x05,                 /* wMaxSegmentSize: 1514 */
    0x00, 0x00,                 /* wNumberMCFilters: 0 */
    0x00,                       /* bNumberPowerFilters: 0 */

    /* Endpoint 3: Interrupt IN (7 bytes) */
    7,                          /* bLength */
    0x05,                       /* bDescriptorType: ENDPOINT */
    0x83,                       /* bEndpointAddress: EP3 IN */
    0x03,                       /* bmAttributes: Interrupt */
    0x08, 0x00,                 /* wMaxPacketSize: 8 */
    0x10,                       /* bInterval: 16 ms */

    /* Interface 1: CDC-ECM Data (9 bytes) */
    9,                          /* bLength */
    0x04,                       /* bDescriptorType: INTERFACE */
    0x01,                       /* bInterfaceNumber: 1 */
    0x00,                       /* bAlternateSetting */
    0x02,                       /* bNumEndpoints: 2 (bulk IN, bulk OUT) */
    0x0A,                       /* bInterfaceClass: CDC Data */
    0x00,                       /* bInterfaceSubClass */
    0x00,                       /* bInterfaceProtocol */
    0x00,                       /* iInterface */

    /* Endpoint 1: Bulk IN (7 bytes) */
    7,                          /* bLength */
    0x05,                       /* bDescriptorType: ENDPOINT */
    0x81,                       /* bEndpointAddress: EP1 IN */
    0x02,                       /* bmAttributes: Bulk */
    0x40, 0x00,                 /* wMaxPacketSize: 64 */
    0x00,                       /* bInterval */

    /* Endpoint 2: Bulk OUT (7 bytes) */
    7,                          /* bLength */
    0x05,                       /* bDescriptorType: ENDPOINT */
    0x02,                       /* bEndpointAddress: EP2 OUT */
    0x02,                       /* bmAttributes: Bulk */
    0x40, 0x00,                 /* wMaxPacketSize: 64 */
    0x00,                       /* bInterval */

    /* ===== CDC-ACM Interface (Debug Console) ===== */

    /* Interface 2: CDC-ACM Communication (9 bytes) */
    9,                          /* bLength */
    0x04,                       /* bDescriptorType: INTERFACE */
    0x02,                       /* bInterfaceNumber: 2 */
    0x00,                       /* bAlternateSetting */
    0x01,                       /* bNumEndpoints: 1 (interrupt) */
    0x02,                       /* bInterfaceClass: CDC */
    0x02,                       /* bInterfaceSubClass: ACM */
    0x01,                       /* bInterfaceProtocol */
    0x05,                       /* iInterface */

    /* CDC Header Functional Descriptor (5 bytes) */
    5, 0x24, 0x00, 0x10, 0x01,

    /* CDC ACM Functional Descriptor (4 bytes) */
    4, 0x24, 0x02, 0x02,       /* bmCapabilities: SendBreak, Notify */

    /* CDC Union Functional Descriptor (5 bytes) */
    5, 0x24, 0x06, 0x02, 0x03, /* bMasterInterface=2, bSlaveInterface=3 */

    /* Endpoint 6: Interrupt IN (7 bytes) */
    7,                          /* bLength */
    0x05,                       /* bDescriptorType: ENDPOINT */
    0x86,                       /* bEndpointAddress: EP6 IN */
    0x03,                       /* bmAttributes: Interrupt */
    0x08, 0x00,                 /* wMaxPacketSize: 8 */
    0x10,                       /* bInterval: 16 ms */

    /* Interface 3: CDC-ACM Data (9 bytes) */
    9,                          /* bLength */
    0x04,                       /* bDescriptorType: INTERFACE */
    0x03,                       /* bInterfaceNumber: 3 */
    0x00,                       /* bAlternateSetting */
    0x02,                       /* bNumEndpoints: 2 (bulk IN, bulk OUT) */
    0x0A,                       /* bInterfaceClass: CDC Data */
    0x00,                       /* bInterfaceSubClass */
    0x00,                       /* bInterfaceProtocol */
    0x00,                       /* iInterface */

    /* Endpoint 4: Bulk IN (7 bytes) */
    7,                          /* bLength */
    0x05,                       /* bDescriptorType: ENDPOINT */
    0x84,                       /* bEndpointAddress: EP4 IN */
    0x02,                       /* bmAttributes: Bulk */
    0x40, 0x00,                 /* wMaxPacketSize: 64 */
    0x00,                       /* bInterval */

    /* Endpoint 5: Bulk OUT (7 bytes) */
    7,                          /* bLength */
    0x05,                       /* bDescriptorType: ENDPOINT */
    0x05,                       /* bEndpointAddress: EP5 OUT */
    0x02,                       /* bmAttributes: Bulk */
    0x40, 0x00,                 /* wMaxPacketSize: 64 */
    0x00,                       /* bInterval */
};

/* ============================================
 * String Descriptors
 * ============================================ */
#define USB_LANG_ID             0x0409  /* English (US) */

/* String descriptor 0: Language IDs */
static const uint8_t string0[] = {
    4,                          /* bLength */
    0x03,                       /* bDescriptorType: STRING */
    0x09, 0x04                  /* wLANGID: English (US) */
};

/* String descriptor 1: Manufacturer */
static const uint8_t string1[] = {
    28,                         /* bLength */
    0x03,                       /* bDescriptorType: STRING */
    'H', 0x00, 'a', 0x00, 'c', 0x00, 'k', 0x00,
    'e', 0x00, 'r', 0x00, ' ', 0x00, 'D', 0x00,
    'e', 0x00, 'v', 0x00, 'i', 0x00, 'c', 0x00,
    'e', 0x00, 's', 0x00
};

/* String descriptor 2: Product */
static const uint8_t string2[] = {
    24,                         /* bLength */
    0x03,                       /* bDescriptorType: STRING */
    'S', 0x00, 'u', 0x00, 'b', 0x00, 's', 0x00,
    't', 0x00, 'a', 0x00, 't', 0x00, 'i', 0x00,
    'o', 0x00, 'n', 0x00
};

/* String descriptor 3: Serial Number (unique per device) */
static const uint8_t string3[] = {
    26,                         /* bLength */
    0x03,                       /* bDescriptorType: STRING */
    'S', 0x00, 'U', 0x00, 'B', 0x00, '0', 0x00,
    '0', 0x00, '0', 0x00, '0', 0x00, '0', 0x00,
    '0', 0x00, '1', 0x00, '2', 0x00, '3', 0x00
};

/* String descriptor 4: ECM function */
static const uint8_t string4[] = {
    20,                         /* bLength */
    0x03,                       /* bDescriptorType: STRING */
    'E', 0x00, 'C', 0x00, 'M', 0x00, ' ', 0x00,
    'N', 0x00, 'e', 0x00, 't', 0x00, 'w', 0x00,
    'o', 0x00, 'r', 0x00, 'k', 0x00
};

/* MAC address string descriptor (for ECM) */
static const uint8_t mac_string[] = {
    28,                         /* bLength */
    0x03,                       /* bDescriptorType: STRING */
    '0', 0x00, '2', 0x00, 'A', 0x00, '0', 0x00,
    '0', 0x00, '0', 0x00, 'S', 0x00, 'U', 0x00,
    'B', 0x00, '0', 0x00, '1', 0x00, '2', 0x00,
    '3', 0x00
};

/* String descriptor 5: ACM function */
static const uint8_t string5[] = {
    30,                         /* bLength */
    0x03,                       /* bDescriptorType: STRING */
    'D', 0x00, 'e', 0x00, 'b', 0x00, 'u', 0x00,
    'g', 0x00, ' ', 0x00, 'C', 0x00, 'o', 0x00,
    'n', 0x00, 's', 0x00, 'o', 0x00, 'l', 0x00,
    'e', 0x00
};

/* String descriptor table */
static const uint8_t *string_descriptors[] = {
    string0,    /* Index 0: Language */
    string1,    /* Index 1: Manufacturer */
    string2,    /* Index 2: Product */
    string3,    /* Index 3: Serial */
    string4,    /* Index 4: ECM */
    string5,    /* Index 5: ACM */
};

#endif /* USB_DESCRIPTORS_H */