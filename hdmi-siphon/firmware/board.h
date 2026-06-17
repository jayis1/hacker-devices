/**
 * @file    board.h
 * @brief   HDMI-Siphon board-level pin definitions and hardware configuration
 * @author  jayis1
 * @version 1.0.0
 * @license Proprietary — Authorized Security Research Use Only
 *
 * This header defines all pin assignments, peripheral configuration, and
 * board-level constants for the HDMI-Siphon transparent HDMI interposer.
 * Modify these values when porting to a different PCB revision or
 * custom hardware variant.
 *
 * Target: ESP32-S3 (ESP32-S3-WROOM-1 module)
 * Toolchain: ESP-IDF v5.1+
 */

#ifndef BOARD_H
#define BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* =========================================================================
 * System Clock Configuration
 * ========================================================================= */

/** CPU clock frequency for ESP32-S3 (240 MHz max) */
#define CONFIG_CPU_FREQ_MHZ            240

/** APB peripheral clock frequency (80 MHz typical) */
#define CONFIG_APB_FREQ_MHZ             80

/** SPI clock frequency for FPGA communication (Hz) */
#define CONFIG_SPI_FPGA_FREQ_HZ   20000000U

/** SPI clock frequency for SD card (Hz) */
#define CONFIG_SPI_SD_FREQ_HZ      20000000U

/** I²C clock frequency for EDID bus (Hz) — 100 kHz per HDMI spec */
#define CONFIG_I2C_FREQ_HZ           100000U

/** UART baud rate for debug console */
#define CONFIG_UART_BAUD             115200U

/** Watchdog timeout in milliseconds */
#define CONFIG_WDT_TIMEOUT_MS         10000U

/* =========================================================================
 * GPIO Pin Assignments
 * ========================================================================= */

/* ---- FPGA SPI Interface ---- */
#define PIN_SPI_FPGA_MOSI              1   /**< FPGA SPI MOSI          */
#define PIN_SPI_FPGA_MISO              2   /**< FPGA SPI MISO          */
#define PIN_SPI_FPGA_SCLK              3   /**< FPGA SPI clock         */
#define PIN_SPI_FPGA_CS                4   /**< FPGA SPI chip select   */

/* ---- FPGA Control Signals ---- */
#define PIN_FPGA_DONE                  5   /**< FPGA config done (input)   */
#define PIN_FPGA_CRESET                6   /**< FPGA config reset (output) */
#define PIN_FPGA_CDONE                 7   /**< FPGA config done (input)   */
#define PIN_FPGA_IRQ                  16   /**< FPGA interrupt request     */
#define PIN_FPGA_RDY                  17   /**< FPGA ready signal          */

/* ---- EDID/DDC I²C Bus ---- */
#define PIN_I2C_SDA                    8   /**< I²C data line           */
#define PIN_I2C_SCL                    9   /**< I²C clock line          */

/* ---- SD Card SPI Interface ---- */
#define PIN_SD_CS                     10   /**< SD card chip select     */
#define PIN_SD_MOSI                   11   /**< SD card SPI MOSI        */
#define PIN_SD_MISO                   12   /**< SD card SPI MISO        */
#define PIN_SD_SCLK                   13   /**< SD card SPI clock       */

/* ---- CEC Bus (HDMI Consumer Electronics Control) ---- */
#define PIN_CEC_RX                    14   /**< CEC bus receive (input) */
#define PIN_CEC_TX                    15   /**< CEC bus transmit (output) */

/* ---- USB Serial Debug ---- */
#define PIN_UART_TXD                  18   /**< UART TX to CH340       */
#define PIN_UART_RXD                  19   /**< UART RX from CH340     */

/* ---- Analog & Status ---- */
#define PIN_BAT_ADC                   20   /**< Battery voltage ADC    */
#define PIN_LED_DATA                  21   /**< RGB LED data (SK6812)  */

/* ---- User Buttons ---- */
#define PIN_BTN_CONFIG                42   /**< Configuration button   */
#define PIN_BTN_CAPTURE               43   /**< Capture trigger button */

/* =========================================================================
 * GPIO Configuration Macros
 * ========================================================================= */

/** @brief Configure a pin as GPIO input with pull-up */
#define GPIO_INPUT_PU(pin) do {                                         \
    gpio_config_t cfg = {                                               \
        .pin_bit_mask = (1ULL << (pin)),                                \
        .mode = GPIO_MODE_INPUT,                                        \
        .pull_up_en = GPIO_PULLUP_ENABLE,                               \
        .pull_down_en = GPIO_PULLDOWN_DISABLE,                          \
        .intr_type = GPIO_INTR_DISABLE                                  \
    };                                                                  \
    gpio_config(&cfg);                                                  \
} while(0)

/** @brief Configure a pin as GPIO output */
#define GPIO_OUTPUT(pin) do {                                           \
    gpio_config_t cfg = {                                               \
        .pin_bit_mask = (1ULL << (pin)),                                \
        .mode = GPIO_MODE_OUTPUT,                                       \
        .pull_up_en = GPIO_PULLUP_DISABLE,                              \
        .pull_down_en = GPIO_PULLDOWN_DISABLE,                          \
        .intr_type = GPIO_INTR_DISABLE                                  \
    };                                                                  \
    gpio_config(&cfg);                                                  \
} while(0)

/* =========================================================================
 * Power Management
 * ========================================================================= */

/** Battery ADC reference voltage (ESP32-S3 internal, mV) */
#define BAT_ADC_VREF_MV              2500U

/** Voltage divider ratio: R1=100k, R2=47k => ratio = (100+47)/47 = 3.128 */
#define BAT_DIVIDER_RATIO             3.128f

/** Battery full charge voltage (mV) — Li-Po 4.2V */
#define BAT_FULL_MV                   4200U

/** Battery critical voltage (mV) — Li-Po 3.3V cutoff */
#define BAT_CRITICAL_MV               3300U

/** Low battery warning threshold (mV) */
#define BAT_LOW_MV                    3600U

/* =========================================================================
 * EDID Configuration
 * ========================================================================= */

/** I²C address of the EDID EEPROM (24LC02, A0=0, A1=0, A2=0) */
#define EDID_EEPROM_ADDR              0x50

/** I²C address of the TCA9548 I²C mux */
#define TCA9548_ADDR                  0x70

/** TCA9548 channel for HDMI IN DDC */
#define TCA9548_CH_HDMI_IN            0x01

/** TCA9548 channel for HDMI OUT DDC */
#define TCA9548_CH_HDMI_OUT           0x02

/** TCA9548 channel for local EDID EEPROM */
#define TCA9548_CH_EDID_LOCAL         0x04

/** Standard EDID block size (128 bytes per block) */
#define EDID_BLOCK_SIZE               128U

/** Maximum EDID blocks (2 blocks = 256 bytes for extended EDID) */
#define EDID_MAX_BLOCKS                 2U

/** EDID header magic bytes */
#define EDID_HEADER_0                 0x00
#define EDID_HEADER_1                 0xFF
#define EDID_HEADER_2                 0xFF
#define EDID_HEADER_3                 0xFF
#define EDID_HEADER_4                 0xFF
#define EDID_HEADER_5                 0xFF
#define EDID_HEADER_6                 0xFF
#define EDID_HEADER_7                 0x00

/* =========================================================================
 * Capture & Storage Configuration
 * ========================================================================= */

/** Maximum capture resolution width (pixels) */
#define CAPTURE_MAX_WIDTH             1920U

/** Maximum capture resolution height (pixels) */
#define CAPTURE_MAX_HEIGHT            1080U

/** JPEG quality for captured frames (1-100) */
#define CAPTURE_JPEG_QUALITY           85U

/** Frame buffer size for double buffering (bytes) */
#define FRAME_BUFFER_SIZE         (CAPTURE_MAX_WIDTH * CAPTURE_MAX_HEIGHT * 3)

/** SD card mount point path */
#define SD_MOUNT_POINT                "/sdcard"

/** Capture output directory on SD card */
#define CAPTURE_DIR                   "/sdcard/captures"

/** Covert mode capture directory */
#define COVERT_DIR                    "/sdcard/covert"

/* =========================================================================
 * WiFi Configuration
 * ========================================================================= */

/** Default AP SSID for captive portal */
#define WIFI_AP_SSID                  "HDMI-Siphon-AP"

/** Default AP password (minimum 8 chars) */
#define WIFI_AP_PASS                  "siphon2026"

/** WiFi AP mode IP address */
#define WIFI_AP_IP                    "192.168.4.1"

/** WiFi AP mode netmask */
#define WIFI_AP_NETMASK               "255.255.255.0"

/** HTTP server port */
#define HTTP_SERVER_PORT               80

/** WebSocket server port */
#define WS_SERVER_PORT                 8080

/* =========================================================================
 * BLE GATT Service UUIDs
 * ========================================================================= */

/** HDMI-Siphon BLE service UUID (custom 128-bit) */
#define BLE_SERVICE_UUID              "6a4e9c80-1b3c-11ef-a35b-2b1a5c5a8b6c"

/** Command characteristic UUID */
#define BLE_CHAR_CMD_UUID             "6a4e9c81-1b3c-11ef-a35b-2b1a5c5a8b6c"

/** Status notification characteristic UUID */
#define BLE_CHAR_STATUS_UUID          "6a4e9c82-1b3c-11ef-a35b-2b1a5c5a8b6c"

/** Frame transfer characteristic UUID */
#define BLE_CHAR_FRAME_UUID           "6a4e9c83-1b3c-11ef-a35b-2b1a5c5a8b6c"

/* =========================================================================
 * Version Information
 * ========================================================================= */

#define FIRMWARE_VERSION_MAJOR           1
#define FIRMWARE_VERSION_MINOR           0
#define FIRMWARE_VERSION_PATCH           0

#define FIRMWARE_VERSION_STRING      "1.0.0"
#define DEVICE_NAME                   "HDMI-Siphon"
#define AUTHOR_STRING                 "jayis1"

/* =========================================================================
 * Inline Helper Functions
 * ========================================================================= */

/**
 * @brief Convert raw ADC reading to battery voltage in millivolts
 * @param adc_raw Raw 12-bit ADC value (0-4095)
 * @return Battery voltage in mV
 */
static inline uint32_t board_adc_to_mv(uint32_t adc_raw)
{
    uint32_t mv = (adc_raw * BAT_ADC_VREF_MV) / 4095U;
    mv = (uint32_t)((float)mv * BAT_DIVIDER_RATIO);
    return mv;
}

/**
 * @brief Get battery level as percentage
 * @param mv Current battery voltage in mV
 * @return Battery percentage (0-100)
 */
static inline uint32_t board_batt_percent(uint32_t mv)
{
    if (mv >= BAT_FULL_MV)     return 100U;
    if (mv <= BAT_CRITICAL_MV) return 0U;
    return ((mv - BAT_CRITICAL_MV) * 100U) / (BAT_FULL_MV - BAT_CRITICAL_MV);
}

/**
 * @brief Check if battery is critically low
 * @param mv Current battery voltage in mV
 * @return true if battery should shut down
 */
static inline bool board_batt_critical(uint32_t mv)
{
    return (mv <= BAT_CRITICAL_MV);
}

/**
 * @brief Check if battery is low (warning level)
 * @param mv Current battery voltage in mV
 * @return true if battery is low
 */
static inline bool board_batt_low(uint32_t mv)
{
    return (mv <= BAT_LOW_MV);
}

/**
 * @brief Initialize all board GPIO pins to default states
 *
 * Sets up every pin with correct direction, pull mode, and initial level.
 * Must be called once at startup before any peripheral initialization.
 */
void board_gpio_init(void);

/**
 * @brief Get the board's unique identifier (MAC address based)
 * @param id_out Output buffer (must be 8 bytes minimum)
 * @return Number of bytes written
 */
uint8_t board_get_unique_id(uint8_t *id_out);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */
