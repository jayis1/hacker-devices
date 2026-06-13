/*
 * usb_descriptors.h — USB CDC Device Descriptors for RF Transceiver Tool
 *
 * Defines USB VID/PID, device descriptor, configuration descriptor,
 * and string descriptors for the CDC virtual serial port interface.
 * The device appears as a serial modem for host communication.
 */

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

/* ========================================================================
 * USB Device Configuration
 * ======================================================================== */
#define USB_VID                 0x1209     /* pid.codes open-source VID */
#define USB_PID                 0xCC02     /* RF Transceiver Tool PID */
#define USB_DEVICE_CLASS        0x02       /* CDC (Communication Device Class) */
#define USB_DEVICE_SUBCLASS     0x00
#define USB_DEVICE_PROTOCOL     0x00
#define USB_DEVICE_VERSION       0x0100    /* v1.0 */
#define USB_CONFIG_VALUE         1
#define USB_MAX_POWER_MA        250        /* 250 mA (within USB 500 mA limit) */

/* Endpoint addresses */
#define EP0_OUT                 0x00       /* Control OUT */
#define EP0_IN                  0x80       /* Control IN */
#define EP1_IN                  0x81       /* CDC Interrupt IN */
#define EP2_OUT                 0x02       /* CDC Data OUT */
#define EP2_IN                  0x82       /* CDC Data IN */

/* Endpoint sizes */
#define EP0_SIZE                64         /* Control endpoint size */
#define EP1_SIZE                8          /* CDC interrupt endpoint size */
#define EP2_SIZE                64         /* CDC data endpoint size */

/* String descriptor indices */
#define USB_STR_LANG            0
#define USB_STR_MANUFACTURER    1
#define USB_STR_PRODUCT         2
#define USB_STR_SERIAL          3
#define USB_STR_CDC_IFACE       4

/* ========================================================================
 * Device Descriptor (18 bytes)
 * ======================================================================== */
static const uint8_t usb_device_descriptor[] = {
    0x12,       /* bLength: 18 bytes */
    0x01,       /* bDescriptorType: DEVICE */
    0x00, 0x02, /* bcdUSB: USB 2.0 */
    0x02,       /* bDeviceClass: CDC */
    0x00,       /* bDeviceSubClass */
    0x00,       /* bDeviceProtocol */
    EP0_SIZE,   /* bMaxPacketSize0 */
    0x09, 0x12, /* idVendor: 0x1209 */
    0x02, 0xCC, /* idProduct: 0xCC02 */
    0x00, 0x01, /* bcdDevice: v1.0 */
    0x01,       /* iManufacturer: String 1 */
    0x02,       /* iProduct: String 2 */
    0x03,       /* iSerialNumber: String 3 */
    0x01        /* bNumConfigurations: 1 */
};

/* ========================================================================
 * Configuration Descriptor (67 bytes total)
 * Includes CDC interface with interrupt and bulk endpoints
 * ======================================================================== */

/* Configuration descriptor (9 bytes) */
static const uint8_t usb_config_descriptor[] = {
    /* Configuration descriptor */
    0x09,       /* bLength */
    0x02,       /* bDescriptorType: CONFIGURATION */
    0x43, 0x00, /* wTotalLength: 67 bytes */
    0x02,       /* bNumInterfaces: 2 (CDC control + CDC data) */
    0x01,       /* bConfigurationValue: 1 */
    0x00,       /* iConfiguration: no string */
    0x80,       /* bmAttributes: bus-powered */
    USB_MAX_POWER_MA / 2,  /* bMaxPower: 250 mA (in 2mA units) */

    /* Interface Association Descriptor (8 bytes) */
    0x08,       /* bLength */
    0x0B,       /* bDescriptorType: INTERFACE ASSOCIATION */
    0x00,       /* bFirstInterface: 0 */
    0x02,       /* bInterfaceCount: 2 */
    0x02,       /* bFunctionClass: CDC */
    0x02,       /* bFunctionSubClass: ACM */
    0x01,       /* bFunctionProtocol: V.25ter */
    0x04,       /* iFunction: String 4 */

    /* CDC Control Interface descriptor (9 bytes) */
    0x09,       /* bLength */
    0x04,       /* bDescriptorType: INTERFACE */
    0x00,       /* bInterfaceNumber: 0 */
    0x00,       /* bAlternateSetting: 0 */
    0x01,       /* bNumEndpoints: 1 (interrupt) */
    0x02,       /* bInterfaceClass: CDC */
    0x02,       /* bInterfaceSubClass: ACM */
    0x01,       /* bInterfaceProtocol: V.25ter */
    0x04,       /* iInterface: String 4 */

    /* CDC Header Functional Descriptor (5 bytes) */
    0x05,       /* bLength */
    0x24,       /* bDescriptorType: CS_INTERFACE */
    0x00,       /* bDescriptorSubtype: HEADER */
    0x10, 0x01, /* bcdCDC: 1.10 */

    /* CDC ACM Functional Descriptor (4 bytes) */
    0x04,       /* bLength */
    0x24,       /* bDescriptorType: CS_INTERFACE */
    0x02,       /* bDescriptorSubtype: ACM */
    0x02,       /* bmCapabilities: support Set_Line_Coding and Serial_State */

    /* CDC Union Functional Descriptor (5 bytes) */
    0x05,       /* bLength */
    0x24,       /* bDescriptorType: CS_INTERFACE */
    0x06,       /* bDescriptorSubtype: UNION */
    0x00,       /* bControlInterface: 0 */
    0x01,       /* bSubordinateInterface0: 1 */

    /* CDC Call Management Functional Descriptor (5 bytes) */
    0x05,       /* bLength */
    0x24,       /* bDescriptorType: CS_INTERFACE */
    0x01,       /* bDescriptorSubtype: CALL_MANAGEMENT */
    0x01,       /* bmCapabilities: call management */
    0x01,       /* bDataInterface: 1 */

    /* Endpoint 1 IN (Interrupt) descriptor (7 bytes) */
    0x07,       /* bLength */
    0x05,       /* bDescriptorType: ENDPOINT */
    EP1_IN,     /* bEndpointAddress: IN 1 */
    0x03,       /* bmAttributes: Interrupt */
    EP1_SIZE, 0x00, /* wMaxPacketSize */
    0x10,       /* bInterval: 16 ms */

    /* CDC Data Interface descriptor (9 bytes) */
    0x09,       /* bLength */
    0x04,       /* bDescriptorType: INTERFACE */
    0x01,       /* bInterfaceNumber: 1 */
    0x00,       /* bAlternateSetting: 0 */
    0x02,       /* bNumEndpoints: 2 (bulk IN and OUT) */
    0x0A,       /* bInterfaceClass: CDC Data */
    0x00,       /* bInterfaceSubClass */
    0x00,       /* bInterfaceProtocol */
    0x00,       /* iInterface: no string */

    /* Endpoint 2 OUT (Bulk) descriptor (7 bytes) */
    0x07,       /* bLength */
    0x05,       /* bDescriptorType: ENDPOINT */
    EP2_OUT,    /* bEndpointAddress: OUT 2 */
    0x02,       /* bmAttributes: Bulk */
    EP2_SIZE, 0x00, /* wMaxPacketSize */
    0x00,       /* bInterval: ignored for bulk */

    /* Endpoint 2 IN (Bulk) descriptor (7 bytes) */
    0x07,       /* bLength */
    0x05,       /* bDescriptorType: ENDPOINT */
    EP2_IN,     /* bEndpointAddress: IN 2 */
    0x02,       /* bmAttributes: Bulk */
    EP2_SIZE, 0x00, /* wMaxPacketSize */
    0x00,       /* bInterval: ignored for bulk */
};

/* ========================================================================
 * String Descriptors
 * ======================================================================== */

/* Language ID string (English US) */
static const uint8_t usb_string_lang[] = {
    0x04, 0x03,     /* bLength, bDescriptorType: STRING */
    0x09, 0x04      /* wLANGID: English (US) */
};

/* Manufacturer string */
static const uint8_t usb_string_manufacturer[] = {
    0x1E, 0x03,     /* bLength, bDescriptorType */
    'H', 0x00, 'a', 0x00, 'c', 0x00, 'k', 0x00,
    'e', 0x00, 'r', 0x00, ' ', 0x00,
    'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00,
    'c', 0x00, 'e', 0x00, 's', 0x00
};

/* Product string */
static const uint8_t usb_string_product[] = {
    0x34, 0x03,     /* bLength, bDescriptorType */
    'R', 0x00, 'F', 0x00, ' ', 0x00,
    'T', 0x00, 'r', 0x00, 'a', 0x00, 'n', 0x00, 's', 0x00,
    'c', 0x00, 'e', 0x00, 'i', 0x00, 'v', 0x00, 'e', 0x00,
    'r', 0x00, ' ', 0x00,
    'T', 0x00, 'o', 0x00, 'o', 0x00, 'l', 0x00
};

/* Serial number string */
static const uint8_t usb_string_serial[] = {
    0x1A, 0x03,     /* bLength, bDescriptorType */
    'R', 0x00, 'F', 0x00, 'T', 0x00, 'O', 0x00, 'O', 0x00,
    'L', 0x00, '0', 0x00, '0', 0x00, '0', 0x00, '1', 0x00
};

/* CDC interface string */
static const uint8_t usb_string_cdc_iface[] = {
    0x22, 0x03,     /* bLength, bDescriptorType */
    'C', 0x00, 'D', 0x00, 'C', 0x00, ' ', 0x00,
    'S', 0x00, 'e', 0x00, 'r', 0x00, 'i', 0x00, 'a', 0x00,
    'l', 0x00, ' ', 0x00,
    'P', 0x00, 'o', 0x00, 'r', 0x00, 't', 0x00
};

/* String descriptor pointer table */
static const uint8_t * const usb_string_descriptors[] = {
    usb_string_lang,          /* String 0: Language */
    usb_string_manufacturer,  /* String 1: Manufacturer */
    usb_string_product,       /* String 2: Product */
    usb_string_serial,        /* String 3: Serial */
    usb_string_cdc_iface     /* String 4: CDC Interface */
};

#endif /* USB_DESCRIPTORS_H */