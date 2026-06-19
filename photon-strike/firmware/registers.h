/**
 * registers.h — STM32H757 register definitions used outside the HAL
 * Author: jayis1
 * License: GPL-2.0
 *
 * Only the registers the bare-metal firmware touches directly are listed.
 * Everything else comes from the CMSIS device header (not included here;
 * the Makefile pulls in stm32h757xx.h from CMSIS). This file holds the
 * PhotonStrike-specific peripheral base addresses and bit definitions
 * the drivers need without dragging in the full CMSIS layer in every
 * translation unit.
 */

#ifndef PHOTONSTRIKE_REGISTERS_H
#define PHOTONSTRIKE_REGISTERS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ─── Peripheral base addresses (STM32H7 dual-core) ──────────────────── */
#define RCC_BASE           0x58024400u
#define PWR_BASE           0x58024800u
#define FLASH_BASE         0x52002000u
#define SYSCFG_BASE        0x58000400u

#define GPIOA_BASE         0x58020000u
#define GPIOB_BASE         0x58020400u
#define GPIOC_BASE         0x58020800u
#define GPIOD_BASE         0x58020C00u
#define GPIOE_BASE         0x58021000u
#define GPIOH_BASE         0x58021C00u

#define SPI1_BASE          0x40013000u
#define SPI2_BASE          0x40003800u
#define USART3_BASE        0x40004800u
#define I2C1_BASE          0x40005400u
#define SDMMC1_BASE        0x52007000u
#define USB_OTG_BASE       0x40080000u

#define ADC1_BASE          0x40022000u
#define DAC1_BASE          0x40017000u
#define TIM1_BASE          0x40010000u
#define TIM2_BASE          0x40000000u
#define TIM6_BASE          0x40001000u
#define DMA1_BASE          0x40026000u
#define DMA2_BASE          0x40027000u

#define D2_SRAM_BASE       0x10000000u
#define D3_SRAM_BASE       0x38000000u
#define ITCM_BASE          0x00000000u
#define DTCM_BASE          0x20000000u

/* ─── GPIO register offsets ───────────────────────────────────────────── */
#define GPIO_MODER   0x00u
#define GPIO_OTYPER  0x04u
#define GPIO_OSPEEDR 0x08u
#define GPIO_PUPDR   0x0Cu
#define GPIO_IDR     0x10u
#define GPIO_ODR     0x14u
#define GPIO_BSRR    0x18u
#define GPIO_LCKR    0x1Cu
#define GPIO_AFRL    0x20u
#define GPIO_AFRH    0x24u

#define GPIO_MODE_IN    0x0u
#define GPIO_MODE_OUT   0x1u
#define GPIO_MODE_AF    0x2u
#define GPIO_MODE_AN    0x3u

#define GPIO_PUPD_NONE  0x0u
#define GPIO_PUPD_PU    0x1u
#define GPIO_PUPD_PD    0x2u

#define GPIO_SPEED_LOW  0x0u
#define GPIO_SPEED_HIGH 0x2u

/* ─── RCC enable bits (subset used by PhotonStrike) ───────────────────── */
#define RCC_AHB4ENR_GPIOAEN  (1u << 0)
#define RCC_AHB4ENR_GPIOBEN  (1u << 1)
#define RCC_AHB4ENR_GPIOCEN  (1u << 2)
#define RCC_AHB4ENR_GPIODEN  (1u << 3)
#define RCC_AHB4ENR_GPIOEEN  (1u << 4)
#define RCC_AHB4ENR_GPIOHEN  (1u << 7)

#define RCC_AHB3ENR_ADC1EN   (1u << 24)
#define RCC_AHB1ENR_DMA1EN   (1u << 0)
#define RCC_AHB1ENR_DMA2EN   (1u << 1)
#define RCC_AHB3ENR_SDMMC1EN (1u << 16)

#define RCC_APB1ENR1_TIM6EN  (1u << 4)
#define RCC_APB1ENR1_USART3EN (1u << 18)
#define RCC_APB1ENR1_I2C1EN  (1u << 21)
#define RCC_APB1ENR1_DAC1EN  (1u << 29)
#define RCC_APB2ENR_TIM1EN   (1u << 0)
#define RCC_APB2ENR_SPI1EN   (1u << 12)
#define RCC_APB2ENR_ADC1EN_  (1u << 24)

/* ─── RCC register offsets ────────────────────────────────────────────── */
#define RCC_CR         0x00u
#define RCC_PLL1CFGR   0x10u
#define RCC_AHB3ENR    0xD4u
#define RCC_AHB1ENR    0xD8u
#define RCC_AHB4ENR    0xE0u
#define RCC_APB1ENR1   0xE8u
#define RCC_APB2ENR    0xF8u

/* ─── USART register offsets / bits ───────────────────────────────────── */
#define USART_CR1      0x00u
#define USART_CR2      0x04u
#define USART_CR3      0x08u
#define USART_BRR      0x0Cu
#define USART_ISR      0x14u
#define USART_RDR      0x18u
#define USART_TDR      0x1Cu

#define USART_CR1_UE    (1u << 0)
#define USART_CR1_RE    (1u << 2)
#define USART_CR1_TE    (1u << 3)
#define USART_CR1_RXNEIE (1u << 5)
#define USART_CR1_TCIE  (1u << 6)
#define USART_ISR_RXNE  (1u << 5)
#define USART_ISR_TC    (1u << 6)
#define USART_ISR_TXE   (1u << 7)

/* ─── SPI register offsets / bits ─────────────────────────────────────── */
#define SPI_CR1        0x00u
#define SPI_CR2        0x04u
#define SPI_SR         0x08u
#define SPI_DR         0x0Cu

#define SPI_CR1_SPE     (1u << 6)
#define SPI_CR1_MSTR    (1u << 2)
#define SPI_CR1_CPHA    (1u << 0)
#define SPI_CR1_CPOL    (1u << 1)
#define SPI_CR1_SSM     (1u << 9)
#define SPI_CR1_SSI     (1u << 8)
#define SPI_CR1_LSBFIRST (1u << 7)
#define SPI_SR_RXNE     (1u << 0)
#define SPI_SR_TXE      (1u << 1)
#define SPI_SR_BSY      (1u << 7)

/* ─── I2C register offsets / bits ─────────────────────────────────────── */
#define I2C_CR1        0x00u
#define I2C_CR2        0x04u
#define I2C_ISR        0x10u
#define I2C_TXDR       0x24u
#define I2C_RXDR       0x28u
#define I2C_OAR1       0x0Cu
#define I2C_TIMINGR    0x10u
#define I2C_CR1_PE      (1u << 0)
#define I2C_CR2_START   (1u << 13)
#define I2C_CR2_STOP    (1u << 14)
#define I2C_ISR_TXE     (1u << 0)
#define I2C_ISR_RXNE    (1u << 2)
#define I2C_ISR_TC      (1u << 6)
#define I2C_ISR_NACKF   (1u << 4)

/* ─── TIM1 register offsets (laser gate PWM) ──────────────────────────── */
#define TIM_CR1        0x00u
#define TIM_CR2        0x04u
#define TIM_DIER       0x0Cu
#define TIM_SR         0x10u
#define TIM_EGR        0x14u
#define TIM_CCMR1      0x18u
#define TIM_CCER       0x20u
#define TIM_CNT        0x24u
#define TIM_PSC        0x28u
#define TIM_ARR        0x2Cu
#define TIM_CCR1       0x34u
#define TIM_BDTR       0x44u

#define TIM_CR1_CEN     (1u << 0)
#define TIM_CR1_OPM     (1u << 3)
#define TIM_BDTR_MOE    (1u << 15)

/* ─── ADC register offsets ────────────────────────────────────────────── */
#define ADC_CR         0x08u
#define ADC_CFGR       0x0Cu
#define ADC_SQR1       0x30u
#define ADC_DR         0x4Cu
#define ADC_ISR        0x00u
#define ADC_CR_ADEN    (1u << 0)
#define ADC_CR_ADCAL   (1u << 16)
#define ADC_ISR_ADRDY  (1u << 0)
#define ADC_ISR_EOC    (1u << 2)

/* ─── DAC register offsets ────────────────────────────────────────────── */
#define DAC_CR         0x00u
#define DAC_DHR12R1    0x08u
#define DAC_CR_EN1      (1u << 0)

/* ─── DMA register offsets (stream/channel) ───────────────────────────── */
#define DMA_SxCR       0x10u
#define DMA_SxNDTR     0x14u
#define DMA_SxPAR      0x18u
#define DMA_SxM0AR     0x1Cu
#define DMA_SxFCR      0x24u
#define DMA_LIFCR      0x08u
#define DMA_SxCR_EN     (1u << 0)
#define DMA_SxCR_TCIE   (1u << 4)
#define DMA_SxCR_MINC   (1u << 10)

/* ─── System / Cortex-M control ───────────────────────────────────────── */
#define SCB_BASE       0xE000ED00u
#define SCB_VTOR       0x08u
#define SCB_ICSR       0x04u
#define NVIC_ISER0     0xE000E100u
#define NVIC_ICER0     0xE000E180u

#define SYSTICK_BASE   0xE000E010u
#define SYSTICK_LOAD   0xE000E014u
#define SYSTICK_VAL    0xE000E018u
#define SYSTICK_CSR    0xE000E010u
#define SYSTICK_ENABLE (1u << 0)
#define SYSTICK_CLKSRC (1u << 2)

/* ─── D3 SRAM handshake (M7 → M4 boot) ───────────────────────────────── */
#define M4_BOOT_MAGIC  0xDEADBEEFu

/* ─── Flash key / lock (for option-byte writes if needed) ─────────────── */
#define FLASH_KEYR     0x08u
#define FLASH_SR       0x10u
#define FLASH_CR       0x14u
#define FLASH_KEY1     0x45670123u
#define FLASH_KEY2     0xCDEF89ABu

/* ─── Helper macros ───────────────────────────────────────────────────── */
#define REG32(addr)    (*(volatile uint32_t *)(addr))
#define REG16(addr)    (*(volatile uint16_t *)(addr))
#define REG8(addr)     (*(volatile uint8_t  *)(addr))
#define BIT(n)         (1u << (n))

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifdef __cplusplus
}
#endif
#endif /* PHOTONSTRIKE_REGISTERS_H */