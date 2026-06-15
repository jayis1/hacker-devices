/*
 * usb_descriptors.h — USB device descriptors for Infrared Phantom
 * VID: 0x1209 (pid.codes) PID: 0x1R01
 * Composite device: CDC (control) + Bulk (waveform) + HID (buttons)
 */

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

/* USB Vendor/Product IDs */
#define USBD_VID                    0x1209U
#define USBD_PID                    0x1R01U
#define USBD_LANGID                 0x0409U    /* English (US) */
#define USBD_MANUFACTURER_STR       "Infrared Phantom"
#define USBD_PRODUCT_STR            "IR Security Research Toolkit"
#define USBD_SERIAL_STR             "IRPH-00000001"
#define USBD_CONFIG_MAX_POWER       250U       /* 500 mA (in 2 mA units) */

/* USB version */
#define USBD_BCD_USB                0x0200U    /* USB 2.0 */
#define USBD_BCD_DEVICE             0x0101U    /* v1.1 */

/* Interface numbers */
#define USBD_ITF_CDC_CTRL           0U
#define USBD_ITF_CDC_DATA           1U
#define USBD_ITF_BULK                2U
#define USBD_ITF_HID                 3U
#define USBD_ITF_NUM                 4U

/* Endpoints */
#define USBD_EP_CDC_CTRL_OUT        0x01U     /* CDC control OUT (EP1) */
#define USBD_EP_CDC_CTRL_IN         0x81U     /* CDC control IN  (EP1) */
#define USBD_EP_CDC_DATA_OUT        0x02U     /* CDC data OUT     (EP2) */
#define USBD_EP_CDC_DATA_IN         0x82U     /* CDC data IN      (EP2) */
#define USBD_EP_BULK_OUT            0x03U     /* Bulk data OUT    (EP3) */
#define USBD_EP_BULK_IN             0x83U     /* Bulk data IN     (EP3) */
#define USBD_EP_HID_IN              0x84U     /* HID interrupt IN  (EP4) */

/* Endpoint sizes */
#define USBD_EP0_SIZE               64U       /* Control endpoint */
#define USBD_EP_CDC_MAX_SIZE        64U       /* CDC data endpoint */
#define USBD_EP_BULK_MAX_SIZE       512U      /* Bulk endpoint (HS) */
#define USBD_EP_HID_MAX_SIZE        64U       /* HID interrupt endpoint */
#define USBD_EP_BULK_MAX_SIZE_FS    64U       /* Bulk endpoint (FS) */

/* CDC interface class */
#define USB_CLASS_CDC                0x02U
#define USB_CLASS_CDC_DATA           0x0AU
#define CDC_SUBCLASS_ACM             0x02U     /* Abstract Control Model */
#define CDC_PROTOCOL_AT_COMMAND      0x01U     /* AT Commands (PSTN) */

/* HID interface class */
#define USB_CLASS_HID                 0x03U
#define HID_SUBCLASS_NONE             0x00U
#define HID_PROTOCOL_NONE             0x00U

/* IAD (Interface Association Descriptor) for CDC */
#define USBD_IAD_FIRST_INTERFACE      USBD_ITF_CDC_CTRL
#define USBD_IAD_INTERFACE_COUNT      2U
#define USBD_IAD_FUNCTION_CLASS       USB_CLASS_CDC
#define USBD_IAD_FUNCTION_SUBCLASS    CDC_SUBCLASS_ACM
#define USBD_IAD_FUNCTION_PROTOCOL    CDC_PROTOCOL_AT_COMMAND

/* String descriptor indices */
#define USBD_IDX_LANGID              0U
#define USBD_IDX_MANUFACTURER        1U
#define USBD_IDX_PRODUCT             2U
#define USBD_IDX_SERIAL              3U
#define USBD_IDX_CONFIG              4U
#define USBD_IDX_INTERFACE           5U

/* ========================================
 * Device Descriptor (18 bytes)
 * ======================================== */

static const uint8_t usbd_device_descriptor[] = {
    0x12,                           /* bLength: 18 bytes */
    0x01,                           /* bDescriptorType: DEVICE */
    (USBD_BCD_USB & 0xFF),          /* bcdUSB: 0x0200 (LSB) */
    (USBD_BCD_USB >> 8),            /* bcdUSB: 0x0200 (MSB) */
    0xEF,                           /* bDeviceClass: Misc */
    0x02,                           /* bDeviceSubClass: Common class */
    0x01,                           /* bDeviceProtocol: IAD */
    USBD_EP0_SIZE,                   /* bMaxPacketSize0: 64 */
    (USBD_VID & 0xFF),              /* idVendor: 0x1209 (LSB) */
    (USBD_VID >> 8),                /* idVendor: 0x1209 (MSB) */
    (USBD_PID & 0xFF),              /* idProduct: 0x1R01 (LSB) */
    (USBD_PID >> 8),                /* idProduct: 0x1R01 (MSB) */
    (USBD_BCD_DEVICE & 0xFF),       /* bcdDevice: 0x0101 (LSB) */
    (USBD_BCD_DEVICE >> 8),         /* bcdDevice: 0x0101 (MSB) */
    USBD_IDX_MANUFACTURER,           /* iManufacturer: 1 */
    USBD_IDX_PRODUCT,                /* iProduct: 2 */
    USBD_IDX_SERIAL,                 /* iSerialNumber: 3 */
    0x01                            /* bNumConfigurations: 1 */
};

/* ========================================
 * Configuration Descriptor (composite CDC + Bulk + HID)
 * ======================================== */

/* Total length calculation:
 *   Config: 9
 *   IAD: 8
 *   CDC Ctrl IF: 9 + 5 + 4 + 5 + 7 + 9 = 39
 *   CDC Data IF: 9 + 7 + 7 = 23
 *   Bulk IF: 9 + 7 + 7 = 23
 *   HID IF: 9 + 9 + 7 = 25
 *   Total: 9 + 8 + 39 + 23 + 23 + 25 = 127
 */

static const uint8_t usbd_config_descriptor[] = {
    /* Configuration Descriptor (9 bytes) */
    0x09,                           /* bLength */
    0x02,                           /* bDescriptorType: CONFIGURATION */
    0x7F, 0x00,                     /* wTotalLength: 127 bytes */
    USBD_ITF_NUM,                   /* bNumInterfaces: 4 */
    0x01,                           /* bConfigurationValue: 1 */
    USBD_IDX_CONFIG,                 /* iConfiguration: 4 */
    0x80,                           /* bmAttributes: Bus powered */
    USBD_CONFIG_MAX_POWER,           /* bMaxPower: 500 mA */

    /* IAD for CDC (8 bytes) */
    0x08,                           /* bLength */
    0x0B,                           /* bDescriptorType: IAD */
    USBD_IAD_FIRST_INTERFACE,       /* bFirstInterface: 0 */
    USBD_IAD_INTERFACE_COUNT,       /* bInterfaceCount: 2 */
    USBD_IAD_FUNCTION_CLASS,         /* bFunctionClass: CDC */
    USBD_IAD_FUNCTION_SUBCLASS,      /* bFunctionSubClass: ACM */
    USBD_IAD_FUNCTION_PROTOCOL,      /* bFunctionProtocol: AT Commands */
    USBD_IDX_INTERFACE,              /* iFunction: 5 */

    /* CDC Control Interface Descriptor (9 bytes) */
    0x09,                           /* bLength */
    0x04,                           /* bDescriptorType: INTERFACE */
    USBD_ITF_CDC_CTRL,              /* bInterfaceNumber: 0 */
    0x00,                           /* bAlternateSetting: 0 */
    0x01,                           /* bNumEndpoints: 1 (interrupt IN) */
    USB_CLASS_CDC,                   /* bInterfaceClass: CDC */
    CDC_SUBCLASS_ACM,                /* bInterfaceSubClass: ACM */
    CDC_PROTOCOL_AT_COMMAND,          /* bInterfaceProtocol: AT Commands */
    USBD_IDX_INTERFACE,              /* iInterface: 5 */

    /* CDC Header Functional Descriptor (5 bytes) */
    0x05,                           /* bLength */
    0x24,                           /* bDescriptorType: CS_INTERFACE */
    0x00,                           /* bDescriptorSubtype: Header */
    0x10, 0x01,                     /* bcdCDC: 1.10 */

    /* CDC ACM Functional Descriptor (4 bytes) */
    0x04,                           /* bLength */
    0x24,                           /* bDescriptorType: CS_INTERFACE */
    0x02,                           /* bDescriptorSubtype: ACM */
    0x02,                           /* bmCapabilities: Support Set_Line_Coding */

    /* CDC Union Functional Descriptor (5 bytes) */
    0x05,                           /* bLength */
    0x24,                           /* bDescriptorType: CS_INTERFACE */
    0x06,                           /* bDescriptorSubtype: Union */
    USBD_ITF_CDC_CTRL,              /* bMasterInterface: 0 */
    USBD_ITF_CDC_DATA,              /* bSlaveInterface: 1 */

    /* CDC Call Management Functional Descriptor (5 bytes) */
    0x05,                           /* bLength */
    0x24,                           /* bDescriptorType: CS_INTERFACE */
    0x01,                           /* bDescriptorSubtype: Call Management */
    0x00,                           /* bmCapabilities: None */
    USBD_ITF_CDC_DATA,              /* bDataInterface: 1 */

    /* CDC Interrupt IN Endpoint Descriptor (7 bytes) */
    0x07,                           /* bLength */
    0x05,                           /* bDescriptorType: ENDPOINT */
    USBD_EP_CDC_CTRL_IN,            /* bEndpointAddress: EP1 IN */
    0x03,                           /* bmAttributes: Interrupt */
    0x08, 0x00,                     /* wMaxPacketSize: 8 */
    0x10,                           /* bInterval: 16 ms */

    /* CDC Data Interface Descriptor (9 bytes) */
    0x09,                           /* bLength */
    0x04,                           /* bDescriptorType: INTERFACE */
    USBD_ITF_CDC_DATA,              /* bInterfaceNumber: 1 */
    0x00,                           /* bAlternateSetting: 0 */
    0x02,                           /* bNumEndpoints: 2 (bulk IN/OUT) */
    USB_CLASS_CDC_DATA,              /* bInterfaceClass: CDC Data */
    0x00,                           /* bInterfaceSubClass: None */
    0x00,                           /* bInterfaceProtocol: None */
    USBD_IDX_INTERFACE,              /* iInterface: 5 */

    /* CDC Bulk OUT Endpoint Descriptor (7 bytes) */
    0x07,                           /* bLength */
    0x05,                           /* bDescriptorType: ENDPOINT */
    USBD_EP_CDC_DATA_OUT,            /* bEndpointAddress: EP2 OUT */
    0x02,                           /* bmAttributes: Bulk */
    0x40, 0x00,                     /* wMaxPacketSize: 64 (FS) */
    0x00,                           /* bInterval: N/A for bulk */

    /* CDC Bulk IN Endpoint Descriptor (7 bytes) */
    0x07,                           /* bLength */
    0x05,                           /* bDescriptorType: ENDPOINT */
    USBD_EP_CDC_DATA_IN,             /* bEndpointAddress: EP2 IN */
    0x02,                           /* bmAttributes: Bulk */
    0x40, 0x00,                     /* wMaxPacketSize: 64 (FS) */
    0x00,                           /* bInterval: N/A for bulk */

    /* Bulk Data Interface Descriptor (9 bytes) */
    0x09,                           /* bLength */
    0x04,                           /* bDescriptorType: INTERFACE */
    USBD_ITF_BULK,                   /* bInterfaceNumber: 2 */
    0x00,                           /* bAlternateSetting: 0 */
    0x02,                           /* bNumEndpoints: 2 (bulk IN/OUT) */
    0xFF,                           /* bInterfaceClass: Vendor */
    0x01,                           /* bInterfaceSubClass: IR Data */
    0x01,                           /* bInterfaceProtocol: Raw Waveform */
    USBD_IDX_INTERFACE,              /* iInterface: 5 */

    /* Bulk OUT Endpoint Descriptor (7 bytes) */
    0x07,                           /* bLength */
    0x05,                           /* bDescriptorType: ENDPOINT */
    USBD_EP_BULK_OUT,                /* bEndpointAddress: EP3 OUT */
    0x02,                           /* bmAttributes: Bulk */
    0x00, 0x02,                     /* wMaxPacketSize: 512 (HS) */
    0x00,                           /* bInterval: N/A */

    /* Bulk IN Endpoint Descriptor (7 bytes) */
    0x07,                           /* bLength */
    0x05,                           /* bDescriptorType: ENDPOINT */
    USBD_EP_BULK_IN,                 /* bEndpointAddress: EP3 IN */
    0x02,                           /* bmAttributes: Bulk */
    0x00, 0x02,                     /* wMaxPacketSize: 512 (HS) */
    0x00,                           /* bInterval: N/A */

    /* HID Interface Descriptor (9 bytes) */
    0x09,                           /* bLength */
    0x04,                           /* bDescriptorType: INTERFACE */
    USBD_ITF_HID,                    /* bInterfaceNumber: 3 */
    0x00,                           /* bAlternateSetting: 0 */
    0x01,                           /* bNumEndpoints: 1 (interrupt IN) */
    USB_CLASS_HID,                   /* bInterfaceClass: HID */
    HID_SUBCLASS_NONE,               /* bInterfaceSubClass: None */
    HID_PROTOCOL_NONE,               /* bInterfaceProtocol: None */
    USBD_IDX_INTERFACE,              /* iInterface: 5 */

    /* HID Descriptor (9 bytes) */
    0x09,                           /* bLength */
    0x21,                           /* bDescriptorType: HID */
    0x11, 0x01,                     /* bcdHID: 1.11 */
    0x00,                           /* bCountryCode: Not localized */
    0x01,                           /* bNumDescriptors: 1 */
    0x22,                           /* bDescriptorType: Report */
    0x22, 0x00,                     /* wDescriptorLength: 34 bytes */

    /* HID Interrupt IN Endpoint Descriptor (7 bytes) */
    0x07,                           /* bLength */
    0x05,                           /* bDescriptorType: ENDPOINT */
    USBD_EP_HID_IN,                  /* bEndpointAddress: EP4 IN */
    0x03,                           /* bmAttributes: Interrupt */
    0x40, 0x00,                     /* wMaxPacketSize: 64 */
    0x01,                           /* bInterval: 1 ms */
};

/* ========================================
 * HID Report Descriptor
 * ======================================== */

static const uint8_t usbd_hid_report_descriptor[] = {
    0x05, 0x01,                     /* Usage Page (Generic Desktop) */
    0x09, 0x01,                     /* Usage (Vendor Defined) */
    0xA1, 0x01,                     /* Collection (Application) */
    /* Mode report: 1 byte */
    0x05, 0x01,                     /*   Usage Page (Generic Desktop) */
    0x09, 0x30,                     /*   Usage (X) — repurposed as mode */
    0x15, 0x00,                     /*   Logical Minimum (0) */
    0x25, 0x05,                     /*   Logical Maximum (5) */
    0x75, 0x08,                     /*   Report Size (8) */
    0x95, 0x01,                     /*   Report Count (1) */
    0x81, 0x02,                     /*   Input (Data, Variable, Absolute) */
    /* Button state: 1 byte */
    0x05, 0x09,                     /*   Usage Page (Button) */
    0x09, 0x01,                     /*   Usage (Button 1) */
    0x15, 0x00,                     /*   Logical Minimum (0) */
    0x25, 0x01,                     /*   Logical Maximum (1) */
    0x75, 0x01,                     /*   Report Size (1) */
    0x95, 0x01,                     /*   Report Count (1) */
    0x81, 0x02,                     /*   Input (Data, Variable, Absolute) */
    /* Frame count: 4 bytes */
    0x05, 0x01,                     /*   Usage Page (Generic Desktop) */
    0x09, 0x31,                     /*   Usage (Y) — frame count */
    0x15, 0x00,                     /*   Logical Minimum (0) */
    0x26, 0xFF, 0xFF,               /*   Logical Maximum (65535) */
    0x75, 0x20,                     /*   Report Size (32) */
    0x95, 0x01,                     /*   Report Count (1) */
    0x81, 0x02,                     /*   Input (Data, Variable, Absolute) */
    0xC0                            /* End Collection */
};

/* ========================================
 * String Descriptors
 * ======================================== */

/* Language ID descriptor (USB spec) */
static const uint8_t usbd_string_langid[] = {
    0x04,                           /* bLength: 4 bytes */
    0x03,                           /* bDescriptorType: STRING */
    0x09, 0x04                      /* wLANGID: 0x0409 (English - US) */
};

/* Manufacturer string */
static const uint8_t usbd_string_manufacturer[] = {
    0x22,                           /* bLength */
    0x03,                           /* bDescriptorType: STRING */
    'I', 0x00, 'n', 0x00, 'f', 0x00, 'r', 0x00, 'a', 0x00, 'r', 0x00,
    'e', 0x00, 'd', 0x00, ' ', 0x00, 'P', 0x00, 'h', 0x00, 'a', 0x00,
    'n', 0x00, 't', 0x00, 'o', 0x00, 'm', 0x00
};

/* Product string */
static const uint8_t usbd_string_product[] = {
    0x3E,                           /* bLength */
    0x03,                           /* bDescriptorType: STRING */
    'I', 0x00, 'R', 0x00, ' ', 0x00, 'S', 0x00, 'e', 0x00, 'c', 0x00,
    'u', 0x00, 'r', 0x00, 'i', 0x00, 't', 0x00, 'y', 0x00, ' ', 0x00,
    'R', 0x00, 'e', 0x00, 's', 0x00, 'e', 0x00, 'a', 0x00, 'r', 0x00,
    'c', 0x00, 'h', 0x00, ' ', 0x00, 'T', 0x00, 'o', 0x00, 'o', 0x00,
    'l', 0x00, 'k', 0x00, 'i', 0x00, 't', 0x00
};

/* Serial number string */
static const uint8_t usbd_string_serial[] = {
    0x22,                           /* bLength */
    0x03,                           /* bDescriptorType: STRING */
    'I', 0x00, 'R', 0x00, 'P', 0x00, 'H', 0x00, '-', 0x00, '0', 0x00,
    '0', 0x00, '0', 0x00, '0', 0x00, '0', 0x00, '0', 0x00, '1', 0x00,
    '2', 0x00, '6', 0x00
};

#endif /* USB_DESCRIPTORS_H */