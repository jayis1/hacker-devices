/*
 * board.h — SideProbe hardware pin & clock definitions
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Target MCU: STM32H743VIT6 (LQFP100)
 *
 * Pin map (high-level). See registers.h for raw bit definitions.
 *
 *   PA2  / PA3   -> USART2 TX/RX     (nRF52840 BLE bridge, 115200)
 *   PA9  / PA10  -> USART1 TX/RX     (target UART plaintext feed, up to 6 Mbaud)
 *   PA5          -> ADC2 channel (shunt amplifier output, analogue, via on-chip
 *                                   ADC for low-rate preview; main path is ADS8686S)
 *   PB10 / PB11  -> SPI2 SCK/MOSI   (ADS8686S ADC SPI + AD8429 gain + ADL5205 VGA)
 *   PB12         -> SPI2_NSS         (ADC chip-select)
 *   PB13         -> SPI2_MISO
 *   PC4          -> SPI2_NSS for VGA chain (ADL5205 #1)
 *   PC5          -> SPI2_NSS for PGA (AD8429 gain resistor network)
 *   PD0..PD3     -> FMC D0..D3 (SDRAM data bus, 16-bit total PD0..PD15)
 *   PE0..PE12    -> FMC address + control to SDRAM
 *   PA11 / PA12  -> USB DM/DP (CDC + bulk trace endpoint)
 *   PB3 / PB4 / PB5 -> SPI1 SCK/MISO/MOSI (FPGA iCE40 config + readback)
 *   PB6          -> FPGA CRESET
 *   PB7          -> FPGA CDONE
 *   PC8..PC12   -> SDIO (microSD)
 *   PC13         -> trigger-in GPIO (target trigger, 3.3/5V tolerant via TXS0108E)
 *   PC14         -> trigger-out GPIO (reset target / start-crypto strobe)
 *   PC15         -> target clock-sniff input (FPGA counter)
 *   PD8 / PD9    -> I2C1 SDA/SCL (MAX17048 fuel gauge)
 *   PD11         -> OLED reset
 *   PD12..PD15   -> OLED SPI (SH1106)
 *   PA0..PA3     -> 5 tactile buttons (modality/gain-up/gain-down/arm/attack)
 */
#ifndef SIDEPROBE_BOARD_H
#define SIDEPROBE_BOARD_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Clock tree (after PLL1) ---- */
#define HSE_HZ                 25000000u   /* external 25 MHz XO on board */
#define SYSCLK_HZ              480000000u /* M7 core = 480 MHz */
#define AHB_HZ                 240000000u
#define APB1_HZ                 120000000u
#define APB2_HZ                 120000000u
#define SDRAM_CLK_HZ            100000000u /* FMC SDCLK = 100 MHz */

/* ---- Peripheral IRQ priorities (lower = higher) ---- */
#define IRQ_PRIO_ADC_DMA        2
#define IRQ_PRIO_FPGA_SPI       3
#define IRQ_PRIO_TRIGGER        1   /* highest: trigger gating */
#define IRQ_PRIO_BLE_UART       5
#define IRQ_PRIO_USB_CDC        5
#define IRQ_PRIO_SDIO           6
#define IRQ_PRIO_OLED_SPI       8
#define IRQ_PRIO_BUTTONS        9

/* ---- GPIO pin definitions (port, pin number) ---- */
typedef struct { uint8_t port; uint8_t pin; } sp_pin_t;

#define PIN_BLE_TX        ((sp_pin_t){0, 2})   /* PA2 USART2_TX */
#define PIN_BLE_RX        ((sp_pin_t){0, 3})   /* PA3 USART2_RX */
#define PIN_TARGET_TX    ((sp_pin_t){0, 9})   /* PA9 USART1_TX */
#define PIN_TARGET_RX    ((sp_pin_t){0, 10})  /* PA10 USART1_RX */
#define PIN_TARGET_SPI_NSS ((sp_pin_t){1, 12})/* PB12 (reused for SPI2) */
#define PIN_ADC_NSS       ((sp_pin_t){1, 12}) /* PB12 SPI2_NSS (ADC) */
#define PIN_VGA_NSS       ((sp_pin_t){2, 4})  /* PC4 VGA chain */
#define PIN_PGA_NSS       ((sp_pin_t){2, 5})  /* PC5 PGA gain */
#define PIN_FPGA_SCK      ((sp_pin_t){1, 3})
#define PIN_FPGA_MISO     ((sp_pin_t){1, 4})
#define PIN_FPGA_MOSI     ((sp_pin_t){1, 5})
#define PIN_FPGA_NSS      ((sp_pin_t){1, 8})  /* PB8 */
#define PIN_FPGA_CRESET   ((sp_pin_t){1, 6})
#define PIN_FPGA_CDONE    ((sp_pin_t){1, 7})
#define PIN_TRIGGER_IN   ((sp_pin_t){2, 13}) /* PC13 */
#define PIN_TRIGGER_OUT   ((sp_pin_t){2, 14})/* PC14 */
#define PIN_CLK_SNIFF     ((sp_pin_t){2, 15}) /* PC15 */
#define PIN_FUEL_SDA      ((sp_pin_t){3, 9})  /* PD9 I2C1_SDA */
#define PIN_FUEL_SCL      ((sp_pin_t){3, 8})  /* PD8 I2C1_SCL */
#define PIN_OLED_RST      ((sp_pin_t){3, 11})
#define PIN_OLED_DC       ((sp_pin_t){3, 12})
#define PIN_OLED_SCK      ((sp_pin_t){3, 13})
#define PIN_OLED_MOSI     ((sp_pin_t){3, 15})
#define PIN_BTN_MODALITY  ((sp_pin_t){0, 0})
#define PIN_BTN_GAINUP    ((sp_pin_t){0, 1})
#define PIN_BTN_GAINDOWN  ((sp_pin_t){0, 2})
#define PIN_BTN_ARM       ((sp_pin_t){0, 3})
#define PIN_BTN_ATTACK    ((sp_pin_t){0, 4})

/* ---- SDRAM geometry (IS42S16160J, 16-bit, 256 MB) ---- */
#define SDRAM_BASE_ADDR   0xC0000000u
#define SDRAM_SIZE_BYTES  (256u * 1024u * 1024u)
#define SDRAM_ROWS        8192
#define SDRAM_COLS        512
#define SDRAM_BANKS       4
#define SDRAM_COL_BITS    9
#define SDRAM_ROW_BITS    13
#define SDRAM_CAS_LATENCY 2

/* ---- Capture parameters ---- */
#define TRACE_SAMPLES          2048u   /* samples per trace (first AES round window) */
#define TRACE_SAMPLE_BYTES    2u      /* 16-bit ADC */
#define TRACE_MAX_IN_SDRAM     ((SDRAM_SIZE_BYTES) / (TRACE_SAMPLES * TRACE_SAMPLE_BYTES))
#define TRACES_PER_ATTACK      5000u  /* default */
#define KEY_BYTES_AES128       16
#define KEY_GUESS_COUNT        256
#define TRIG_SRC_ANALOG        0
#define TRIG_SRC_EXT_GPIO      1
#define TRIG_SRC_UART_SIG      2
#define TRIG_SRC_FREERUN       3

/* ---- Modality ---- */
#define MODALITY_POWER         0
#define MODALITY_EM            1

/* ---- Hypothesis model ---- */
#define MODEL_HAMMING_WEIGHT   0
#define MODEL_HAMMING_DISTANCE 1
#define MODEL_SBOX_OUT         2  /* HW of S-box output (standard CPA) */

#ifdef __cplusplus
}
#endif
#endif /* SIDEPROBE_BOARD_H */