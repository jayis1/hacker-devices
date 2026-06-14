/*
 * board.h — CAN Bus Infiltrator hardware definitions
 * STM32F407VGT6 target
 */

#ifndef BOARD_H
#define BOARD_H

#include "registers.h"

/* Clock configuration */
#define HSE_VALUE       8000000UL    /* 8 MHz external crystal */
#define SYSCLK_FREQ     168000000UL  /* 168 MHz system clock */
#define APB1_FREQ        42000000UL  /* 42 MHz APB1 */
#define APB2_FREQ        84000000UL  /* 84 MHz APB2 */
#define AHB_FREQ         168000000UL  /* 168 MHz AHB */

/* CAN bus timing (for APB1 = 42 MHz, APB1_TIM = 84 MHz) */
#define CAN_APB1_TIM_FREQ  84000000UL  /* APB1 timer clock (APB1 × 2) */

/* Pin definitions */
/* CAN Channel 1 */
#define CAN1_RX_PIN       0   /* PB0 */
#define CAN1_RX_PORT      GPIOB
#define CAN1_TX_PIN       1   /* PB1 */
#define CAN1_TX_PORT      GPIOB
#define CAN1_STB_PIN      2   /* PE2 */
#define CAN1_STB_PORT      GPIOE

/* CAN Channel 2 */
#define CAN2_RX_PIN       10  /* PE10 */
#define CAN2_RX_PORT      GPIOE
#define CAN2_TX_PIN       11  /* PE11 */
#define CAN2_TX_PORT      GPIOE
#define CAN2_STB_PIN      3   /* PE3 */
#define CAN2_STB_PORT      GPIOE

/* SPI3 to nRF52840 */
#define SPI3_SCK_PIN      3   /* PB3 */
#define SPI3_SCK_PORT     GPIOB
#define SPI3_MISO_PIN     4   /* PB4 */
#define SPI3_MISO_PORT    GPIOB
#define SPI3_MOSI_PIN     5   /* PB5 */
#define SPI3_MOSI_PORT    GPIOB
#define SPI3_NSS_PIN      7   /* PD7 */
#define SPI3_NSS_PORT     GPIOD

/* nRF52840 control */
#define NRF_IRQ_PIN       1   /* PE1 */
#define NRF_IRQ_PORT      GPIOE
#define NRF_BOOT_PIN      7   /* PB7 */
#define NRF_BOOT_PORT     GPIOB
#define NRF_RESET_PIN     6   /* PB6 (shared with I2C1_SDA, use alternate) */
#define NRF_RESET_PORT    GPIOB

/* USB */
#define USB_DM_PIN        11  /* PA11 */
#define USB_DM_PORT       GPIOA
#define USB_DP_PIN        12  /* PA12 */
#define USB_DP_PORT       GPIOA

/* I2C1 (EEPROM) */
#define I2C1_SCL_PIN      15  /* PA15 */
#define I2C1_SCL_PORT     GPIOA
#define I2C1_SDA_PIN      6   /* PB6 */
#define I2C1_SDA_PORT     GPIOB

/* SDIO (microSD) */
#define SDIO_D0_PIN       8   /* PC8 */
#define SDIO_D0_PORT      GPIOC
#define SDIO_D1_PIN       9   /* PC9 */
#define SDIO_D1_PORT      GPIOC
#define SDIO_D2_PIN       10  /* PC10 */
#define SDIO_D2_PORT      GPIOC
#define SDIO_D3_PIN       11  /* PC11 */
#define SDIO_D3_PORT      GPIOC
#define SDIO_CK_PIN       12  /* PC12 */
#define SDIO_CK_PORT      GPIOC
#define SDIO_CMD_PIN      2   /* PD2 */
#define SDIO_CMD_PORT     GPIOD

/* SWD Debug */
#define SWDIO_PIN         13  /* PA13 */
#define SWDIO_PORT        GPIOA
#define SWCLK_PIN         14  /* PA14 */
#define SWCLK_PORT        GPIOA

/* LED strip (WS2812B) */
#define LED_DIN_PIN       7   /* PE7 */
#define LED_DIN_PORT      GPIOE
#define LED_COUNT         4

/* EEPROM config */
#define EEPROM_I2C_ADDR  0x50  /* AT24C02 7-bit address */
#define EEPROM_PAGE_SIZE  8

/* GPIO mode definitions */
#define GPIO_MODE_INPUT    0x00
#define GPIO_MODE_OUTPUT   0x01
#define GPIO_MODE_AF       0x02
#define GPIO_MODE_ANALOG   0x03

#define GPIO_SPEED_LOW     0x00
#define GPIO_SPEED_MED     0x01
#define GPIO_SPEED_HIGH    0x02
#define GPIO_SPEED_VHIGH   0x03

#define GPIO_PULL_NONE     0x00
#define GPIO_PULL_UP       0x01
#define GPIO_PULL_DOWN     0x02

#define GPIO_OTYPE_PP      0x00
#define GPIO_OTYPE_OD      0x01

/* Alternate function mappings */
#define AF_CAN1    9   /* CAN1 on PB0/PB1 */
#define AF_CAN2    9   /* CAN2 on PE10/PE11 */
#define AF_SPI3    6   /* SPI3 on PB3/PB4/PB5 */
#define AF_I2C1    4   /* I2C1 on PA15/PB6 */
#define AF_SDIO    12  /* SDIO on PC8-12, PD2 */
#define AF_USB     10  /* USB OTG FS on PA11/PA12 */
#define AF_SWJ     0   /* SWD on PA13/PA14 */

/* Utility macros */
#define BIT(n)           (1U << (n))
#define REG(addr)        (*(volatile uint32_t *)(addr))
#define ARRAY_SIZE(arr)  (sizeof(arr) / sizeof((arr)[0]))
#define MIN(a, b)        ((a) < (b) ? (a) : (b))
#define MAX(a, b)        ((a) > (b) ? (a) : (b))

static inline void gpio_set(GPIO_TypeDef *port, uint8_t pin) {
    port->BSRR = (1U << pin);
}

static inline void gpio_clear(GPIO_TypeDef *port, uint8_t pin) {
    port->BSRR = (1U << (pin + 16));
}

static inline void gpio_toggle(GPIO_TypeDef *port, uint8_t pin) {
    port->ODR ^= (1U << pin);
}

static inline uint8_t gpio_read(GPIO_TypeDef *port, uint8_t pin) {
    return (port->IDR >> pin) & 1;
}

static inline void delay_us(volatile uint32_t us) {
    while (us--) {
        for (volatile int i = 0; i < 42; i++);  /* ~1us at 168 MHz */
    }
}

static inline void delay_ms(volatile uint32_t ms) {
    while (ms--) delay_us(1000);
}

#endif /* BOARD_H */