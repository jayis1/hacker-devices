/*
 * PHANTOM — USB HID Driver Implementation
 * Keyboard, Mouse, and Consumer Control HID for RP2040
 *
 * Copyright (C) 2024 Hacker Devices
 * Licensed under GPL-2.0
 */

#include "usb_hid.h"
#include "registers.h"
#include "board.h"
#include "usb_descriptors.h"

/* ============================================================
 * USB Device State
 * ============================================================ */

typedef enum {
    USB_STATE_DISCONNECTED = 0,
    USB_STATE_DEFAULT,
    USB_STATE_ADDRESSED,
    USB_STATE_CONFIGURED,
} usb_state_t;

static volatile usb_state_t g_usb_state = USB_STATE_DISCONNECTED;
static volatile bool g_usb_hid_mode = false;  /* false=MSC, true=HID */
static volatile uint8_t g_usb_config = 0;
static volatile uint16_t g_usb_address = 0;

/* HID report buffers */
static hid_keyboard_report_t g_keyboard_report;
static hid_mouse_report_t g_mouse_report;
static hid_consumer_report_t g_consumer_report;
static uint8_t g_keyboard_led_state = 0;

/* USB setup packet buffer */
static volatile uint32_t g_setup_packet[4];  /* 8 bytes = 2 words */

/* ============================================================
 * Keycode Name Lookup
 * ============================================================ */

typedef struct {
    const char *name;
    uint8_t keycode;
} keycode_entry_t;

static const keycode_entry_t keycode_table[] = {
    {"A", KEY_A}, {"B", KEY_B}, {"C", KEY_C}, {"D", KEY_D},
    {"E", KEY_E}, {"F", KEY_F}, {"G", KEY_G}, {"H", KEY_H},
    {"I", KEY_I}, {"J", KEY_J}, {"K", KEY_K}, {"L", KEY_L},
    {"M", KEY_M}, {"N", KEY_N}, {"O", KEY_O}, {"P", KEY_P},
    {"Q", KEY_Q}, {"R", KEY_R}, {"S", KEY_S}, {"T", KEY_T},
    {"U", KEY_U}, {"V", KEY_V}, {"W", KEY_W}, {"X", KEY_X},
    {"Y", KEY_Y}, {"Z", KEY_Z},
    {"1", KEY_1}, {"2", KEY_2}, {"3", KEY_3}, {"4", KEY_4},
    {"5", KEY_5}, {"6", KEY_6}, {"7", KEY_7}, {"8", KEY_8},
    {"9", KEY_9}, {"0", KEY_0},
    {"ENTER", KEY_ENTER}, {"ESC", KEY_ESCAPE},
    {"ESCAPE", KEY_ESCAPE}, {"BACKSPACE", KEY_BACKSPACE},
    {"TAB", KEY_TAB}, {"SPACE", KEY_SPACE},
    {"MINUS", KEY_MINUS}, {"EQUAL", KEY_EQUAL},
    {"LBRACKET", KEY_LBRACKET}, {"RBRACKET", KEY_RBRACKET},
    {"BACKSLASH", KEY_BACKSLASH},
    {"SEMICOLON", KEY_SEMICOLON}, {"QUOTE", KEY_QUOTE},
    {"GRAVE", KEY_GRAVE},
    {"COMMA", KEY_COMMA}, {"PERIOD", KEY_PERIOD}, {"SLASH", KEY_SLASH},
    {"CAPSLOCK", KEY_CAPSLOCK},
    {"F1", KEY_F1}, {"F2", KEY_F2}, {"F3", KEY_F3}, {"F4", KEY_F4},
    {"F5", KEY_F5}, {"F6", KEY_F6}, {"F7", KEY_F7}, {"F8", KEY_F8},
    {"F9", KEY_F9}, {"F10", KEY_F10}, {"F11", KEY_F11}, {"F12", KEY_F12},
    {"PRTSCR", KEY_PRTSCR}, {"SCROLLLOCK", KEY_SCROLLLOCK},
    {"PAUSE", KEY_PAUSE},
    {"INSERT", KEY_INSERT}, {"HOME", KEY_HOME},
    {"PAGEUP", KEY_PAGEUP}, {"DELETE", KEY_DELETE},
    {"END", KEY_END}, {"PAGEDOWN", KEY_PAGEDOWN},
    {"UP", KEY_UP}, {"DOWN", KEY_DOWN},
    {"LEFT", KEY_LEFT}, {"RIGHT", KEY_RIGHT},
    {"NUMLOCK", KEY_NUMLOCK},
    {NULL, 0}  /* Sentinel */
};

typedef struct {
    const char *name;
    uint16_t keycode;
} consumer_keycode_entry_t;

static const consumer_keycode_entry_t consumer_keycode_table[] = {
    {"PLAY_PAUSE", CONSUMER_PLAY_PAUSE},
    {"SCAN_NEXT", CONSUMER_SCAN_NEXT},
    {"SCAN_PREV", CONSUMER_SCAN_PREV},
    {"STOP", CONSUMER_STOP},
    {"VOL_UP", CONSUMER_VOL_UP},
    {"VOL_DOWN", CONSUMER_VOL_DOWN},
    {"MUTE", CONSUMER_MUTE},
    {NULL, 0}  /* Sentinel */
};

/* ============================================================
 * USB Device Initialization
 * ============================================================ */

void usb_device_init(void) {
    /* Reset USB controller */
    RESETS_RESET |= RESETS_RESET_USB;
    busy_wait_us(100);
    RESETS_RESET &= ~RESETS_RESET_USB;
    while (!(RESETS_RESET_DONE & RESETS_RESET_USB));

    /* Clear all USB interrupts */
    USB_INTE = 0;
    USB_INTF = 0xFFFFFFFF;

    /* Enable USB controller */
    USB_MAIN_CTRL = USB_MAIN_CTRL_CONTROLLER_EN;

    /* Enable pull-up (connect to host) */
    USB_SIE_CTRL = USB_SIE_CTRL_PULLUP_EN;

    /* Enable interrupts */
    USB_INTE = USB_INTS_SETUP_REQ |
               USB_INTS_BUFF_STATUS |
               USB_INTS_BUS_RESET |
               USB_INTS_DEV_SUSPEND |
               USB_INTS_DEV_RESUME;

    /* Initialize report buffers */
    memset(&g_keyboard_report, 0, sizeof(g_keyboard_report));
    memset(&g_mouse_report, 0, sizeof(g_mouse_report));
    memset(&g_consumer_report, 0, sizeof(g_consumer_report));

    g_usb_state = USB_STATE_DEFAULT;
}

/* ============================================================
 * USB Enumeration
 * ============================================================ */

void usb_enumerate_msc(void) {
    g_usb_hid_mode = false;
    /* Disconnect and reconnect to force re-enumeration */
    USB_SIE_CTRL &= ~USB_SIE_CTRL_PULLUP_EN;
    busy_wait_ms(100);
    USB_SIE_CTRL |= USB_SIE_CTRL_PULLUP_EN;
    g_usb_state = USB_STATE_DEFAULT;
}

void usb_enumerate_hid(void) {
    g_usb_hid_mode = true;
    USB_SIE_CTRL &= ~USB_SIE_CTRL_PULLUP_EN;
    busy_wait_ms(100);
    USB_SIE_CTRL |= USB_SIE_CTRL_PULLUP_EN;
    g_usb_state = USB_STATE_DEFAULT;
}

void usb_switch_to_hid(void) {
    usb_enumerate_hid();
}

void usb_switch_to_msc(void) {
    usb_enumerate_msc();
}

void usb_disconnect(void) {
    USB_SIE_CTRL &= ~USB_SIE_CTRL_PULLUP_EN;
    g_usb_state = USB_STATE_DISCONNECTED;
}

/* ============================================================
 * USB Setup Request Handler
 * ============================================================ */

static void usb_handle_setup_request(void) {
    /* Read setup packet from buffer */
    uint8_t bmRequestType = g_setup_packet[0] & 0xFF;
    uint8_t bRequest = (g_setup_packet[0] >> 8) & 0xFF;
    uint16_t wValue = g_setup_packet[0] >> 16;
    uint16_t wIndex = g_setup_packet[1] & 0xFFFF;
    uint16_t wLength = g_setup_packet[1] >> 16;

    (void)wIndex;  /* May be used for interface-specific requests */

    /* Standard USB requests */
    uint8_t reqType = bmRequestType & 0x60;  /* Request type */
    uint8_t reqDir = bmRequestType & 0x80;    /* Direction */

    if (reqType == 0x00) {
        /* Standard requests */
        switch (bRequest) {
            case 0x00:  /* GET_STATUS */
                /* Device is bus-powered, no remote wakeup */
                USB_EP0_BUF_CTRL = (2 << USB_BUF_CTRL_LENGTH_SHIFT) | USB_BUF_CTRL_AVAILABLE;
                *(volatile uint16_t *)(USB_BASE + 0x100) = 0x0000;  /* Status: no errors */
                break;

            case 0x05:  /* SET_ADDRESS */
                g_usb_address = wValue;
                g_usb_state = USB_STATE_ADDRESSED;
                /* ACK the status phase */
                USB_EP0_BUF_CTRL = USB_BUF_CTRL_AVAILABLE;
                break;

            case 0x06:  /* GET_DESCRIPTOR */
                switch (wValue >> 8) {
                    case 0x01:  /* Device descriptor */
                        if (g_usb_hid_mode) {
                            USB_EP0_BUF_CTRL = (sizeof(hid_device_descriptor) << USB_BUF_CTRL_LENGTH_SHIFT) |
                                               USB_BUF_CTRL_AVAILABLE;
                            /* Copy descriptor to EP0 buffer */
                            memcpy((void *)(USB_BASE + 0x100), hid_device_descriptor,
                                   sizeof(hid_device_descriptor));
                        } else {
                            USB_EP0_BUF_CTRL = (sizeof(msc_device_descriptor) << USB_BUF_CTRL_LENGTH_SHIFT) |
                                               USB_BUF_CTRL_AVAILABLE;
                            memcpy((void *)(USB_BASE + 0x100), msc_device_descriptor,
                                   sizeof(msc_device_descriptor));
                        }
                        break;

                    case 0x02:  /* Configuration descriptor */
                        if (g_usb_hid_mode) {
                            USB_EP0_BUF_CTRL = (sizeof(hid_config_descriptor) << USB_BUF_CTRL_LENGTH_SHIFT) |
                                               USB_BUF_CTRL_AVAILABLE;
                            memcpy((void *)(USB_BASE + 0x100), hid_config_descriptor,
                                   sizeof(hid_config_descriptor));
                        } else {
                            USB_EP0_BUF_CTRL = (sizeof(msc_config_descriptor) << USB_BUF_CTRL_LENGTH_SHIFT) |
                                               USB_BUF_CTRL_AVAILABLE;
                            memcpy((void *)(USB_BASE + 0x100), msc_config_descriptor,
                                   sizeof(msc_config_descriptor));
                        }
                        break;

                    case 0x03:  /* String descriptor */
                        {
                            uint8_t index = wValue & 0xFF;
                            if (index < 4) {
                                const uint8_t *desc = string_descriptors[index];
                                uint8_t len = desc[0];
                                USB_EP0_BUF_CTRL = (len << USB_BUF_CTRL_LENGTH_SHIFT) |
                                                   USB_BUF_CTRL_AVAILABLE;
                                memcpy((void *)(USB_BASE + 0x100), desc, len);
                            }
                        }
                        break;

                    case 0x22:  /* HID Report descriptor */
                        switch (wIndex & 0xFF) {
                            case 0:  /* Keyboard */
                                USB_EP0_BUF_CTRL = (sizeof(keyboard_report_descriptor) << USB_BUF_CTRL_LENGTH_SHIFT) |
                                                   USB_BUF_CTRL_AVAILABLE;
                                memcpy((void *)(USB_BASE + 0x100), keyboard_report_descriptor,
                                       sizeof(keyboard_report_descriptor));
                                break;
                            case 1:  /* Mouse */
                                USB_EP0_BUF_CTRL = (sizeof(mouse_report_descriptor) << USB_BUF_CTRL_LENGTH_SHIFT) |
                                                   USB_BUF_CTRL_AVAILABLE;
                                memcpy((void *)(USB_BASE + 0x100), mouse_report_descriptor,
                                       sizeof(mouse_report_descriptor));
                                break;
                            case 2:  /* Consumer */
                                USB_EP0_BUF_CTRL = (sizeof(consumer_report_descriptor) << USB_BUF_CTRL_LENGTH_SHIFT) |
                                                   USB_BUF_CTRL_AVAILABLE;
                                memcpy((void *)(USB_BASE + 0x100), consumer_report_descriptor,
                                       sizeof(consumer_report_descriptor));
                                break;
                        }
                        break;
                }
                break;

            case 0x09:  /* SET_CONFIGURATION */
                g_usb_config = wValue;
                g_usb_state = USB_STATE_CONFIGURED;
                /* ACK the status phase */
                USB_EP0_BUF_CTRL = USB_BUF_CTRL_AVAILABLE;
                break;

            default:
                /* Stall EP0 for unsupported requests */
                USB_EP0_BUF_CTRL = 0;
                break;
        }
    } else if (reqType == 0x20 && reqDir == 0x00) {
        /* Class-specific OUT requests */
        /* HID SET_REPORT (keyboard LED state) */
        if (bRequest == 0x09 && wValue == 0x0200) {
            /* Keyboard LED state will be received in data phase */
            g_keyboard_led_state = 0;
        }
    }
}

/* ============================================================
 * USB Task (called from main loop)
 * ============================================================ */

void usb_task(void) {
    uint32_t ints = USB_INTS & USB_INTE;

    if (ints & USB_INTS_SETUP_REQ) {
        /* Copy setup packet */
        g_setup_packet[0] = USB_SETUP_PACKET;
        g_setup_packet[1] = *(volatile uint32_t *)(USB_BASE + 0x118);
        usb_handle_setup_request();
        USB_INTF = USB_INTS_SETUP_REQ;
    }

    if (ints & USB_INTS_BUS_RESET) {
        g_usb_state = USB_STATE_DEFAULT;
        g_usb_address = 0;
        g_usb_config = 0;
        USB_INTF = USB_INTS_BUS_RESET;
    }

    if (ints & USB_INTS_BUFF_STATUS) {
        /* Handle buffer status - process endpoint data */
        uint32_t buf_status = USB_BUFF_STATUS;

        /* Check EP4 OUT (keyboard LED state) */
        if (buf_status & (1 << (EP4_OUT + 16))) {
            /* Read LED state from EP4 */
            g_keyboard_led_state = *(volatile uint8_t *)(USB_BASE + 0x180);
        }

        USB_BUFF_STATUS = buf_status;  /* Clear all status */
    }
}

/* ============================================================
 * HID Keyboard Functions
 * ============================================================ */

void hid_keyboard_send(uint8_t modifier, uint8_t keycode) {
    g_keyboard_report.modifier = modifier;
    g_keyboard_report.reserved = 0;
    g_keyboard_report.keycode[0] = keycode;
    g_keyboard_report.keycode[1] = 0;
    g_keyboard_report.keycode[2] = 0;
    g_keyboard_report.keycode[3] = 0;
    g_keyboard_report.keycode[4] = 0;
    g_keyboard_report.keycode[5] = 0;

    /* Send report on EP1 IN */
    if (g_usb_state == USB_STATE_CONFIGURED && g_usb_hid_mode) {
        memcpy((void *)(USB_BASE + 0x100), &g_keyboard_report,
               sizeof(g_keyboard_report));
        USB_EP_BUF_CTRL(1) = (sizeof(g_keyboard_report) << USB_BUF_CTRL_LENGTH_SHIFT) |
                              USB_BUF_CTRL_AVAILABLE | USB_BUF_CTRL_LAST;
    }
}

void hid_keyboard_send_raw(const hid_keyboard_report_t *report) {
    if (g_usb_state == USB_STATE_CONFIGURED && g_usb_hid_mode) {
        memcpy((void *)(USB_BASE + 0x100), report, sizeof(*report));
        USB_EP_BUF_CTRL(1) = (sizeof(*report) << USB_BUF_CTRL_LENGTH_SHIFT) |
                              USB_BUF_CTRL_AVAILABLE | USB_BUF_CTRL_LAST;
    }
}

uint8_t hid_parse_keycode(const char *name) {
    if (!name || !*name) return 0;

    /* Convert to uppercase for comparison */
    char upper[16];
    int i;
    for (i = 0; name[i] && i < 15; i++) {
        upper[i] = (name[i] >= 'a' && name[i] <= 'z') ? (name[i] - 32) : name[i];
    }
    upper[i] = '\0';

    /* Single character */
    if (strlen(upper) == 1 && upper[0] >= 'A' && upper[0] <= 'Z') {
        return KEY_A + (upper[0] - 'A');
    }
    if (strlen(upper) == 1 && upper[0] >= '0' && upper[0] <= '9') {
        return KEY_1 + (upper[0] - '1');
    }

    /* Lookup table */
    for (i = 0; keycode_table[i].name != NULL; i++) {
        if (strcmp(upper, keycode_table[i].name) == 0) {
            return keycode_table[i].keycode;
        }
    }

    return 0;
}

/* ============================================================
 * HID Mouse Functions
 * ============================================================ */

void hid_mouse_move(int8_t dx, int8_t dy, int8_t wheel) {
    g_mouse_report.buttons = 0;
    g_mouse_report.x = dx;
    g_mouse_report.y = dy;
    g_mouse_report.wheel = wheel;

    if (g_usb_state == USB_STATE_CONFIGURED && g_usb_hid_mode) {
        memcpy((void *)(USB_BASE + 0x100), &g_mouse_report,
               sizeof(g_mouse_report));
        USB_EP_BUF_CTRL(2) = (sizeof(g_mouse_report) << USB_BUF_CTRL_LENGTH_SHIFT) |
                              USB_BUF_CTRL_AVAILABLE | USB_BUF_CTRL_LAST;
    }
}

void hid_mouse_click(uint8_t buttons) {
    g_mouse_report.buttons = buttons;
    g_mouse_report.x = 0;
    g_mouse_report.y = 0;
    g_mouse_report.wheel = 0;

    if (g_usb_state == USB_STATE_CONFIGURED && g_usb_hid_mode) {
        memcpy((void *)(USB_BASE + 0x100), &g_mouse_report,
               sizeof(g_mouse_report));
        USB_EP_BUF_CTRL(2) = (sizeof(g_mouse_report) << USB_BUF_CTRL_LENGTH_SHIFT) |
                              USB_BUF_CTRL_AVAILABLE | USB_BUF_CTRL_LAST;
    }
}

void hid_mouse_report(const hid_mouse_report_t *report) {
    if (g_usb_state == USB_STATE_CONFIGURED && g_usb_hid_mode) {
        memcpy((void *)(USB_BASE + 0x100), report, sizeof(*report));
        USB_EP_BUF_CTRL(2) = (sizeof(*report) << USB_BUF_CTRL_LENGTH_SHIFT) |
                              USB_BUF_CTRL_AVAILABLE | USB_BUF_CTRL_LAST;
    }
}

/* ============================================================
 * HID Consumer Control Functions
 * ============================================================ */

void hid_consumer_send(uint16_t key) {
    g_consumer_report.key = key;

    if (g_usb_state == USB_STATE_CONFIGURED && g_usb_hid_mode) {
        memcpy((void *)(USB_BASE + 0x100), &g_consumer_report,
               sizeof(g_consumer_report));
        USB_EP_BUF_CTRL(3) = (sizeof(g_consumer_report) << USB_BUF_CTRL_LENGTH_SHIFT) |
                              USB_BUF_CTRL_AVAILABLE | USB_BUF_CTRL_LAST;
    }
}

uint16_t hid_parse_consumer_keycode(const char *name) {
    if (!name || !*name) return 0;

    char upper[16];
    int i;
    for (i = 0; name[i] && i < 15; i++) {
        upper[i] = (name[i] >= 'a' && name[i] <= 'z') ? (name[i] - 32) : name[i];
    }
    upper[i] = '\0';

    for (i = 0; consumer_keycode_table[i].name != NULL; i++) {
        if (strcmp(upper, consumer_keycode_table[i].name) == 0) {
            return consumer_keycode_table[i].keycode;
        }
    }

    return 0;
}

/* ============================================================
 * HID Task (periodic)
 * ============================================================ */

void hid_task(void) {
    /* This is called from the main loop when in HID mode.
     * The actual HID reports are sent by the DuckyScript parser
     * via the hid_*_send functions. This function can be used
     * for periodic reports or idle handling. */
    static uint32_t last_idle = 0;
    uint32_t now = time_ms();

    /* Send idle report every 1 second if configured */
    if ((now - last_idle) > 1000) {
        last_idle = now;
        /* No action needed — reports are sent on demand */
    }
}