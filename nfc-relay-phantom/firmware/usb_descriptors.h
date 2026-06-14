/*
 * NFC Relay Phantom — USB Descriptors
 * Composite device: CDC-ACM (serial) + Mass Storage (capture access)
 *
 * Copyright (c) 2026 Hacker Devices. Licensed under GPL-2.0.
 */

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

/* Device Descriptor */
static const uint8_t usb_dev_descriptor[] = {
    0x12,       /* bLength */
    0x01,       /* bDescriptorType: Device */
    0x00, 0x02, /* bcdUSB: 2.00 */
    0xEF,       /* bDeviceClass: Miscellaneous */
    0x02,       /* bDeviceSubClass: Common */
    0x01,       /* bDeviceProtocol: Interface Association */
    0x40,       /* bMaxPacketSize0: 64 bytes */
    0x09, 0x12, /* idVendor: 0x1209 (pid.codes) */
    0x52, 0x0F, /* idProduct: 0x0F52 (NFC Phantom) */
    0x00, 0x01, /* bcdDevice: 1.00 */
    0x01,       /* iManufacturer: 1 */
    0x02,       /* iProduct: 2 */
    0x03,       /* iSerialNumber: 3 */
    0x01,       /* bNumConfigurations: 1 */
};

/* Configuration Descriptor (CDC + MSC) */
static const uint8_t usb_cfg_descriptor[] = {
    /* Configuration Descriptor */
    0x09,       /* bLength */
    0x02,       /* bDescriptorType: Configuration */
    0x58, 0x00, /* wTotalLength: 88 bytes */
    0x03,       /* bNumInterfaces: 3 (CDC: 2, MSC: 1) */
    0x01,       /* bConfigurationValue: 1 */
    0x00,       /* iConfiguration: 0 */
    0x80,       /* bmAttributes: Bus powered */
    0xFA,       /* MaxPower: 500 mA */

    /* Interface Association Descriptor (CDC) */
    0x08,       /* bLength */
    0x0B,       /* bDescriptorType: IAD */
    0x00,       /* bFirstInterface: 0 */
    0x02,       /* bInterfaceCount: 2 */
    0x02,       /* bFunctionClass: CDC */
    0x02,       /* bFunctionSubClass: ACM */
    0x01,       /* bFunctionProtocol: V.25ter */
    0x04,       /* iFunction: 4 */

    /* CDC Interface 0 (Communication) */
    0x09,       /* bLength */
    0x04,       /* bDescriptorType: Interface */
    0x00,       /* bInterfaceNumber: 0 */
    0x00,       /* bAlternateSetting: 0 */
    0x01,       /* bNumEndpoints: 1 */
    0x02,       /* bInterfaceClass: CDC */
    0x02,       /* bInterfaceSubClass: ACM */
    0x01,       /* bInterfaceProtocol: V.25ter */
    0x04,       /* iInterface: 4 */

    /* CDC Header Functional Descriptor */
    0x05, 0x24, 0x01, 0x10, 0x01,

    /* CDC ACM Functional Descriptor */
    0x04, 0x24, 0x02, 0x02,

    /* CDC Union Functional Descriptor */
    0x05, 0x24, 0x06, 0x00, 0x01,

    /* CDC Call Management Functional Descriptor */
    0x05, 0x24, 0x01, 0x00, 0x01,

    /* CDC Endpoint 1 (Interrupt IN) */
    0x07,       /* bLength */
    0x05,       /* bDescriptorType: Endpoint */
    0x81,       /* bEndpointAddress: EP1 IN */
    0x03,       /* bmAttributes: Interrupt */
    0x08, 0x00, /* wMaxPacketSize: 8 */
    0x10,       /* bInterval: 16 ms */

    /* CDC Interface 1 (Data) */
    0x09,       /* bLength */
    0x04,       /* bDescriptorType: Interface */
    0x01,       /* bInterfaceNumber: 1 */
    0x00,       /* bAlternateSetting: 0 */
    0x02,       /* bNumEndpoints: 2 */
    0x0A,       /* bInterfaceClass: CDC Data */
    0x00,       /* bInterfaceSubClass: 0 */
    0x00,       /* bInterfaceProtocol: 0 */
    0x05,       /* iInterface: 5 */

    /* CDC Endpoint 2 (Bulk OUT) */
    0x07,       /* bLength */
    0x05,       /* bDescriptorType: Endpoint */
    0x02,       /* bEndpointAddress: EP2 OUT */
    0x02,       /* bmAttributes: Bulk */
    0x40, 0x00, /* wMaxPacketSize: 64 */
    0x00,       /* bInterval: 0 */

    /* CDC Endpoint 3 (Bulk IN) */
    0x07,       /* bLength */
    0x05,       /* bDescriptorType: Endpoint */
    0x83,       /* bEndpointAddress: EP3 IN */
    0x02,       /* bmAttributes: Bulk */
    0x40, 0x00, /* wMaxPacketSize: 64 */
    0x00,       /* bInterval: 0 */

    /* MSC Interface 2 */
    0x09,       /* bLength */
    0x04,       /* bDescriptorType: Interface */
    0x02,       /* bInterfaceNumber: 2 */
    0x00,       /* bAlternateSetting: 0 */
    0x02,       /* bNumEndpoints: 2 */
    0x08,       /* bInterfaceClass: Mass Storage */
    0x06,       /* bInterfaceSubClass: SCSI */
    0x50,       /* bInterfaceProtocol: BBB */
    0x06,       /* iInterface: 6 */

    /* MSC Endpoint 4 (Bulk OUT) */
    0x07,       /* bLength */
    0x05,       /* bDescriptorType: Endpoint */
    0x04,       /* bEndpointAddress: EP4 OUT */
    0x02,       /* bmAttributes: Bulk */
    0x40, 0x00, /* wMaxPacketSize: 64 */
    0x00,       /* bInterval: 0 */

    /* MSC Endpoint 5 (Bulk IN) */
    0x07,       /* bLength */
    0x05,       /* bDescriptorType: Endpoint */
    0x85,       /* bEndpointAddress: EP5 IN */
    0x02,       /* bmAttributes: Bulk */
    0x40, 0x00, /* wMaxPacketSize: 64 */
    0x00,       /* bInterval: 0 */
};

/* String Descriptors */
static const uint8_t usb_str_lang[] = {
    0x04, 0x03, 0x09, 0x04  /* English (US) */
};

static const uint8_t usb_str_manufacturer[] = {
    0x1A, 0x03,
    'H', 0x00, 'a', 0x00, 'c', 0x00, 'k', 0x00, 'e', 0x00,
    'r', 0x00, ' ', 0x00, 'D', 0x00, 'e', 0x00, 'v', 0x00,
    'i', 0x00, 'c', 0x00, 'e', 0x00, 's', 0x00
};

static const uint8_t usb_str_product[] = {
    0x24, 0x03,
    'N', 0x00, 'F', 0x00, 'C', 0x00, ' ', 0x00,
    'R', 0x00, 'e', 0x00, 'l', 0x00, 'a', 0x00,
    'y', 0x00, ' ', 0x00, 'P', 0x00, 'h', 0x00,
    'a', 0x00, 'n', 0x00, 't', 0x00, 'o', 0x00,
    'm', 0x00
};

static const uint8_t usb_str_serial[] = {
    0x1C, 0x03,
    'N', 0x00, 'F', 0x00, 'C', 0x00, 'P', 0x00,
    'H', 0x00, 'A', 0x00, 'N', 0x00, 'T', 0x00,
    '0', 0x00, '0', 0x00, '0', 0x00, '0', 0x00,
    '1', 0x00
};

/* Endpoint addresses */
#define CDC_ENDPOINT_INT_IN     0x81
#define CDC_ENDPOINT_DATA_OUT   0x02
#define CDC_ENDPOINT_DATA_IN    0x83
#define MSC_ENDPOINT_OUT        0x04
#define MSC_ENDPOINT_IN         0x85

#endif /* USB_DESCRIPTORS_H */