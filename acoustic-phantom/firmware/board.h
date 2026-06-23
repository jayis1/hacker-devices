/*
 * ACOUSTIC-PHANTOM — Board configuration
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * Pin assignments, clock tree, and hardware constants for the
 * STM32H743VIT6-based acoustic side-channel capture platform.
 */
#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ---- Author / version metadata ----------------------------------------- */
#define AUTHOR              "jayis1"
#define FW_VERSION_MAJOR    1
#define FW_VERSION_MINOR    0
#define FW_VERSION_PATCH    0
#define FW_VERSION_STR      "1.0.0"

/* ---- MCU clock configuration ------------------------------------------- */
#define SYSTEM_CLOCK_HZ     480000000UL
#define SYSCLK_HZ           SYSTEM_CLOCK_HZ
#define HCLK_HZ             SYSTEM_CLOCK_HZ
#define APB1_HZ             (HCLK_HZ / 4)   /* 120 MHz */
#define APB2_HZ             (HCLK_HZ / 2)   /* 240 MHz */
#define APB1L_HZ            APB1_HZ
#define SYSTICK_HZ          1000

/* ---- Audio configuration ----------------------------------------------- */
#define AUDIO_SAMPLE_RATE   48000
#define AUDIO_BITS          16
#define AUDIO_CHANNELS      2       /* 0=beamformed MEMS, 1=contact piezo */
#define AUDIO_FRAME_MS      25      /* 25 ms analysis frame */
#define AUDIO_HOP_MS        10      /* 10 ms hop */
#define FRAME_SAMPLES       (AUDIO_SAMPLE_RATE * AUDIO_FRAME_MS / 1000)  /* 1200 */
#define HOP_SAMPLES         (AUDIO_SAMPLE_RATE * AUDIO_HOP_MS / 1000)    /* 480 */
#define FFT_SIZE            512
#define MFCC_COEFFS         13
#define MFCC_FEATURES       (MFCC_COEFFS * 3)  /* + delta + delta-delta = 39 */

/* ---- MEMS microphone array --------------------------------------------- */
#define NUM_MICS            4
#define MIC_SPACING_MM      13.0f   /* linear array spacing */
#define SPEED_OF_SOUND      343000.0f  /* mm/s */
#define MAX_BEAM_STEPS      32      /* beamforming steering directions */

/* ---- Event detection --------------------------------------------------- */
#define EVENT_CONTEXT_MS    150     /* ±150 ms context window */
#define EVENT_CONTEXT_SAMP  (AUDIO_SAMPLE_RATE * EVENT_CONTEXT_MS / 1000)
#define ONSET_THRESHOLD     0.18f   /* energy onset detection threshold */
#define MIN_EVENT_GAP_MS    30      /* minimum gap between events */
#define MIN_EVENT_GAP_SAMP  (AUDIO_SAMPLE_RATE * MIN_EVENT_GAP_MS / 1000)

/* ---- TFLM model sizes -------------------------------------------------- */
#define MODEL_MAX_SIZE      (256 * 1024)
#define CALIB_CLASSES_MAX   128     /* max keys in calibration */
#define CALIB_SAMPLES_PER   20      /* samples per key during calibration */
#define NGRAM_LM_SIZE       (512 * 1024)

/* ---- Storage ----------------------------------------------------------- */
#define NOR_PAGE_SIZE       256
#define NOR_SECTOR_SIZE     4096
#define NOR_TOTAL_SIZE      (16 * 1024 * 1024)
#define SD_AUDIO_CHUNK      8192

/* ---- Pin assignments (STM32H743VIT6 LQFP-100) -------------------------- */

/* I²S2 — MEMS microphone array (4× SPH0641LU4H-1, PDM via I²S) */
#define I2S2_CK_PIN         12      /* PB12 */
#define I2S2_CK_PORT        GPIOB
#define I2S2_WS_PIN         12      /* PB12 shared, alt config */
#define I2S2_SD_PIN         14      /* PB14 */
#define I2S2_SD_PORT        GPIOB
#define I2S2_AF             5       /* SPI2/I2S2 alternate function */

/* I²S3 — WM8904 codec (contact piezo + ambient reference) */
#define I2S3_CK_PIN         3       /* PB3 */
#define I2S3_CK_PORT        GPIOB
#define I2S3_WS_PIN         4       /* PA4 */
#define I2S3_WS_PORT        GPIOA
#define I2S3_SD_PIN         15      /* PB15 */
#define I2S3_SD_PORT        GPIOB
#define I2S3_AF             6       /* SPI3/I2S3 alternate function */

/* I2C1 — OLED (SSD1306), fuel gauge (BQ27421), WM8904 control */
#define I2C1_SCL_PIN        6       /* PB6 */
#define I2C1_SCL_PORT       GPIOB
#define I2C1_SDA_PIN        7       /* PB7 */
#define I2C1_SDA_PORT       GPIOB
#define I2C1_AF             4
#define I2C1_ADDR_OLED      0x3C
#define I2C1_ADDR_FUEL      0x55
#define I2C1_ADDR_CODEC     0x1A

/* QSPI — W25Q128 NOR flash (model storage, XIP) */
#define QSPI_CS_PIN         6       /* PB6 — note: shared via mux, dedicated alt */
#define QSPI_CLK_PIN        2       /* PB2 */
#define QSPI_IO0_PIN        9       /*PD9 */
#define QSPI_IO1_PIN        8       /* PD8 */
#define QSPI_IO2_PIN        11      /* PE11 */
#define QSPI_IO3_PIN        12      /* PE12 */

/* USART1 — BLE module (CYW20819) */
#define BLE_UART_TX_PIN     9       /* PA9 */
#define BLE_UART_TX_PORT    GPIOA
#define BLE_UART_RX_PIN     10      /* PA10 */
#define BLE_UART_RX_PORT    GPIOA
#define BLE_UART_BAUD       115200
#define BLE_UART_AF         7

/* SDMMC1 — microSD */
#define SDMMC_CK_PIN        12      /* PC12 */
#define SDMMC_CMD_PIN       2       /* PD2 */
#define SDMMC_D0_PIN        8       /* PC8 */
#define SDMMC_D1_PIN        9       /* PC9 */
#define SDMMC_D2_PIN        10      /* PC10 */
#define SDMMC_D3_PIN        11      /* PC11 */
#define SDMMC_AF            12

/* USB-C — USB 2.0 FS (USART/DFU/charging) */
#define USB_DM_PIN          11      /* PA11 */
#define USB_DP_PIN          12      /* PA12 */

/* GPIO — buttons, LEDs, tamper */
#define BTN_POWER_PIN       0       /* PA0 */
#define BTN_POWER_PORT      GPIOA
#define BTN_PROFILE_PIN     1       /* PC1 */
#define BTN_PROFILE_PORT    GPIOC
#define BTN_ARM_PIN         13      /* PC13 */
#define BTN_ARM_PORT        GPIOC

#define LED_WS2812_PIN      5       /* PA5, TIM2_CH1 for WS2812B */
#define LED_WS2812_PORT     GPIOA
#define LED_COUNT           3

#define TAMPER_IRQ_PIN      15      /* PC15, EXTI15 */
#define TAMPER_IRQ_PORT     GPIOC

/* ---- Attack profiles --------------------------------------------------- */
typedef enum {
    PROFILE_KEYBOARD = 0,
    PROFILE_HDD      = 1,
    PROFILE_PRINTER  = 2,
    PROFILE_SMPS     = 3,
    PROFILE_RELAY    = 4,
    PROFILE_COUNT
} attack_profile_t;

/* ---- Device state machine ---------------------------------------------- */
typedef enum {
    STATE_IDLE       = 0,
    STATE_ARMED      = 1,
    STATE_CAPTURING  = 2,
    STATE_CALIBRATING= 3,
    STATE_STORAGE    = 4,
    STATE_TAMPER     = 5
} device_state_t;

/* ---- Function prototypes (board-level) --------------------------------- */
void board_init(void);
void board_get_uid(uint32_t uid[3]);
uint32_t board_millis(void);
void board_delay_ms(uint32_t ms);

/* LED control */
void led_set(uint8_t idx, uint8_t r, uint8_t g, uint8_t b);
void led_update(void);

/* Button state (debounced) */
uint8_t btn_pressed(uint8_t idx);

/* OLED */
void oled_init(void);
void oled_clear(void);
void oled_draw_str(uint8_t x, uint8_t y, const char *str);
void oled_draw_line(uint8_t y, const char *str);
void oled_refresh(void);

#endif /* BOARD_H */