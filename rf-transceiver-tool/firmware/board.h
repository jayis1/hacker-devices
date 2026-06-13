/*
 * board.h — RF Transceiver Tool Board-Level Definitions
 *
 * Pin mappings, LED/button assignments, and hardware constants
 * for the STM32F401CCU6-based RF Transceiver Tool.
 *
 * MCU: STM32F401CCU6 (UFQFPN-48)
 * Peripherals:
 *   - CC1101 Sub-GHz transceiver on SPI1
 *   - nRF24L01+ 2.4 GHz transceiver on SPI2
 *   - SSD1306 OLED on I2C1
 *   - USB CDC on USB OTG FS
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

/* ========================================================================
 * Clock Configuration
 * ======================================================================== */
#define HSE_FREQ_HZ         8000000U    /* 8 MHz external crystal */
#define SYSCLK_FREQ_HZ      84000000U   /* 84 MHz system clock */
#define AHB_FREQ_HZ         84000000U   /* 84 MHz AHB */
#define APB1_FREQ_HZ        42000000U   /* 42 MHz APB1 */
#define APB2_FREQ_HZ        84000000U   /* 84 MHz APB2 */

/* ========================================================================
 * GPIO Port Base Addresses
 * ======================================================================== */
#define GPIOA_BASE           0x40020000U
#define GPIOB_BASE           0x40020400U
#define GPIOC_BASE           0x40020800U

/* ========================================================================
 * LED Definitions (Active-High)
 * ======================================================================== */
#define LED1_PORT            GPIOB_BASE    /* PB0 — Sub-GHz status (green) */
#define LED1_PIN             0U
#define LED2_PORT            GPIOB_BASE    /* PB1 — 2.4 GHz status (blue) */
#define LED2_PIN             1U

/* LED convenience macros */
#define LED1_ON()            (GPIOx_ODR(LED1_PORT) |= (1U << LED1_PIN))
#define LED1_OFF()           (GPIOx_ODR(LED1_PORT) &= ~(1U << LED1_PIN))
#define LED1_TOGGLE()        (GPIOx_ODR(LED1_PORT) ^= (1U << LED1_PIN))

#define LED2_ON()            (GPIOx_ODR(LED2_PORT) |= (1U << LED2_PIN))
#define LED2_OFF()           (GPIOx_ODR(LED2_PORT) &= ~(1U << LED2_PIN))
#define LED2_TOGGLE()        (GPIOx_ODR(LED2_PORT) ^= (1U << LED2_PIN))

/* ========================================================================
 * Button Definitions (Active-Low, pulled high)
 * ======================================================================== */
#define BTN1_PORT            GPIOA_BASE    /* PA0 — Mode select */
#define BTN1_PIN             0U
#define BTN2_PORT            GPIOA_BASE    /* PA1 — Action/trigger */
#define BTN2_PIN             1U

#define BTN1_PRESSED()       (!(GPIOx_IDR(BTN1_PORT) & (1U << BTN1_PIN)))
#define BTN2_PRESSED()       (!(GPIOx_IDR(BTN2_PORT) & (1U << BTN2_PIN)))

/* ========================================================================
 * CC1101 Sub-GHz Transceiver (SPI1)
 * ======================================================================== */
#define CC1101_SPI_BASE      SPI1_BASE
#define CC1101_CSN_PORT      GPIOA_BASE
#define CC1101_CSN_PIN       4U           /* PA4 — SPI1_NSS (bit-banged) */
#define CC1101_SCLK_PORT     GPIOA_BASE
#define CC1101_SCLK_PIN      5U           /* PA5 — SPI1_SCK */
#define CC1101_MISO_PORT     GPIOA_BASE
#define CC1101_MISO_PIN      6U           /* PA6 — SPI1_MISO */
#define CC1101_MOSI_PORT     GPIOA_BASE
#define CC1101_MOSI_PIN      7U           /* PA7 — SPI1_MOSI */
#define CC1101_GDO0_PORT     GPIOA_BASE
#define CC1101_GDO0_PIN      8U           /* PA8 */
#define CC1101_GDO2_PORT     GPIOA_BASE
#define CC1101_GDO2_PIN      9U           /* PA9 */
#define CC1101_RESET_PORT    GPIOA_BASE
#define CC1101_RESET_PIN     3U           /* PA3 — active-low reset */

/* CSN control macros */
#define CC1101_CSN_LOW()     (GPIOx_ODR(CC1101_CSN_PORT) &= ~(1U << CC1101_CSN_PIN))
#define CC1101_CSN_HIGH()    (GPIOx_ODR(CC1101_CSN_PORT) |= (1U << CC1101_CSN_PIN))

/* RESET control macros */
#define CC1101_RESET_LOW()   (GPIOx_ODR(CC1101_RESET_PORT) &= ~(1U << CC1101_RESET_PIN))
#define CC1101_RESET_HIGH()  (GPIOx_ODR(CC1101_RESET_PORT) |= (1U << CC1101_RESET_PIN))

/* GDO pin read */
#define CC1101_GDO0_READ()   ((GPIOx_IDR(CC1101_GDO0_PORT) >> CC1101_GDO0_PIN) & 1U)
#define CC1101_GDO2_READ()   ((GPIOx_IDR(CC1101_GDO2_PORT) >> CC1101_GDO2_PIN) & 1U)

/* ========================================================================
 * nRF24L01+ 2.4 GHz Transceiver (SPI2)
 * ======================================================================== */
#define NRF24_SPI_BASE       SPI2_BASE
#define NRF24_CSN_PORT       GPIOB_BASE
#define NRF24_CSN_PIN        12U          /* PB12 — SPI2_NSS (bit-banged) */
#define NRF24_SCLK_PORT      GPIOB_BASE
#define NRF24_SCLK_PIN       13U          /* PB13 — SPI2_SCK */
#define NRF24_MISO_PORT      GPIOB_BASE
#define NRF24_MISO_PIN       14U          /* PB14 — SPI2_MISO */
#define NRF24_MOSI_PORT      GPIOB_BASE
#define NRF24_MOSI_PIN       15U          /* PB15 — SPI2_MOSI */
#define NRF24_CE_PORT        GPIOA_BASE
#define NRF24_CE_PIN         15U          /* PA15 — Chip Enable */
#define NRF24_IRQ_PORT       GPIOB_BASE
#define NRF24_IRQ_PIN        3U           /* PB3 — Active-low interrupt */

/* CSN control macros */
#define NRF24_CSN_LOW()      (GPIOx_ODR(NRF24_CSN_PORT) &= ~(1U << NRF24_CSN_PIN))
#define NRF24_CSN_HIGH()    (GPIOx_ODR(NRF24_CSN_PORT) |= (1U << NRF24_CSN_PIN))

/* CE control macros */
#define NRF24_CE_LOW()      (GPIOx_ODR(NRF24_CE_PORT) &= ~(1U << NRF24_CE_PIN))
#define NRF24_CE_HIGH()     (GPIOx_ODR(NRF24_CE_PORT) |= (1U << NRF24_CE_PIN))

/* IRQ read macro */
#define NRF24_IRQ_READ()    (!(GPIOx_IDR(NRF24_IRQ_PORT) & (1U << NRF24_IRQ_PIN)))

/* ========================================================================
 * SSD1306 OLED Display (I2C1)
 * ======================================================================== */
#define SSD1306_I2C_BASE     I2C1_BASE
#define SSD1306_I2C_ADDR     0x3CU        /* 7-bit address 0x3C, left-shifted */
#define SSD1306_WIDTH        128U
#define SSD1306_HEIGHT       64U
#define SSD1306_PAGES        (SSD1306_HEIGHT / 8U)

/* ========================================================================
 * USB CDC Configuration
 * ======================================================================== */
#define USB_VID              0x1209U       /* pid.codes open-source VID */
#define USB_PID              0xCC02U       /* RF Transceiver Tool PID */
#define USB_SERIAL_STR       "RFTOOL00001"

/* ========================================================================
 * DMA Channel Assignments
 * ======================================================================== */
/* SPI1 RX: DMA2 Stream 0 Channel 3 */
#define SPI1_RX_DMA_STREAM   0U
#define SPI1_RX_DMA_CHANNEL  3U
/* SPI1 TX: DMA2 Stream 3 Channel 3 */
#define SPI1_TX_DMA_STREAM   3U
#define SPI1_TX_DMA_CHANNEL  3U
/* SPI2 RX: DMA1 Stream 4 Channel 1 (note: DMA1 for SPI2 on STM32F4) */
/* Actually SPI2_RX is on DMA1 Stream 3 Channel 0 */
#define SPI2_RX_DMA_STREAM   3U
#define SPI2_RX_DMA_CHANNEL  0U
/* SPI2 TX: DMA1 Stream 5 Channel 0 */
#define SPI2_TX_DMA_STREAM   5U
#define SPI2_TX_DMA_CHANNEL  0U

#define DMA1_BASE            0x40026000U
#define DMA2_BASE            0x40026400U

/* ========================================================================
 * Application Configuration
 * ======================================================================== */
#define FIRMWARE_VERSION      "1.0.0"
#define DEFAULT_SUBGHZ_FREQ   868000000U  /* 868 MHz default (EU) */
#define DEFAULT_NRF24_CHANNEL 76U          /* Channel 76 (2476 MHz) */
#define MAX_PACKET_SIZE       64U
#define USB_RX_BUFFER_SIZE    256U
#define USB_TX_BUFFER_SIZE    256U

/* ========================================================================
 * GPIO Register Access Macros
 * ======================================================================== */
#define GPIOx_MODER(b)       (*(volatile uint32_t *)((b) + 0x00U))
#define GPIOx_OTYPER(b)      (*(volatile uint32_t *)((b) + 0x04U))
#define GPIOx_OSPEEDR(b)     (*(volatile uint32_t *)((b) + 0x08U))
#define GPIOx_PUPDR(b)       (*(volatile uint32_t *)((b) + 0x0CU))
#define GPIOx_IDR(b)         (*(volatile uint32_t *)((b) + 0x10U))
#define GPIOx_ODR(b)         (*(volatile uint32_t *)((b) + 0x14U))
#define GPIOx_BSRR(b)        (*(volatile uint32_t *)((b) + 0x18U))
#define GPIOx_LCKR(b)        (*(volatile uint32_t *)((b) + 0x1CU))
#define GPIOx_AFRL(b)        (*(volatile uint32_t *)((b) + 0x20U))
#define GPIOx_AFRH(b)        (*(volatile uint32_t *)((b) + 0x24U))

/* SPI Register Access Macros */
#define SPIx_CR1(b)          (*(volatile uint32_t *)((b) + 0x00U))
#define SPIx_CR2(b)          (*(volatile uint32_t *)((b) + 0x04U))
#define SPIx_SR(b)           (*(volatile uint32_t *)((b) + 0x08U))
#define SPIx_DR(b)           (*(volatile uint32_t *)((b) + 0x0CU))

/* I2C Register Access Macros */
#define I2Cx_CR1(b)          (*(volatile uint32_t *)((b) + 0x00U))
#define I2Cx_CR2(b)          (*(volatile uint32_t *)((b) + 0x04U))
#define I2Cx_SR1(b)          (*(volatile uint32_t *)((b) + 0x14U))
#define I2Cx_SR2(b)          (*(volatile uint32_t *)((b) + 0x18U))
#define I2Cx_CCR(b)          (*(volatile uint32_t *)((b) + 0x1CU))
#define I2Cx_TRISE(b)        (*(volatile uint32_t *)((b) + 0x20U))
#define I2Cx_DR(b)           (*(volatile uint32_t *)((b) + 0x10U))

/* RCC Base */
#define RCC_BASE             0x40023800U
#define RCC_CR               (*(volatile uint32_t *)(RCC_BASE + 0x00U))
#define RCC_PLLCFGR          (*(volatile uint32_t *)(RCC_BASE + 0x04U))
#define RCC_CFGR             (*(volatile uint32_t *)(RCC_BASE + 0x08U))
#define RCC_AHB1ENR          (*(volatile uint32_t *)(RCC_BASE + 0x30U))
#define RCC_APB1ENR          (*(volatile uint32_t *)(RCC_BASE + 0x40U))
#define RCC_APB2ENR          (*(volatile uint32_t *)(RCC_BASE + 0x44U))

/* SPI Base Addresses */
#define SPI1_BASE            0x40013000U
#define SPI2_BASE            0x40003800U

/* I2C Base Addresses */
#define I2C1_BASE            0x40005400U

/* Peripheral Base */
#define PERIPH_BASE          0x40000000U
#define APB1PERIPH_BASE      PERIPH_BASE
#define APB2PERIPH_BASE      (PERIPH_BASE + 0x00010000U)
#define AHB1PERIPH_BASE      (PERIPH_BASE + 0x00020000U)

/* RCC Bit Definitions */
#define RCC_CR_HSEON         (1U << 16)
#define RCC_CR_HSERDY        (1U << 17)
#define RCC_CR_PLLON         (1U << 24)
#define RCC_CR_PLLRDY        (1U << 25)
#define RCC_AHB1ENR_GPIOAEN  (1U << 0)
#define RCC_AHB1ENR_GPIOBEN  (1U << 1)
#define RCC_AHB1ENR_GPIOCEN  (1U << 2)
#define RCC_AHB1ENR_DMA1EN   (1U << 21)
#define RCC_AHB1ENR_DMA2EN   (1U << 22)
#define RCC_APB1ENR_SPI2EN   (1U << 14)
#define RCC_APB1ENR_I2C1EN   (1U << 21)
#define RCC_APB1ENR_PWREN    (1U << 28)
#define RCC_APB2ENR_SPI1EN   (1U << 12)
#define RCC_APB2ENR_SYSCFGEN (1U << 14)

/* SPI CR1 Bit Definitions */
#define SPI_CR1_CPHA         (1U << 0)
#define SPI_CR1_CPOL         (1U << 1)
#define SPI_CR1_MSTR         (1U << 2)
#define SPI_CR1_BR_SHIFT     3U
#define SPI_CR1_BR_MASK      (7U << 3)
#define SPI_CR1_SPE          (1U << 6)
#define SPI_CR1_LSBFIRST     (1U << 7)
#define SPI_CR1_SSI          (1U << 8)
#define SPI_CR1_SSM          (1U << 9)
#define SPI_CR1_RXONLY       (1U << 10)
#define SPI_CR1_DFF          (1U << 11)
#define SPI_CR1_BIDIMODE     (1U << 15)

/* SPI SR Bit Definitions */
#define SPI_SR_RXNE          (1U << 0)
#define SPI_SR_TXE           (1U << 1)
#define SPI_SR_BSY           (1U << 7)

/* I2C CR1 Bit Definitions */
#define I2C_CR1_PE           (1U << 0)
#define I2C_CR1_ACK          (1U << 10)
#define I2C_CR1_START        (1U << 8)
#define I2C_CR1_STOP         (1U << 9)

/* I2C SR1 Bit Definitions */
#define I2C_SR1_SB           (1U << 0)
#define I2C_SR1_ADDR         (1U << 1)
#define I2C_SR1_TXE          (1U << 7)
#define I2C_SR1_RXNE         (1U << 6)
#define I2C_SR1_BTF          (1U << 2)

/* PWR Registers */
#define PWR_BASE             0x40007000U
#define PWR_CR                (*(volatile uint32_t *)(PWR_BASE + 0x00U))
#define PWR_CSR               (*(volatile uint32_t *)(PWR_BASE + 0x04U))
#define PWR_CR_VOS_MASK      (3U << 14)
#define PWR_CR_VOS_SCALE_2   (2U << 14)
#define PWR_CSR_VOSRDY       (1U << 14)

/* Flash Registers */
#define FLASH_BASE           0x40023C00U
#define FLASH_ACR            (*(volatile uint32_t *)(FLASH_BASE + 0x00U))
#define FLASH_ACR_ICEN       (1U << 9)
#define FLASH_ACR_DCEN       (1U << 10)
#define FLASH_ACR_PRFTEN     (1U << 8)

#endif /* BOARD_H */