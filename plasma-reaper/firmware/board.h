/*
 * board.h — Board-level definitions for PlasmaReaper
 * Multi-Vector Fault Injection Toolkit
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef PLASMA_REAPER_BOARD_H
#define PLASMA_REAPER_BOARD_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ---- Pin assignments (STM32H723ZGT6) ------------------------------- */

/* FPGA SPI (SPI1) */
#define FPGA_SPI_SCK_PIN    5    /* PA5 */
#define FPGA_SPI_MISO_PIN   6    /* PA6 */
#define FPGA_SPI_MOSI_PIN   7    /* PA7 */
#define FPGA_NSS_PIN        4    /* PA4 */
#define FPGA_CDONE_PIN      3    /* PA3 */
#define FPGA_CRESET_PIN     2    /* PA2 */

/* DAC (SPI2) for glitch depth */
#define DAC_NSS_PIN         12   /* PB12 */

/* USB CDC (USB peripheral) — fixed pins PA11/PA12 */

/* USART1 — debug console */
#define DEBUG_UART_TX_PIN   9    /* PA9 */
#define DEBUG_UART_RX_PIN   10   /* PA10 */

/* USART2 — BLE module (STM32WB55) */
#define BLE_UART_TX_PIN     2    /* PD2 */
#define BLE_UART_RX_PIN     3    /* PD3 */

/* USART3 — target UART monitor */
#define TARGET_UART_TX_PIN  8    /* PD8 */
#define TARGET_UART_RX_PIN  9    /* PD9 */
#define TARGET_UART_RTS_PIN 11   /* PD11 */

/* SDIO — SD card (fixed pins PC8-PC12, PD2) */

/* I2C1 — OLED, SiT9102 oscillator */
#define I2C_SCL_PIN         8    /* PB8 */
#define I2C_SDA_PIN         9    /* PB9 */

/* HRTIM outputs */
#define HRTIM_POWER_GLITCH  3    /* HRTIM Timer F, output HF2 */
#define HRTIM_CLOCK_GLITCH  4    /* HRTIM Timer D, output HD2 */
#define HRTIM_EM_PULSE      5    /* HRTIM Timer E, output HE2 */

/* ADC1 — VCC rail monitor (PA0, channel 0) */
#define VCC_MONITOR_PIN     0    /* PA0 */
#define VCC_MONITOR_CHANNEL 0

/* Trigger inputs (FPGA) */
#define TRIG_GPIO_INPUT_PIN 0    /* PC0 */
#define TRIG_EXT_SYNC_PIN   1    /* PC1 */
#define TRIG_UART_WORD_PIN  2    /* PC2 (MCU drives FPGA) */
#define TRIG_MANUAL_PIN     3    /* PC3 (MCU drives FPGA) */
#define TRIG_TIMER_PIN      4    /* PC4 (MCU HRTIM output to FPGA) */

/* Glitch output control (FPGA -> output stage) */
#define POWER_SHUNT_GATE    0    /* PB0 (FPGA output) */
#define CLOCK_MUX_SEL0      1    /* PB1 */
#define CLOCK_MUX_SEL1      2    /* PB2 */
#define EM_PULSE_GATE       3    /* PB3 */

/* Series resistance mux (MCU direct) */
#define SERIES_R_SEL0       14   /* PE14 */
#define SERIES_R_SEL1       15   /* PE15 */
#define SERIES_R_SEL2       13   /* PE13 */

/* Status LED */
#define STATUS_LED_PIN      1    /* PB1 — reuse conflicts; moved to PE1 */
#define STATUS_LED_PORT     4    /* Port E index */
#define STATUS_LED_PIN_ALT  1    /* PE1 */

/* OLED reset */
#define OLED_RESET_PIN      7    /* PE7 */

/* Boost converter enable (EM pulse supply) */
#define BOOST_EN_PIN        5    /* PE5 */

/* ---- Clock configuration ------------------------------------------- */

#define SYSTEM_CLOCK_HZ     550000000ULL  /* 550 MHz */
#define HRTIM_CLOCK_HZ      275000000ULL  /* HRTIM runs at SYSCLK/2 */
#define HRTIM_RESOLUTION_PS 184           /* 1 / 275 MHz */

/* ---- Glitch vector masks ------------------------------------------- */

#define GLITCH_VECTOR_POWER 0x01
#define GLITCH_VECTOR_CLOCK 0x02
#define GLITCH_VECTOR_EM    0x04

/* ---- Series resistance table (ohms) -------------------------------- */

#define SERIES_R_TABLE_SIZE 8
extern const float series_resistance_table[SERIES_R_TABLE_SIZE];

/* ---- Glitch parameter structure ------------------------------------ */

typedef struct {
    uint8_t  vector_mask;        /* POWER | CLOCK | EM bit flags       */
    uint32_t trigger_offset_ns;  /* delay from trigger to glitch start */
    uint32_t glitch_width_ns;    /* glitch duration                    */
    uint16_t power_depth_mv;     /* power glitch depth (mV)            */
    uint8_t  power_series_r;     /* series resistance index (0-7)      */
    uint8_t  clock_shape;        /* 0 = extra edge, 1 = suppress       */
    uint32_t clock_cycle_offset; /* glitch position in clock cycles    */
    uint16_t em_pulse_mv;        /* EM pulse voltage (mV)              */
    uint16_t em_pulse_width_ns;  /* EM pulse width (ns)                */
    uint32_t inter_vector_ns;    /* delay between vectors (joint)      */
    uint16_t repeat_count;       /* shots per trigger                  */
    uint32_t repeat_delay_ns;    /* delay between repeats              */
} glitch_params_t;

/* ---- Trigger configuration ----------------------------------------- */

typedef enum {
    TRIG_SOURCE_GPIO = 0,
    TRIG_SOURCE_UART_WORD,
    TRIG_SOURCE_POWER_ENV,
    TRIG_SOURCE_MANUAL,
    TRIG_SOURCE_TIMER,
    TRIG_SOURCE_EXT_SYNC,
    TRIG_SOURCE_COUNT
} trigger_source_t;

typedef struct {
    trigger_source_t source;
    uint8_t  uart_word[16];      /* trigger word for UART source       */
    uint8_t  uart_word_len;      /* length of trigger word             */
    uint16_t power_env_threshold_mv; /* power envelope threshold      */
    uint8_t  trigger_edge;       /* 0 = rising, 1 = falling           */
} trigger_config_t;

/* ---- Result classification ----------------------------------------- */

typedef enum {
    RESULT_PENDING = 0,
    RESULT_SUCCESS,
    RESULT_FAILURE,
    RESULT_NO_RESPONSE,
    RESULT_GLITCH_INVALID,
    RESULT_TIMEOUT,
    RESULT_COUNT
} result_class_t;

typedef struct {
    result_class_t outcome;
    uint32_t shots_fired;
    uint32_t elapsed_us;
    uint16_t waveform[256];      /* VCC rail samples during glitch     */
    uint16_t waveform_len;
    char     target_response[128];
} glitch_result_t;

/* ---- Success/failure pattern matching ------------------------------ */

#define MAX_PATTERNS 16
#define MAX_PATTERN_LEN 32

typedef struct {
    char success_patterns[MAX_PATTERNS][MAX_PATTERN_LEN];
    uint8_t success_pattern_count;
    char failure_patterns[MAX_PATTERNS][MAX_PATTERN_LEN];
    uint8_t failure_pattern_count;
} pattern_config_t;

/* ---- Sweep configuration ------------------------------------------- */

typedef enum {
    SWEEP_PARAM_OFFSET = 0,
    SWEEP_PARAM_WIDTH,
    SWEEP_PARAM_DEPTH,
    SWEEP_PARAM_EM_VOLTAGE,
    SWEEP_PARAM_EM_WIDTH,
    SWEEP_PARAM_CLOCK_CYCLE,
    SWEEP_PARAM_SERIES_R,
    SWEEP_PARAM_COUNT
} sweep_param_t;

typedef struct {
    sweep_param_t x_param;
    uint32_t x_start, x_end, x_step;
    sweep_param_t y_param;
    uint32_t y_start, y_end, y_step;
    sweep_param_t z_param;       /* outer loop, 0 = unused             */
    uint32_t z_start, z_end, z_step;
    glitch_params_t base_params; /* template for each shot             */
    uint16_t shots_per_cell;     /* repeats per cell for statistics    */
    uint32_t cell_timeout_ms;    /* max wait per cell                  */
    bool adaptive;               /* hill-climbing mode                 */
} sweep_config_t;

typedef struct {
    uint32_t total_cells;
    uint32_t completed_cells;
    uint32_t success_count;
    uint32_t failure_count;
    uint32_t no_response_count;
    uint32_t invalid_count;
    bool running;
} sweep_status_t;

/* ---- Function prototypes ------------------------------------------- */

void board_init(void);
void board_led_set(bool on);
void board_led_toggle(void);

#endif /* PLASMA_REAPER_BOARD_H */