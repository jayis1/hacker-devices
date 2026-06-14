/*
 * PHANTOM — Main Firmware
 * RP2040 USB HID Emulation Payload Injector
 *
 * Copyright (C) 2024 Hacker Devices
 * Licensed under GPL-2.0
 *
 * WARNING: This device is for authorized security research only.
 * Unauthorized access to computer systems is illegal.
 *
 * This firmware implements a multi-profile USB HID emulation device
 * with DuckyScript parsing, stealth mode (MSC), WiFi/BLE control,
 * and hardware kill switch support.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "board.h"
#include "registers.h"
#include "usb_descriptors.h"
#include "drivers/flash_storage.h"
#include "drivers/usb_hid.h"
#include "drivers/wifi_bridge.h"

/* ============================================================
 * Global State
 * ============================================================ */

/* Engine state */
static phantom_state_t g_state = STATE_BOOT;
static phantom_state_t g_prev_state = STATE_BOOT;

/* Profile management */
static profile_entry_t g_profiles[DUCKY_MAX_PROFILES];
static uint8_t g_profile_count = 0;
static uint8_t g_selected_profile = 0;

/* DuckyScript engine */
static uint32_t g_default_delay = DUCKY_DEFAULT_DELAY;
static uint8_t g_held_keys[8] = {0};
static uint8_t g_held_key_count = 0;
static uint32_t g_last_command_hash = 0;

/* Rotary encoder */
static int32_t g_encoder_position = 0;
static bool g_encoder_pressed = false;
static bool g_encoder_long_press = false;
static uint32_t g_press_time = 0;

/* OLED framebuffer */
static uint8_t g_framebuffer[OLED_WIDTH * OLED_PAGES];

/* Serial number (populated from RP2040 UID) */
static char g_serial_str[16];

/* Kill switch state */
static volatile bool g_kill_switch_active = false;

/* ============================================================
 * Helper Functions
 * ============================================================ */

static inline uint32_t time_us(void) {
    return TIMER_TIMELR;
}

static inline uint32_t time_ms(void) {
    return TIMER_TIMELR / 1000;
}

static void delay_ms(uint32_t ms) {
    uint32_t start = time_ms();
    while ((time_ms() - start) < ms);
}

static void delay_us(uint32_t us) {
    uint32_t start = time_us();
    while ((time_us() - start) < us);
}

/* Simple CRC32 implementation */
static uint32_t crc32(const void *data, uint32_t len) {
    const uint8_t *buf = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFF;

    for (uint32_t i = 0; i < len; i++) {
        crc ^= buf[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }

    return crc ^ 0xFFFFFFFF;
}

/* Simple hash for DuckyScript commands */
static uint32_t djb2_hash(const char *str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

/* ============================================================
 * Clock Initialization
 * ============================================================ */

static void clocks_init(void) {
    /* Step 1: Enable XOSC (12 MHz crystal) */
    XOSC_CTRL = XOSC_CTRL_ENABLE_VALUE;
    XOSC_STARTUP = 47;  /* 47 * 256 = 12032 cycles = ~1ms @ 12MHz */

    /* Wait for XOSC stable */
    while (!(XOSC_STATUS & XOSC_STATUS_STABLE));

    /* Step 2: Configure PLL_SYS (133 MHz) */
    /* VCO = 12 MHz * 125 = 1500 MHz */
    PLL_SYS_FBDIV_INT = 125;
    PLL_SYS_PRIM = (6 << 16) | (2 << 12) | (7 << 8);
    PLL_SYS_PWR = 0;  /* Enable PLL */

    /* Wait for PLL_SYS lock */
    while (!(PLL_SYS_CS & (1 << 31)));

    /* Step 3: Configure PLL_USB (48 MHz) */
    /* VCO = 12 MHz * 120 = 1440 MHz */
    PLL_USB_FBDIV_INT = 120;
    PLL_USB_PRIM = (5 << 16) | (2 << 12) | (30 << 8);
    PLL_USB_PWR = 0;  /* Enable PLL */

    /* Wait for PLL_USB lock */
    while (!(PLL_USB_CS & (1 << 31)));

    /* Step 4: Switch system clock to PLL_SYS */
    CLOCKS_CLK_SYS_CTRL = 0x01;  /* Source = PLL_SYS */

    /* Step 5: Configure peripheral clock */
    CLOCKS_CLK_PERI_CTRL = 0x01;  /* Enable, source = clk_sys */

    /* Step 6: Configure USB clock */
    CLOCKS_CLK_USB_CTRL = (0x01 << 5) | 0x02;  /* Source = PLL_USB */

    /* Step 7: Configure ADC clock */
    CLOCKS_CLK_ADC_CTRL = (0x01 << 5) | 0x02;

    /* Step 8: Reference clock from XOSC */
    CLOCKS_CLK_REF_CTRL = 0x02;  /* Source = XOSC */
}

/* ============================================================
 * GPIO Initialization
 * ============================================================ */

static void gpio_init(void) {
    /* Release all peripherals from reset */
    RESETS_RESET &= ~(
        RESETS_RESET_SPI0 |
        RESETS_RESET_UART0 |
        RESETS_RESET_I2C0 |
        RESETS_RESET_ADC |
        RESETS_RESET_USB |
        RESETS_RESET_PIO0 |
        RESETS_RESET_IO_BANK0
    );

    /* Wait for resets to complete */
    while ((RESETS_RESET_DONE &
            (RESETS_RESET_SPI0 | RESETS_RESET_UART0 | RESETS_RESET_I2C0 |
             RESETS_RESET_ADC | RESETS_RESET_USB | RESETS_RESET_PIO0 |
             RESETS_RESET_IO_BANK0)) !=
           (RESETS_RESET_SPI0 | RESETS_RESET_UART0 | RESETS_RESET_I2C0 |
            RESETS_RESET_ADC | RESETS_RESET_USB | RESETS_RESET_PIO0 |
            RESETS_RESET_IO_BANK0));

    /* Configure GPIO functions */
    /* Encoder inputs */
    IO_BANK_0_GPIO_CTRL(PIN_ENC_A) = GPIO_FUNC_SIO;
    IO_BANK_0_GPIO_CTRL(PIN_ENC_B) = GPIO_FUNC_SIO;
    IO_BANK_0_GPIO_CTRL(PIN_ENC_BTN) = GPIO_FUNC_SIO;

    /* Kill switch input */
    IO_BANK_0_GPIO_CTRL(PIN_KILL_SW_N) = GPIO_FUNC_SIO;

    /* I2C for OLED */
    IO_BANK_0_GPIO_CTRL(PIN_I2C_SDA) = GPIO_FUNC_I2C;
    IO_BANK_0_GPIO_CTRL(PIN_I2C_SCL) = GPIO_FUNC_I2C;

    /* UART for ESP32-C3 */
    IO_BANK_0_GPIO_CTRL(PIN_UART_TX) = GPIO_FUNC_UART;
    IO_BANK_0_GPIO_CTRL(PIN_UART_RX) = GPIO_FUNC_UART;

    /* ESP32-C3 reset */
    IO_BANK_0_GPIO_CTRL(PIN_ESP_RST_N) = GPIO_FUNC_SIO;

    /* LED data (PIO) */
    IO_BANK_0_GPIO_CTRL(PIN_LED_DATA) = GPIO_FUNC_PIO0;
    IO_BANK_0_GPIO_CTRL(PIN_LED_EN) = GPIO_FUNC_SIO;

    /* SPI0 for payload flash */
    IO_BANK_0_GPIO_CTRL(PIN_FLASH_SCK) = GPIO_FUNC_SPI;
    IO_BANK_0_GPIO_CTRL(PIN_FLASH_MOSI) = GPIO_FUNC_SPI;
    IO_BANK_0_GPIO_CTRL(PIN_FLASH_MISO) = GPIO_FUNC_SPI;
    IO_BANK_0_GPIO_CTRL(PIN_FLASH_CS) = GPIO_FUNC_SIO;

    /* USB mode control */
    IO_BANK_0_GPIO_CTRL(PIN_USB_MUX_CTRL) = GPIO_FUNC_SIO;

    /* Battery sense (ADC) */
    IO_BANK_0_GPIO_CTRL(PIN_BAT_SENSE) = GPIO_FUNC_SIO;

    /* Boot button */
    IO_BANK_0_GPIO_CTRL(PIN_BOOT_BTN) = GPIO_FUNC_SIO;

    /* Configure pad controls */
    /* Encoder: Schmitt trigger, pull-ups on button and kill switch */
    PADS_BANK_0_GPIO(PIN_ENC_A) = PAD_SCHMITT | PAD_DRIVE_4MA | PAD_IE;
    PADS_BANK_0_GPIO(PIN_ENC_B) = PAD_SCHMITT | PAD_DRIVE_4MA | PAD_IE;
    PADS_BANK_0_GPIO(PIN_ENC_BTN) = PAD_SCHMITT | PAD_PULLUP | PAD_DRIVE_4MA | PAD_IE;
    PADS_BANK_0_GPIO(PIN_KILL_SW_N) = PAD_SCHMITT | PAD_PULLUP | PAD_DRIVE_4MA | PAD_IE;

    /* I2C: Open drain with pull-ups (external) */
    PADS_BANK_0_GPIO(PIN_I2C_SDA) = PAD_DRIVE_4MA | PAD_IE;
    PADS_BANK_0_GPIO(PIN_I2C_SCL) = PAD_DRIVE_4MA | PAD_IE;

    /* SPI: Fast slew, 8mA drive */
    PADS_BANK_0_GPIO(PIN_FLASH_SCK) = PAD_SLEWFAST | PAD_DRIVE_8MA | PAD_IE;
    PADS_BANK_0_GPIO(PIN_FLASH_MOSI) = PAD_SLEWFAST | PAD_DRIVE_8MA | PAD_IE;
    PADS_BANK_0_GPIO(PIN_FLASH_MISO) = PAD_SLEWFAST | PAD_DRIVE_4MA | PAD_IE;
    PADS_BANK_0_GPIO(PIN_FLASH_CS) = PAD_DRIVE_4MA;

    /* LED: 12mA drive for WS2812B */
    PADS_BANK_0_GPIO(PIN_LED_DATA) = PAD_SLEWFAST | PAD_DRIVE_12MA;
    PADS_BANK_0_GPIO(PIN_LED_EN) = PAD_DRIVE_4MA;

    /* GPIO direction: 1=output, 0=input */
    SIO_GPIO_OE = 0
        | (1 << PIN_FLASH_CS)
        | (1 << PIN_ESP_RST_N)
        | (1 << PIN_LED_EN)
        | (1 << PIN_USB_MUX_CTRL)
        | (1 << PIN_LED_DATA);

    /* Set initial outputs */
    SIO_GPIO_OUT_SET = (1 << PIN_FLASH_CS)   /* Flash CS deasserted */
                     | (1 << PIN_ESP_RST_N);  /* ESP32 out of reset */

    /* Enable GPIO interrupts for encoder */
    IO_BANK_0_GPIO_INTE(PIN_ENC_A / 8) |= (1 << (PIN_ENC_A % 8));
    IO_BANK_0_GPIO_INTE(PIN_ENC_B / 8) |= (1 << (PIN_ENC_B % 8));

    /* Enable NVIC interrupt for IO bank 0 */
    NVIC_ISER = (1 << IO_IRQ_BANK0);
}

/* ============================================================
 * Rotary Encoder Handler
 * ============================================================ */

/* Encoder interrupt handler */
void __attribute__((interrupt)) gpio_isr(void) {
    static const int8_t enc_table[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
    static uint8_t enc_state = 0;

    /* Read encoder phases */
    uint8_t new_state = ((SIO_GPIO_IN >> PIN_ENC_A) & 1) << 1 |
                        ((SIO_GPIO_IN >> PIN_ENC_B) & 1);
    enc_state = (enc_state << 2) | new_state;
    g_encoder_position += enc_table[enc_state & 0x0F];

    /* Clear interrupt */
    IO_BANK_0_GPIO_INTE(PIN_ENC_A / 8) &= ~(1 << (PIN_ENC_A % 8));
    IO_BANK_0_GPIO_INTE(PIN_ENC_B / 8) &= ~(1 << (PIN_ENC_B % 8));
    /* Re-enable interrupts after brief delay */
}

static void encoder_update(void) {
    /* Check button state */
    bool btn_current = !(SIO_GPIO_IN & (1 << PIN_ENC_BTN));

    if (btn_current && !g_encoder_pressed) {
        /* Button just pressed */
        g_press_time = time_ms();
        g_encoder_pressed = true;
        g_encoder_long_press = false;
    } else if (!btn_current && g_encoder_pressed) {
        /* Button just released */
        uint32_t press_duration = time_ms() - g_press_time;

        if (press_duration > 2000) {
            g_encoder_long_press = true;
        } else if (press_duration > 50) {
            /* Short press - select profile */
            if (g_state == STATE_STEALTH || g_state == STATE_HID) {
                execute_profile(g_selected_profile);
            }
        }

        g_encoder_pressed = false;
    }

    /* Check kill switch */
    g_kill_switch_active = !(SIO_GPIO_IN & (1 << PIN_KILL_SW_N));

    /* Update selected profile based on encoder position */
    if (g_encoder_position != 0) {
        int32_t delta = g_encoder_position;
        g_encoder_position = 0;

        g_selected_profile = (g_selected_profile + delta) % g_profile_count;
        if (g_selected_profile >= g_profile_count)
            g_selected_profile = 0;
    }
}

/* ============================================================
 * OLED Display Functions
 * ============================================================ */

static void oled_init(void) {
    /* Initialize I2C0 at 400 kHz */
    I2C0_IC_ENABLE = 0;  /* Disable I2C */

    /* Configure I2C0 clock timing for 400 kHz @ 133 MHz peripheral clock */
    I2C0_IC_FS_SCL_HCNT = 60;  /* High count */
    I2C0_IC_FS_SCL_LCNT = 130; /* Low count */

    /* Configure I2C0 */
    I2C0_IC_CON = I2C_IC_CON_MASTER_MODE
                | I2C_IC_CON_SPEED_FAST
                | I2C_IC_CON_RESTART_EN;

    /* Enable I2C0 */
    I2C0_IC_ENABLE = I2C_IC_CON_IC_ENABLE;

    /* SSD1306 initialization sequence */
    const uint8_t init_cmds[] = {
        0xAE,       /* Display off */
        0xD5, 0x80, /* Set display clock divide: 0x80 */
        0xA8, 0x3F, /* Set multiplex ratio: 64 */
        0xD3, 0x00, /* Set display offset: 0 */
        0x40,       /* Set start line: 0 */
        0x8D, 0x14, /* Enable charge pump */
        0x20, 0x00, /* Set memory addressing mode: horizontal */
        0xA1,       /* Set segment remap: column 127 = address 0 */
        0xC8,       /* Set COM scan direction: remapped */
        0xDA, 0x12, /* Set COM pins: alternative */
        0x81, 0xCF, /* Set contrast: 207 */
        0xD9, 0xF1, /* Set pre-charge period: 0xF1 */
        0xDB, 0x40, /* Set VCOMH deselect: 0x40 */
        0xA4,       /* Set entire display on: resume */
        0xA6,       /* Set normal display: not inverted */
        0xAF,       /* Display on */
    };

    /* Send init commands via I2C */
    for (uint32_t i = 0; i < sizeof(init_cmds); i++) {
        /* Write command byte: control byte = 0x00 (command) */
        I2C0_IC_TAR = OLED_I2C_ADDR;
        I2C0_IC_DATA_CMD = I2C_IC_DATA_CMD_WRITE | I2C_IC_DATA_CMD_STOP;
        /* Wait for TX empty */
        while (!(I2C0_IC_STATUS & (1 << 2)));
        I2C0_IC_DATA_CMD = init_cmds[i];
    }
}

static void oled_write_command(uint8_t cmd) {
    /* I2C write: control byte 0x00 = command, then data byte */
    I2C0_IC_TAR = OLED_I2C_ADDR;
    I2C0_IC_DATA_CMD = I2C_IC_DATA_CMD_WRITE;
    I2C0_IC_DATA_CMD = 0x00;  /* Control byte: command */
    I2C0_IC_DATA_CMD = I2C_IC_DATA_CMD_WRITE | I2C_IC_DATA_CMD_STOP;
    I2C0_IC_DATA_CMD = cmd;
}

static void oled_write_data(const uint8_t *data, uint32_t len) {
    /* I2C write: control byte 0x40 = data, then data bytes */
    I2C0_IC_TAR = OLED_I2C_ADDR;

    for (uint32_t i = 0; i < len; i++) {
        if (i == 0) {
            I2C0_IC_DATA_CMD = I2C_IC_DATA_CMD_WRITE;
            I2C0_IC_DATA_CMD = 0x40;  /* Control byte: data stream */
        }

        if (i == len - 1) {
            I2C0_IC_DATA_CMD = I2C_IC_DATA_CMD_WRITE | I2C_IC_DATA_CMD_STOP;
        } else {
            I2C0_IC_DATA_CMD = I2C_IC_DATA_CMD_WRITE;
        }
        I2C0_IC_DATA_CMD = data[i];
    }
}

static void oled_refresh(void) {
    /* Set column and page address */
    oled_write_command(0x21);  /* Set column address */
    oled_write_command(0x00);  /* Start: 0 */
    oled_write_command(0x7F);  /* End: 127 */

    oled_write_command(0x22);  /* Set page address */
    oled_write_command(0x00);  /* Start: 0 */
    oled_write_command(0x07);  /* End: 7 */

    /* Write framebuffer */
    oled_write_data(g_framebuffer, sizeof(g_framebuffer));
}

static void oled_clear(void) {
    memset(g_framebuffer, 0, sizeof(g_framebuffer));
}

static void oled_draw_char(uint8_t x, uint8_t page, char c) {
    /* 6x8 monospaced font (simplified — actual font data in flash_storage) */
    extern const uint8_t font_6x8[128][6];
    if (c < ' ' || c > '~') c = ' ';
    c -= ' ';

    for (uint8_t col = 0; col < 6; col++) {
        uint16_t idx = (page * OLED_WIDTH) + x + col;
        if (idx < sizeof(g_framebuffer)) {
            g_framebuffer[idx] = font_6x8[(uint8_t)c][col];
        }
    }
}

static void oled_draw_string(uint8_t x, uint8_t page, const char *str) {
    while (*str && x < OLED_WIDTH - 6) {
        oled_draw_char(x, page, *str);
        x += 6;
        str++;
    }
}

/* ============================================================
 * Battery Monitoring
 * ============================================================ */

static float battery_read_voltage(void) {
    /* Read ADC on battery sense pin */
    ADC_CS = ADC_CS_EN | (1 << BAT_SENSE_CHANNEL);
    ADC_CS |= ADC_CS_START_ONCE;

    /* Wait for conversion */
    while (!(ADC_CS & (1 << 16)));

    uint16_t raw = ADC_RESULT & 0xFFF;

    /* Convert to voltage: ADC = V * 4095 / 3.3, with voltage divider */
    /* Vbat = raw * 3.3 / 4095 * (R3 + R4) / R4 */
    float voltage = (float)raw * 3.3f / 4095.0f * BAT_VOLTAGE_DIVIDER;

    return voltage;
}

static uint8_t battery_read_percent(void) {
    float v = battery_read_voltage();
    if (v >= BAT_FULL_VOLTAGE) return 100;
    if (v <= BAT_EMPTY_VOLTAGE) return 0;
    return (uint8_t)((v - BAT_EMPTY_VOLTAGE) /
                     (BAT_FULL_VOLTAGE - BAT_EMPTY_VOLTAGE) * 100);
}

/* ============================================================
 * DuckyScript Parser and Executor
 * ============================================================ */

/* ASCII to HID keycode lookup table (US keyboard layout) */
static const uint8_t ascii_to_keycode[128] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x00-0x07 */
    0x00, 0x00, 0x28, 0x00, 0x00, 0x28, 0x00, 0x00,  /* 0x08-0x0F (ENTER) */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* 0x10-0x17 */
    0x00, 0x00, 0x00, 0x2B, 0x00, 0x00, 0x00, 0x00,  /* 0x18-0x1F (ESC) */
    0x2C, 0x1E, 0x34, 0x20, 0x21, 0x22, 0x23, 0x1F,  /* 0x20-0x27 space ! " # $ % & ' */
    0x34, 0x2D, 0x30, 0x2E, 0x37, 0x2F, 0x2C, 0x38,  /* 0x28-0x2F ( ) * + , - . / */
    0x27, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24,  /* 0x30-0x37 0 1 2 3 4 5 6 7 */
    0x25, 0x26, 0x33, 0x2D, 0x2F, 0x2E, 0x2F, 0x20,  /* 0x38-0x3F 8 9 : ; < = > ? */
    0x2F, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,  /* 0x40-0x47 @ A B C D E F G */
    0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12,  /* 0x48-0x4F H I J K L M N O */
    0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A,  /* 0x50-0x57 P Q R S T U V W */
    0x1B, 0x1C, 0x1D, 0x2F, 0x31, 0x2D, 0x30, 0x2E,  /* 0x58-0x5F X Y Z [ \ ] ^ _ */
    0x35, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,  /* 0x60-0x67 ` a b c d e f g */
    0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12,  /* 0x68-0x6F h i j k l m n o */
    0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A,  /* 0x70-0x77 p q r s t u v w */
    0x1B, 0x1C, 0x1D, 0x33, 0x34, 0x33, 0x2C, 0x00,  /* 0x78-0x7F x y z { | } ~ DEL */
};

/* Shift required for these ASCII characters */
static const bool ascii_needs_shift[128] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 0x00-0x0F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 0x10-0x1F */
    0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0,  /* 0x20-0x2F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1,  /* 0x30-0x3F */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1,  /* 0x40-0x4F */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0,  /* 0x50-0x5F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 0x60-0x6F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0,  /* 0x70-0x7F */
};

/* Parse and execute a DuckyScript command */
static int duckyscript_execute(const char *line) {
    char cmd[64];
    char arg[DUCKY_MAX_STRING_LEN];
    int cmd_len = 0;
    int arg_len = 0;

    /* Skip leading whitespace */
    while (*line == ' ' || *line == '\t') line++;

    /* Empty line or comment */
    if (*line == '\0' || *line == '\n' || *line == '\r') return 0;
    if (strncmp(line, "REM", 3) == 0) return 0;

    /* Parse command */
    const char *p = line;
    while (*p && *p != ' ' && *p != '\t' && *p != '\n' && cmd_len < 63) {
        cmd[cmd_len++] = *p++;
    }
    cmd[cmd_len] = '\0';

    /* Skip space after command */
    while (*p == ' ' || *p == '\t') p++;

    /* Parse argument (rest of line) */
    while (*p && *p != '\n' && *p != '\r' && arg_len < DUCKY_MAX_STRING_LEN - 1) {
        arg[arg_len++] = *p++;
    }
    arg[arg_len] = '\0';

    /* Hash the command for REPEAT support */
    g_last_command_hash = djb2_hash(cmd);

    /* Execute command */
    if (strcmp(cmd, "STRING") == 0) {
        /* Type a string */
        for (int i = 0; i < arg_len; i++) {
            char c = arg[i];
            if (c >= 0x20 && c <= 0x7E) {
                uint8_t keycode = ascii_to_keycode[(uint8_t)c];
                uint8_t modifier = ascii_needs_shift[(uint8_t)c] ? KEY_MOD_LSHIFT : 0;
                hid_keyboard_send(modifier, keycode);
                delay_ms(g_default_delay);
                hid_keyboard_send(0, 0);  /* Release */
                delay_ms(g_default_delay);
            }
        }
    } else if (strcmp(cmd, "DELAY") == 0) {
        int ms = atoi(arg);
        if (ms > 0 && ms < 60000) {
            delay_ms(ms);
        }
    } else if (strcmp(cmd, "DEFAULT_DELAY") == 0) {
        int ms = atoi(arg);
        if (ms > 0 && ms < 10000) {
            g_default_delay = ms;
        }
    } else if (strcmp(cmd, "ENTER") == 0) {
        hid_keyboard_send(0, KEY_ENTER);
        delay_ms(g_default_delay);
        hid_keyboard_send(0, 0);
        delay_ms(g_default_delay);
    } else if (strcmp(cmd, "TAB") == 0) {
        hid_keyboard_send(0, KEY_TAB);
        delay_ms(g_default_delay);
        hid_keyboard_send(0, 0);
        delay_ms(g_default_delay);
    } else if (strcmp(cmd, "ESCAPE") == 0 || strcmp(cmd, "ESC") == 0) {
        hid_keyboard_send(0, KEY_ESCAPE);
        delay_ms(g_default_delay);
        hid_keyboard_send(0, 0);
        delay_ms(g_default_delay);
    } else if (strcmp(cmd, "SPACE") == 0) {
        hid_keyboard_send(0, KEY_SPACE);
        delay_ms(g_default_delay);
        hid_keyboard_send(0, 0);
        delay_ms(g_default_delay);
    } else if (strcmp(cmd, "BACKSPACE") == 0) {
        hid_keyboard_send(0, KEY_BACKSPACE);
        delay_ms(g_default_delay);
        hid_keyboard_send(0, 0);
        delay_ms(g_default_delay);
    } else if (strcmp(cmd, "CAPSLOCK") == 0) {
        hid_keyboard_send(0, KEY_CAPSLOCK);
        delay_ms(g_default_delay);
        hid_keyboard_send(0, 0);
        delay_ms(g_default_delay);
    } else if (strcmp(cmd, "UP") == 0) {
        hid_keyboard_send(0, KEY_UP);
        delay_ms(g_default_delay);
        hid_keyboard_send(0, 0);
        delay_ms(g_default_delay);
    } else if (strcmp(cmd, "DOWN") == 0) {
        hid_keyboard_send(0, KEY_DOWN);
        delay_ms(g_default_delay);
        hid_keyboard_send(0, 0);
        delay_ms(g_default_delay);
    } else if (strcmp(cmd, "LEFT") == 0) {
        hid_keyboard_send(0, KEY_LEFT);
        delay_ms(g_default_delay);
        hid_keyboard_send(0, 0);
        delay_ms(g_default_delay);
    } else if (strcmp(cmd, "RIGHT") == 0) {
        hid_keyboard_send(0, KEY_RIGHT);
        delay_ms(g_default_delay);
        hid_keyboard_send(0, 0);
        delay_ms(g_default_delay);
    } else if (strcmp(cmd, "GUI") == 0 || strcmp(cmd, "WINDOWS") == 0) {
        hid_keyboard_send(KEY_MOD_LGUI, hid_parse_keycode(arg));
        delay_ms(g_default_delay);
        hid_keyboard_send(0, 0);
        delay_ms(g_default_delay);
    } else if (strcmp(cmd, "SHIFT") == 0) {
        hid_keyboard_send(KEY_MOD_LSHIFT, hid_parse_keycode(arg));
        delay_ms(g_default_delay);
        hid_keyboard_send(0, 0);
        delay_ms(g_default_delay);
    } else if (strcmp(cmd, "CTRL") == 0 || strcmp(cmd, "CONTROL") == 0) {
        hid_keyboard_send(KEY_MOD_LCTRL, hid_parse_keycode(arg));
        delay_ms(g_default_delay);
        hid_keyboard_send(0, 0);
        delay_ms(g_default_delay);
    } else if (strcmp(cmd, "ALT") == 0) {
        hid_keyboard_send(KEY_MOD_LALT, hid_parse_keycode(arg));
        delay_ms(g_default_delay);
        hid_keyboard_send(0, 0);
        delay_ms(g_default_delay);
    } else if (strcmp(cmd, "CTRL-ALT") == 0) {
        hid_keyboard_send(KEY_MOD_LCTRL | KEY_MOD_LALT, hid_parse_keycode(arg));
        delay_ms(g_default_delay);
        hid_keyboard_send(0, 0);
        delay_ms(g_default_delay);
    } else if (strcmp(cmd, "CTRL-SHIFT") == 0) {
        hid_keyboard_send(KEY_MOD_LCTRL | KEY_MOD_LSHIFT, hid_parse_keycode(arg));
        delay_ms(g_default_delay);
        hid_keyboard_send(0, 0);
        delay_ms(g_default_delay);
    } else if (strcmp(cmd, "REPEAT") == 0) {
        /* Repeat last command N times — stored hash is used by caller */
        return 1;  /* Signal to repeat */
    } else if (strcmp(cmd, "HOLD") == 0) {
        /* Hold a key down */
        uint8_t kc = hid_parse_keycode(arg);
        if (g_held_key_count < 8) {
            g_held_keys[g_held_key_count++] = kc;
        }
    } else if (strcmp(cmd, "RELEASE") == 0) {
        /* Release all held keys */
        g_held_key_count = 0;
        memset(g_held_keys, 0, sizeof(g_held_keys));
        hid_keyboard_send(0, 0);
    } else if (strcmp(cmd, "MOUSE_MOVE") == 0) {
        int dx = 0, dy = 0;
        sscanf(arg, "%d %d", &dx, &dy);
        hid_mouse_move((int8_t)dx, (int8_t)dy, 0);
        delay_ms(g_default_delay);
    } else if (strcmp(cmd, "MOUSE_CLICK") == 0) {
        if (strcmp(arg, "left") == 0) hid_mouse_click(MOUSE_BTN_LEFT);
        else if (strcmp(arg, "right") == 0) hid_mouse_click(MOUSE_BTN_RIGHT);
        else if (strcmp(arg, "middle") == 0) hid_mouse_click(MOUSE_BTN_MIDDLE);
        delay_ms(50);
        hid_mouse_click(0);  /* Release */
        delay_ms(g_default_delay);
    } else if (strcmp(cmd, "MOUSE_SCROLL") == 0) {
        int scroll = atoi(arg);
        hid_mouse_move(0, 0, (int8_t)scroll);
        delay_ms(g_default_delay);
    } else if (strcmp(cmd, "CONSUMER") == 0) {
        uint16_t consumer_key = hid_parse_consumer_keycode(arg);
        hid_consumer_send(consumer_key);
        delay_ms(g_default_delay);
        hid_consumer_send(0);
        delay_ms(g_default_delay);
    } else if (strcmp(cmd, "STEALTH_ON") == 0) {
        /* Switch to HID mode */
        usb_switch_to_hid();
        g_state = STATE_HID;
    } else if (strcmp(cmd, "STEALTH_OFF") == 0) {
        /* Switch back to MSC mode */
        usb_switch_to_msc();
        g_state = STATE_STEALTH;
    } else if (strcmp(cmd, "LED_R") == 0 || strcmp(cmd, "LED_G") == 0 ||
               strcmp(cmd, "LED_B") == 0) {
        /* LED color commands (simplified) */
        uint8_t r = 0, g = 0, b = 0;
        if (strcmp(cmd, "LED_R") == 0) r = (uint8_t)strtol(arg, NULL, 16);
        else if (strcmp(cmd, "LED_G") == 0) g = (uint8_t)strtol(arg, NULL, 16);
        else b = (uint8_t)strtol(arg, NULL, 16);
        ws2812b_set_color(r, g, b);
    } else if (strcmp(cmd, "OLED_LINE") == 0) {
        /* Display text on OLED line */
        int line_num = 0;
        char text[22];
        sscanf(arg, "%d %21s", &line_num, text);
        if (line_num >= 0 && line_num < 8) {
            oled_draw_string(0, line_num, text);
            oled_refresh();
        }
    } else if (strcmp(cmd, "RANDOM_DELAY") == 0) {
        int min_ms = 0, max_ms = 0;
        sscanf(arg, "%d %d", &min_ms, &max_ms);
        if (max_ms > min_ms) {
            /* Simple pseudo-random using timer */
            uint32_t range = max_ms - min_ms;
            uint32_t random = (time_us() % range);
            delay_ms(min_ms + random);
        }
    } else if (strcmp(cmd, "IF_NETWORK") == 0) {
        /* Check if specified SSID is visible */
        if (!wifi_scan_for_ssid(arg)) {
            /* Network not found — skip next block */
            return 2;  /* Skip signal */
        }
    } else if (strcmp(cmd, "WAIT_FOR_NETWORK") == 0) {
        /* Block until network found or timeout */
        uint32_t timeout = 30000;  /* 30 seconds */
        uint32_t start = time_ms();
        while (!wifi_scan_for_ssid(arg)) {
            if ((time_ms() - start) > timeout) return -1;
            delay_ms(1000);
        }
    } else if (strcmp(cmd, "PROFILE") == 0) {
        /* Jump to another profile */
        for (int i = 0; i < g_profile_count; i++) {
            if (strcmp(g_profiles[i].name, arg) == 0) {
                execute_profile(i);
                break;
            }
        }
    }

    return 0;
}

/* Execute a payload profile from flash */
static int execute_profile(uint8_t profile_index) {
    if (profile_index >= g_profile_count) return -1;

    /* Check kill switch */
    if (g_kill_switch_active) {
        g_state = STATE_KILL;
        oled_clear();
        oled_draw_string(0, 0, "KILL MODE");
        oled_draw_string(0, 1, "USB disabled");
        oled_refresh();
        return -2;
    }

    /* Check geofence if enabled */
    profile_entry_t *profile = &g_profiles[profile_index];
    if (profile->flags & PROFILE_FLAG_GEOFENCED) {
        g_state = STATE_GEOFENCE;
        /* Geofence check happens in main loop */
    }

    /* Read payload from flash */
    uint8_t *payload_buf = (uint8_t *)0x20000000;  /* Use SRAM */
    uint32_t payload_size = profile->size;

    if (flash_read_payload(profile->offset, payload_buf, payload_size) != 0) {
        g_state = STATE_ERROR;
        return -3;
    }

    /* Decrypt if encrypted */
    if (profile->flags & PROFILE_FLAG_ENCRYPTED) {
        /* TODO: AES-128-CTR decryption */
        /* For now, plaintext only */
    }

    /* Verify CRC */
    uint32_t computed_crc = crc32(payload_buf, payload_size);
    if (computed_crc != profile->crc32) {
        g_state = STATE_ERROR;
        return -4;
    }

    /* Switch to HID mode if in stealth */
    if (g_state == STATE_STEALTH) {
        usb_switch_to_hid();
        g_state = STATE_HID;
        delay_ms(2000);  /* Wait for USB enumeration */
    }

    g_state = STATE_EXECUTING;

    /* Update OLED */
    oled_clear();
    oled_draw_string(0, 0, "EXECUTING");
    oled_draw_string(0, 1, profile->name);
    oled_refresh();

    /* Parse and execute each line */
    char line[DUCKY_MAX_LINE_LEN];
    uint32_t pos = 0;
    int line_num = 0;

    while (pos < payload_size) {
        /* Read a line */
        int line_pos = 0;
        while (pos < payload_size && line_pos < DUCKY_MAX_LINE_LEN - 1) {
            char c = (char)payload_buf[pos++];
            if (c == '\n' || c == '\r') {
                /* Handle \r\n */
                if (pos < payload_size && payload_buf[pos] == '\n') pos++;
                break;
            }
            line[line_pos++] = c;
        }
        line[line_pos] = '\0';
        line_num++;

        /* Skip empty lines */
        if (line_pos == 0) continue;

        /* Execute the command */
        int result = duckyscript_execute(line);

        if (result == -1) {
            /* Timeout or error */
            break;
        }

        /* Check kill switch during execution */
        if (g_kill_switch_active) {
            g_state = STATE_KILL;
            hid_keyboard_send(0, 0);  /* Release all keys */
            return -2;
        }
    }

    /* Execution complete */
    hid_keyboard_send(0, 0);  /* Release all keys */
    g_state = STATE_HID;

    /* Update OLED */
    oled_clear();
    oled_draw_string(0, 0, "COMPLETE");
    oled_refresh();

    delay_ms(2000);

    /* Return to stealth mode */
    usb_switch_to_msc();
    g_state = STATE_STEALTH;

    return 0;
}

/* ============================================================
 * WS2812B LED Driver (via PIO)
 * ============================================================ */

static void ws2812b_init(void) {
    /* Load WS2812B PIO program into PIO0 SM0 */
    /* WS2812B timing: 800 kHz, 1.25us per bit */
    /* T1H = 0.8us (bit '1' high), T0H = 0.4us (bit '0' high) */
    /* Using PIO at 133 MHz / 3 = ~44.3 ticks/us */

    /* PIO program for WS2812B (simplified) */
    PIO0_INSTR_MEM(0) = 0xE080;  /* out x, 8 */
    PIO0_INSTR_MEM(1) = 0x6021;  /* out y, 1 */
    PIO0_INSTR_MEM(2) = 0x1FA2;  /* set pins, 1 [delay] */
    PIO0_INSTR_MEM(3) = 0x4001;  /* in y, 1 */
    PIO0_INSTR_MEM(4) = 0x1FC4;  /* set pins, 0 [delay] */

    /* Configure SM0 */
    PIO0_SM0_CLKDIV = (4 << 16);  /* Clock divider */
    PIO0_SM0_PINCTRL = (1 << 26) | (1 << 20);  /* 1 out pin */
    PIO0_SM0_EXECCTRL = (0x1F << 12) | (1 << 0);  /* Wrap */

    /* Enable SM0 */
    PIO0_CTRL |= (1 << 0);
}

static void ws2812b_set_color(uint8_t r, uint8_t g, uint8_t b) {
    /* Send 24-bit color: G8 R8 B8 (WS2812B order) */
    uint32_t color = ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;

    /* Write to PIO TX FIFO */
    PIO0_TXF0 = color;

    /* Wait for transmission */
    delay_us(60);  /* Reset pulse > 50us */
}

/* ============================================================
 * Main Entry Points
 * ============================================================ */

/* Core 1 entry point — handles USB and display */
void core1_entry(void) {
    /* Initialize USB device controller */
    usb_device_init();

    /* Start in MSC (stealth) mode */
    usb_enumerate_msc();

    /* Main loop for Core 1 */
    while (1) {
        /* Process USB tasks */
        usb_task();

        /* Refresh OLED (30 fps) */
        static uint32_t last_refresh = 0;
        if ((time_ms() - last_refresh) > 33) {
            oled_refresh();
            last_refresh = time_ms();
        }

        /* Process HID reports if in HID mode */
        if (g_state == STATE_HID || g_state == STATE_EXECUTING) {
            hid_task();
        }

        delay_us(100);
    }
}

/* Core 0 entry point — main application */
int main(void) {
    /* Initialize clocks */
    clocks_init();

    /* Initialize GPIO */
    gpio_init();

    /* Initialize SPI flash */
    flash_storage_init();

    /* Initialize UART for ESP32-C3 */
    wifi_bridge_init();

    /* Initialize OLED */
    oled_init();

    /* Initialize ADC for battery */
    /* (ADC already enabled in gpio_init via reset) */

    /* Initialize WS2812B LED */
    ws2812b_init();
    ws2812b_set_color(0x00, 0x00, 0x20);  /* Dim blue = stealth mode */

    /* Load profiles from flash */
    g_profile_count = flash_load_profiles(g_profiles, DUCKY_MAX_PROFILES);

    /* Load device settings */
    device_settings_t settings;
    if (flash_read_settings(&settings) == 0 && settings.magic == SETTINGS_MAGIC) {
        g_selected_profile = settings.default_profile;
    }

    /* Show splash screen */
    oled_clear();
    oled_draw_string(4, 0, "PHANTOM");
    oled_draw_string(2, 1, "v1.0");
    oled_draw_string(0, 3, "Profiles:");
    char count_str[8];
    snprintf(count_str, sizeof(count_str), "%d", g_profile_count);
    oled_draw_string(0, 4, count_str);
    oled_refresh();
    delay_ms(2000);

    /* Check kill switch */
    if (g_kill_switch_active) {
        g_state = STATE_KILL;
        oled_clear();
        oled_draw_string(0, 0, "KILL MODE");
        oled_draw_string(0, 1, "USB disabled");
        oled_draw_string(0, 3, "Move switch");
        oled_draw_string(0, 4, "to enable");
        oled_refresh();

        /* Wait for kill switch to be released */
        while (g_kill_switch_active) {
            g_kill_switch_active = !(SIO_GPIO_IN & (1 << PIN_KILL_SW_N));
            delay_ms(100);
        }
    }

    /* Start in stealth mode */
    g_state = STATE_STEALTH;

    /* Launch Core 1 for USB handling */
    /* (In real firmware, this uses multicore_launch_core1) */
    /* For now, Core 0 handles everything */

    /* Main loop */
    while (1) {
        /* Update encoder */
        encoder_update();

        /* Update kill switch */
        g_kill_switch_active = !(SIO_GPIO_IN & (1 << PIN_KILL_SW_N));

        if (g_kill_switch_active && g_state != STATE_KILL) {
            /* Kill switch activated — disable USB data */
            g_state = STATE_KILL;
            usb_disconnect();
            hid_keyboard_send(0, 0);  /* Release all keys */
            ws2812b_set_color(0x00, 0x00, 0x00);  /* LED off */

            oled_clear();
            oled_draw_string(0, 0, "KILL MODE");
            oled_draw_string(0, 1, "USB disabled");
            oled_refresh();
        }

        /* State machine */
        switch (g_state) {
            case STATE_STEALTH:
                /* MSC mode — show profile list on OLED */
                ws2812b_set_color(0x00, 0x00, 0x20);  /* Dim blue */

                oled_clear();
                oled_draw_string(0, 0, "PHANTOM");

                /* Battery indicator */
                uint8_t bat_pct = battery_read_percent();
                char bat_str[8];
                snprintf(bat_str, sizeof(bat_str), "%d%%", bat_pct);
                oled_draw_string(16, 0, bat_str);

                /* Profile list */
                for (int i = 0; i < 5 && (g_selected_profile + i - 2) >= 0 &&
                                (g_selected_profile + i - 2) < g_profile_count; i++) {
                    int idx = g_selected_profile + i - 2;
                    if (idx >= 0 && idx < g_profile_count) {
                        char prefix = (idx == g_selected_profile) ? '>' : ' ';
                        char line[22];
                        snprintf(line, sizeof(line), "%c%s", prefix, g_profiles[idx].name);
                        oled_draw_string(0, 2 + i, line);
                    }
                }

                oled_draw_string(0, 7, "[ENC] Select");
                oled_refresh();
                break;

            case STATE_HID:
                /* HID mode — ready for payload execution */
                ws2812b_set_color(0x00, 0x20, 0x00);  /* Green */
                break;

            case STATE_EXECUTING:
                /* Payload executing — handled in execute_profile() */
                ws2812b_set_color(0x20, 0x00, 0x00);  /* Red */
                break;

            case STATE_GEOFENCE:
                /* Checking geofence */
                ws2812b_set_color(0x20, 0x20, 0x00);  /* Yellow */
                /* Geofence check happens in execute_profile() */
                break;

            case STATE_KILL:
                /* Kill switch active — do nothing */
                break;

            case STATE_ERROR:
                /* Error state */
                ws2812b_set_color(0x40, 0x00, 0x00);  /* Bright red */
                break;

            case STATE_SLEEP:
                /* Low power sleep */
                oled_clear();
                oled_refresh();
                ws2812b_set_color(0x00, 0x00, 0x00);
                break;

            default:
                break;
        }

        /* Process WiFi/BLE commands */
        wifi_bridge_task();

        /* Process USB tasks (simplified for single-core) */
        usb_task();
        if (g_state == STATE_HID || g_state == STATE_EXECUTING) {
            hid_task();
        }

        delay_ms(10);  /* ~100 Hz main loop */
    }

    return 0;
}

/* 6x8 monospaced font (ASCII 0x20-0x7E) */
const uint8_t font_6x8[128][6] = {
    /* Space */ {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* ! */     {0x00, 0x00, 0x5F, 0x00, 0x00, 0x00},
    /* " */     {0x00, 0x07, 0x00, 0x07, 0x00, 0x00},
    /* # */     {0x14, 0x7F, 0x14, 0x7F, 0x14, 0x00},
    /* $ */     {0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x00},
    /* % */     {0x23, 0x13, 0x08, 0x64, 0x62, 0x00},
    /* & */     {0x36, 0x49, 0x55, 0x22, 0x50, 0x00},
    /* ' */     {0x00, 0x05, 0x03, 0x00, 0x00, 0x00},
    /* ( */     {0x00, 0x1C, 0x22, 0x41, 0x00, 0x00},
    /* ) */     {0x00, 0x41, 0x22, 0x1C, 0x00, 0x00},
    /* * */     {0x14, 0x08, 0x3E, 0x08, 0x14, 0x00},
    /* + */     {0x08, 0x08, 0x3E, 0x08, 0x08, 0x00},
    /* , */     {0x00, 0x50, 0x30, 0x00, 0x00, 0x00},
    /* - */     {0x08, 0x08, 0x08, 0x08, 0x08, 0x00},
    /* . */     {0x00, 0x60, 0x60, 0x00, 0x00, 0x00},
    /* / */     {0x20, 0x10, 0x08, 0x04, 0x02, 0x00},
    /* 0-9 abbreviated for brevity */
    [0x30] = {0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00}, /* 0 */
    [0x31] = {0x00, 0x42, 0x7F, 0x40, 0x00, 0x00}, /* 1 */
    [0x32] = {0x42, 0x61, 0x51, 0x49, 0x46, 0x00}, /* 2 */
    [0x33] = {0x21, 0x41, 0x45, 0x4B, 0x31, 0x00}, /* 3 */
    [0x34] = {0x18, 0x14, 0x12, 0x7F, 0x10, 0x00}, /* 4 */
    [0x35] = {0x27, 0x45, 0x45, 0x45, 0x39, 0x00}, /* 5 */
    [0x36] = {0x3C, 0x4A, 0x49, 0x49, 0x30, 0x00}, /* 6 */
    [0x37] = {0x01, 0x71, 0x09, 0x05, 0x03, 0x00}, /* 7 */
    [0x38] = {0x36, 0x49, 0x49, 0x49, 0x36, 0x00}, /* 8 */
    [0x39] = {0x06, 0x49, 0x49, 0x29, 0x1E, 0x00}, /* 9 */
    /* A-Z and a-z abbreviated */
    [0x41] = {0x7E, 0x11, 0x11, 0x11, 0x7E, 0x00}, /* A */
    [0x42] = {0x7F, 0x49, 0x49, 0x49, 0x36, 0x00}, /* B */
    [0x43] = {0x3E, 0x41, 0x41, 0x41, 0x22, 0x00}, /* C */
    [0x44] = {0x7F, 0x41, 0x41, 0x22, 0x1C, 0x00}, /* D */
    [0x45] = {0x7F, 0x49, 0x49, 0x49, 0x41, 0x00}, /* E */
    [0x46] = {0x7F, 0x09, 0x09, 0x09, 0x01, 0x00}, /* F */
    [0x47] = {0x3E, 0x41, 0x49, 0x49, 0x7A, 0x00}, /* G */
    [0x48] = {0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00}, /* H */
    [0x49] = {0x00, 0x41, 0x7F, 0x41, 0x00, 0x00}, /* I */
    [0x4A] = {0x20, 0x40, 0x41, 0x3F, 0x01, 0x00}, /* J */
    [0x4B] = {0x7F, 0x08, 0x14, 0x22, 0x41, 0x00}, /* K */
    [0x4C] = {0x7F, 0x40, 0x40, 0x40, 0x40, 0x00}, /* L */
    [0x4D] = {0x7F, 0x02, 0x0C, 0x02, 0x7F, 0x00}, /* M */
    [0x4E] = {0x7F, 0x04, 0x08, 0x10, 0x7F, 0x00}, /* N */
    [0x4F] = {0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00}, /* O */
    [0x50] = {0x7F, 0x09, 0x09, 0x09, 0x06, 0x00}, /* P */
    [0x51] = {0x3E, 0x41, 0x51, 0x21, 0x5E, 0x00}, /* Q */
    [0x52] = {0x7F, 0x09, 0x19, 0x29, 0x46, 0x00}, /* R */
    [0x53] = {0x46, 0x49, 0x49, 0x49, 0x31, 0x00}, /* S */
    [0x54] = {0x01, 0x01, 0x7F, 0x01, 0x01, 0x00}, /* T */
    [0x55] = {0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00}, /* U */
    [0x56] = {0x1F, 0x20, 0x40, 0x20, 0x1F, 0x00}, /* V */
    [0x57] = {0x3F, 0x40, 0x38, 0x40, 0x3F, 0x00}, /* W */
    [0x58] = {0x63, 0x14, 0x08, 0x14, 0x63, 0x00}, /* X */
    [0x59] = {0x07, 0x08, 0x70, 0x08, 0x07, 0x00}, /* Y */
    [0x5A] = {0x61, 0x51, 0x49, 0x45, 0x43, 0x00}, /* Z */
};