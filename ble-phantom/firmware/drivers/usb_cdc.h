/*
 * usb_cdc.h — USB CDC ACM device driver for STM32F401
 *
 * Provides virtual serial port over USB for host communication.
 */

#ifndef USB_CDC_H
#define USB_CDC_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================
 * USB CDC configuration
 * ============================================================ */
#define USB_CDC_EP_SIZE     64
#define USB_CDC_BUF_SIZE    512

/* ============================================================
 * Callback type for received data
 * ============================================================ */
typedef void (*usb_rx_callback_t)(const uint8_t *data, uint16_t len);

/* ============================================================
 * Public API
 * ============================================================ */

/**
 * Initialize USB CDC device.
 * Configures OTG FS peripheral, sets up endpoints, enables interrupts.
 * @return 0 on success
 */
int usb_cdc_init(void);

/**
 * Send data over CDC virtual serial port.
 * Non-blocking; data is written to the IN FIFO.
 * @param data Data to send
 * @param len  Number of bytes to send (max 64 per call)
 * @return Number of bytes actually sent, or negative on error
 */
int usb_cdc_send(const uint8_t *data, uint16_t len);

/**
 * Register a callback for received data.
 * Called from interrupt context when data arrives on the OUT endpoint.
 * @param cb Callback function
 */
void usb_cdc_set_rx_callback(usb_rx_callback_t cb);

/**
 * Check if USB is configured (connected to host).
 * @return 1 if configured, 0 if not
 */
int usb_cdc_is_configured(void);

/**
 * USB ISR handler. Call from OTG_FS_IRQHandler().
 */
void usb_cdc_isr(void);

#endif /* USB_CDC_H */