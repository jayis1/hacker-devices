/*
 * board.h — Powerline-Reaper pin map, clock tree, peripheral assignments
 *
 * Hardware: STM32H743VIT6 + QCA7420 PLC SoC (SPI) + nRF52840 BLE (UART)
 *           + MicroSD (SDIO) + W25Q128 QSPI + MAX17048 fuel gauge (I2C)
 *           + USB-C composite gadget (CDC-ACM + CDC-ECM)
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef POWERLINE_REAPER_BOARD_H
#define POWERLINE_REAPER_BOARD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Clock tree (STM32H743) ---- */
#define BOARD_HSE_HZ          25000000UL   /* external 25 MHz xtal */
#define BOARD_SYSCLK_HZ       480000000UL  /* SYS @ 480 MHz (VOS0) */
#define BOARD_HCLK_HZ         240000000UL  /* HCLK /2 = 240 MHz AHB  */
#define BOARD_APB1_HZ         120000000UL  /* APB1 /4 from SYS  */
#define BOARD_APB2_HZ         120000000UL  /* APB2 /4 from SYS  */
#define BOARD_APB4_HZ         120000000UL  /* APB4 /4 from SYS  */

/* ---- GPIO pin assignments (STM32H743 VIT6 LQFP100) ----
 * Convention: <port><pin>. All pins at 3V3 logic; SPI to QCA7420 crosses
 * the 5kV isolation barrier via ADuM1402 quad digital isolators.
 */

/* QCA7420 PLC SoC — SPI2, isolated */
#define PLC_SPI               SPI2
#define PLC_SPI_CLK_PIN       GPIO_PIN('D', 3)   /* PD3  SPI2_SCK  -> ADuM1402 -> QCA SCK  */
#define PLC_SPI_MISO_PIN      GPIO_PIN('D', 4)   /* PD4  SPI2_MISO <- ADuM1402 <- QCA MISO */
#define PLC_SPI_MOSI_PIN      GPIO_PIN('D', 5)   /* PD5  SPI2_MOSI -> ADuM1402 -> QCA MOSI */
#define PLC_SPI_CS_PIN        GPIO_PIN('D', 6)   /* PD6  GPIO CS   -> ADuM1402 -> QCA CSB  */
#define PLC_IRQ_PIN           GPIO_PIN('D', 7)   /* PD7  EXTI9     <- ADuM1402 <- QCA IRQ */
#define PLC_RST_PIN           GPIO_PIN('D', 8)   /* PD8  GPIO      -> ADuM1402 -> QCA RSTB */
#define PLC_PWR_EN_PIN        GPIO_PIN('D', 9)   /* PD9  GPIO      -> QCA 3V3 LDO enable  */

/* nRF52840 BLE — USART3 */
#define BLE_UART              USART3
#define BLE_TX_PIN            GPIO_PIN('C', 10)  /* PC10 USART3_TX  -> nRF52 RX  */
#define BLE_RX_PIN            GPIO_PIN('C', 11)  /* PC11 USART3_RX  <- nRF52 TX  */
#define BLE_CTS_PIN           GPIO_PIN('C', 12)  /* PC12 USART3_CTS <- nRF52 RTS */
#define BLE_RTS_PIN           GPIO_PIN('D', 2)   /* PD2  USART3_RTS -> nRF52 CTS */
#define BLE_RST_PIN            GPIO_PIN('C', 13)  /* PC13 GPIO       -> nRF52 RST */
#define BLE_IRQ_PIN           GPIO_PIN('C', 14)  /* PC14 EXTI15     <- nRF52 IRQ  */

/* MicroSD — SDMMC1, 4-bit */
#define SD_SDMMC              SDMMC1
#define SD_CK_PIN              GPIO_PIN('C', 12)  /* PC12 SDMMC1_CK  (NOTE: alias conflict
                                                    resolved on rev B; rev A uses SDMMC1) */
/* Use alternate SDMMC2 on rev A to avoid clash with BLE_CTS */
#define SD_SDMMC_ALT          SDMMC2
#define SD2_CK_PIN            GPIO_PIN('D', 6)   /* redefined below — see notes */
/* Final rev A: SD on SDMMC2 */
#undef  SD_SDMMC
#define SD_SDMMC              SDMMC2
#define SD_CK_PIN             GPIO_PIN('D', 6)   /* PD6 SDMMC2_CK  */
#define SD_CMD_PIN            GPIO_PIN('D', 7)   /* PD7 SDMMC2_CMD  */
#define SD_D0_PIN             GPIO_PIN('B', 14)  /* PB14 SDMMC2_D0  */
#define SD_D1_PIN             GPIO_PIN('B', 15)  /* PB15 SDMMC2_D1  */
#define SD_D2_PIN             GPIO_PIN('B', 4)   /* PB4  SDMMC2_D2  */
#define SD_D3_PIN             GPIO_PIN('B', 5)   /* PB5  SDMMC2_D3  */
#define SD_CD_PIN             GPIO_PIN('C', 5)   /* PC5  GPIO card-detect (active low) */

/* W25Q128 external flash — QSPI1 */
#define QSPI                  QUADSPI
#define QSPI_CLK_PIN          GPIO_PIN('B', 2)   /* PB2  QSPI_CLK  */
#define QSPI_CS_PIN           GPIO_PIN('B', 6)  /* PB6  QSPI_CS   */
#define QSPI_D0_PIN           GPIO_PIN('C', 9)   /* PC9  QSPI_D0   */
#define QSPI_D1_PIN           GPIO_PIN('C', 10)  /* PC10 QSPI_D1   */
#define QSPI_D2_PIN           GPIO_PIN('D', 11)  /* PD11 QSPI_D2   */
#define QSPI_D3_PIN           GPIO_PIN('D', 12)  /* PD12 QSPI_D3   */

/* USB-C — USB1 (OTG HS in FS mode, internal PHY) */
#define USB_PERIPH            USB1_OTG_HS
#define USB_DM_PIN            GPIO_PIN('A', 11)  /* PA11 USB_DM  */
#define USB_DP_PIN            GPIO_PIN('A', 12)  /* PA12 USB_DP  */
#define USB_VBUS_PIN          GPIO_PIN('A', 9)   /* PA9  VBUS sense */

/* I²C — MAX17048 fuel gauge + DS18B20 (1-wire on separate GPIO) */
#define I2C_PERIPH            I2C1
#define I2C_SCL_PIN           GPIO_PIN('B', 8)   /* PB8  I2C1_SCL  */
#define I2C_SDA_PIN           GPIO_PIN('B', 9)   /* PB9  I2C1_SDA  */
#define FUEL_GAUGE_ADDR       0x36               /* MAX17048 7-bit addr */
#define DS18B20_PIN           GPIO_PIN('C', 8)   /* PC8  1-wire bus (bit-bang) */

/* ACS712 mains current sensor — ADC1 */
#define CURRENT_ADC           ADC1
#define CURRENT_ADC_PIN       GPIO_PIN('A', 0)   /* PA0  ADC1_INP0 / INN0 (differential optional) */
#define CURRENT_ADC_CH        0

/* Status RGB LED — PWM via TIM2 channels (active high, common cathode) */
#define LED_TIM               TIM2
#define LED_R_PIN              GPIO_PIN('A', 15)  /* PA15 TIM2_CH1 -> R */
#define LED_G_PIN              GPIO_PIN('B', 3)   /* PB3  TIM2_CH2 -> G */
#define LED_B_PIN              GPIO_PIN('B', 10)  /* PB10 TIM2_CH3 -> B */

/* Mode button (active low, internal pull-up) */
#define BTN_PIN               GPIO_PIN('C', 4)   /* PC4  EXTI4  */
#define BTN_PORT               GPIOC

/* Tamper switch (chassis-open, active low with pull-up) */
#define TAMPER_PIN            GPIO_PIN('C', 0)   /* PC0  EXTI0  */
#define TAMPER_PORT           GPIOC

/* LiPo charge status from MCP73871 */
#define CHG_STAT1_PIN         GPIO_PIN('C', 6)   /* PC6  GPIO in  */
#define CHG_STAT2_PIN         GPIO_PIN('C', 7)  /* PC7  GPIO in  */

/* ---- Helper macros ---- */
#define GPIO_PIN(port, num)   (((port - 'A') << 8) | (num & 0xFF))
#define GPIO_PORT(pin)        ((pin) >> 8)
#define GPIO_NUM(pin)         ((pin) & 0xFF)

/* ---- Mode enumeration ---- */
typedef enum {
    MODE_INIT = 0,
    MODE_LINKING,        /* waiting for PLC sync */
    MODE_SNIFF,           /* promiscuous capture only */
    MODE_BRIDGE,         /* USB CDC-ECM bridge to PLC segment */
    MODE_INJECT,         /* active injection / rogue CCo */
    MODE_NMK_ATTACK,    /* NMK dictionary attack running */
    MODE_TUNNEL,         /* air-gap tunnel peer mode */
    MODE_COVERT,         /* battery: capture to SD + BLE only */
    MODE_FAULT            /* tamper / overtemp / power fault */
} board_mode_t;

/* ---- Capability flags ---- */
#define CAP_PLC_LINK         0x0001
#define CAP_USB_HOST         0x0002
#define CAP_SD_PRESENT      0x0004
#define CAP_BLE_PAIRED      0x0008
#define CAP_BATTERY_OK      0x0010
#define CAP_NM_KNOWN         0x0020   /* we have a working NMK */
#define CAP_INJECTING       0x0040
#define CAP_TUNNEL_UP       0x0080

#ifdef __cplusplus
}
#endif

#endif /* POWERLINE_REAPER_BOARD_H */