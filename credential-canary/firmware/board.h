/* Credential Canary board definition
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef CREDENTIAL_CANARY_BOARD_H
#define CREDENTIAL_CANARY_BOARD_H
#include <stdint.h>
#include <stdbool.h>

#define FW_VERSION_MAJOR 1u
#define FW_VERSION_MINOR 0u
#define SYSTEM_CLOCK_HZ 170000000u
#define APB_CLOCK_HZ 85000000u
#define CAPTURE_CAPACITY 1024u
#define EVENT_DATA_MAX 32u
#define COMMAND_MAX 96u
#define WIEGAND_GAP_US 25000u
#define WIEGAND_GLITCH_US 15u
#define OSDP_BAUD_DEFAULT 9600u
#define OSDP_FRAME_MAX 256u
#define FAILSAFE_WATCHDOG_MS 100u

#define PIN_WA_D0 0u
#define PIN_WA_D1 1u
#define PIN_WB_D0 2u
#define PIN_WB_D1 3u
#define PIN_TAMPER 4u
#define PIN_BYPASS 5u
#define PIN_LED_R 6u
#define PIN_LED_G 7u
#define PIN_LED_B 8u
#define PIN_RS485_A_DE 9u
#define PIN_RS485_B_DE 10u
#define PIN_AUTH 11u

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define MIN_U32(a,b) ((uint32_t)(a) < (uint32_t)(b) ? (uint32_t)(a) : (uint32_t)(b))

typedef enum { MODE_SAFE_BYPASS=0, MODE_MONITOR, MODE_BRIDGE, MODE_LAB_TEST } operating_mode_t;
typedef enum { IFACE_SYSTEM=0, IFACE_WIEGAND_A, IFACE_WIEGAND_B, IFACE_OSDP_READER, IFACE_OSDP_PANEL } interface_t;
typedef enum { EVT_BOOT=1, EVT_WIEGAND_BIT, EVT_WIEGAND_FRAME, EVT_OSDP_FRAME, EVT_TAMPER, EVT_FAULT, EVT_MARKER, EVT_POLICY } event_type_t;

typedef struct {
    uint32_t timestamp_us;
    uint16_t sequence;
    uint8_t type;
    uint8_t interface_id;
    uint8_t length;
    uint8_t flags;
    uint8_t data[EVENT_DATA_MAX];
} capture_event_t;

typedef struct {
    operating_mode_t mode;
    bool armed;
    bool usb_connected;
    bool ble_connected;
    bool capture_overrun;
    uint32_t uptime_ms;
    uint32_t wiegand_frames;
    uint32_t osdp_frames;
    uint32_t malformed_frames;
    uint32_t policy_alerts;
} system_status_t;

void board_init(void);
uint32_t board_micros(void);
uint32_t board_millis(void);
void board_delay_us(uint32_t us);
void board_feed_watchdog(void);
void board_set_led(uint8_t r, uint8_t g, uint8_t b);
void board_set_bypass(bool bypass);
bool board_tamper_asserted(void);
bool board_authorization_asserted(void);
void board_usb_write(const uint8_t *data, uint16_t length);
int board_usb_read(uint8_t *data, uint16_t capacity);
void board_rs485_drive(interface_t iface, bool enabled);
void board_rs485_write(interface_t iface, const uint8_t *data, uint16_t length);

#endif
