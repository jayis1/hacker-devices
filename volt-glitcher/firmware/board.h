/*
 * board.h — Volt Glitcher board definitions
 * STM32F407VGT6-based voltage glitch injection platform
 * with Lattice iCE40HX1K-TQ144 FPGA co-processor
 *
 * Copyright (c) 2026 Hacker Devices
 * SPDX-License-Identifier: MIT
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * MCU: STM32F407VGT6
 * Package: LQFP-100
 * Core: ARM Cortex-M4F @ 168 MHz
 * Flash: 1024 KB  RAM: 192 KB (128+64 CCM)
 * ======================================================================== */

#define MCU_CORE_CLOCK          168000000U   /* 168 MHz max */
#define MCU_HSE_CLOCK           8000000U     /* 8 MHz external crystal Y1 */
#define MCU_LSE_CLOCK           32768U       /* 32.768 kHz RTC crystal Y2 */
#define MCU_AHB_PRESCALER       1U
#define MCU_APB1_PRESCALER      4U           /* APB1 = 42 MHz max */
#define MCU_APB2_PRESCALER      2U           /* APB2 = 84 MHz max */
#define MCU_FLASH_WAIT_STATES   5U           /* 5 WS for 168 MHz @ 3.3V */
#define MCU_CCM_RAM_SIZE        (64U * 1024U)
#define MCU_SRAM1_SIZE          (112U * 1024U)
#define MCU_SRAM2_SIZE          (16U * 1024U)

/* ========================================================================
 * FPGA: Lattice iCE40HX1K-TQ144
 * Package: TQFP-144
 * Logic: 1280 LCs, 64 BRAM (16Kb), 2 PLLs
 * VCC: 1.2V core, 3.3V I/O
 * ======================================================================== */

#define FPGA_FAMILY             "iCE40HX"
#define FPGA_PART               "iCE40HX1K-TQ144"
#define FPGA_LC_COUNT           1280U
#define FPGA_BRAM_COUNT         64U
#define FPGA_BRAM_BITS          (64U * 1024U)  /* 16Kb each * 4 = 64Kb */
#define FPGA_PLL_COUNT          2U
#define FPGA_CORE_VOLTAGE       1200U         /* 1.2V */
#define FPGA_IO_VOLTAGE         3300U         /* 3.3V */
#define FPGA_MAX_FREQ           273000000U    /* 273 MHz */
#define FPGA_SPI_SPEED          20000000U     /* 20 MHz config SPI */

/* ========================================================================
 * Voltage Regulator Map
 * ======================================================================== */

/* U2: TLV62569DBVR — 5V→1.2V/3A buck for FPGA core (VCC_CORE_1V2) */
#define REG_1V2_ENABLE_PIN      GPIOA, 1      /* PA1 — EN for U2 */
#define REG_1V2_POWER_GOOD_PIN  GPIOB, 8      /* PB8 — PG from U2 */
#define REG_1V2_I2C_ADDR        0x60          /* PMBus if variant used */

/* U3: TLV62569DBVR — 5V→3.3V/3A buck for FPGA I/O & MCU (VCC_IO_3V3) */
#define REG_3V3_ENABLE_PIN      GPIOA, 2      /* PA2 — EN for U3 */
#define REG_3V3_POWER_GOOD_PIN  GPIOB, 9      /* PB9 — PG from U3 */

/* U4: TPS7A7001DDAR — 3.3V→1.8V/2A LDO for FPGA aux bank (VCC_AUX_1V8) */
#define REG_1V8_ENABLE_PIN      GPIOA, 3      /* PA3 — EN for U4 */

/* U5: TPS7A7001DDAR — 3.3V→2.5V/2A LDO for FPGA PLL (VCC_PLL_2V5) */
#define REG_2V5_ENABLE_PIN      GPIOA, 4      /* PA4 — EN for U5 */

/* ========================================================================
 * Glitch Output Stage
 * ======================================================================== */

/* Q1: SiS426DN — N-ch MOSFET, Vgs(th)=1.2V, Rds(on)=6mΩ @ Vgs=4.5V
 *   Primary VCC shunt MOSFET for voltage glitch mode.
 *   Driven by U6 gate driver. */
#define GLITCH_SHUNT_GATE_PIN   GPIOC, 6      /* PC6 — gate drive from U6 */
#define GLITCH_SHUNT_EN_PIN     GPIOC, 7      /* PC7 — enable U6 driver */
#define GLITCH_SHUNT_FAULT_PIN  GPIOC, 8      /* PC8 — fault feedback */

/* U6: UCC27511DBVR — 4A/5A low-side gate driver for Q1 */
#define GATE_DRIVER_INPUT_PIN   GPIOC, 6      /* PC6 — IN to U6 */
#define GATE_DRIVER_ENABLE_PIN  GPIOC, 7      /* PC7 — EN to U6 */
#define GATE_DRIVER_FAULT_PIN   GPIOC, 8      /* PC8 — FAULT from U6 */

/* Q2: IRLML6344TRPBF — N-ch MOSFET for EM coil driver
 *   Rds(on)=60mΩ @ Vgs=2.5V, Id=1.2A continuous */
#define GLITCH_EM_GATE_PIN      GPIOC, 9      /* PC9 — gate drive for Q2 */
#define GLITCH_EM_EN_PIN        GPIOC, 10     /* PC10 — EM coil enable */

/* Q3: BSS84P — P-ch MOSFET for clock glitch (inserts clock stretch)
 *   Vgs(th)=-1.4V, Rds(on)=10Ω @ Vgs=-4.5V */
#define GLITCH_CLK_GATE_PIN     GPIOB, 14     /* PB14 — gate for Q3 */
#define GLITCH_CLK_EN_PIN       GPIOB, 15     /* PB15 — clock glitch enable */

/* ========================================================================
 * Voltage Sensing / Measurement
 * ======================================================================== */

/* ADC1 channel pins for target voltage monitoring */
#define ADC_TARGET_VCC_PIN      GPIOA, 0      /* PA0 — ADC1_IN0  */
#define ADC_TARGET_VCC_CHAN     0U
#define ADC_GLITCH_OUT_PIN      GPIOA, 5      /* PA5 — ADC1_IN5  */
#define ADC_GLITCH_OUT_CHAN     5U
#define ADC_SHUNT_CURR_PIN      GPIOA, 6      /* PA6 — ADC1_IN6  */
#define ADC_SHUNT_CURR_CHAN     6U
#define ADC_EM_COIL_VOLT_PIN    GPIOA, 7      /* PA7 — ADC1_IN7  */
#define ADC_EM_COIL_VOLT_CHAN   7U

/* ADC reference */
#define ADC_VREF                3300U         /* 3.3V reference */
#define ADC_RESOLUTION          12U           /* 12-bit */
#define ADC_FULL_SCALE          ((1U << 12U) - 1U)

/* Current sense amplifier U7: INA210NA — 0.1mΩ shunt, gain=500 V/V */
#define CURRENT_SENSE_GAIN      500U
#define CURRENT_SENSE_RSHUNT    100U          /* 0.1 mΩ = 100 μΩ */
#define CURRENT_SENSE_VREF      1250U        /* 1.25V reference output */

/* ========================================================================
 * Trigger Inputs
 * ======================================================================== */

/* TRIG0: GPIO trigger — active-low, opto-isolated via U8 TLP291 */
#define TRIG0_PIN               GPIOB, 0      /* PB0 — opto-isolated input */
#define TRIG0_TIM_CHAN          TIM3_CH3      /* TIM3 CH3 for timestamp */

/* TRIG1: UART pattern match — receives target UART via level shifters */
#define TRIG1_UART_RX_PIN       GPIOA, 10    /* PA10 — USART1_RX / TIM1_CH3 */
#define TRIG1_UART_TX_PIN       GPIOA, 9     /* PA9  — USART1_TX */
#define TRIG1_UART_INSTANCE      USART1
#define TRIG1_UART_BAUD         115200U

/* TRIG2: JTAG trigger — TCK/TMS/TDI/TDO pass-through */
#define TRIG2_JTAG_TCK_PIN      GPIOA, 14    /* PA14 — SWCLK */
#define TRIG2_JTAG_TMS_PIN      GPIOA, 13    /* PA13 — SWDIO */
#define TRIG2_JTAG_TDI_PIN      GPIOB, 3     /* PB3  — TDI  */
#define TRIG2_JTAG_TDO_PIN      GPIOB, 4     /* PB4  — TDO  */

/* TRIG3: Manual trigger button with debounce */
#define TRIG3_BTN_PIN           GPIOC, 13    /* PC13 — user button */
#define TRIG3_BTN_DEBOUNCE_MS   50U

/* ========================================================================
 * FPGA-MCU Interface (SPI1)
 * ======================================================================== */

#define FPGA_SPI_INSTANCE       SPI1
#define FPGA_SPI_SCK_PIN        GPIOA, 5     /* PA5  — SPI1_SCK */
#define FPGA_SPI_MISO_PIN       GPIOB, 4    /* PB4  — SPI1_MISO (shared w/ TDO) */
#define FPGA_SPI_MOSI_PIN       GPIOB, 5    /* PB5  — SPI1_MOSI */
#define FPGA_SPI_NSS_PIN        GPIOA, 4    /* PA4  — SPI1_NSS (manual CS) */

/* FPGA control signals */
#define FPGA_CRESET_B_PIN       GPIOB, 6    /* PB6 — config reset (active-low) */
#define FPGA_CDONE_PIN          GPIOB, 7    /* PB7 — config done */
#define FPGA_INT_PIN            GPIOC, 1    /* PC1 — FPGA→MCU interrupt */
#define FPGA_WAKE_PIN           GPIOC, 2    /* PC2 — MCU→FPGA wake */
#define FPGA_SUSPEND_PIN        GPIOC, 3    /* PC3 — MCU→FPGA suspend */

/* ========================================================================
 * USB Interface (OTG FS)
 * ======================================================================== */

#define USB_OTG_INSTANCE        USB_OTG_FS
#define USB_VBUS_PIN            GPIOA, 9     /* PA9  — VBUS sensing (shared w/ UART TX) */
#define USB_DM_PIN              GPIOA, 11   /* PA11 — USB_DM */
#define USB_DP_PIN              GPIOA, 12   /* PA12 — USB_DP */
#define USB_ENUM_CURRENT_MA     100U        /* 100 mA during enumeration */
#define USB_MAX_CURRENT_MA      500U        /* 500 mA operational */

/* ========================================================================
 * Debug / Logger (USART2)
 * ======================================================================== */

#define DEBUG_UART_INSTANCE     USART2
#define DEBUG_UART_TX_PIN       GPIOA, 2    /* PA2 — USART2_TX */
#define DEBUG_UART_RX_PIN       GPIOA, 3    /* PA3 — USART2_RX */
#define DEBUG_UART_BAUD         115200U

/* ========================================================================
 * Status LEDs
 * ======================================================================== */

/* LED1: Power (green) — driven direct from GPIO */
#define LED_POWER_PIN           GPIOD, 12   /* PD12 — TIM4_CH1 (PWM capable) */

/* LED2: Glitch active (red) — driven direct from GPIO */
#define LED_GLITCH_PIN          GPIOD, 13   /* PD13 — TIM4_CH2 */

/* LED3: Trigger detected (yellow) — driven direct from GPIO */
#define LED_TRIGGER_PIN         GPIOD, 14   /* PD14 — TIM4_CH3 */

/* LED4: Error / fault (orange) — driven direct from GPIO */
#define LED_ERROR_PIN           GPIOD, 15   /* PD15 — TIM4_CH4 */

/* ========================================================================
 * I2C — Configuration EEPROM & Voltage Monitor
 * ======================================================================== */

#define I2C_INSTANCE            I2C1
#define I2C_SCL_PIN             GPIOB, 6    /* PB6 — I2C1_SCL */
#define I2C_SDA_PIN             GPIOB, 7    /* PB7 — I2C1_SDA */
#define I2C_SPEED_HZ           400000U     /* 400 kHz Fast Mode */

/* U9: CAT24C256WI-GT3 — 256Kbit I2C EEPROM for glitch profiles */
#define EEPROM_I2C_ADDR        0x50U       /* A0=A1=A2=0 */
#define EEPROM_PAGE_SIZE       64U
#define EEPROM_TOTAL_BYTES     (32U * 1024U)

/* U10: INA219AIDCNR — I2C voltage/current monitor for target supply */
#define INA219_I2C_ADDR        0x40U
#define INA219_SHUNT_RESISTOR  100U         /* 0.1Ω shunt for target current */

/* ========================================================================
 * SD Card (SPI2) — glitch waveform storage
 * ======================================================================== */

#define SD_SPI_INSTANCE         SPI2
#define SD_SPI_SCK_PIN          GPIOB, 13   /* PB13 — SPI2_SCK */
#define SD_SPI_MISO_PIN         GPIOB, 14   /* PB14 — SPI2_MISO */
#define SD_SPI_MOSI_PIN         GPIOB, 15   /* PB15 — SPI2_MOSI */
#define SD_CS_PIN               GPIOB, 12   /* PB12 — SPI2_NSS */
#define SD_DETECT_PIN           GPIOC, 4    /* PC4  — card detect */

/* ========================================================================
 * Operating Modes
 * ======================================================================== */

typedef enum {
    MODE_IDLE = 0,
    MODE_VOLTAGE_GLITCH,         /* VCC shunt via MOSFET */
    MODE_EM_GLITCH,              /* EM pulse via coil driver */
    MODE_CLOCK_GLITCH,           /* Clock stretch/insertion */
    MODE_TRIGGER_SCAN,           /* Passive trigger monitoring only */
    MODE_CALIBRATION,            /* Self-calibration routine */
    MODE_COUNT
} glitch_mode_t;

/* ========================================================================
 * Trigger Modes
 * ======================================================================== */

typedef enum {
    TRIG_NONE = 0,
    TRIG_GPIO,                   /* GPIO pin edge trigger */
    TRIG_UART_PATTERN,           /* UART pattern match */
    TRIG_JTAG_STATE,             /* JTAG TAP state trigger */
    TRIG_MANUAL,                 /* Manual button press */
    TRIG_FPGA_PATTERN,           /* FPGA-based complex pattern */
    TRIG_COUNT
} trigger_mode_t;

/* ========================================================================
 * Glitch Shape Definitions
 * ======================================================================== */

typedef enum {
    GLITCH_SHAPE_RECTANGLE = 0,
    GLITCH_SHAPE_TRIANGLE,
    GLITCH_SHAPE_GAUSSIAN,
    GLITCH_SHAPE_SAWTOOTH,
    GLITCH_SHAPE_CUSTOM,
    GLITCH_SHAPE_COUNT
} glitch_shape_t;

/* ========================================================================
 * Board-level Functions
 * ======================================================================== */

void board_init(void);
void board_deinit(void);

/* Power rail control */
void board_power_fpga_on(void);
void board_power_fpga_off(void);
int  board_power_good(void);

/* LED control */
void led_power_set(int on);
void led_glitch_set(int on);
void led_trigger_set(int on);
void led_error_set(int on);
void led_all_off(void);

/* Debug logging */
void debug_printf(const char *fmt, ...);

/* Delay utilities */
void delay_us(uint32_t us);
void delay_ms(uint32_t ms);

/* ADC reading */
uint16_t adc_read_channel(uint8_t channel);
int32_t  adc_to_mv(uint16_t raw);
int32_t  adc_read_target_vcc_mv(void);
int32_t  adc_read_shunt_current_ma(void);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */