/*
 * board.h — Hardware definitions and constants for GNSS-Phantom
 *
 * Author:  jayis1
 * License: Proprietary — Authorized Security Research Use Only
 */

#ifndef GNSS_PHANTOM_BOARD_H
#define GNSS_PHANTOM_BOARD_H

#include <stdint.h>
#include <stddef.h>

#define FIRMWARE_VERSION_MAJOR  1
#define FIRMWARE_VERSION_MINOR  0
#define FIRMWARE_VERSION_PATCH  0
#define FIRMWARE_VERSION_STR    "1.0.0"

#define DEVICE_NAME             "GNSS-Phantom"
#define DEVICE_MODEL             "GPX-1"
#define AUTHOR                   "jayis1"

/* ----- Clock configuration -----
 * HSE = 25 MHz TCXO (shared with Si4463 RF synth for coherent timing)
 * SYSCLK = 480 MHz via PLL1 (M=25/12, N=240, P=2)
 * AHB = 240 MHz, APB1 = 120 MHz, APB2 = 120 MHz
 */
#define HSE_VALUE_HZ            25000000U
#define SYSCLK_HZ               480000000U
#define AHB_HZ                  240000000U
#define APB1_HZ                  120000000U
#define APB2_HZ                  120000000U

/* ----- GPIO pin assignments -----
 * STM32H743VI (BGA-100)
 */
#define LED_STATUS_PIN          0   /* PA0  - status LED */
#define LED_SPOOF_PIN           1   /* PA1  - spoof indicator (red) */
#define LED_RX_PIN              2   /* PA2  - RX indicator (green) */
#define LED_PWR_PIN             3   /* PA3  - power LED (blue) */
#define GPIO_LED_PORT(x)       GPIOA

#define BTN_MODE_PIN            0   /* PB0  - mode select button */
#define BTN_FIRE_PIN            1   /* PB1  - fire/engage button */
#define BTN_SAFE_PIN            2   /* PB2  - safety switch */
#define BTN_PORT                GPIOB

#define SI4463_SDN_PIN          8   /* PC8  - Si4463 GPS shutdown */
#define SI4463_SEL_PIN          9   /* PC9  - Si4463 GPS SPI CS */
#define SI4463_IRQ_PIN          10  /* PC10 - Si4463 GPS IRQ */
#define SI4463_GPIO0_PIN        11  /* PC11 - Si4463 GPIO0 */
#define SI4463_GPIO1_PIN        12  /* PC12 - Si4463 GPIO1 */
#define SI4463_PORT              GPIOC

#define SI4463B_SDN_PIN         14  /* PD14 - Si4463 #2 shutdown */
#define SI4463B_SEL_PIN         15  /* PD15 - Si4463 #2 SPI CS */
#define SI4463B_IRQ_PIN          13  /* PD13 - Si4463 #2 IRQ */
#define SI4463B_PORT             GPIOD

#define TCXO_EN_PIN             4   /* PE4 - TCXO enable */
#define PA_EN_PIN               5   /* PE5 - LNA/PA bias enable */
#define TX_RX_SW_PIN            6   /* PE6 - TX/RX switch */
#define RF_CTRL_PORT             GPIOE

#define SPI_MOSI_PIN            7   /* PB7  - SPI2 MOSI to Si4463s */
#define SPI_MISO_PIN            6   /* PB6  - SPI2 MISO */
#define SPI_SCK_PIN             8   /* PB8  - SPI2 SCK */
#define SPI2_CS_PORT             GPIOB

#define DBG_UART_TX_PIN         8   /* PD8  - USART3 TX (debug) */
#define DBG_UART_RX_PIN         9   /* PD9  - USART3 RX (debug) */
#define DBG_UART_PORT             GPIOD

#define C2_UART_TX_PIN          2   /* PA2  - USART2 TX to nRF52840 */
#define C2_UART_RX_PIN          3   /* PA3  - USART2 RX from nRF52840 */

#define DAC1_OUT_PIN             4   /* PA4  - DAC1 ch1 (I baseband) */
#define DAC2_OUT_PIN             5   /* PA5  - DAC1 ch2 (Q baseband) */

#define PPS_OUT_PIN              0   /* PG0  - TIM8 CH1 PPS output */
#define PPS_IN_PIN               1   /* PG1  - PPS input from GNSS RX */

#define BATT_ADC_PIN             0   /* ADC1_IN0 (PA0 alt) — but we use PC0 */
#define BATT_ADC_PORT            GPIOC
#define BATT_ADC_CHANNEL         10

#define I2C_SCL_PIN              8   /* PB8  - I2C1 SCL (fuel gauge, IMU) */
#define I2C_SDA_PIN              9   /* PB9  - I2C1 SDA */

/* ----- RF Front-end frequencies (MHz) ----- */
#define GPS_L1_FREQ_MHZ          1575.420f
#define GALILEO_E1_FREQ_MHZ      1575.420f
#define GLONASS_L1_FREQ_MHZ      1602.000f
#define BEIDOU_B1_FREQ_MHZ       1561.098f

/* IF frequency for Si4463 (we use direct modulation via FSK) */
#define IF_FREQ_MHZ             2.400f

/* ----- GNSS spoofing parameters ----- */
#define GNSS_SV_COUNT_MAX        12  /* max simulated satellites per constellation */
#define GPS_CA_CODE_PERIOD_CHIPS 1023
#define GPS_CA_CODE_RATE_HZ     1023000  /* 1.023 Mcps */
#define GPS_BIT_RATE_HZ         50       /* 50 bps navigation data */
#define GPS_NAV_FRAME_BITS      1500     /* 30 sec * 50 bps */

/* C/A code generator polynomial: G1 = x^10+x^3+1, G2 = configurable taps */
#define GPS_G1_POLY             0x0201  /* taps at bit 3 and 10 */
#define GPS_G2_POLY_BASE        0x0400  /* base for G2 with tap select */

/* Default ephemeris / almanac scenario file stored in external flash */
#define SCENARIO_FLASH_OFFSET   0x00000000U
#define SCENARIO_FLASH_SIZE     (2 * 1024 * 1024)  /* 2 MB region */

/* ----- Battery / Power ----- */
#define BATT_LOW_MV             3300   /* mV threshold for low battery */
#define BATT_CRIT_MV            3000   /* mV critical */
#define BATT_FULL_MV            4200
#define BATT_ADC_DIVIDER        2      /* voltage divider ratio */

/* ----- C2 protocol (BLE via nRF52840 UART bridge) ----- */
#define C2_MAX_PAYLOAD          512
#define C2_BAUD                 230400
#define C2_SOF                  0xA5
#define C2_EOF                  0x5A
#define C2_DLE                  0x7D

/* C2 command opcodes */
#define C2_CMD_PING              0x01
#define C2_CMD_GET_STATUS        0x02
#define C2_CMD_SET_FREQ          0x10
#define C2_CMD_SET_POWER         0x11
#define C2_CMD_SET_SV            0x12
#define C2_CMD_LOAD_EPHEMERIS    0x20
#define C2_CMD_LOAD_SCENARIO     0x21
#define C2_CMD_START_SPOOF       0x30
#define C2_CMD_STOP_SPOOF        0x31
#define C2_CMD_START_RX          0x32
#define C2_CMD_STOP_RX           0x33
#define C2_CMD_SET_TRAJECTORY    0x40
#define C2_CMD_SET_TIME_OFFSET   0x41
#define C2_CMD_ARM               0x50
#define C2_CMD_DISARM            0x51
#define C2_CMD_FACTORY_RESET     0xFF

/* C2 response codes */
#define C2_RSP_OK               0x00
#define C2_RSP_ERROR            0x01
#define C2_RSP_BUSY              0x02
#define C2_RSP_UNARMED          0x03
#define C2_RSP_BAD_PARAM         0x04

/* ----- Safety interlock ----- */
#define SAFETY_CHECKSUM_SEED     0xDEADBEEFU
#define SAFETY_ACK_TIMEOUT_MS    5000
#define SAFETY_ARM_HOLD_MS       3000  /* must hold fire button 3s */
#define ARMED_AUTO_DISARM_MS     300000 /* auto-disarm after 5 min */

/* ----- Ring buffers ----- */
#define C2_RX_BUF_SIZE          2048
#define C2_TX_BUF_SIZE          2048
#define UART_DBG_BUF_SIZE       1024

#endif /* GNSS_PHANTOM_BOARD_H */