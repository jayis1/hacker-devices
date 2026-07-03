/*
 * board.h — RadarPhantom pin map, peripheral assignments, constants
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Target: STM32H743VIT6 (Cortex-M7, 480 MHz, 2 MB Flash, 1 MB SRAM).
 * Defines all GPIO pin roles, peripheral mappings, board constants, and
 * compile-time configuration for the RadarPhantom mmWave DRFM repeater.
 */
#ifndef RADARPHANTOM_BOARD_H
#define RADARPHANTOM_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ---- Author / build metadata ----------------------------------------- */
#define RP_AUTHOR          "jayis1"
#define RP_FW_VERSION      0x0100   /* 1.0 */
#define RP_HW_VERSION      0x0100
#define RP_DEVICE_NAME     "RadarPhantom"

/* ---- Clock tree ------------------------------------------------------ */
#define HSE_VALUE_HZ       25000000ULL   /* 25 MHz XTAL on X1 */
#define LSE_VALUE_HZ       32768ULL
#define SYSCLK_HZ          480000000ULL  /* 480 MHz core */
#define AHB_HZ             240000000ULL
#define APB1_HZ            120000000ULL
#define APB2_HZ            120000000ULL
#define APB4_HZ            120000000ULL

/* ---- LED + status ---------------------------------------------------- */
#define LED_STATUS_PORT     GPIOB
#define LED_STATUS_PIN      0            /* PB0  green  */
#define LED_ARM_PORT        GPIOB
#define LED_ARM_PIN         1            /* PB1  red    */
#define LED_FAULT_PORT      GPIOB
#define LED_FAULT_PIN       2            /* PB2  amber  */

/* ---- OLED (SH1107, 128x96) on SPI1 ---------------------------------- */
#define OLED_SPI            SPI1
#define OLED_CS_PORT        GPIOA
#define OLED_CS_PIN         4             /* PA4  */
#define OLED_DC_PORT         GPIOA
#define OLED_DC_PIN         5             /* PA5  (0=cmd,1=data) */
#define OLED_RST_PORT       GPIOA
#define OLED_RST_PIN        6             /* PA6  */
/* SPI1 SCK=PA5? conflict — use alternate: SCK=PB3, MISO=PB4, MOSI=PB5 */
#define OLED_SCK_PORT       GPIOB
#define OLED_SCK_PIN        3
#define OLED_MISO_PORT      GPIOB
#define OLED_MISO_PIN       4
#define OLED_MOSI_PORT      GPIOB
#define OLED_MOSI_PIN       5
#define OLED_WIDTH          128
#define OLED_HEIGHT         96

/* ---- 5-way joystick on ADC1 + GPIO ---------------------------------- */
/* Analog: UP/DN/L/R form a resistor ladder on PA0 (ADC1_INP0) */
#define JOY_ADC             ADC1
#define JOY_ADC_CHANNEL     5             /* channel 5 (PA0_C) */
#define JOY_PRESS_PORT      GPIOC
#define JOY_PRESS_PIN       13           /* PC13 center press */
/* Thresholds (12-bit ADC, 0..4095) for the ladder */
#define JOY_TH_NONE_HI      4050
#define JOY_TH_RIGHT_LO     3200
#define JOY_TH_RIGHT_HI     3400
#define JOY_TH_DOWN_LO      2700
#define JOY_TH_DOWN_HI      2900
#define JOY_TH_LEFT_LO      2200
#define JOY_TH_LEFT_HI     2400
#define JOY_TH_UP_LO        1600
#define JOY_TH_UP_HI        1800

/* ---- DRFM FPGA (iCE40UP5K) on SPI2 ---------------------------------- */
#define DRFM_SPI            SPI2
#define DRFM_NSS_PORT       GPIOB
#define DRFM_NSS_PIN        12            /* PB12 */
#define DRFM_SCK_PORT       GPIOB
#define DRFM_SCK_PIN        13            /* PB13 */
#define DRFM_MISO_PORT      GPIOB
#define DRFM_MISO_PIN      14            /* PB14 */
#define DRFM_MOSI_PORT      GPIOB
#define DRFM_MOSI_PIN       15            /* PB15 */
#define DRFM_CDONE_PORT     GPIOC
#define DRFM_CDONE_PIN      6             /* PC6 config-done */
#define DRFM_CRESET_PORT    GPIOC
#define DRFM_CRESET_PIN     7             /* PC7 reset */

/* DRFM FPGA SPI register map (8-bit addr, 8-bit cmd, payload) */
#define DRFM_REG_STATUS     0x00         /* RO: bit0=ready, bit1=armed, bit2=overflow */
#define DRFM_REG_CTRL       0x01         /* W: bit0=arm, bit1=clr, bit2=noise_mode */
#define DRFM_REG_DELAY_LSB  0x02         /* 24-bit delay in 5 ns units */
#define DRFM_REG_DELAY_MID  0x03
#define DRFM_REG_DELAY_MSB  0x04
#define DRFM_REG_DOPP_LSB   0x05         /* 24-bit signed Doppler NCO freq (mHz) */
#define DRFM_REG_DOPP_MID   0x06
#define DRFM_REG_DOPP_MSB   0x07
#define DRFM_REG_GAIN       0x08         /* 8-bit amplitude scale (0..255) */
#define DRFM_REG_NTAPS      0x09         /* number of active taps (1..4) */
#define DRFM_REG_TAP0_DLY   0x0A         /* tap relative delay LSB..MSB (3 each) */
#define DRFM_REG_TAP0_GAIN  0x0D
#define DRFM_REG_TAP0_DOPP  0x0E         /* tap relative doppler */
#define DRFM_REG_TAP1_DLY   0x0F
#define DRFM_REG_TAP1_GAIN  0x12
#define DRFM_REG_TAP1_DOPP  0x13
#define DRFM_REG_TAP2_DLY   0x14
#define DRFM_REG_TAP2_GAIN  0x17
#define DRFM_REG_TAP2_DOPP  0x18
#define DRFM_REG_TAP3_DLY   0x19
#define DRFM_REG_TAP3_GAIN  0x1C
#define DRFM_REG_TAP3_DOPP  0x1D
#define DRFM_REG_MD_SRC     0x1E         /* micro-doppler waveform select */
#define DRFM_REG_MD_DEPTH   0x1F         /* micro-doppler modulation depth */

/* ---- LO PLL (ADF41520) on SPI3 ------------------------------------- */
#define LO_SPI              SPI3
#define LO_NSS_PORT         GPIOA
#define LO_NSS_PIN          15            /* PA15 */
#define LO_LE_PORT          GPIOC
#define LO_LE_PIN           10            /* PC10 latch-enable */
#define LO_SCK_PORT         GPIOC
#define LO_SCK_PIN          11            /* PC11 */
#define LO_MOSI_PORT        GPIOC
#define LO_MOSI_PIN         12            /* PC12 */
#define LO_RF_EN_PORT       GPIOD
#define LO_RF_EN_PIN        2             /* PD2 RF enable */
#define LO_REF_HZ           50000000ULL   /* 50 MHz reference */
#define LO_OUTPUT_HZ        38000000000ULL /* 38 GHz sub-harmonic LO */

/* ---- BLE (nRF52840) over USART3 ------------------------------------ */
#define BLE_UART            USART3
#define BLE_TX_PORT         GPIOD
#define BLE_TX_PIN          8             /* PD8  TX */
#define BLE_RX_PORT         GPIOD
#define BLE_RX_PIN          9             /* PD9  RX */
#define BLE_BAUD            115200
#define BLE_BUF_SZ         256
#define BLE_BAUD_UART       115200

/* ---- USB-C CDC on USART2 (CP2102N) --------------------------------- */
#define USB_UART            USART2
#define USB_TX_PORT         GPIOD
#define USB_TX_PIN          5             /* PD5  TX */
#define USB_RX_PORT         GPIOD
#define USB_RX_PIN          6             /* PD6  RX */
#define USB_BAUD            115200
#define USB_BUF_SZ          512

/* ---- microSD on SDMMC1 / fallback SPI4 ----------------------------- */
/* Using SPI4 for simplicity (SPI mode) */
#define SD_SPI              SPI4
#define SD_CS_PORT          GPIOE
#define SD_CS_PIN           4             /* PE4 */
#define SD_SCK_PORT         GPIOE
#define SD_SCK_PIN          2             /* PE2 */
#define SD_MISO_PORT        GPIOE
#define SD_MISO_PIN         5             /* PE5 */
#define SD_MOSI_PORT        GPIOE
#define SD_MOSI_PIN         6             /* PE6 */
#define SD_CD_PORT          GPIOE
#define SD_CD_PIN           3             /* PE3 card detect (low=present) */
#define SD_BLOCK_SZ         512

/* ---- Power / battery monitoring (ADC3) ----------------------------- */
#define BAT_ADC             ADC3
#define BAT_ADC_CHANNEL     7             /* channel 7 */
#define BAT_PORT            GPIOF
#define BAT_PIN             9             /* PF9 (1/3 divider: 7.4V -> 2.47V) */
#define BAT_DIV             3             /* divider ratio */
#define BAT_FULL_MV         8400          /* 2x 18650 full */
#define BAT_EMPTY_MV        6000          /* cutoff */
#define BAT_WARN_MV         6600          /* 20% */
#define PWR_EN_5V_PORT      GPIOG
#define PWR_EN_5V_PIN       7             /* PG7 enable 5V buck */
#define PWR_EN_RF_PORT      GPIOG
#define PWR_EN_RF_PIN       8             /* PG8 enable RF rails */

/* ---- RF safety interlock ------------------------------------------- */
/* Two-stage arm: ARM then CONFIRM within 2 s */
#define ARM_TIMEOUT_MS      2000
/* Hard ceiling on programmable range to limit misuse */
#define RP_MAX_RANGE_CM     25000        /* 250 m */
#define RP_MAX_VEL_MPS      84000        /* ±300 km/h ≈ ±83333 mm/s; cap 84000 */
#define RP_MAX_RCS_QDB      160          /* +20 dBsm */
#define RP_MIN_RCS_QDB      -320         /* -40 dBsm */
/* RF power ceiling (programmable attenuator code) */
#define RP_MAX_POWER_CODE   200

/* ---- Scenario VM --------------------------------------------------- */
#define RP_SCN_TICK_HZ      1000         /* 1 kHz scenario tick */
#define RP_SCN_MAX_OPS      128          /* max opcodes per scenario */
#define RP_SCN_MAX_SLOTS    16           /* scenario slots on SD */

/* ---- Logging ------------------------------------------------------- */
#define RP_LOG_MAX_ENTRIES  8192
#define RP_LOG_MAGIC        0x52504C47   /* 'RPLG' */

/* ---- Convenience --------------------------------------------------- */
#define ARRAY_LEN(a)       (sizeof(a) / sizeof((a)[0]))
#define MIN(a, b)          ((a) < (b) ? (a) : (b))
#define MAX(a, b)          ((a) > (b) ? (a) : (b))
#define CLAMP(x, lo, hi)   (MAX((lo), MIN((hi), (x))))

/* ---- Public board API (implemented in drivers/board_init.c) -------- */
void     board_init(void);
void     board_delay_ms(uint32_t ms);
void     board_delay_us(uint32_t us);
uint32_t board_millis(void);
void     board_watchdog_kick(void);
void     board_power_enable_rf(void);
void     board_power_disable_rf(void);
void     board_led_set(uint32_t port, uint8_t pin, uint8_t on);

/* SPI / UART helpers (board_init.c) */
void     spi_init(uint32_t base, uint32_t baud_div, uint8_t cpol, uint8_t cpha);
uint8_t  spi_xfer(uint32_t base, uint8_t data);
void     spi_cs_low(uint32_t port, uint8_t pin);
void     spi_cs_high(uint32_t port, uint8_t pin);
void     uart_init(uint32_t base, uint32_t baud);
void     uart_putc(uint32_t base, uint8_t c);
uint8_t  uart_getc_nb(uint32_t base, uint8_t *c);

/* Two-stage arm interlock (main.c) */
void     rp_arm_request(void);

#endif /* RADARPHANTOM_BOARD_H */