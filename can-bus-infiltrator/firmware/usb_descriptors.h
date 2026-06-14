/*
 * usb_descriptors.h — USB CDC device descriptors for CAN Bus Infiltrator
 * VID: 0x1209 (pid.codes), PID: 0xC470
 */

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

#define USBD_VID        0x1209
#define USBD_PID        0xC470
#define USBD_LANGID     0x0409

#define USBD_CONFIG_NUM 1
#define USBD_ITF_NUM    2  /* CDC requires 2 interfaces */
#define USBD_STR_IDX_NUM 4

/* Device descriptor (18 bytes) */
static const uint8_t usbd_device_desc[] = {
    0x12,                         /* bLength */
    0x01,                         /* bDescriptorType = Device */
    0x00, 0x02,                   /* bcdUSB = 2.00 */
    0x02,                         /* bDeviceClass = CDC */
    0x00,                         /* bDeviceSubClass */
    0x00,                         /* bDeviceProtocol */
    0x40,                         /* bMaxPacketSize0 = 64 */
    0x09, 0x12,                   /* idVendor = 0x1209 */
    0x70, 0xC4,                   /* idProduct = 0xC470 */
    0x00, 0x01,                   /* bcdDevice = 1.00 */
    0x01,                         /* iManufacturer */
    0x02,                         /* iProduct */
    0x03,                         /* iSerialNumber */
    0x01                          /* bNumConfigurations */
};

/* Configuration descriptor (67 bytes total) */
static const uint8_t usbd_config_desc[] = {
    /* Configuration descriptor (9 bytes) */
    0x09,                         /* bLength */
    0x02,                         /* bDescriptorType = Configuration */
    0x43, 0x00,                   /* wTotalLength = 67 bytes */
    0x02,                         /* bNumInterfaces = 2 */
    0x01,                         /* bConfigurationValue */
    0x00,                         /* iConfiguration */
    0x80,                         /* bmAttributes = bus-powered */
    0xFA,                         /* bMaxPower = 500 mA */

    /* Interface Association Descriptor (8 bytes) */
    0x08,                         /* bLength */
    0x0B,                         /* bDescriptorType = IAD */
    0x00,                         /* bFirstInterface = 0 */
    0x02,                         /* bInterfaceCount = 2 */
    0x02,                         /* bFunctionClass = CDC */
    0x02,                         /* bFunctionSubClass = ACM */
    0x01,                         /* bFunctionProtocol = V.25ter */
    0x00,                         /* iFunction */

    /* CDC Control Interface descriptor (9 bytes) */
    0x09,                         /* bLength */
    0x04,                         /* bDescriptorType = Interface */
    0x00,                         /* bInterfaceNumber = 0 */
    0x00,                         /* bAlternateSetting */
    0x01,                         /* bNumEndpoints = 1 */
    0x02,                         /* bInterfaceClass = CDC */
    0x02,                         /* bInterfaceSubClass = ACM */
    0x01,                         /* bInterfaceProtocol = V.25ter */
    0x00,                         /* iInterface */

    /* CDC Header Functional Descriptor (5 bytes) */
    0x05, 0x24, 0x00, 0x10, 0x01,

    /* CDC ACM Functional Descriptor (4 bytes) */
    0x04, 0x24, 0x02, 0x02,

    /* CDC Union Functional Descriptor (5 bytes) */
    0x05, 0x24, 0x06, 0x00, 0x01,

    /* CDC Call Management Functional Descriptor (5 bytes) */
    0x05, 0x24, 0x01, 0x00, 0x01,

    /* CDC Notification Endpoint (7 bytes) */
    0x07,                         /* bLength */
    0x05,                         /* bDescriptorType = Endpoint */
    0x81,                         /* bEndpointAddress = EP1 IN */
    0x03,                         /* bmAttributes = Interrupt */
    0x08, 0x00,                   /* wMaxPacketSize = 8 */
    0x10,                         /* bInterval = 16 ms */

    /* CDC Data Interface descriptor (9 bytes) */
    0x09,                         /* bLength */
    0x04,                         /* bDescriptorType = Interface */
    0x01,                         /* bInterfaceNumber = 1 */
    0x00,                         /* bAlternateSetting */
    0x02,                         /* bNumEndpoints = 2 */
    0x0A,                         /* bInterfaceClass = CDC Data */
    0x00,                         /* bInterfaceSubClass */
    0x00,                         /* bInterfaceProtocol */
    0x00,                         /* iInterface */

    /* CDC Data OUT Endpoint (7 bytes) */
    0x07,                         /* bLength */
    0x05,                         /* bDescriptorType = Endpoint */
    0x02,                         /* bEndpointAddress = EP2 OUT */
    0x02,                         /* bmAttributes = Bulk */
    0x40, 0x00,                   /* wMaxPacketSize = 64 */
    0x00,                         /* bInterval */

    /* CDC Data IN Endpoint (7 bytes) */
    0x07,                         /* bLength */
    0x05,                         /* bDescriptorType = Endpoint */
    0x82,                         /* bEndpointAddress = EP2 IN */
    0x02,                         /* bmAttributes = Bulk */
    0x40, 0x00,                   /* wMaxPacketSize = 64 */
    0x00                          /* bInterval */
};

/* String descriptors */
static const uint8_t usbd_str_lang[]   = { 0x04, 0x03, 0x09, 0x04 };
static const uint8_t usbd_str_mfr[]    = { 22, 0x03,
    'h', 0, 'a', 0, 'c', 0, 'k', 0, 'd', 0, 'e', 0, 'v', 0, '.', 0, 'i', 0, 'o', 0 };
static const uint8_t usbd_str_prod[]   = { 30, 0x03,
    'C', 0, 'A', 0, 'N', 0, ' ', 0, 'I', 0, 'n', 0, 'f', 0, 'i', 0,
    'l', 0, 't', 0, 'r', 0, 'a', 0, 't', 0, 'o', 0, 'r', 0 };
static const uint8_t usbd_str_serial[] = { 20, 0x03,
    'C', 0, '4', 0, '7', 0, '0', 0, '-', 0, '0', 0, '0', 0, '0', 0, '0', 0, '1', 0 };

#endif /* USB_DESCRIPTORS_H */