/*
 * usb_iface.h — USB CDC serial interface for MagLance
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef USB_IFACE_H
#define USB_IFACE_H

#include "board.h"
#include <stdint.h>

/* Initialize USB CDC interface */
int usb_iface_init(void);

/* Process incoming commands (call from main loop) */
void usb_iface_poll(void);

/* Send a response string */
void usb_iface_send(const char *str);

/* Send formatted response (printf-style) */
void usb_iface_sendf(const char *fmt, ...);

/* Send binary data (for SENSE mode streaming) */
void usb_iface_send_binary(const uint8_t *data, uint16_t len);

/* Check if USB is connected */
int usb_iface_is_connected(void);

/* Command handler type */
typedef int (*command_handler_t)(int argc, char **argv);

/* Register a command handler */
void usb_iface_register_command(const char *cmd, command_handler_t handler);

#endif /* USB_IFACE_H */