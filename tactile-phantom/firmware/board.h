/*
 * Tactile-Phantom — Touch-Controller Bus MITM Implant
 * board.h — Pin definitions, bus constants, board configuration
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Hardware: Raspberry Pi RP2040 (QFN-56) + ESP32-C3-MINI-1
 *
 * Pin assignment (RP2040):
 *   GP0  - I2C0_SDA  (to ESP32-C3 UART TX)       / SPI0 RX
 *   GP1  - I2C0_SCL  (to ESP32-C3 UART RX)       / SPI0 CSn
 *   GP2  - UART1_TX  (to ESP32-C3 BLE module)
 *   GP3  - UART1_RX  (from ESP32-C3)
 *   GP4  - PIO0_SM0  I2C/SPI through-path: SDA/MOSI (target side)
 *   GP5  - PIO0_SM0  I2C/SPI through-path: SCL/SCK (target side)
 *   GP6  - PIO0_SM1  I2C/SPI through-path: SDA/MOSI (host side)
 *   GP7  - PIO0_SM1  I2C/SPI through-path: SCL/SCK (host side)
 *   GP8  - PIO0_SM2  Tap: SDA/MOSI (monitor target side)
 *   GP9  - PIO0_SM2  Tap: SCL/SCK (monitor target side)
 *   GP10 - PIO1_SM0  Injection: SDA/MOSI drive
 *   GP11 - PIO1_SM0  Injection: SCL/SCK drive
 *   GP12 - SPI1_RX   (SD card MISO)
 *   GP13 - SPI1_CSn  (SD card CS)
 *   GP14 - SPI1_SCK  (SD card SCK)
 *   GP15 - SPI1_TX   (SD card MOSI)
 *   GP16 - GPIO      (status LED)
 *   GP17 - GPIO      (OLED I2C SDA via bitbang or secondary I2C)
 *   GP18 - GPIO      (OLED I2C SCL)
 *   GP19 - GPIO      (target VCC detect ADC)
 *   GP20 - GPIO      (battery ADC via divider)
 *   GP21 - GPIO      (bus mode select: 0=I2C, 1=SPI, 2=auto)
 *   GP22 - GPIO      (inject armed LED)
 *   GP23 - GPIO      (TXB0108 OE)
 *   GP24 - GPIO      (boot button)
 *   GP25 - GPIO      (USB CDC active LED)
 *   GP26 - ADC0      (target bus voltage sense)
 *   GP27 - ADC1      (LiPo voltage sense)
 *   GP28 - ADC2      (reference voltage / temp)
 */

#ifndef TACTILE_PHANTOM_BOARD_H
#define TACTILE_PHANTOM_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* --- Author / version metadata ----------------------------------------- */
#define TP_AUTHOR        "jayis1"
#define TP_VERSION_MAJOR  1
#define TP_VERSION_MINOR  0
#define TP_VERSION_PATCH  0
#define TP_FIRMWARE_NAME  "tactile-phantom"

/* --- RP2040 pin map (Pico SDK pin numbers) ----------------------------- */
#define PIN_ESP_UART_TX   2u   /* RP2040 -> ESP32-C3 RX */
#define PIN_ESP_UART_RX   3u   /* ESP32-C3 TX -> RP2040 */
#define PIN_ESP_UART_BAUD  1000000u  /* 1 Mbps inter-MCU link */

/* Bus tap: target-side lines (connect to touch-controller IC) */
#define PIN_TGT_SDA_MOSI  4u
#define PIN_TGT_SCL_SCK   5u
/* Bus tap: host-side lines (connect to application processor) */
#define PIN_HOST_SDA_MOSI 6u
#define PIN_HOST_SCL_SCK  7u
/* Tap-only monitor (read, no drive) */
#define PIN_TAP_SDA_MOSI  8u
#define PIN_TAP_SCL_SCK   9u
/* Injection drive lines */
#define PIN_INJ_SDA_MOSI  10u
#define PIN_INJ_SCL_SCK   11u

/* SD card on SPI1 */
#define PIN_SD_MISO        12u
#define PIN_SD_CS          13u
#define PIN_SD_SCK         14u
#define PIN_SD_MOSI        15u

/* Status indicators */
#define PIN_LED_STATUS     16u
#define PIN_LED_INJECT     22u
#define PIN_LED_USB        25u

/* OLED (SSD1306) via bit-banged I2C on GP17/GP18 */
#define PIN_OLED_SDA       17u
#define PIN_OLED_SCL       18u

/* Analog / power */
#define PIN_SENSE_TARGET   19u  /* digital: target VCC present */
#define PIN_ADC_TARGET     26u  /* ADC0: target bus voltage */
#define PIN_ADC_BATT       27u  /* ADC1: LiPo voltage */
#define PIN_ADC_REF        28u  /* ADC2: spare/temp */

/* Control */
#define PIN_BUS_MODE       21u  /* 0=I2C, 1=SPI, float=auto */
#define PIN_TXB_OE         23u  /* TXB0108 output enable */
#define PIN_BOOT_BTN       24u

/* --- Bus mode constants ----------------------------------------------- */
typedef enum {
    TP_BUS_AUTO = 0,
    TP_BUS_I2C  = 1,
    TP_BUS_SPI  = 2,
} tp_bus_mode_t;

/* --- Supported touch-controller vendors ------------------------------- */
typedef enum {
    TP_TC_UNKNOWN   = 0,
    TP_TC_GOODIX    = 1,   /* Goodix GT911/GT9147/GT9271 etc. */
    TP_TC_FOCALTECH = 2,   /* FocalTech FT5406/FT5x06/FT6x06 */
    TP_TC_SYNAPTICS = 3,   /* Synaptics S3320/S3325/R63353 */
    TP_TC_ILITEK   = 4,   /* Ilitek ILI2130/ILI2510 */
    TP_TC_CYPRESS  = 5,   /* Cypress/Infineon TMA/TT2xx */
    TP_TC_ATMEL    = 6,   /* Atmel maXTouch */
    TP_TC_NOVATEK  = 7,   /* Novatek NT36xxx */
} tp_tc_vendor_t;

/* Vendor name strings (for logging / display) */
static const char *const tp_vendor_names[] = {
    [TP_TC_UNKNOWN]   = "unknown",
    [TP_TC_GOODIX]    = "Goodix",
    [TP_TC_FOCALTECH] = "FocalTech",
    [TP_TC_SYNAPTICS] = "Synaptics",
    [TP_TC_ILITEK]   = "Ilitek",
    [TP_TC_CYPRESS]  = "Cypress",
    [TP_TC_ATMEL]    = "Atmel",
    [TP_TC_NOVATEK]  = "Novatek",
};

/* --- I2C address defaults (touch controllers commonly use these) ------ */
#define TP_I2C_ADDR_GOODIX_PRIMARY   0x5D
#define TP_I2C_ADDR_GOODIX_SECONDARY  0x14
#define TP_I2C_ADDR_FOCALTECH         0x38
#define TP_I2C_ADDR_SYNAPTICS         0x20
#define TP_I2C_ADDR_ILITEK            0x41
#define TP_I2C_ADDR_CYPRESS           0x24
#define TP_I2C_ADDR_ATMEL             0x4A
#define TP_I2C_ADDR_NOVATEK           0x01

/* --- Capture ring buffer sizes ---------------------------------------- */
#define TP_RING_BUF_WORDS    2048u   /* 8 KB DMA ring (32-bit words) */
#define TP_EVENT_QUEUE_DEPTH 256u    /* decoded touch events */
#define TP_INJECT_QUEUE_DEPTH 64u    /* pending injection commands */

/* --- Timing constants ------------------------------------------------- */
#define TP_AUTO_DETECT_MS    100u   /* bus-type auto-detect window */
#define TP_INJECT_GAP_US     2000u  /* min gap after host poll before inject */
#define TP_SD_ROTATE_SEC     600u   /* 10-minute file rotation */
#define TP_BLE_NOTIFY_MS     50u    /* 20 Hz BLE notification rate */
#define TP_HEARTBEAT_MS      1000u

/* --- OLED display constants ------------------------------------------- */
#define TP_OLED_I2C_ADDR    0x3C
#define TP_OLED_WIDTH        128u
#define TP_OLED_HEIGHT       32u

/* --- Touch event types ------------------------------------------------ */
typedef enum {
    TP_EV_NONE      = 0,
    TP_EV_TOUCH     = 1,   /* finger down/move */
    TP_EV_RELEASE   = 2,   /* finger up */
    TP_EV_GESTURE   = 3,   /* decoded gesture (swipe, tap, etc.) */
    TP_EV_CONFIG    = 4,   /* host wrote config register */
    TP_EV_FW_UPDATE = 5,   /* firmware update traffic */
    TP_EV_BUS_EVENT = 6,   /* START, STOP, error */
    TP_EV_INJECT    = 7,   /* injection performed */
} tp_event_type_t;

/* --- Touch event structure -------------------------------------------- */
#define TP_MAX_FINGERS 10
typedef struct {
    uint8_t  finger_id;     /* 0..9 */
    uint16_t x;             /* 0..4095 (controller-dependent) */
    uint16_t y;             /* 0..4095 */
    uint16_t pressure;      /* 0..255 (controller-dependent) */
    uint8_t  width;          /* contact area width */
    uint8_t  flags;          /* bit0: down, bit1: in-contact, bit2: palm */
} tp_touch_t;

typedef struct {
    uint32_t timestamp_us;   /* microseconds since boot */
    tp_event_type_t type;
    uint8_t vendor;
    uint8_t finger_count;
    tp_touch_t fingers[TP_MAX_FINGERS];
    uint16_t gesture_id;    /* vendor-specific gesture code */
    uint16_t reg_addr;      /* for CONFIG/FW events */
    uint16_t reg_len;       /* data length */
} tp_event_t;

/* --- Injection command structure -------------------------------------- */
typedef enum {
    TP_INJ_TAP        = 0,
    TP_INJ_SWIPE      = 1,
    TP_INJ_LONG_PRESS = 2,
    TP_INJ_MULTI      = 3,
    TP_INJ_PATTERN    = 4,
    TP_INJ_TYPE       = 5,
    TP_INJ_RAW_REG    = 6,
} tp_inj_type_t;

typedef struct {
    tp_inj_type_t type;
    uint16_t x1, y1;
    uint16_t x2, y2;
    uint16_t duration_ms;
    uint8_t  finger_count;
    tp_touch_t touches[TP_MAX_FINGERS];
    uint16_t reg_addr;
    uint16_t reg_data_len;
    uint8_t  reg_data[64];
    /* For TYPE command: ASCII text (up to 64 chars) */
    char     text[64];
} tp_inj_cmd_t;

/* --- Bus state -------------------------------------------------------- */
typedef struct {
    tp_bus_mode_t  mode;
    tp_tc_vendor_t vendor;
    uint8_t        i2c_addr;
    uint8_t        spi_cs_polarity; /* 0=active-low, 1=active-high */
    uint32_t       clock_hz;
    uint16_t       x_resolution;
    uint16_t       y_resolution;
    uint8_t        max_fingers;
    bool           attached;
    bool           capturing;
    bool           inject_armed;
    uint32_t       total_transactions;
    uint32_t       total_events;
} tp_bus_state_t;

/* --- Function prototypes (implemented across modules) ----------------- */
/* main.c */
void        tp_board_init(void);
void        tp_pio_init(void);
void        tp_dma_init(void);
void        tp_status_led_set(bool on);
void        tp_inject_led_set(bool on);

/* protocol.c */
bool        tp_protocol_auto_detect(tp_bus_state_t *state);
const char *tp_protocol_vendor_name(uint8_t v);

/* touch_decode.c */
bool        tp_decode_transaction(const uint8_t *raw, uint16_t len,
                                  tp_bus_state_t *state,
                                  tp_event_t *out_event);

/* injection.c */
bool        tp_injection_queue(tp_inj_cmd_t *cmd);
bool        tp_injection_execute(tp_inj_cmd_t *cmd, tp_bus_state_t *state);
uint32_t    tp_injection_pending(void);

/* layout_infer.c */
bool        tp_layout_set_keyboard(const uint8_t *layout, uint16_t len);
bool        tp_layout_set_grid(uint16_t width, uint16_t height, uint8_t rows, uint8_t cols);
bool        tp_layout_infer_keystroke(uint16_t x, uint16_t y, char *out_char);

/* storage.c */
bool        tp_storage_init(void);
bool        tp_storage_log_event(const tp_event_t *ev);
bool        tp_storage_rotate(void);
void        tp_storage_shutdown(void);

/* ble_link.c */
bool        tp_ble_link_init(void);
bool        tp_ble_link_send_event(const tp_event_t *ev);
bool        tp_ble_link_poll_command(tp_inj_cmd_t *out_cmd);
void        tp_ble_link_task(void);

/* core1.c */
void        core1_main(void);
void        core1_submit_raw(const uint8_t *data, uint16_t len);

#endif /* TACTILE_PHANTOM_BOARD_H */