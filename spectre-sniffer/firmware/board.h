//==============================================================================
// board.h — Spectre-Sniffer Board Configuration and Pin Definitions
// Author: jayis1
// Description: Hardware pin mappings, peripheral configuration, and board-level
//              constants for the Spectre-Sniffer EM side-channel analysis device.
//==============================================================================

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stdbool.h>

//==============================================================================
// Version Information
//==============================================================================
#define BOARD_NAME              "Spectre-Sniffer"
#define BOARD_VERSION_MAJOR     1
#define BOARD_VERSION_MINOR     0
#define BOARD_VERSION_PATCH     0
#define BOARD_AUTHOR            "jayis1"
#define BOARD_HW_REVISION       "Rev A"

//==============================================================================
// System Clock Configuration (ESP32-S3)
//==============================================================================
#define CPU_FREQ_HZ             240000000       // 240 MHz dual-core
#define APB_CLK_FREQ_HZ         80000000        // APB bus clock
#define XTAL_FREQ_HZ            40000000        // External crystal
#define RTC_CLK_FREQ_HZ         32768           // RTC crystal

//==============================================================================
// Memory Configuration
//==============================================================================
#define PSRAM_SIZE_BYTES        (8 * 1024 * 1024)   // 8 MB external PSRAM
#define SRAM_SIZE_BYTES         (512 * 1024)         // 512 KB internal SRAM
#define CACHE_SIZE_BYTES        (16 * 1024)          // 16 KB instruction cache
#define FPGA_BITSTREAM_MAX_SIZE (256 * 1024)          // Max FPGA bitstream size

//==============================================================================
// GPIO Pin Mappings
//==============================================================================

// --- FPGA Interface (SPI + Control) ---
#define FPGA_SPI_HOST           2               // SPI2 controller
#define FPGA_SPI_MISO_GPIO      11              // FPGA SPI MISO
#define FPGA_SPI_MOSI_GPIO      12              // FPGA SPI MOSI
#define FPGA_SPI_CLK_GPIO       13              // FPGA SPI clock
#define FPGA_SPI_CS_GPIO        14              // FPGA SPI chip select
#define FPGA_CRESET_GPIO        15              // FPGA configuration reset
#define FPGA_CDONE_GPIO         16              // FPGA configuration done
#define FPGA_IRQ_GPIO           17              // FPGA interrupt request
#define FPGA_BUSY_GPIO          18              // FPGA busy signal

// --- ADC Interface (via FPGA bridge) ---
#define ADC_CLOCK_DIVIDER       2               // ADC clock = APB / 2 = 40 MHz
#define ADC_SAMPLE_RATE_HZ      130000000       // LTC2208 sample rate
#define ADC_RESOLUTION_BITS     16              // LTC2208 resolution
#define ADC_CHANNELS            1               // Single channel
#define ADC_VREF_MV             1500            // ADC reference voltage (mV)
#define ADC_FULL_SCALE_MV       2000            // ADC full-scale input (mV)
#define ADC_OVER_RANGE_THRESH   32750           // Near-full-scale warning

// --- Display (ILI9341 TFT + Touch) ---
#define DISPLAY_SPI_HOST        1               // SPI1 controller
#define DISPLAY_SPI_MISO_GPIO   37              // Display MISO (unused)
#define DISPLAY_SPI_MOSI_GPIO   35              // Display MOSI
#define DISPLAY_SPI_CLK_GPIO    36              // Display SPI clock
#define DISPLAY_CS_GPIO         34              // Display chip select
#define DISPLAY_DC_GPIO         33              // Display data/command
#define DISPLAY_RST_GPIO        32              // Display reset
#define DISPLAY_BL_GPIO         21              // Backlight PWM
#define DISPLAY_WIDTH_PX        320             // Display width
#define DISPLAY_HEIGHT_PX       240             // Display height
#define DISPLAY_BIT_DEPTH       18              // 18-bit color (RGB666)
#define DISPLAY_SPI_FREQ_HZ     40000000        // 40 MHz SPI clock

// --- Touch Controller (XPT2046, integrated on ILI9341 board) ---
#define TOUCH_SPI_HOST          1               // Shared SPI bus
#define TOUCH_CS_GPIO           38              // Touch chip select
#define TOUCH_IRQ_GPIO          39              // Touch interrupt

// --- SD Card (SPI mode) ---
#define SDCARD_SPI_HOST         2               // Shared SPI2 with FPGA (muxed)
#define SDCARD_CS_GPIO          10              // SD card chip select
#define SDCARD_DETECT_GPIO      9               // SD card detect switch
#define SDCARD_SPI_FREQ_HZ      20000000        // 20 MHz SPI clock

// --- RF Frontend Control ---
#define LNA1_ENABLE_GPIO        1               // LNA channel 1 enable
#define LNA2_ENABLE_GPIO        2               // LNA channel 2 enable
#define LNA_GAIN_CTRL_GPIO      3               // LNA gain select (0=low, 1=high)
#define FILTER_TUNE_CLK_GPIO    4               // Tunable filter clock (PE82305)
#define FILTER_TUNE_DATA_GPIO   5               // Tunable filter data
#define FILTER_TUNE_LE_GPIO     6               // Tunable filter latch enable
#define DOWNCONV_ENABLE_GPIO    7               // Downconverter enable
#define DOWNCONV_LO_SEL_GPIO    8               // Downconverter LO select

// --- RTC (DS3231) ---
#define RTC_SDA_GPIO            40              // I2C data
#define RTC_SCL_GPIO            41              // I2C clock
#define RTC_I2C_ADDR            0x68            // DS3231 I2C address
#define RTC_I2C_PORT            0               // I2C0 controller

// --- USB-UART (CP2102N) ---
#define USB_TX_GPIO             43              // UART TX
#define USB_RX_GPIO             44              // UART RX
#define USB_BAUD_RATE           115200          // Console baud rate

// --- WiFi / BLE (Internal to ESP32-S3) ---
#define WIFI_ANTENNA_GPIO       0               // WiFi antenna select (internal)
#define BLE_ANTENNA_GPIO        0               // BLE antenna select (internal)

// --- Battery / Power Management ---
#define BATTERY_ADC_GPIO        4               // Battery voltage divider input
#define BATTERY_CHARGING_GPIO   5               // Charging status input
#define BATTERY_FULL_GPIO       6               // Charge complete input
#define POWER_EN_3V3_GPIO       42              // 3.3V rail enable
#define POWER_EN_1V8_GPIO       41              // 1.8V rail enable (shared with RTC SCL)
#define POWER_EN_1V2_GPIO       40              // 1.2V rail enable (shared with RTC SDA)

// --- User Interface ---
#define BUTTON_A_GPIO           0               // Button A (boot/select)
#define BUTTON_B_GPIO           46              // Button B (back)
#define BUTTON_C_GPIO           45              // Button C (action)
#define LED_STATUS_GPIO         48              // Status LED (RGB)
#define LED_R_GPIO              47              // RGB LED red channel
#define LED_G_GPIO              21              // RGB LED green channel (shared with BL)
#define LED_B_GPIO              14              // RGB LED blue channel (shared with FPGA CS)

//==============================================================================
// FPGA Register Address Map (via SPI bridge)
//==============================================================================
#define FPGA_REG_CTRL           0x00    // Control register (R/W)
#define FPGA_REG_STATUS         0x01    // Status register (R)
#define FPGA_REG_ADC_CTRL       0x02    // ADC control (R/W)
#define FPGA_REG_DECIMATION     0x03    // Decimation factor (R/W)
#define FPGA_REG_FFT_CTRL       0x04    // FFT control (R/W)
#define FPGA_REG_FFT_STATUS     0x05    // FFT status (R)
#define FPGA_REG_TRIG_CTRL      0x06    // Trigger control (R/W)
#define FPGA_REG_TRIG_LEVEL     0x07    // Trigger level (R/W)
#define FPGA_REG_BUFFER_CTRL    0x08    // Buffer control (R/W)
#define FPGA_REG_BUFFER_STATUS  0x09    // Buffer status (R)
#define FPGA_REG_DATA_READY     0x0A    // Data ready flags (R)
#define FPGA_REG_SAMPLE_COUNT_L 0x0B    // Sample count low 32 bits (R)
#define FPGA_REG_SAMPLE_COUNT_H 0x0C    // Sample count high 32 bits (R)
#define FPGA_REG_FW_VERSION     0x0D    // FPGA firmware version (R)
#define FPGA_REG_TEMP_SENSOR    0x0E    // FPGA die temperature (R)
#define FPGA_REG_DEBUG          0x0F    // Debug register (R/W)

//==============================================================================
// FPGA Control Register Bit Definitions
//==============================================================================
#define FPGA_CTRL_RESET         (1 << 0)    // FPGA soft reset
#define FPGA_CTRL_ADC_ENABLE    (1 << 1)    // ADC capture enable
#define FPGA_CTRL_FFT_ENABLE    (1 << 2)    // FFT engine enable
#define FPGA_CTRL_TRIG_ENABLE   (1 << 3)    // Trigger enable
#define FPGA_CTRL_CONT_CAPTURE  (1 << 4)    // Continuous capture mode
#define FPGA_CTRL_SINGLE_CAPTURE (1 << 5)   // Single-shot capture
#define FPGA_CTRL_BYPASS_FILTER (1 << 6)    // Bypass FIR decimation
#define FPGA_CTRL_LOOPBACK      (1 << 7)    // Test mode: loopback data

//==============================================================================
// FPGA Status Register Bit Definitions
//==============================================================================
#define FPGA_STAT_ADC_LOCKED    (1 << 0)    // ADC PLL locked
#define FPGA_STAT_ADC_OVERFLOW  (1 << 1)    // ADC overflow detected
#define FPGA_STAT_FFT_DONE      (1 << 2)    // FFT computation done
#define FPGA_STAT_TRIGGERED     (1 << 3)    // Trigger event occurred
#define FPGA_STAT_BUFFER_FULL   (1 << 4)    // Capture buffer full
#define FPGA_STAT_BUFFER_HALF   (1 << 5)    // Capture buffer half-full
#define FPGA_STAT_FIFO_EMPTY    (1 << 6)    // Data FIFO empty
#define FPGA_STAT_FIFO_FULL     (1 << 7)    // Data FIFO full

//==============================================================================
// Signal Processing Constants
//==============================================================================
#define FFT_SIZE                1024            // FFT points
#define FFT_BINS                (FFT_SIZE / 2)  // Real FFT output bins
#define FFT_WINDOW_HANNING      0               // Hanning window type
#define FFT_WINDOW_BLACKMAN     1               // Blackman window type
#define FFT_WINDOW_FLATTOP      2               // Flat-top window type
#define FFT_DEFAULT_WINDOW      FFT_WINDOW_HANNING

#define MAX_DECIMATION_FACTOR   256             // Maximum FIR decimation
#define MIN_DECIMATION_FACTOR   1               // Minimum decimation (no decimation)
#define DEFAULT_DECIMATION      8               // Default decimation factor

#define CAPTURE_BUFFER_SIZE     (1024 * 1024)   // 1M sample capture buffer
#define TRACE_LENGTH            1024            // Samples per crypto trace
#define MAX_TRACES              65536           // Maximum stored traces
#define TEMPEST_LINE_WIDTH      1024            // Tempest line buffer width
#define TEMPEST_MAX_LINES       768             // Tempest max lines per frame

//==============================================================================
// RF Frontend Configuration
//==============================================================================
#define RF_MIN_FREQ_HZ          10000000        // 10 MHz minimum
#define RF_MAX_FREQ_HZ          8000000000ULL   // 8 GHz maximum
#define RF_DOWNCONV_THRESHOLD   400000000       // 400 MHz: use downconverter above
#define RF_LNA_GAIN_LOW         10              // Low gain setting (dB)
#define RF_LNA_GAIN_HIGH        20              // High gain setting (dB)
#define RF_DEFAULT_CENTER_FREQ  100000000       // 100 MHz default center freq
#define RF_DEFAULT_BANDWIDTH    10000000        // 10 MHz default bandwidth

//==============================================================================
// Display Configuration
//==============================================================================
#define SPECTRUM_WATERFALL_LINES 200            // Waterfall history lines
#define SPECTRUM_PEAK_HOLD_MS   2000            // Peak hold persistence (ms)
#define SPECTRUM_REFRESH_MS     50              // Spectrum refresh interval (ms)
#define TEMPEST_REFRESH_MS      100             // Tempest refresh interval (ms)
#define UI_TICK_MS              20              // UI update tick interval (ms)

//==============================================================================
// Storage Configuration
//==============================================================================
#define SDCARD_MOUNT_POINT      "/sdcard"
#define CAPTURE_DIR             "/sdcard/captures"
#define TEMPEST_DIR             "/sdcard/tempest"
#define TRACES_DIR              "/sdcard/traces"
#define CONFIG_FILE             "/sdcard/config.json"
#define CALIBRATION_FILE        "/sdcard/calibration.json"
#define LOG_FILE                "/sdcard/spectre.log"
#define MAX_FILENAME_LEN        64
#define MAX_FILE_SIZE_MB        4096            // Max single capture file (4 GB)

//==============================================================================
// Network Configuration
//==============================================================================
#define WIFI_AP_SSID            "Spectre-Sniffer"
#define WIFI_AP_PASS            "spectre123"
#define WIFI_AP_CHANNEL         6
#define HTTP_PORT               80
#define WEBSOCKET_PORT          81
#define BLE_ADVERTISE_INTERVAL  100             // BLE advertisement interval (ms)
#define BLE_STATUS_UUID         "180F"          // Battery service UUID

//==============================================================================
// Battery and Power Management
//==============================================================================
#define BATTERY_FULL_MV         4200            // Full battery voltage (mV)
#define BATTERY_EMPTY_MV        3200            // Empty battery voltage (mV)
#define BATTERY_DIVIDER_RATIO   2.0f            // Voltage divider ratio
#define BATTERY_ADC_ATTEN       ADC_ATTEN_DB_11 // 0-3.3V input range
#define POWER_SAVE_TIMEOUT_MS   300000          // 5 min inactivity -> sleep
#define DEEP_SLEEP_WAKEUP_GPIO  BUTTON_A_GPIO   // Button A wakes from sleep

//==============================================================================
// Error Codes
//==============================================================================
typedef enum {
    SPECTRE_OK                  = 0,
    SPECTRE_ERR_GENERAL         = -1,
    SPECTRE_ERR_TIMEOUT         = -2,
    SPECTRE_ERR_NOMEM           = -3,
    SPECTRE_ERR_INVALID_PARAM   = -4,
    SPECTRE_ERR_NOT_INIT        = -5,
    SPECTRE_ERR_BUSY            = -6,
    SPECTRE_ERR_IO              = -7,
    SPECTRE_ERR_FPGA            = -8,
    SPECTRE_ERR_ADC             = -9,
    SPECTRE_ERR_SD_CARD         = -10,
    SPECTRE_ERR_WIFI            = -11,
    SPECTRE_ERR_BLE             = -12,
    SPECTRE_ERR_FILE            = -13,
    SPECTRE_ERR_CALIBRATION     = -14,
    SPECTRE_ERR_OVERFLOW        = -15,
    SPECTRE_ERR_ABORTED         = -16,
} spectre_err_t;

//==============================================================================
// Operating Modes
//==============================================================================
typedef enum {
    MODE_IDLE                   = 0,
    MODE_SPECTRUM_ANALYZER      = 1,
    MODE_IQ_CAPTURE             = 2,
    MODE_TEMPEST_DECODE         = 3,
    MODE_CRYPTO_ANALYSIS        = 4,
    MODE_KEYSTROKE_ANALYSIS     = 5,
    MODE_SIGNAL_GENERATOR       = 6,
    MODE_CALIBRATION            = 7,
    MODE_SETTINGS               = 8,
    MODE_FILE_BROWSER           = 9,
    MODE_SLEEP                  = 10,
} operating_mode_t;

//==============================================================================
// Probe Types
//==============================================================================
typedef enum {
    PROBE_H_FIELD               = 0,    // Magnetic field (shielded loop)
    PROBE_E_FIELD               = 1,    // Electric field (monopole tip)
    PROBE_LOG_PERIODIC          = 2,    // Log-periodic directional antenna
    PROBE_ACTIVE_DIFFERENTIAL   = 3,    // Active differential H-field
    PROBE_UNKNOWN               = 0xFF,
} probe_type_t;

//==============================================================================
// Calibration Data Structure
//==============================================================================
typedef struct {
    probe_type_t    probe_type;
    float           freq_start_hz;
    float           freq_end_hz;
    float           freq_step_hz;
    float           gain_correction_db[256];    // Per-frequency gain correction
    float           phase_correction_deg[256];  // Per-frequency phase correction
    float           noise_floor_dbm[256];       // Per-frequency noise floor
    uint32_t        calibration_date;           // Unix timestamp
    uint32_t        checksum;                   // Data integrity check
} __attribute__((packed)) calibration_data_t;

//==============================================================================
// Capture Configuration
//==============================================================================
typedef struct {
    uint64_t        center_freq_hz;
    uint32_t        bandwidth_hz;
    uint32_t        sample_rate_hz;
    uint16_t        decimation_factor;
    uint32_t        duration_ms;
    bool            use_downconverter;
    bool            enable_lna;
    uint8_t         lna_gain_db;
    bool            enable_filter;
    uint32_t        filter_center_hz;
    bool            enable_trigger;
    int16_t         trigger_level;
    uint32_t        trigger_delay_samples;
    uint32_t        pretrigger_samples;
    char            filename[MAX_FILENAME_LEN];
} capture_config_t;

//==============================================================================
// Tempest Decode Configuration
//==============================================================================
typedef struct {
    uint64_t        pixel_clock_hz;
    uint32_t        h_resolution;
    uint32_t        v_resolution;
    uint32_t        h_front_porch;
    uint32_t        h_sync_pulse;
    uint32_t        h_back_porch;
    uint32_t        v_front_porch;
    uint32_t        v_sync_pulse;
    uint32_t        v_back_porch;
    bool            interlaced;
    bool            invert_sync;
    uint8_t         bit_depth;
} tempest_config_t;

//==============================================================================
// DPA/CPA Configuration
//==============================================================================
typedef struct {
    uint32_t        num_traces;
    uint32_t        trace_length;
    uint32_t        num_key_bytes;
    uint8_t         target_algorithm;   // 0=AES-128, 1=AES-256, 2=DES, 3=TDES
    uint8_t         leakage_model;      // 0=Hamming weight, 1=Hamming distance
    uint8_t         analysis_type;      // 0=DPA, 1=CPA
    uint32_t        sample_point_start;
    uint32_t        sample_point_end;
    bool            use_reference_trace;
    uint32_t        known_plaintext[16]; // Known plaintext bytes
} crypto_analysis_config_t;

//==============================================================================
// Inline Helper Functions
//==============================================================================

// Convert frequency to downconverter LO frequency
static inline uint64_t board_lo_freq(uint64_t center_freq_hz) {
    if (center_freq_hz <= RF_DOWNCONV_THRESHOLD) {
        return 0;  // Direct mode, no downconversion
    }
    // Downconvert to 100 MHz IF
    return center_freq_hz - 100000000;
}

// Convert ADC sample to millivolts
static inline int32_t board_sample_to_mv(int16_t sample) {
    return (int32_t)sample * ADC_FULL_SCALE_MV / 32768;
}

// Convert battery ADC reading to percentage
static inline uint8_t board_battery_percent(uint16_t adc_reading) {
    uint32_t voltage_mv = (uint32_t)adc_reading * 3300 / 4096 * 2;  // *2 for divider
    if (voltage_mv >= BATTERY_FULL_MV) return 100;
    if (voltage_mv <= BATTERY_EMPTY_MV) return 0;
    return (uint8_t)((voltage_mv - BATTERY_EMPTY_MV) * 100 /
                     (BATTERY_FULL_MV - BATTERY_EMPTY_MV));
}

#endif // BOARD_H
