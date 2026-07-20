/*
 * hart-bleeder — board.h
 * Board pin definitions and hardware constants for the
 * HART Fieldbus Covert In-Line Attack Implant.
 *
 * MCU: STM32G474CEU6 (Cortex-M4 @ 170 MHz, CORDIC + FMAC)
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef HART_BLEEDER_BOARD_H
#define HART_BLEEDER_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ── MCU identity ─────────────────────────────────────────────── */
#define BOARD_MCU_NAME      "STM32G474CEU6"
#define BOARD_MCU_CORE_CLK  170000000UL   /* 170 MHz core clock      */
#define BOARD_MCU_HSE_CLK   24000000UL    /* 24 MHz external crystal */
#define BOARD_MCU_HSI_CLK   16000000UL    /* 16 MHz internal HSI     */

/* ── Base addresses (STM32G474) ───────────────────────────────── */
#define PERIPH_BASE         0x40000000UL
#define AHB1_BASE           (PERIPH_BASE + 0x00020000UL)
#define AHB2_BASE           (PERIPH_BASE + 0x00080000UL)
#define APB1_BASE           (PERIPH_BASE + 0x00010000UL)
#define APB2_BASE           (PERIPH_BASE + 0x00040000UL)

#define GPIOA_BASE          (AHB2_BASE + 0x0000)
#define GPIOB_BASE          (AHB2_BASE + 0x0400)
#define GPIOC_BASE          (AHB2_BASE + 0x0800)
#define GPIOF_BASE          (AHB2_BASE + 0x1400)
#define GPIOG_BASE          (AHB2_BASE + 0x1800)

#define USART1_BASE          (APB2_BASE + 0x4C00)
#define USART2_BASE          (APB1_BASE + 0x4400)
#define USART3_BASE          (APB1_BASE + 0x4800)
#define UART4_BASE           (APB1_BASE + 0x4C00)
#define UART5_BASE           (APB1_BASE + 0x5000)
#define LPUART1_BASE         (APB1_BASE + 0x5400)

#define SPI1_BASE            (APB2_BASE + 0x3000)
#define SPI2_BASE            (APB1_BASE + 0x3800)
#define SPI3_BASE            (APB1_BASE + 0x3C00)

#define ADC1_BASE            (AHB2_BASE + 0x8000)
#define ADC2_BASE            (AHB2_BASE + 0x8400)
#define DAC1_BASE            (AHB1_BASE + 0x7800)

#define TIM2_BASE            (APB1_BASE + 0x0000)
#define TIM3_BASE            (APB1_BASE + 0x0400)
#define TIM6_BASE            (APB1_BASE + 0x1000)
#define TIM7_BASE            (APB1_BASE + 0x1400)
#define TIM8_BASE            (APB2_BASE + 0x1400)

#define DMAMUX1_BASE         (AHB1_BASE + 0x0800)
#define DMA1_BASE            (AHB1_BASE + 0x2000)
#define DMA2_BASE            (AHB1_BASE + 0x2400)

#define PWR_BASE             (AHB1_BASE + 0x0000)
#define RCC_BASE             (AHB1_BASE + 0x1000)
#define FLASH_BASE_REG       (AHB1_BASE + 0x2000)
#define SYSCFG_BASE          (APB2_BASE + 0x1000)
#define EXTI_BASE            (APB2_BASE + 0x0C00)

#define QSPI_BASE            (AHB3_BASE + 0x0000)  /* QUADSPI */
#define AHB3_BASE            (PERIPH_BASE + 0x00040000UL)

/* ── Pin map (HART-Bleeder rev1) ────────────────────────────────
 * PA0  ADC1_IN1  Loop current monitor (shunt sense, 0-5V)
 * PA1  ADC1_IN2  Loop voltage monitor (after sense R)
 * PA2  USART2_TX HART modem UART (MAX13440E / TH8032)
 * PA3  USART2_RX
 * PA4  DAC1_OUT1 Loop resistance modulator (digital potentiometer wiper)
 * PA5  GPIO      HART modem /RTS (transmit enable, active low)
 * PA6  GPIO      HART modem /CD  (carrier detect input)
 * PA7  GPIO      HART modem /SHDN (shutdown control)
 * PA8  GPIO      Reversible sense-resistor bypass relay drive
 * PA9  USART1_TX USB CDC console (ST-Link VCP)
 * PA10 USART1_RX
 * PA11 PA12      USB FS (internal PHY)
 * PB0  ADC1_IN9  Loop current secondary sense (redundant)
 * PB1  ADC1_IN10 Battery voltage monitor (LiPo divider)
 * PB2  GPIO      Boot0 strap / status button
 * PB3  SPI1_SCK  microSD card
 * PB4  SPI1_MISO
 * PB5  SPI1_MOSI
 * PB6  GPIO      microSD CS
 * PB7  GPIO      microSD card detect
 * PB8  GPIO      OLED /DC
 * PB9  GPIO      OLED /RES
 * PB10 I2C2_SCL  OLED + EEPROM config store
 * PB11 I2C2_SDA
 * PB12 GPIO      LED1 (red — armed/attack)
 * PB13 GPIO      LED2 (green — passive monitor)
 * PB14 GPIO      LED3 (blue — loop powered OK)
 * PB15 GPIO      Loop isolation transformer bypass (safety)
 * PC13 GPIO      User button (active low)
 * PC14 PC15      32.768 kHz LSE crystal
 * PF0  PF1       n.c. (alternate HSE option)
 */

#define PIN_USART2_TX        2    /* PA2 */
#define PIN_USART2_RX        3    /* PA3 */
#define PIN_HART_RTS        5    /* PA5 */
#define PIN_HART_CD         6    /* PA6 */
#define PIN_HART_SHDN       7    /* PA7 */
#define PIN_BYPASS_RELAY    8    /* PA8 */
#define PIN_UART1_TX        9    /* PA9 */
#define PIN_UART1_RX        10   /* PA10 */
#define PIN_USBDM           11   /* PA11 */
#define PIN_USBDP           12   /* PA12 */

#define PIN_SD_CS            6    /* PB6 */
#define PIN_SD_CD            7    /* PB7 */
#define PIN_OLED_DC          8    /* PB8 */
#define PIN_OLED_RES         9    /* PB9 */

#define PIN_LED_RED          12   /* PB12 */
#define PIN_LED_GREEN        13   /* PB13 */
#define PIN_LED_BLUE         14   /* PB14 */
#define PIN_ISO_BYPASS       15   /* PB15 */

#define PIN_USER_BUTTON      13   /* PC13 */

/* ── Analog channel map ───────────────────────────────────────── */
#define ADC_CH_LOOP_I        1    /* PA0 — primary loop-current sense */
#define ADC_CH_LOOP_V        2    /* PA1 — loop-voltage sense */
#define ADC_CH_LOOP_I2       9    /* PB0 — redundant current sense */
#define ADC_CH_VBAT          10   /* PB1 — battery divider */

/* ── HART physical-layer constants ──────────────────────────────
 * HART FSK: 1200 Hz = logic 1 (mark), 2200 Hz = logic 0 (space).
 * Bell 202 modem, 1200 baud, half-duplex, 1 bit per tone burst.
 * The modem IC (MAX13440E or TH8032) handles FSK <-> digital.
 * Loop current = 4 mA (0% live zero) to 20 mA (100% full scale).
 * HART digital signal = ±0.5 mA superimposed on the loop current.
 */
#define HART_FREQ_MARK       1200UL   /* Hz — bit value 1 */
#define HART_FREQ_SPACE      2200UL   /* Hz — bit value 0 */
#define HART_BAUD            1200UL
#define HART_BIT_US          (1000000UL / HART_BAUD)   /* 833.3 us */
#define HART_PREAMBLE_LEN    5         /* 0xFF preambles min */
#define HART_PREAMBLE_MAX    20
#define HART_LONG_TIMEOUT_US 280000UL  /* ~336 bit times — char spacing */

/* ── Loop electrical limits ────────────────────────────────────── */
#define LOOP_I_MIN_MA        3.5f    /* below this = fault / broken loop */
#define LOOP_I_MAX_MA         25.0f  /* over-current ceiling */
#define LOOP_I_NOMINAL_MA     4.0f   /* 0% analog value live zero */
#define LOOP_I_FULLSCALE_MA   20.0f  /* 100% analog value */
#define LOOP_V_MAX_V          42.0f  /* common industrial supply rail */
#define SHUNT_RES_OHM         250.0f /* 250R => 5V @ 20mA */
#define VREF_V                3.3f

/* ── Buffers ──────────────────────────────────────────────────── */
#define HART_MAX_FRAME       253     /* max HART PDU payload + overhead */
#define LOG_RING_ENTRIES     256
#define CAPTURE_BUF_SAMPLES  4096
#define SD_SECTOR_SIZE       512

/* ── BLE C2 link (Nordic nRF52832 companion module over UART4) ── */
#define BLE_UART_BASE        UART4_BASE
#define BLE_BAUD             115200UL

/* ── QSPI external flash (W25Q128JVSIQ, 16 MB) ─────────────────── */
#define QSPI_FLASH_SIZE      (16UL * 1024 * 1024)
#define QSPI_SECTOR_SIZE      4096
#define QSPI_PAGE_SIZE       256

/* ── Battery / power ───────────────────────────────────────────── */
#define VBAT_DIVIDER         (2.0f)  /* 1M / 1M divider */
#define VBAT_LOW_MV          3300
#define VBAT_CRIT_MV          3100

/* ── OLED (128x64 SSD1306 over I2C2) ──────────────────────────── */
#define OLED_I2C_ADDR        0x3C
#define OLED_WIDTH            128
#define OLED_HEIGHT           64

/* ── Helper macros ─────────────────────────────────────────────── */
#define ARRAY_SIZE(a)         (sizeof(a) / sizeof((a)[0]))
#define MIN(a, b)             ((a) < (b) ? (a) : (b))
#define MAX(a, b)             ((a) > (b) ? (a) : (b))
#define CLAMP(x, lo, hi)      (MAX((lo), MIN((x), (hi))))

#endif /* HART_BLEEDER_BOARD_H */