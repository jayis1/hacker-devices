/*
 * PHANTOM — Board Configuration
 * RP2040 USB HID Emulation Payload Injector
 *
 * Copyright (C) 2024 Hacker Devices
 * Licensed under GPL-2.0
 *
 * WARNING: This device is for authorized security research only.
 * Unauthorized access to computer systems is illegal.
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stdbool.h>

/*
 * System clock configuration
 */
#define SYS_CLOCK_HZ        133000000   /* 133 MHz system clock */
#define USB_CLOCK_HZ        48000000    /* 48 MHz USB clock */
#define XOSC_FREQ_HZ        12000000    /* 12 MHz crystal */
#define PERI_CLOCK_HZ       133000000   /* 133 MHz peripheral clock */
#define UART_BAUD_RATE      115200      /* ESP32-C3 UART baud rate */
#define I2C_CLOCK_HZ        400000      /* I2C fast mode (400 kHz) */
#define SPI_CLOCK_HZ        50000000    /* SPI0 clock (50 MHz max) */

/*
 * GPIO Pin Assignments (RP2040 QFN-56)
 */

/* QSPI Boot Flash (GPIO 0-5) - not for general use */
#define PIN_QSPI_SCLK       0
#define PIN_QSPI_SS          1
#define PIN_QSPI_SD0         2
#define PIN_QSPI_SD1         3
#define PIN_QSPI_SD2         4
#define PIN_QSPI_SD3         5

/* Rotary Encoder */
#define PIN_ENC_A            6    /* Encoder phase A, interrupt-capable */
#define PIN_ENC_B            7    /* Encoder phase B, interrupt-capable */
#define PIN_ENC_BTN          8    /* Encoder push button, active-low */

/* Kill Switch */
#define PIN_KILL_SW_N        9    /* Hardware kill switch, active-low */

/* I2C0 (SSD1306 OLED) */
#define PIN_I2C_SDA          10   /* OLED data */
#define PIN_I2C_SCL          11   /* OLED clock */

/* UART0 (ESP32-C3) */
#define PIN_UART_TX          12   /* RP2040 TX → ESP32-C3 RX */
#define PIN_UART_RX          13   /* RP2040 RX ← ESP32-C3 TX */
#define PIN_ESP_RST_N        14   /* ESP32-C3 reset, active-low */

/* Status LED */
#define PIN_LED_DATA          15   /* WS2812B data line */
#define PIN_LED_EN            22   /* LED enable (HIGH = on) */

/* SPI0 (W25Q128 payload flash) */
#define PIN_FLASH_CS          16   /* SPI0 chip select */
#define PIN_FLASH_SCK         17   /* SPI0 clock */
#define PIN_FLASH_MOSI        18   /* SPI0 MOSI */
#define PIN_FLASH_MISO        19   /* SPI0 MISO */

/* USB Mode Control */
#define PIN_USB_MUX_CTRL      20   /* USB mode: HIGH=HID, LOW=MSC */

/* Battery ADC */
#define PIN_BAT_SENSE         21   /* Battery voltage divider (ADC1) */

/* Boot Button */
#define PIN_BOOT_BTN          24   /* Boot button, active-low */

/* USB (hardware pins, not GPIO-mapped) */
/* USB_DP and USB_DM are fixed-function pins on RP2040 */

/*
 * GPIO Function Select Values (for IO_BANK_0)
 */
#define GPIO_FUNC_XIP        0     /* QSPI flash */
#define GPIO_FUNC_SPI        1     /* SPI */
#define GPIO_FUNC_UART       2     /* UART */
#define GPIO_FUNC_I2C        3     /* I2C */
#define GPIO_FUNC_PWM        4     /* PWM */
#define GPIO_FUNC_SIO        5     /* Software GPIO */
#define GPIO_FUNC_PIO0       6     /* PIO0 */
#define GPIO_FUNC_PIO1       7     /* PIO1 */
#define GPIO_FUNC_CLOCK      8     /* Clock output */
#define GPIO_FUNC_USB        9     /* USB (fixed) */
#define GPIO_FUNC_NULL       0x1f  /* Unconnected */

/*
 * Pad Control Configuration
 */
#define PAD_SLEWFAST         (1 << 0)  /* Fast slew rate */
#define PAD_SCHMITT         (1 << 1)  /* Schmitt trigger enable */
#define PAD_PULLUP          (1 << 2)  /* Pull-up enable */
#define PAD_PULLDOWN        (1 << 3)  /* Pull-down enable */
#define PAD_DRIVE_2MA       (0 << 4)  /* 2 mA drive */
#define PAD_DRIVE_4MA       (1 << 4)  /* 4 mA drive */
#define PAD_DRIVE_8MA       (2 << 4)  /* 8 mA drive */
#define PAD_DRIVE_12MA      (3 << 4)  /* 12 mA drive */
#define PAD_IE              (1 << 6)  /* Input enable */
#define PAD_OD              (1 << 7)  /* Open drain */

/*
 * I2C OLED Configuration
 */
#define OLED_I2C_ADDR        0x3C  /* SSD1306 I2C address */
#define OLED_WIDTH            128
#define OLED_HEIGHT           64
#define OLED_PAGES            8     /* 64 / 8 = 8 pages */
#define OLED_FONT_WIDTH       6     /* 6x8 monospaced font */
#define OLED_FONT_HEIGHT      8

/*
 * SPI Flash Configuration (W25Q128JVSIQ)
 */
#define FLASH_SECTOR_SIZE     4096    /* 4 KB sectors */
#define FLASH_BLOCK_SIZE      65536   /* 64 KB blocks */
#define FLASH_PAGE_SIZE       256     /* 256 byte pages */
#define FLASH_TOTAL_SIZE      16777216  /* 16 MB */

/* Flash layout offsets */
#define FLASH_HEADER_OFFSET      0x000000  /* 4 KB: Header */
#define FLASH_INDEX_OFFSET       0x001000  /* 4 KB: Profile index */
#define FLASH_META_OFFSET        0x002000  /* 124 KB: Profile metadata */
#define FLASH_DATA_OFFSET        0x020000  /* 15.75 MB: Profile data */
#define FLASH_GEOFENCE_OFFSET   0xFF0000   /* 64 KB: Geofence config */
#define FLASH_WIFI_OFFSET       0xFFFC00   /* 1 KB: WiFi config */
#define FLASH_SETTINGS_OFFSET   0xFFFF00   /* 256 B: Device settings */

/* Flash JEDEC IDs */
#define FLASH_JEDEC_MANUFACTURER   0xEF    /* Winbond */
#define FLASH_JEDEC_DEVICE_HI      0x40    /* W25Q128 */
#define FLASH_JEDEC_DEVICE_LO      0x18    /* 16 MB */

/* Flash commands */
#define FLASH_CMD_READ_DATA        0x03
#define FLASH_CMD_FAST_READ        0x0B
#define FLASH_CMD_PAGE_PROGRAM     0x02
#define FLASH_CMD_SECTOR_ERASE     0x20
#define FLASH_CMD_BLOCK_ERASE_32K  0x52
#define FLASH_CMD_BLOCK_ERASE_64K  0xD8
#define FLASH_CMD_CHIP_ERASE       0xC7
#define FLASH_CMD_WRITE_ENABLE     0x06
#define FLASH_CMD_WRITE_DISABLE    0x04
#define FLASH_CMD_READ_STATUS1     0x05
#define FLASH_CMD_READ_STATUS2     0x35
#define FLASH_CMD_READ_STATUS3     0x15
#define FLASH_CMD_READ_JEDEC_ID    0x9F
#define FLASH_CMD_RELEASE_PWRDOWN  0xAB
#define FLASH_CMD_POWER_DOWN       0xB9

/*
 * USB Configuration
 */
#define USB_VID               0x1A86  /* Custom VID */
#define USB_PID_STEALTH       0xPH01  /* MSC mode PID */
#define USB_PID_HID           0xPH02  /* HID mode PID */
#define USB_LANGUAGE          0x0409  /* English (US) */
#define USB_MAX_POWER_MA      100     /* 100 mA bus-powered */

/* USB endpoints */
#define EP0_SIZE              64      /* Control endpoint max packet */
#define EP1_IN_SIZE           8       /* Keyboard report size */
#define EP2_IN_SIZE           4       /* Mouse report size */
#define EP3_IN_SIZE           2       /* Consumer control report size */
#define EP4_OUT_SIZE          1       /* Keyboard LED state */
#define EP5_IN_SIZE           64      /* MSC bulk-in max packet */
#define EP5_OUT_SIZE          64      /* MSC bulk-out max packet */

/* USB descriptor string indices */
#define USB_STR_INDEX_LANG    0
#define USB_STR_INDEX_MFR     1
#define USB_STR_INDEX_PRODUCT  2
#define USB_STR_INDEX_SERIAL   3

/*
 * Battery Configuration
 */
#define BAT_SENSE_CHANNEL     1       /* ADC1 = GPIO27 */
#define BAT_SENSE_GAIN         1       /* ADC gain = 1x */
#define BAT_VOLTAGE_DIVIDER    4.3     /* R3/(R3+R4) = 3.3k/(1k+3.3k) */
#define BAT_FULL_VOLTAGE       4.2    /* LiPo full charge */
#define BAT_EMPTY_VOLTAGE      3.0    /* LiPo cutoff */
#define BAT_CHARGE_MA          470    /* MCP73831 charge current */

/*
 * Kill Switch Configuration
 */
#define KILL_SWITCH_ACTIVE     false   /* true = kill mode (no USB data) */
#define KILL_SWITCH_GPIO       PIN_KILL_SW_N

/*
 * Rotary Encoder Configuration
 */
#define ENC_DEBOUNCE_MS        5       /* Debounce time */
#define ENC_LONG_PRESS_MS      2000   /* Long press threshold */
#define ENC_DOUBLE_PRESS_MS     300   /* Double press threshold */

/*
 * DuckyScript Parser Configuration
 */
#define DUCKY_MAX_LINE_LEN     256     /* Max line length */
#define DUCKY_MAX_REPEAT        1000   /* Max repeat count */
#define DUCKY_MAX_STRING_LEN    4096   /* Max STRING length */
#define DUCKY_MAX_PROFILES      128     /* Max stored profiles */
#define DUCKY_MAX_PROFILE_NAME  16     /* Max profile name length */
#define DUCKY_DEFAULT_DELAY      5     /* Default delay between keys (ms) */

/*
 * WiFi/BLE Configuration
 */
#define WIFI_UART_BAUD          115200
#define WIFI_SCAN_TIMEOUT_MS    5000
#define WIFI_CONNECT_TIMEOUT_MS 15000
#define BLE_DEVICE_NAME         "PHANTOM-01"
#define BLE_SERVICE_CONTROL     0xPH01
#define BLE_SERVICE_DATA        0xPH02
#define BLE_SERVICE_CONFIG      0xPH03

/*
 * LED Configuration (WS2812B)
 */
#define LED_COLOR_STEALTH       0x000020  /* Dim blue in MSC mode */
#define LED_COLOR_HID           0x002000  /* Green in HID mode */
#define LED_COLOR_EXECUTING     0x200000  /* Red during execution */
#define LED_COLOR_ERROR         0x400000  /* Bright red on error */
#define LED_COLOR_GEOFENCE      0x202000  /* Yellow during geofence check */
#define LED_COLOR_CHARGING      0x000002  /* Dim orange during charging */
#define LED_COLOR_KILL          0x000000  /* Off in kill mode */

/*
 * OLED Display Configuration
 */
#define OLED_SPLASH_TIME_MS     2000     /* Splash screen duration */
#define OLED_SCROLL_SPEED_MS    80       /* Scroll speed between items */
#define OLED_REFRESH_HZ          30      /* Display refresh rate */

/*
 * Profile Flags
 */
#define PROFILE_FLAG_ENCRYPTED    (1 << 0)
#define PROFILE_FLAG_AUTOEXEC     (1 << 1)
#define PROFILE_FLAG_GEOFENCED    (1 << 2)
#define PROFILE_FLAG_ENABLED      (1 << 3)

/*
 * Engine States
 */
typedef enum {
    STATE_STEALTH = 0,      /* USB MSC mode (appears as flash drive) */
    STATE_HID,              /* USB HID mode (keyboard/mouse/consumer) */
    STATE_EXECUTING,        /* Active payload execution */
    STATE_GEOFENCE,         /* Checking geofence before execution */
    STATE_ERROR,            /* Error state */
    STATE_SLEEP,            /* Low power sleep */
    STATE_KILL,             /* Kill switch activated, USB data disabled */
    STATE_BOOT,             /* Boot initialization */
} phantom_state_t;

/*
 * Profile structure
 */
typedef struct {
    uint32_t offset;          /* Start offset in flash data area */
    uint32_t size;            /* Payload size in bytes */
    uint32_t flags;           /* Profile flags (encrypted, autoexec, etc.) */
    char name[16];           /* Null-terminated profile name */
    uint32_t crc32;          /* Payload CRC32 integrity check */
} profile_entry_t;

/*
 * Profile metadata structure
 */
typedef struct {
    uint8_t index;            /* Profile index (0-127) */
    char name[16];            /* Profile name */
    char description[64];     /* Profile description */
    uint8_t icon;             /* Profile icon (enum) */
    uint32_t flags;           /* Profile flags */
    uint32_t last_executed;   /* Timestamp of last execution */
    uint32_t exec_count;      /* Number of times executed */
    uint32_t checksum;         /* Metadata checksum */
} profile_meta_t;

/*
 * HID Keyboard Report (8 bytes)
 */
typedef struct __attribute__((packed)) {
    uint8_t modifier;         /* Modifier keys (Ctrl, Shift, Alt, GUI) */
    uint8_t reserved;         /* Reserved, must be 0 */
    uint8_t keycode[6];       /* Keycodes (0 = no key) */
} hid_keyboard_report_t;

/* Keyboard modifier bits */
#define KEY_MOD_LCTRL    (1 << 0)
#define KEY_MOD_LSHIFT   (1 << 1)
#define KEY_MOD_LALT     (1 << 2)
#define KEY_MOD_LGUI     (1 << 3)
#define KEY_MOD_RCTRL    (1 << 4)
#define KEY_MOD_RSHIFT   (1 << 5)
#define KEY_MOD_RALT     (1 << 6)
#define KEY_MOD_RGUI     (1 << 7)

/*
 * HID Mouse Report (4 bytes)
 */
typedef struct __attribute__((packed)) {
    uint8_t buttons;          /* Button state (bit 0=left, 1=right, 2=middle) */
    int8_t x;                 /* X displacement (-127 to +127) */
    int8_t y;                 /* Y displacement (-127 to +127) */
    int8_t wheel;             /* Wheel displacement */
} hid_mouse_report_t;

/* Mouse button bits */
#define MOUSE_BTN_LEFT   (1 << 0)
#define MOUSE_BTN_RIGHT  (1 << 1)
#define MOUSE_BTN_MIDDLE (1 << 2)

/*
 * HID Consumer Control Report (2 bytes)
 */
typedef struct __attribute__((packed)) {
    uint16_t key;             /* Consumer key code */
} hid_consumer_report_t;

/* Consumer key codes */
#define CONSUMER_PLAY_PAUSE    0x00CD
#define CONSUMER_SCAN_NEXT     0x00B5
#define CONSUMER_SCAN_PREV     0x00B6
#define CONSUMER_STOP          0x00B7
#define CONSUMER_VOL_UP        0x00E9
#define CONSUMER_VOL_DOWN      0x00EA
#define CONSUMER_MUTE          0x00E2
#define CONSUMER_BASS_UP       0x00E5
#define CONSUMER_BASS_DOWN     0x00E6

/*
 * USB HID Keycodes (US keyboard layout)
 */
#define KEY_A          0x04
#define KEY_B          0x05
#define KEY_C          0x06
#define KEY_D          0x07
#define KEY_E          0x08
#define KEY_F          0x09
#define KEY_G          0x0A
#define KEY_H          0x0B
#define KEY_I          0x0C
#define KEY_J          0x0D
#define KEY_K          0x0E
#define KEY_L          0x0F
#define KEY_M          0x10
#define KEY_N          0x11
#define KEY_O          0x12
#define KEY_P          0x13
#define KEY_Q          0x14
#define KEY_R          0x15
#define KEY_S          0x16
#define KEY_T          0x17
#define KEY_U          0x18
#define KEY_V          0x19
#define KEY_W          0x1A
#define KEY_X          0x1B
#define KEY_Y          0x1C
#define KEY_Z          0x1D
#define KEY_1          0x1E
#define KEY_2          0x1F
#define KEY_3          0x20
#define KEY_4          0x21
#define KEY_5          0x22
#define KEY_6          0x23
#define KEY_7          0x24
#define KEY_8          0x25
#define KEY_9          0x26
#define KEY_0          0x27
#define KEY_ENTER      0x28
#define KEY_ESCAPE     0x29
#define KEY_BACKSPACE  0x2A
#define KEY_TAB        0x2B
#define KEY_SPACE      0x2C
#define KEY_MINUS      0x2D
#define KEY_EQUAL      0x2E
#define KEY_LBRACKET   0x2F
#define KEY_RBRACKET   0x30
#define KEY_BACKSLASH  0x31
#define KEY_SEMICOLON  0x33
#define KEY_QUOTE      0x34
#define KEY_GRAVE      0x35
#define KEY_COMMA      0x36
#define KEY_PERIOD     0x37
#define KEY_SLASH      0x38
#define KEY_CAPSLOCK   0x39
#define KEY_F1         0x3A
#define KEY_F2         0x3B
#define KEY_F3         0x3C
#define KEY_F4         0x3D
#define KEY_F5         0x3E
#define KEY_F6         0x3F
#define KEY_F7         0x40
#define KEY_F8         0x41
#define KEY_F9         0x42
#define KEY_F10        0x43
#define KEY_F11        0x44
#define KEY_F12        0x45
#define KEY_PRTSCR    0x46
#define KEY_SCROLLLOCK 0x47
#define KEY_PAUSE      0x48
#define KEY_INSERT     0x49
#define KEY_HOME       0x4A
#define KEY_PAGEUP     0x4B
#define KEY_DELETE     0x4C
#define KEY_END        0x4D
#define KEY_PAGEDOWN   0x4E
#define KEY_RIGHT      0x4F
#define KEY_LEFT       0x50
#define KEY_DOWN       0x51
#define KEY_UP         0x52
#define KEY_NUMLOCK    0x53

/*
 * Geofence configuration
 */
typedef struct {
    uint8_t mode;             /* 0=disabled, 1=whitelist, 2=blacklist */
    uint8_t network_count;    /* Number of network entries */
    uint8_t scan_interval_s; /* Seconds between WiFi scans */
    uint8_t fail_action;      /* 0=block, 1=log, 2=execute anyway */
    struct {
        char ssid[33];         /* Network SSID (max 32 chars + null) */
        uint8_t bssid[6];     /* BSSID */
        bool allowed;          /* true=allowed in whitelist, false=blocked */
    } networks[16];
} geofence_config_t;

/*
 * Device settings
 */
typedef struct {
    uint32_t magic;            /* Magic number for validation */
    uint32_t version;          /* Settings version */
    uint8_t default_profile;   /* Default profile index */
    uint8_t usb_mode;          /* 0=stealth, 1=HID */
    uint8_t default_delay;     /* Default delay between keys (ms) */
    uint8_t oled_brightness;   /* OLED brightness (0-255) */
    uint8_t led_brightness;    /* LED brightness (0-255) */
    uint8_t stealth_hide_oled; /* Hide OLED in stealth mode */
    uint8_t auto_hid_delay;    /* Seconds before auto-switch to HID (0=off) */
    char device_name[16];      /* BLE device name */
    uint32_t settings_crc;     /* CRC32 of settings */
} device_settings_t;

#define SETTINGS_MAGIC     0x504E5400  /* "PNT\0" */
#define SETTINGS_VERSION   0x00010000  /* v1.0 */

#endif /* BOARD_H */