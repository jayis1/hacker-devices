/*
 * usb_data_switch.c — FSUSB42 USB 2.0 data switch driver
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Controls the FSUSB42 USB 2.0 high-speed (480 Mbps) switch that routes
 * the USB D+/D- signals between the source and sink USB-C ports.
 * When passthrough is enabled, USB data flows normally between the source
 * (host) and sink (target). When disabled, the USB data path is isolated,
 * forcing the target into a power-only connection.
 *
 * The FSUSB42 is controlled by two GPIO pins:
 *   PB5 (EN): 0 = enabled (switch active), 1 = disabled (high-impedance)
 *   PB4 (SEL): 0 = port A connected, 1 = port B connected
 *
 * For WattPhantom, "passthrough" means connecting source D+/D- to sink D+/D-.
 */

#include <stdint.h>
#include "board.h"
#include "registers.h"

/* ---- GPIO pin definitions ---- */
#define USB_SWITCH_EN_PIN   5   /* PB5 */
#define USB_SWITCH_SEL_PIN 4   /* PB4 */

/* ---- GPIO write helper ---- */
static void gpio_write_pb(uint8_t pin, uint8_t val) {
    volatile uint32_t *bsrr = (volatile uint32_t *)(GPIOB_BASE + GPIO_BSRR_OFFSET);
    if (val) {
        *bsrr = (1u << pin);
    } else {
        *bsrr = (1u << (pin + 16));
    }
}

/* ---- Init USB data switch ---- */
void usb_data_switch_init(void) {
    /* Default: disabled (isolated) */
    gpio_write_pb(USB_SWITCH_EN_PIN, 1);  /* Disabled */
    gpio_write_pb(USB_SWITCH_SEL_PIN, 0); /* Port A selected */
}

/* ---- Enable/disable USB data passthrough ---- */
void usb_data_switch_set_passthrough(uint8_t enable) {
    if (enable) {
        /* Enable switch: EN = 0 (active low), SEL = 0 (port A → B) */
        gpio_write_pb(USB_SWITCH_SEL_PIN, 0);
        gpio_write_pb(USB_SWITCH_EN_PIN, 0);  /* Enable */
    } else {
        /* Disable switch: EN = 1 (high-impedance) */
        gpio_write_pb(USB_SWITCH_EN_PIN, 1);  /* Disable */
    }
}