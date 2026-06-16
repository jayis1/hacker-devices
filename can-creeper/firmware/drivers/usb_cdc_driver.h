/**
 * @file usb_cdc_driver.h
 * @brief USB CDC-ACM Driver API
 *
 * Provides wired communication between CAN Creeper and a host computer.
 * Implements USB Communications Device Class Abstract Control Model (CDC-ACM).
 *
 * USB Hardware: nRF52840 USBD peripheral
 * Speed: Full Speed 12 Mbps
 * Class: CDC (0x02), Subclass: ACM (0x02)
 * VID: 0x1915 (Nordic Semiconductor)
 * PID: 0xCAFE (CAN Creeper)
 *
 * Endpoint Configuration:
 *   EP0: Control (required)
 *   EP1 IN:  Bulk, 64-byte max packet (CDC data to host)
 *   EP1 OUT: Bulk, 64-byte max packet (CDC data from host)
 *   EP2 IN:  Interrupt, 16-byte max packet, 64ms interval (CDC notifications)
 *
 * Line Coding (reported to host, actual data is raw USB):
 *   Baud: 921600, Data: 8, Stop: 1, Parity: None
 */

#ifndef USB_CDC_DRIVER_H
#define USB_CDC_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * USB DESCRIPTOR CONSTANTS
 *===========================================================================*/

/** USB Vendor and Product IDs */
#define USB_CDC_VID                 0x1915  /* Nordic Semiconductor */
#define USB_CDC_PID                 0xCAFE  /* CAN Creeper */
#define USB_CDC_BCD_DEVICE          0x0100  /* Device Release 1.0 */

/** USB String Descriptor Indices */
#define USB_CDC_STRING_LANGID       0
#define USB_CDC_STRING_MANUFACTURER 1
#define USB_CDC_STRING_PRODUCT      2
#define USB_CDC_STRING_SERIAL       3
#define USB_CDC_STRING_CDC_INTF     4

/** String Descriptor Content */
#define USB_CDC_MANUFACTURER_STRING  "Nous Research"
#define USB_CDC_PRODUCT_STRING       "CAN Creeper"

/** CDC-ACM Interface Numbers */
#define USB_CDC_INTF_CONTROL         0   /* Communication Class Interface */
#define USB_CDC_INTF_DATA            1   /* Data Class Interface */

/** Endpoint Addresses */
#define USB_CDC_EP_BULK_IN           0x81  /* EP1 IN  (device → host) */
#define USB_CDC_EP_BULK_OUT          0x02  /* EP1 OUT (host → device) */
#define USB_CDC_EP_INTERRUPT_IN      0x83  /* EP2 IN  (notifications) */

/** Endpoint Max Packet Sizes */
#define USB_CDC_EP0_MAX_PACKET       64    /* Control endpoint */
#define USB_CDC_EP_BULK_MAX_PACKET   64    /* Bulk endpoints */
#define USB_CDC_EP_INTERRUPT_MAX_PACKET 16 /* Interrupt endpoint */
#define USB_CDC_EP_INTERRUPT_INTERVAL   16 /* 16 × 4ms = 64ms polling interval */

/** CDC-ACM Class-Specific Request Codes */
#define USB_CDC_REQ_SEND_ENCAPSULATED_COMMAND   0x00
#define USB_CDC_REQ_GET_ENCAPSULATED_RESPONSE   0x01
#define USB_CDC_REQ_SET_LINE_CODING             0x20
#define USB_CDC_REQ_GET_LINE_CODING             0x21
#define USB_CDC_REQ_SET_CONTROL_LINE_STATE      0x22
#define USB_CDC_REQ_SEND_BREAK                  0x23

/** CDC-ACM Notification Codes */
#define USB_CDC_NOTIFY_NETWORK_CONNECTION       0x00
#define USB_CDC_NOTIFY_RESPONSE_AVAILABLE       0x01
#define USB_CDC_NOTIFY_SERIAL_STATE             0x20

/** Serial State Bitmap */
#define USB_CDC_SERIAL_STATE_DCD                (1 << 0)
#define USB_CDC_SERIAL_STATE_DSR                (1 << 1)
#define USB_CDC_SERIAL_STATE_BREAK              (1 << 2)
#define USB_CDC_SERIAL_STATE_RING               (1 << 3)
#define USB_CDC_SERIAL_STATE_FRAMING_ERROR      (1 << 4)
#define USB_CDC_SERIAL_STATE_PARITY_ERROR       (1 << 5)
#define USB_CDC_SERIAL_STATE_OVERRUN            (1 << 6)

/** Line Coding Defaults */
#define USB_CDC_DEFAULT_BAUD_RATE    921600
#define USB_CDC_DEFAULT_DATA_BITS    8
#define USB_CDC_DEFAULT_STOP_BITS    0   /* 0 = 1 stop bit */
#define USB_CDC_DEFAULT_PARITY       0   /* 0 = None */

/** Line Coding Structure (7 bytes) */
typedef struct __attribute__((packed)) {
    uint32_t dwDTERate;     /* Baud rate in bits per second */
    uint8_t  bCharFormat;   /* 0=1 stop, 1=1.5 stop, 2=2 stop */
    uint8_t  bParityType;   /* 0=None, 1=Odd, 2=Even, 3=Mark, 4=Space */
    uint8_t  bDataBits;     /* 5, 6, 7, 8, or 16 */
} usb_cdc_line_coding_t;

/** Control Line State from Host */
typedef struct __attribute__((packed)) {
    uint16_t wValue;  /* Bit 0: DTR, Bit 1: RTS */
} usb_cdc_control_line_state_t;

/** Serial State Notification (10 bytes) */
typedef struct __attribute__((packed)) {
    uint8_t  bmRequestType;  /* 0xA1 */
    uint8_t  bNotification;  /* SERIAL_STATE = 0x20 */
    uint16_t wValue;         /* 0x0000 */
    uint16_t wIndex;         /* Interface number */
    uint16_t wLength;        /* 0x0002 */
    uint16_t data;           /* Serial state bitmap */
} usb_cdc_serial_state_t;

/*===========================================================================
 * USB DESCRIPTOR SIZES
 *===========================================================================*/

#define USB_CDC_DEVICE_DESC_SIZE        18
#define USB_CDC_CONFIG_DESC_SIZE        75  /* Total config: CDC header + ACM + Union + CSE + 2×EP + Data IF + 2×EP */
#define USB_CDC_STRING_LANGID_SIZE      4
#define USB_CDC_STRING_MANUF_SIZE       (2 + sizeof(USB_CDC_MANUFACTURER_STRING) * 2)
#define USB_CDC_STRING_PRODUCT_SIZE     (2 + sizeof(USB_CDC_PRODUCT_STRING) * 2)
#define USB_CDC_STRING_SERIAL_SIZE      26  /* 12-character hex serial + header */

/*===========================================================================
 * CALLBACK TYPES
 *===========================================================================*/

/** Callback for received USB data (host → device) */
typedef void (*usb_cdc_rx_callback_t)(const uint8_t *data, uint16_t len);

/** Callback for completed USB transmission (device → host) */
typedef void (*usb_cdc_tx_complete_callback_t)(void);

/** Callback for USB connection state changes */
typedef void (*usb_cdc_state_callback_t)(bool connected);

/*===========================================================================
 * CONFIGURATION
 *===========================================================================*/

typedef struct {
    usb_cdc_rx_callback_t          rx_callback;
    usb_cdc_tx_complete_callback_t tx_complete_callback;
    usb_cdc_state_callback_t       state_callback;
} usb_cdc_config_t;

/*===========================================================================
 * ERROR CODES
 *===========================================================================*/

#define USB_CDC_ERR_OK            0
#define USB_CDC_ERR_NOT_INIT     -1
#define USB_CDC_ERR_PARAM        -2
#define USB_CDC_ERR_NOT_CONNECTED -3
#define USB_CDC_ERR_TX_BUSY      -4
#define USB_CDC_ERR_TX_FULL      -5
#define USB_CDC_ERR_USBD         -6
#define USB_CDC_ERR_ENUMERATION  -7

/*===========================================================================
 * API FUNCTIONS
 *===========================================================================*/

/**
 * @brief Initialize USB CDC-ACM device
 *
 * Configures nRF52840 USBD peripheral, sets up descriptors,
 * enables D+ pull-up for host detection.
 *
 * @param config Pointer to configuration with callbacks
 * @return USB_CDC_ERR_OK on success
 */
int usb_cdc_init(const usb_cdc_config_t *config);

/**
 * @brief Deinitialize USB CDC-ACM
 *
 * Disables USBD peripheral and D+ pull-up.
 *
 * @return USB_CDC_ERR_OK on success
 */
int usb_cdc_deinit(void);

/**
 * @brief Transmit data over USB CDC (device → host)
 *
 * Queues data for transmission on EP1 IN (bulk).
 * Data is sent when host issues IN token.
 *
 * @param data Data to transmit
 * @param len  Data length (max 64 bytes per packet, larger data is split)
 * @return USB_CDC_ERR_OK on success
 */
int usb_cdc_tx(const uint8_t *data, uint16_t len);

/**
 * @brief Get number of pending TX bytes
 *
 * @return Number of bytes queued for transmission
 */
int usb_cdc_tx_pending(void);

/**
 * @brief Check if USB is connected (enumerated)
 *
 * @return true if USB is connected and configured
 */
bool usb_cdc_is_connected(void);

/**
 * @brief Get current line coding from host
 *
 * @param coding Pointer to store line coding
 * @return USB_CDC_ERR_OK on success
 */
int usb_cdc_get_line_coding(usb_cdc_line_coding_t *coding);

/**
 * @brief Send serial state notification to host
 *
 * @param state Serial state bitmap
 * @return USB_CDC_ERR_OK on success
 */
int usb_cdc_send_serial_state(uint16_t state);

/**
 * @brief Handle USBD peripheral events
 *
 * Called from USBD interrupt handler. Processes setup packets,
 * endpoint transfers, and bus events.
 *
 * @param event USBD event structure
 */
void usb_cdc_event_handler(const void *event);

#ifdef __cplusplus
}
#endif

#endif /* USB_CDC_DRIVER_H */
