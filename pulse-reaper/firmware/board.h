/*
 * board.h — Pulse-Reaper hardware board definition
 *
 * Target:  Pulse-Reaper rev1
 * MCU:     STM32H743VIT6 (Cortex-M7 @ 480 MHz, LQFP-100)
 * FPGA:    Lattice iCE40UP5K-SG48
 * ADC:     AD9203ARZ (2x 10-bit 100 MSa/s)
 * BLE:     nRF52840-M.2 (UART C2)
 * OLED:    SSD1306 128x64 (I2C)
 * SD:      MicroSD UHS-I (SDMMC1)
 * Flash:   W25Q128JVSIQ (SPI NOR, 16 MB)
 * Author:  jayis1
 * License: GPL-2.0
 */

#ifndef PULSEREAPER_BOARD_H
#define PULSEREAPER_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ----------------------------------------------------------------------- */
/*  Clock tree                                                             */
/* ----------------------------------------------------------------------- */
#define BOARD_HSI_MHZ           64u
#define BOARD_HSE_MHZ           8u        /* external 8 MHz crystal       */
#define BOARD_PLL_N             120u      /* VCO = 8 * 120 = 960 MHz       */
#define BOARD_PLL_P             2u        /* SYS = 960 / 2 = 480 MHz      */
#define BOARD_PLL_Q             5u        /* USB = 960 / 5 = 192 MHz     */
#define BOARD_PLL_R             2u
#define BOARD_SYS_MHZ           480u
#define BOARD_HCLK_MHZ          240u       /* AHB /2                       */
#define BOARD_PCLK1_MHZ         120u       /* APB1 /2 of HCLK              */
#define BOARD_PCLK2_MHZ         120u       /* APB2 /2 of HCLK              */
#define BOARD_APBX_TIM_MHZ      240u       /* APB timers run at 2x PCLK    */

/* ----------------------------------------------------------------------- */
/*  Pin map — STM32H743 LQFP-100                                           */
/* ----------------------------------------------------------------------- */

/* FPGA SPI (SPI1, Master, up to 50 MHz) */
#define FPGA_SPI                SPI1
#define FPGA_SCK_PORT            GPIOA
#define FPGA_SCK_PIN             5u         /* PA5  CK  */
#define FPGA_MISO_PORT           GPIOA
#define FPGA_MISO_PIN            6u         /* PA6  MISO */
#define FPGA_MOSI_PORT           GPIOB
#define FPGA_MOSI_PIN            5u         /* PB5  MOSI */
#define FPGA_CS_PORT              GPIOB
#define FPGA_CS_PIN               4u         /* PB4  CS   */
#define FPGA_IRQ_PORT             GPIOC
#define FPGA_IRQ_PIN              4u         /* PC4  IRQ (FPGA -> MCU) */
#define FPGA_CDONE_PORT           GPIOC
#define FPGA_CDONE_PIN            5u         /* PC5  CDONE */
#define FPGA_CRESET_PORT          GPIOC
#define FPGA_CRESET_PIN           13u        /* PC13 CRESET (MCU -> FPGA) */

/* ADC parallel bus — the AD9203 is slurped by the FPGA, not the MCU.
 * The MCU reads frames from the FPGA over SPI. The pins below are the
 * FPGA <-> ADC pins defined here only for documentation / layout checks. */

/* nRF52840 BLE C2 UART (USART3) */
#define BLE_UART                 USART3
#define BLE_TX_PORT               GPIOC
#define BLE_TX_PIN                10u        /* PC10 TX  */
#define BLE_RX_PORT               GPIOC
#define BLE_RX_PIN                11u        /* PC11 RX  */
#define BLE_CTS_PORT              GPIOC
#define BLE_CTS_PIN               12u        /* PC12 CTS */
#define BLE_RTS_PORT              GPIOD
#define BLE_RTS_PIN               2u         /* PD2  RTS */
#define BLE_RESET_PORT            GPIOC
#define BLE_RESET_PIN             14u        /* PC14 RESET (MCU -> nRF) */

/* MicroSD (SDMMC1, 4-bit, UHS-I SDR104) */
#define SD_SDMMC                  SDMMC1
#define SD_CK_PORT                 GPIOC
#define SD_CK_PIN                  12u       /* PC12 CK  (conflict? no: BLE_CTS is USART3 CTS, SD uses SDMMC1) */
/* NOTE: pins reassigned to avoid conflict. Final layout: */
#define SD_CK_PORT_FINAL           GPIOC
#define SD_CK_PIN_FINAL            8u        /* PC8  SDMMC1 CK */
#define SD_CMD_PORT                GPIOC
#define SD_CMD_PIN                 9u        /* PC9  CMD       */
#define SD_D0_PORT                 GPIOC
#define SD_D0_PIN                  6u        /* PC6  D0        */
#define SD_D1_PORT                 GPIOC
#define SD_D1_PIN                  7u        /* PC7  D1        */
/* BLE UART is on USART3 (PC10/PC11), so SD D2 uses PB6. */
#define SD_D2_PORT                 GPIOB
#define SD_D2_PIN                  6u        /* PB6  D2        */
#define SD_D3_PORT                 GPIOB
#define SD_D3_PIN                  7u        /* PB7  D3        */
#define SD_DETECT_PORT             GPIOB
#define SD_DETECT_PIN              15u       /* PB15 card detect */

/* OLED SSD1306 128x64 (I2C1) */
#define OLED_I2C                  I2C1
#define OLED_SCL_PORT              GPIOB
#define OLED_SCL_PIN               8u        /* PB8  SCL */
#define OLED_SDA_PORT              GPIOB
#define OLED_SDA_PIN               9u        /* PB9  SDA */
#define OLED_ADDR                  0x3Cu

/* SPI NOR flash W25Q128 (SPI2) */
#define NOR_SPI                    SPI2
#define NOR_SCK_PORT               GPIOB
#define NOR_SCK_PIN                 13u       /* PB13 SCK  */
#define NOR_MISO_PORT               GPIOB
#define NOR_MISO_PIN                14u       /* PB14 MISO */
#define NOR_MOSI_PORT               GPIOB
#define NOR_MOSI_PIN                2u        /* PB2  MOSI */
#define NOR_CS_PORT                 GPIOB
#define NOR_CS_PIN                  1u        /* PB1  CS   */

/* USB-C CDC (USB1 OTG FS) */
#define USB_OTG                    USB1
#define USB_DM_PORT                 GPIOA
#define USB_DM_PIN                  11u       /* PA11 DM */
#define USB_DP_PORT                 GPIOA
#define USB_DP_PIN                  12u       /* PA12 DP */

/* Clamp front-end control (isolated side, via ISO7741 digital isolators) */
#define CLAMP_GAIN_SEL_PORT         GPIOD
#define CLAMP_GAIN_SEL_PIN          3u        /* PD3  gain select (0=low, 1=high) */
#define CLAMP_COUPLING_PORT         GPIOD
#define CLAMP_COUPLING_PIN          4u        /* PD4  0=cap, 1=ind, 2=both */
#define CLAMP_INJECT_EN_PORT         GPIOD
#define CLAMP_INJECT_EN_PIN         5u        /* PD5  inject driver enable */
#define CLAMP_JAW_CLOSED_PORT       GPIOD
#define CLAMP_JAW_CLOSED_PIN        6u        /* PD6  jaw leaf-switch (1=closed) */
#define CLAMP_TDR_DISCHARGE_PORT    GPIOD
#define CLAMP_TDR_DISCHARGE_PIN     7u        /* PD7  TDR path discharge gate */

/* Tactile buttons */
#define BTN_MODE_PORT               GPIOE
#define BTN_MODE_PIN                0u        /* PE0  mode cycle */
#define BTN_ACTION_PORT             GPIOE
#define BTN_ACTION_PIN              1u        /* PE1  action / select */

/* Status LED (dual-color) */
#define LED_R_PORT                   GPIOE
#define LED_R_PIN                    2u       /* PE2 red   */
#define LED_G_PORT                   GPIOE
#define LED_G_PIN                    3u       /* PE3 green */

/* Fuel gauge / charger I2C (I2C4) — BQ25896 + MCP73871 */
#define PM_I2C                       I2C4
#define PM_SCL_PORT                  GPIOD
#define PM_SCL_PIN                   12u      /* PD12 SCL */
#define PM_SDA_PORT                  GPIOD
#define PM_SDA_PIN                   13u      /* PD13 SDA */

/* ----------------------------------------------------------------------- */
/*  Peripheral assignments (high-level handles)                            */
/* ----------------------------------------------------------------------- */

#define PULSEREAPER_FPGA_SPI         FPGA_SPI
#define PULSEREAPER_BLE_UART         BLE_UART
#define PULSEREAPER_SD_SDMMC          SD_SDMMC
#define PULSEREAPER_OLED_I2C          OLED_I2C
#define PULSEREAPER_NOR_SPI           NOR_SPI
#define PULSEREAPER_PM_I2C            PM_I2C

/* ----------------------------------------------------------------------- */
/*  Globals (defined in main.c)                                             */
/* ----------------------------------------------------------------------- */
extern volatile uint32_t g_dwt_cycles_per_us;

/* ----------------------------------------------------------------------- */
/*  Timing helpers                                                         */
/* ----------------------------------------------------------------------- */

#define BOARD_US_PER_TICK(count)   ((uint32_t)((count) * 1000u / BOARD_HCLK_MHZ))
#define BOARD_NS_PER_TICK(count)    ((uint32_t)((count) * 1000u / BOARD_HCLK_MHZ))

static inline void board_delay_us(uint32_t us) {
    /* DWT cycle counter delay — no SysTick dependency */
    extern volatile uint32_t g_dwt_cycles_per_us;
    volatile uint32_t *DWT_CYCCNT = (volatile uint32_t *)0xE0001004u;
    volatile uint32_t *DWT_CTRL   = (volatile uint32_t *)0xE0001000u;
    *DWT_CTRL |= 1u;                          /* enable CYCCNT               */
    uint32_t start = *DWT_CYCCNT;
    uint32_t cycles = us * g_dwt_cycles_per_us;
    while ((uint32_t)(*DWT_CYCCNT - start) < cycles) { /* spin */ }
}

static inline void board_delay_ms(uint32_t ms) {
    while (ms--) board_delay_us(1000u);
}

/* ----------------------------------------------------------------------- */
/*  Board-level enumerations                                               */
/* ----------------------------------------------------------------------- */

typedef enum {
    PR_COUPLING_CAP   = 0,   /* capacitive plates only                  */
    PR_COUPLING_IND   = 1,   /* inductive sense coil only               */
    PR_COUPLING_BOTH  = 2    /* both — best SNR, highest loading        */
} clamp_coupling_t;

typedef enum {
    PR_GAIN_LOW  = 0,        /* -6 dB, for near/full-scale fieldbus     */
    PR_GAIN_MID  = 1,        /* +6 dB  */
    PR_GAIN_HIGH = 2         /* +26 dB, for weak POTS / long cable       */
} clamp_gain_t;

typedef enum {
    PR_MODE_IDLE = 0,
    PR_MODE_TDR,
    PR_MODE_SNIFF,
    PR_MODE_INJECT,
    PR_MODE_COVERT,
    PR_MODE_CABLEMAP
} pr_mode_t;

/* ----------------------------------------------------------------------- */
/*  Board init                                                             */
/* ----------------------------------------------------------------------- */

void board_init(void);
void board_gpio_init(void);
void board_clock_init(void);
void board_nvic_init(void);
void board_dma_init(void);

void clamp_set_coupling(clamp_coupling_t c);
void clamp_set_gain(clamp_gain_t g);
void clamp_inject_enable(int on);
int  clamp_jaw_closed(void);
void clamp_tdr_discharge(void);

void led_set(int red, int green);

#endif /* PULSEREAPER_BOARD_H */