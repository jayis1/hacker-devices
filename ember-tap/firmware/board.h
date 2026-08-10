/*
 * board.h — Pin map, clock constants, and board-level config for Ember-Tap
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Board: Ember-Tap v1 (STM32G474CBT6)
 * PCB: 52 × 22 mm dongle, USB-C plug (J1) + receptacle (J2)
 */

#ifndef EMBER_BOARD_H
#define EMBER_BOARD_H

#include <stdint.h>
#include "registers.h"

/* ---- Clock tree ----
 * HSE: 16 MHz crystal on PD0/PD1 (Y1)
 * SYSCLK target: 170 MHz via PLL (HSI fallback 16 MHz)
 * PLL: HSE/2 × 85 /2 = 170 MHz  (per RM0440)
 */
#define BOARD_HSE_HZ        16000000U
#define BOARD_SYSCLK_HZ     170000000U
#define BOARD_APB1_HZ       (BOARD_SYSCLK_HZ)   /* APB1 = SYSCLK on G4 */
#define BOARD_APB2_HZ       (BOARD_SYSCLK_HZ)
#define BOARD_AHB_HZ        BOARD_SYSCLK_HZ

/* Flash wait states for 170 MHz (VOS range 1): 6 WS */
#define BOARD_FLASH_WS      6

/* ---- Pin map ----
 *
 * GPIOA:
 *   PA0  — BUTTON_UP    (input, pull-up, EXTI0, active low)
 *   PA1  — BUTTON_DN    (input, pull-up, EXTI1, active low)
 *   PA2  — BUTTON_SEL   (input, pull-up, EXTI2, active low)
 *   PA3  — KILL_HW      (input, pull-up — hardware interlock monitor)
 *   PA4  — LED_STATUS   (output, push-pull)  active high
 *   PA8  — FUSB_INT     (input, EXTI8 — FUSB302B interrupt, active low)
 *   PA9  — USB_VBUS_SENSE (input analog — divider 1/11 of VBUS)
 *   PA11 — USB_DM       (AF10: USB FS D-)
 *   PA12 — USB_DP       (AF10: USB FS D+)
 *   PA13 — SWDIO        (AF0: debug)
 *   PA14 — SWCLK        (AF0: debug)
 *
 * GPIOB:
 *   PB6  — I2C1_SCL     (AF4: FUSB302B + TPS25982)
 *   PB7  — I2C1_SDA     (AF4)
 *   PB10 — I2C2_SCL     (AF4: SSD1306 OLED)
 *   PB11 — I2C2_SDA     (AF4)
 *   PB12 — GLITCH_GATE  (output — VBUS glitch N-FET gate, active high)
 *   PB13 — TPS_FLTB     (input — TPS25982 fault output, active low)
 *   PB14 — TPS_EN       (output — TPS25982 enable, active high)
 *   PB15 — THERM        (input analog — NTC thermistor divider)
 *
 * GPIOC:
 *   PC0  — VBUS_VSENSE  (input analog — TPS25982 voltage monitor buffer)
 *   PC1  — VBUS_ISENSE  (input analog — TPS25982 current monitor)
 *
 * GPIOF:
 *   PF0  — nRST         (input, reset button)
 */

#define BUTTON_UP_PORT     GPIOA
#define BUTTON_UP_PIN      0
#define BUTTON_DN_PORT     GPIOA
#define BUTTON_DN_PIN      1
#define BUTTON_SEL_PORT    GPIOA
#define BUTTON_SEL_PIN     2
#define KILL_HW_PORT       GPIOA
#define KILL_HW_PIN        3

#define LED_STATUS_PORT    GPIOA
#define LED_STATUS_PIN     4

#define FUSB_INT_PORT      GPIOA
#define FUSB_INT_PIN       8
#define FUSB_INT_EXTI      8

#define USB_DM_PIN         11
#define USB_DP_PIN         12

#define I2C1_SCL_PIN       6
#define I2C1_SDA_PIN       7
#define I2C2_SCL_PIN       10
#define I2C2_SDA_PIN       11

#define GLITCH_GATE_PORT   GPIOB
#define GLITCH_GATE_PIN    12
#define TPS_FLTB_PORT      GPIOB
#define TPS_FLTB_PIN       13
#define TPS_EN_PORT        GPIOB
#define TPS_EN_PIN         14
#define THERM_PORT         GPIOB
#define THERM_PIN          15

#define VBUS_VSENSE_PORT   GPIOC
#define VBUS_VSENSE_PIN    0
#define VBUS_ISENSE_PORT   GPIOC
#define VBUS_ISENSE_PIN    1

/* ---- I2C addresses (7-bit) ---- */
#define FUSB302_I2C_ADDR   0x22
#define TPS25982_I2C_ADDR  0x10
#define SSD1306_I2C_ADDR   0x3C

/* ---- FUSB302B register map (for driver) ---- */
#define FUSB302_REG_DEVICE_ID   0x01
#define FUSB302_REG_SWITCHES0   0x02
#define FUSB302_REG_SWITCHES1   0x03
#define FUSB302_REG_MEASURE     0x04
#define FUSB302_REG_CONTROL0    0x06
#define FUSB302_REG_CONTROL1    0x07
#define FUSB302_REG_CONTROL2    0x08
#define FUSB302_REG_CONTROL3    0x09
#define FUSB302_REG_MASK        0x0A
#define FUSB302_REG_POWER       0x0B
#define FUSB302_REG_RESET       0x0C
#define FUSB302_REG_MASKA       0x0E
#define FUSB302_REG_MASKB       0x0F
#define FUSB302_REG_STATUS0A    0x3C
#define FUSB302_REG_STATUS1A    0x3D
#define FUSB302_REG_STATUS0     0x40
#define FUSB302_REG_STATUS1     0x41
#define FUSB302_REG_INTERRUPT   0x42
#define FUSB302_REG_FIFOS       0x43

#define FUSB302_SW0_PU_EN       0x01
#define FUSB302_SW0_PD_EN       0x02
#define FUSB302_SW0_CC1_MEAS    0x04
#define FUSB302_SW0_CC2_MEAS    0x08
#define FUSB302_SW0_AUTO_CRC    0x40

#define FUSB302_SW1_TXCC1       0x01
#define FUSB302_SW1_TXCC2       0x02
#define FUSB302_SW1_SPECREV0    0x04
#define FUSB302_SW1_SPECREV1    0x08
#define FUSB302_SW1_AUTO_CRC    0x20
#define FUSB302_SW1_RXCC1       0x40
#define FUSB302_SW1_RXCC2       0x80

#define FUSB302_CTRL0_TX_FLUSH  0x40
#define FUSB302_CTRL0_RX_FLUSH  0x20
#define FUSB302_CTRL0_HOST_MODE 0x10
#define FUSB302_CTRL0_TX_START  0x04

#define FUSB302_FIFO_TX_TOKEN_SOP      0x12
#define FUSB302_FIFO_TX_TOKEN_SOP_PRIME   0x13
#define FUSB302_FIFO_TX_TOKEN_SOP_DOUBLE  0x14

#define FUSB302_INT_RXSOP     0x04
#define FUSB302_INT_TXSOP     0x40

/* ---- TPS25982 register map ---- */
#define TPS_REG_STATUS     0x1A
#define TPS_REG_CTRL       0x1B
#define TPS_REG_ILIM       0x1C
#define TPS_REG_OVP        0x1D
#define TPS_REG_VIN        0x21
#define TPS_REG_IIN        0x23

/* ---- Board constants ---- */
#define BOARD_OLED_W       128
#define BOARD_OLED_H       64

/* VBUS ADC scaling: 1/11 divider → mV = raw * 3300 / 4095 * 11 */
#define VBUS_DIVIDER_NUM   11
#define VBUS_DIVIDER_DEN   1
#define ADC_VREF_MV        3300
#define ADC_FULLSCALE      4095

/* TPS25982 OVP threshold codes (0.1 V units) */
#define TPS_OVP_5V         60
#define TPS_OVP_9V         100
#define TPS_OVP_15V        160
#define TPS_OVP_20V        220

/* Fuzz campaign defaults */
#define FUZZ_MAX_FRAMES    10000
#define FUZZ_LOG_SIZE      8192
#define PD_RING_SIZE       4096
#define CDC_TX_BUF_SIZE    2048
#define CDC_RX_BUF_SIZE    512

/* Hardware interlock: max VBUS (mV) at which glitch is permitted */
#define GLITCH_VBUS_MAX_MV 20000

/* PD message types (subset of USB-PD R3.1) */
#define PD_MSGTYPE_CONTROL       0
#define PD_MSGTYPE_DATA          1
#define PD_MSGTYPE_EXTENDED      2

/* Control message subtypes */
#define PD_CTRL_GOODCRC         0x1
#define PD_CTRL_GOTOMIN         0x2
#define PD_CTRL_ACCEPT          0x3
#define PD_CTRL_REJECT          0x4
#define PD_CTRL_PING            0x5
#define PD_CTRL_PSRDY           0x6
#define PD_CTRL_GETSRCCAP       0x7
#define PD_CTRL_GETSNKCAP       0x8
#define PD_CTRL_DRSWAP          0x9
#define PD_CTRL_PRSWAP          0xA
#define PD_CTRL_VCONNSWAP       0xB
#define PD_CTRL_WAIT            0xC
#define PD_CTRL_SOFTRESET       0xD
#define PD_CTRL_HARDRESET       0x0  /* special */
#define PD_CTRL_NOTSUPPORTED    0x10
#define PD_CTRL_GETSOURCECAP    0x17

/* Data message subtypes */
#define PD_DATA_SRCCAP          0x1
#define PD_DATA_REQUEST         0x2
#define PD_DATA_SNKCAP          0x4
#define PD_DATA_VDM             0xF
#define PD_DATA_SRCCAP_EXT      0x1F

/* Spec revision codes */
#define PD_SPECREV_1_0   0x0
#define PD_SPECREV_2_0   0x1
#define PD_SPECREV_3_0   0x2

/* ---- Helper macros ---- */
#define BIT(n)          (1U << (n))
#define ARRAY_SIZE(a)   (sizeof(a) / sizeof((a)[0]))
#define MIN(a,b)        ((a) < (b) ? (a) : (b))
#define MAX(a,b)        ((a) > (b) ? (a) : (b))

/* ---- Exported globals (defined in main.c) ---- */
typedef enum {
    MODE_PASSIVE = 0,    /* transparent PD pass-through (sniff only) */
    MODE_SPOOF_SRC,      /* act as source to DUT */
    MODE_SINK_MASQ,      /* act as sink to real source */
    MODE_FUZZ,           /* active fuzzing campaign */
    MODE_GLITCH,         /* VBUS glitching */
    MODE_COUNT
} board_mode_t;

#endif /* EMBER_BOARD_H */
/* end of file — author: jayis1 */