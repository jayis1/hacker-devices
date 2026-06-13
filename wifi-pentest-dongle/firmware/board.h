/*
 * WFP-100 — Board Pin Definitions
 * StarFive JH7110 BGA-484 pin assignments for WiFi pentesting dongle.
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef WFP100_BOARD_H
#define WFP100_BOARD_H

#include "registers.h"

/* GPIO0 — System / Power / LEDs */
#define GPIO0_PWR_LED           0    /* Red LED (PWR), active-high */
#define GPIO0_ACT_LED           1    /* Green LED (ACT), active-high */
#define GPIO0_MON_LED           2    /* Blue LED (MON), active-high */
#define GPIO0_MODE_BTN          3    /* Mode button, active-low */
#define GPIO0_WIFI_KILL         4    /* WiFi RF kill, active-high */
#define GPIO0_VBUS_DET          5    /* USB VBUS detect, input */
#define GPIO0_BAT_LOW           6    /* Battery low, active-low */
#define GPIO0_RST_BTN           7    /* Reset button, active-low */

/* GPIO1 — PCIe / WiFi */
#define GPIO1_PCIE_RST          0    /* PCIe reset (AX210), active-low */
#define GPIO1_PCIE_CLKREQ       1    /* PCIe CLKREQ, active-low */
#define GPIO1_PCIE_WAKE         2    /* PCIe WAKE, active-low */
#define GPIO1_AX210_INT         3    /* AX210 interrupt, active-low */

/* GPIO2 — USB / Storage */
#define GPIO2_USB_ID            0    /* USB-C OTG ID */
#define GPIO2_USB_VBUS_EN       1    /* USB VBUS enable (OTG) */
#define GPIO2_EMMC_RST          2    /* eMMC reset, active-low */
#define GPIO2_SDIO_DET          3    /* microSD card detect */

/* LED blink patterns (ms) */
#define LED_PATTERN_IDLE        500
#define LED_PATTERN_CAPTURING   100
#define LED_PATTERN_INJECTING   50
#define LED_PATTERN_ERROR       2000

/* Mode button cycle */
typedef enum {
    MODE_IDLE = 0,
    MODE_MONITOR,
    MODE_INJECT,
    MODE_EVIL_TWIN,
    MODE_COUNT
} wfp_mode_t;

/* Power thresholds */
#define BATTERY_LOW_MV          3400
#define BATTERY_CRITICAL_MV     3100
#define USB_VBUS_VALID_MV       4700

#endif /* WFP100_BOARD_H */