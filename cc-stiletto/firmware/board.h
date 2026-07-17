/*
 * board.h — Pin map, peripheral assignments, and clock constants for CC-Stiletto.
 *
 * Target: STM32G474CEU6 (LQFP48).
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * This file is part of CC-Stiletto, a USB-C Power Delivery Configuration Channel
 * attack tool for authorized security research only.
 */

#ifndef CC_STILETTO_BOARD_H
#define CC_STILETTO_BOARD_H

#include <stdint.h>

/* ---- Clock tree (HSI 16 MHz -> PLL -> 170 MHz system clock) ------------------ */
#define HSI_FREQ_HZ        16000000u
#define SYSCLK_FREQ_HZ     170000000u
#define HCLK_FREQ_HZ       170000000u
#define PCLK1_FREQ_HZ      170000000u   /* APB1 (I2C, TIM2-7, USART2/3) */
#define PCLK2_FREQ_HZ      170000000u   /* APB2 (USART1, TIM1/8/15-17) */
#define HRTIM_FREQ_HZ      170000000u  /* HRTIM clocked at SYSCLK */

/* ---- GPIO pin assignments (STM32G474 LQFP48) ------------------------------- */

/* Source-side FUSB302B (U2) — I2C1 */
#define FUSB_SRC_I2C           I2C1
#define FUSB_SRC_SDA_PORT      GPIOB
#define FUSB_SRC_SDA_PIN       7u        /* PB7  — I2C1_SDA */
#define FUSB_SRC_SCL_PORT      GPIOB
#define FUSB_SRC_SCL_PIN       6u        /* PB6  — I2C1_SCL */
#define FUSB_SRC_INT_PORT       GPIOB
#define FUSB_SRC_INT_PIN        4u       /* PB4  — INT_N from U2 */

/* Sink-side FUSB302B (U3) — I2C2 */
#define FUSB_SNK_I2C           I2C2
#define FUSB_SNK_SDA_PORT      GPIOB
#define FUSB_SNK_SDA_PIN       11u       /* PB11 — I2C2_SDA */
#define FUSB_SNK_SCL_PORT      GPIOB
#define FUSB_SNK_SCL_PIN       10u       /* PB10 — I2C2_SCL */
#define FUSB_SNK_INT_PORT       GPIOA
#define FUSB_SNK_INT_PIN        15u      /* PA15 — INT_N from U3 */

/* VBUS MOSFET bank gate driver — HRTIM output */
#define VBUS_GATE_HRTIM        HRTIM1
#define VBUS_GATE_TIM          HRTIM_TIMA
#define VBUS_GATE_PORT         GPIOA
#define VBUS_GATE_PIN          8u        /* PA8 — HRTIM_CHA1 */
#define VBUS_GATE_AF           13u       /* AF13 for HRTIM_CHA1 on PA8 */

/* MAX5370 DAC — I2C3 */
#define DAC_I2C                I2C3
#define DAC_SDA_PORT            GPIOC
#define DAC_SDA_PIN             9u       /* PC9  — I2C3_SDA */
#define DAC_SCL_PORT            GPIOA
#define DAC_SCL_PIN             8u       /* PA8 conflict resolved: DAC uses PC8/PC9 */
#define DAC_ADDR_7BIT           0x1Cu    /* MAX5370 default addr */

/* TPS25940 eFuse EN + FLT */
#define EFUSE_EN_PORT           GPIOC
#define EFUSE_EN_PIN            13u      /* PC13 */
#define EFUSE_FLT_PORT          GPIOC
#define EFUSE_FLT_PIN           14u      /* PC14 — fault input */

/* INA226 #1 (source leg) — shared I2C1 */
#define INA_SRC_ADDR_7BIT        0x40u
/* INA226 #2 (sink leg) — shared I2C2 */
#define INA_SNK_ADDR_7BIT        0x41u

/* ISO1540 isolator enable */
#define ISO_EN_PORT             GPIOB
#define ISO_EN_PIN              15u      /* PB15 — enable both isolator sides */

/* Status RGB LED (active low, common-anode) */
#define LED_R_PORT              GPIOA
#define LED_R_PIN               0u       /* PA0  — TIM2_CH1 PWM (red) */
#define LED_G_PORT              GPIOA
#define LED_G_PIN               1u       /* PA1  — TIM2_CH2 PWM (green) */
#define LED_B_PORT              GPIOA
#define LED_B_PIN               2u       /* PA2  — TIM2_CH3 PWM (blue) */

/* Tactile button — mode select / emergency Hard Reset */
#define BTN_PORT                GPIOB
#define BTN_PIN                 3u       /* PB3 — active low, EXTI3 */

/* USB-CDC — native USB device peripheral (PA11 D-, PA12 D+) */
#define USB_DM_PORT             GPIOA
#define USB_DM_PIN              11u
#define USB_DP_PORT             GPIOA
#define USB_DP_PIN              12u

/* NTC thermistor on VBUS MOSFET bank — ADC1 channel 5 (PA4) */
#define TEMP_ADC                ADC1
#define TEMP_ADC_CHANNEL        5u
#define TEMP_OVER_LIMIT_C        85u      /* hardware thermal cutoff */
#define TEMP_WARN_C             70u

/* SWD debug header (pogo) */
#define SWD_SWCLK_PORT          GPIOA
#define SWD_SWCLK_PIN           14u
#define SWD_SWDIO_PORT          GPIOA
#define SWD_SWDIO_PIN           13u

/* ---- Physical constants --------------------------------------------------- */
#define VBUS_SAFE_MAX_MV         20000u  /* 20 V default safe ceiling for unguarded modes */
#define VBUS_ABS_MAX_MV          48000u  /* PD 3.1 EPR absolute max */
#define VBUS_OVERV_TRIP_MV       51000u  /* hardware comparator trip */

/* ---- Mode LED colors ------------------------------------------------------ */
typedef enum {
    LED_OFF        = 0,
    LED_SNIFF      = 1,   /* green */
    LED_INJECT     = 2,   /* red */
    LED_SPOOF      = 3,   /* magenta */
    LED_GLITCH     = 4,   /* red strobe */
    LED_ROLE_HIJACK= 5,   /* cyan */
    LED_VCONN      = 6,   /* yellow */
    LED_DEAD_BATT  = 7,   /* blue */
    LED_FAULT      = 8,   /* fast red flash */
} led_state_t;

#endif /* CC_STILETTO_BOARD_H */