/*
 * usb_descriptors.h — USB Device Descriptors for USB DMA Phantom
 * STM32F423 USB OTG Full-Speed CDC ACM + DFU composite device
 *
 * Copyright (c) 2026 Hacker Devices
 * SPDX-License-Identifier: MIT
 */

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

/* ---- VID/PID ---- */
#define USB_VID               0x1209    /* pid.codes open-source VID */
#define USB_PID               0xDMA1    /* DMA Phantom PID */
#define USB_DEVICE_VERSION    0x0100    /* v1.0 */

/* ---- USB Device Descriptor ---- */
static const uint8_t usb_device_descriptor[] = {
    18,                         /* bLength */
    0x01,                       /* bDescriptorType: DEVICE */
    0x00, 0x02,                 /* bcdUSB: 2.00 */
    0xEF,                       /* bDeviceClass: Misc */
    0x02,                       /* bDeviceSubClass: Common Class */
    0x01,                       /* bDeviceProtocol: IAD */
    64,                         /* bMaxPacketSize0: 64 bytes */
    0x09, 0x12,                 /* idVendor: 0x1209 (pid.codes) */
    0xA1, 0xD0,                 /* idProduct: 0xD0A1 (DMA Phantom) */
    0x00, 0x01,                 /* bcdDevice: 1.00 */
    0x01,                       /* iManufacturer: 1 */
    0x02,                       /* iProduct: 2 */
    0x03,                       /* iSerialNumber: 3 */
    0x01                        /* bNumConfigurations: 1 */
};

/* ---- Configuration Descriptor (CDC ACM + DFU) ---- */
static const uint8_t usb_config_descriptor[] = {
    /* Configuration descriptor */
    9,                          /* bLength */
    0x02,                       /* bDescriptorType: CONFIGURATION */
    0x44, 0x00,                 /* wTotalLength: 68 bytes */
    0x02,                       /* bNumInterfaces: 2 (CDC + DFU) */
    0x01,                       /* bConfigurationValue: 1 */
    0x00,                       /* iConfiguration: 0 */
    0x80,                       /* bmAttributes: bus-powered */
    250,                        /* bMaxPower: 500 mA (negotiated up to 900mA) */

    /* IAD: CDC ACM */
    8,                          /* bLength */
    0x0B,                       /* bDescriptorType: IAD */
    0x00,                       /* bFirstInterface: 0 */
    0x02,                       /* bInterfaceCount: 2 */
    0x02,                       /* bFunctionClass: CDC */
    0x02,                       /* bFunctionSubClass: ACM */
    0x01,                       /* bFunctionProtocol: AT Commands */
    0x04,                       /* iFunction: 4 */

    /* CDC Interface 0: Communication */
    9,                          /* bLength */
    0x04,                       /* bDescriptorType: INTERFACE */
    0x00,                       /* bInterfaceNumber: 0 */
    0x00,                       /* bAlternateSetting: 0 */
    0x01,                       /* bNumEndpoints: 1 (INT) */
    0x02,                       /* bInterfaceClass: CDC */
    0x02,                       /* bInterfaceSubClass: ACM */
    0x01,                       /* bInterfaceProtocol: AT Commands */
    0x04,                       /* iInterface: 4 */

    /* CDC Header Functional Descriptor */
    5, 0x24, 0x00, 0x10, 0x01,

    /* CDC ACM Functional Descriptor */
    4, 0x24, 0x02, 0x02, 0x00,

    /* CDC Union Functional Descriptor */
    5, 0x24, 0x06, 0x00, 0x01,

    /* CDC Call Management Functional Descriptor */
    5, 0x24, 0x01, 0x00, 0x01,

    /* CDC INT Endpoint (EP1 IN) */
    7,                          /* bLength */
    0x05,                       /* bDescriptorType: ENDPOINT */
    0x81,                       /* bEndpointAddress: EP1 IN */
    0x03,                       /* bmAttributes: Interrupt */
    0x08, 0x00,                 /* wMaxPacketSize: 8 bytes */
    0x10,                       /* bInterval: 16 ms */

    /* CDC Interface 1: Data */
    9,                          /* bLength */
    0x04,                       /* bDescriptorType: INTERFACE */
    0x01,                       /* bInterfaceNumber: 1 */
    0x00,                       /* bAlternateSetting: 0 */
    0x02,                       /* bNumEndpoints: 2 (BULK IN + BULK OUT) */
    0x0A,                       /* bInterfaceClass: CDC Data */
    0x00,                       /* bInterfaceSubClass: 0 */
    0x00,                       /* bInterfaceProtocol: 0 */
    0x05,                       /* iInterface: 5 */

    /* CDC BULK OUT Endpoint (EP2 OUT) */
    7,                          /* bLength */
    0x05,                       /* bDescriptorType: ENDPOINT */
    0x02,                       /* bEndpointAddress: EP2 OUT */
    0x02,                       /* bmAttributes: Bulk */
    0x40, 0x00,                 /* wMaxPacketSize: 64 bytes */
    0x00,                       /* bInterval: 0 */

    /* CDC BULK IN Endpoint (EP2 IN) */
    7,                          /* bLength */
    0x05,                       /* bDescriptorType: ENDPOINT */
    0x82,                       /* bEndpointAddress: EP2 IN */
    0x02,                       /* bmAttributes: Bulk */
    0x40, 0x00,                 /* wMaxPacketSize: 64 bytes */
    0x00,                       /* bInterval: 0 */

    /* DFU Interface 2 */
    9,                          /* bLength */
    0x04,                       /* bDescriptorType: INTERFACE */
    0x02,                       /* bInterfaceNumber: 2 */
    0x00,                       /* bAlternateSetting: 0 */
    0x00,                       /* bNumEndpoints: 0 */
    0xFE,                       /* bInterfaceClass: Application Specific */
    0x01,                       /* bInterfaceSubClass: DFU */
    0x02,                       /* bInterfaceProtocol: DFU mode */
    0x06,                       /* iInterface: 6 */
};

/* ---- DFU Functional Descriptor ---- */
static const uint8_t usb_dfu_functional[] = {
    9,                          /* bLength */
    0x21,                       /* bDescriptorType: DFU Functional */
    0x0B,                       /* bmAttributes: willDetach | manifestationTolerant | uploadable | downloadable */
    0xFF, 0x00,                 /* wDetachTimeOut: 255 ms */
    0x00, 0x08,                 /* wTransferSize: 2048 bytes */
    0x10, 0x01,                 /* bcdDFUVersion: 1.10 */
};

/* ---- String Descriptors ---- */
static const uint8_t usb_string_lang[] = {
    4, 0x03,                    /* bLength, bDescriptorType: STRING */
    0x09, 0x04                 /* wLANGID: English (US) */
};

static const uint8_t usb_string_manufacturer[] = {
    28, 0x03,                   /* bLength, bDescriptorType: STRING */
    'H', 0x00, 'a', 0x00, 'c', 0x00, 'k', 0x00,
    'e', 0x00, 'r', 0x00, ' ', 0x00, 'D', 0x00,
    'e', 0x00, 'v', 0x00, 'i', 0x00, 'c', 0x00,
    'e', 0x00, 's', 0x00
};

static const uint8_t usb_string_product[] = {
    30, 0x03,                   /* bLength, bDescriptorType: STRING */
    'U', 0x00, 'S', 0x00, 'B', 0x00, ' ', 0x00,
    'D', 0x00, 'M', 0x00, 'A', 0x00, ' ', 0x00,
    'P', 0x00, 'h', 0x00, 'a', 0x00, 'n', 0x00,
    't', 0x00, 'o', 0x00, 'm', 0x00
};

static const uint8_t usb_string_serial[] = {
    26, 0x03,                   /* bLength, bDescriptorType: STRING */
    'D', 0x00, 'M', 0x00, 'A', 0x00, '-', 0x00,
    'P', 0x00, 'H', 0x00, 'A', 0x00, 'N', 0x00,
    'T', 0x00, '0', 0x00, '0', 0x00, '1', 0x00
};

/* ---- String descriptor index table ---- */
#define USB_STRING_IDX_LANG         0
#define USB_STRING_IDX_MANUFACTURER  1
#define USB_STRING_IDX_PRODUCT       2
#define USB_STRING_IDX_SERIAL        3

#endif /* USB_DESCRIPTORS_H */