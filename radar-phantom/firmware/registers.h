/*
 * registers.h — STM32H743 register base addresses & bit definitions
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Minimal set of register definitions for the RadarPhantom firmware.
 * We deliberately avoid the bloated CMSIS device headers and hand-
 * define only what we touch. Bare-metal, no HAL.
 */
#ifndef RADARPHANTOM_REGISTERS_H
#define RADARPHANTOM_REGISTERS_H

#include <stdint.h>

/* ---- Cortex-M7 core ------------------------------------------------ */
#define SCB_BASE            0xE000ED00u
#define SCB_VTOR            (*(volatile uint32_t *)(SCB_BASE + 0x08))
#define SCB_CPACR          (*(volatile uint32_t *)(SCB_BASE + 0x88))
#define SCB_CCR             (*(volatile uint32_t *)(SCB_BASE + 0x14))
#define SYSTICK_BASE        0xE000E010u
#define SYST_CSR            (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYST_RVR           (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYST_CVR           (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))
#define NVIC_ISER0          (*(volatile uint32_t *)(0xE000E100u))
#define NVIC_ICPR0          (*(volatile uint32_t *)(0xE000E280u))
#define NVIC_IPR_BASE       (0xE000E400u)

/* ---- RCC ----------------------------------------------------------- */
#define RCC_BASE            0x58024400u
#define RCC_CR              (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_CFGR            (*(volatile uint32_t *)(RCC_BASE + 0x10))
#define RCC_PLL1CFGR        (*(volatile uint32_t *)(RCC_BASE + 0x20))
#define RCC_PLL2CFGR        (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_CRRC            (*(volatile uint32_t *)(RCC_BASE + 0xC0))
#define RCC_APB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0xE0))
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0xF0))
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0xD0))
#define RCC_AHB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0xD4))
#define RCC_AHB3ENR         (*(volatile uint32_t *)(RCC_BASE + 0xD8))
#define RCC_AHB4ENR         (*(volatile uint32_t *)(RCC_BASE + 0xDC))
#define RCC_APB3ENR         (*(volatile uint32_t *)(RCC_BASE + 0xE4))
#define RCC_APB4ENR         (*(volatile uint32_t *)(RCC_BASE + 0xF8))

/* RCC bit helpers */
#define RCC_CR_HSION        (1u << 0)
#define RCC_CR_HSEON        (1u << 16)
#define RCC_CR_HSERDY       (1u << 17)
#define RCC_CR_PLL1ON       (1u << 24)
#define RCC_CR_PLL1RDY      (1u << 25)
#define RCC_CFGR_SW         (0x7u << 0)
#define RCC_CFGR_SW_PLL1    (3u << 0)

#define RCC_AHB1ENR_DMA1    (1u << 0)
#define RCC_AHB1ENR_DMA2    (1u << 1)
#define RCC_AHB2ENR_GPIOA   (1u << 0)
#define RCC_AHB4ENR_GPIOB   (1u << 1)
#define RCC_AHB4ENR_GPIOC   (1u << 2)
#define RCC_AHB4ENR_GPIOD   (1u << 3)
#define RCC_AHB4ENR_GPIOE   (1u << 4)
#define RCC_AHB4ENR_GPIOF   (1u << 5)
#define RCC_AHB4ENR_GPIOG   (1u << 6)
#define RCC_APB1ENR_SPI2     (1u << 14)
#define RCC_APB1ENR_SPI3     (1u << 15)
#define RCC_APB1ENR_USART2  (1u << 17)
#define RCC_APB1ENR_USART3  (1u << 18)
#define RCC_APB2ENR_SPI1    (1u << 12)
#define RCC_APB2ENR_SPI4    (1u << 13)
#define RCC_APB2ENR_ADC1    (1u << 8)
#define RCC_AHB3ENR_ADC3    (1u << 24)
#define RCC_AHB3ENR_SDMMC1  (1u << 16)
/* Note: ADC3 clock on H7 is on AHBS1/AHB3 — placeholder */

/* ---- PWR ----------------------------------------------------------- */
#define PWR_BASE            0x58024800u
#define PWR_CR1             (*(volatile uint32_t *)(PWR_BASE + 0x00))
#define PWR_CR3            (*(volatile uint32_t *)(PWR_BASE + 0x08))
#define PWR_SR1             (*(volatile uint32_t *)(PWR_BASE + 0x10))
#define PWR_CPUCR           (*(volatile uint32_t *)(PWR_BASE + 0x50))

/* ---- GPIO ---------------------------------------------------------- */
#define GPIOA_BASE          0x58020000u
#define GPIOB_BASE          0x58020400u
#define GPIOC_BASE          0x58020800u
#define GPIOD_BASE          0x58020C00u
#define GPIOE_BASE          0x58021000u
#define GPIOF_BASE          0x58021400u
#define GPIOG_BASE          0x58021800u
#define GPIOH_BASE          0x58021C00u

/* GPIO register offsets */
#define GPIO_MODER_OFF      0x00
#define GPIO_OTYPER_OFF     0x04
#define GPIO_OSPEEDR_OFF    0x08
#define GPIO_PUPDR_OFF      0x0C
#define GPIO_IDR_OFF        0x10
#define GPIO_ODR_OFF        0x14
#define GPIO_BSRR_OFF       0x18
#define GPIO_AFRL_OFF       0x20
#define GPIO_AFRH_OFF       0x24

#define GPIO_MODER(b)       (*(volatile uint32_t *)((b) + GPIO_MODER_OFF))
#define GPIO_OTYPER(b)      (*(volatile uint32_t *)((b) + GPIO_OTYPER_OFF))
#define GPIO_OSPEEDR(b)     (*(volatile uint32_t *)((b) + GPIO_OSPEEDR_OFF))
#define GPIO_PUPDR(b)       (*(volatile uint32_t *)((b) + GPIO_PUPDR_OFF))
#define GPIO_IDR(b)         (*(volatile uint32_t *)((b) + GPIO_IDR_OFF))
#define GPIO_ODR(b)         (*(volatile uint32_t *)((b) + GPIO_ODR_OFF))
#define GPIO_BSRR(b)        (*(volatile uint32_t *)((b) + GPIO_BSRR_OFF))
#define GPIO_AFRL(b)        (*(volatile uint32_t *)((b) + GPIO_AFRL_OFF))
#define GPIO_AFRH(b)        (*(volatile uint32_t *)((b) + GPIO_AFRH_OFF))

/* GPIO pin helpers */
#define GPIO_PIN(n)         (1u << (n))
#define GPIO_BS(n)          (1u << (n))
#define GPIO_BR(n)          (1u << ((n) + 16))
#define GPIO_MODE_OUT       1u
#define GPIO_MODE_AF        2u
#define GPIO_MODE_AN        3u
#define GPIO_PUPD_NONE      0u
#define GPIO_PUPD_PU        1u
#define GPIO_PUPD_PD        2u
#define GPIO_AF5_SPI1       5u
#define GPIO_AF5_SPI2       5u
#define GPIO_AF6_SPI3       6u
#define GPIO_AF7_USART2     7u
#define GPIO_AF7_USART3     7u

/* ---- SPI ----------------------------------------------------------- */
#define SPI1_BASE           0x40013000u
#define SPI2_BASE           0x40003800u
#define SPI3_BASE           0x40003C00u
#define SPI4_BASE           0x40015000u

#define SPI_CR1(b)          (*(volatile uint32_t *)((b) + 0x00))
#define SPI_CR2(b)          (*(volatile uint32_t *)((b) + 0x04))
#define SPI_SR(b)           (*(volatile uint32_t *)((b) + 0x08))
#define SPI_DR(b)           (*(volatile uint32_t *)((b) + 0x0C))
#define SPI_CFG1(b)         (*(volatile uint32_t *)((b) + 0x10))
#define SPI_CFG2(b)         (*(volatile uint32_t *)((b) + 0x14))

#define SPI_CFG1_MBR_DIV8   (3u << 28)
#define SPI_CFG1_DSIZE8     (7u << 0)
#define SPI_CFG2_MASTER     (1u << 2)
#define SPI_CFG2_SSM        (1u << 19)
#define SPI_CFG2_SSI        (1u << 18)
#define SPI_CFG2_CPHA       (1u << 23)
#define SPI_CFG2_CPOL       (1u << 22)
#define SPI_CR1_SPE         (1u << 0)
#define SPI_CR1_CSTART      (1u << 9)
#define SPI_CR1_SSI         (1u << 12)
#define SPI_SR_RXP          (1u << 0)
#define SPI_SR_TXP          (1u << 1)
#define SPI_SR_TXC          (1u << 0)
#define SPI_SR_EOT          (1u << 3)
#define SPI_SR_OVR          (1u << 6)
#define SPI_SR_MODF         (1u << 9)

/* ---- USART --------------------------------------------------------- */
#define USART2_BASE         0x40004400u
#define USART3_BASE         0x40004800u
#define USART6_BASE         0x40011400u

#define USART_CR1(b)        (*(volatile uint32_t *)((b) + 0x00))
#define USART_CR2(b)        (*(volatile uint32_t *)((b) + 0x04))
#define USART_CR3(b)        (*(volatile uint32_t *)((b) + 0x08))
#define USART_BRR(b)        (*(volatile uint32_t *)((b) + 0x0C))
#define USART_RDR(b)        (*(volatile uint32_t *)((b) + 0x10))
#define USART_TDR(b)        (*(volatile uint32_t *)((b) + 0x14))
#define USART_ISR(b)        (*(volatile uint32_t *)((b) + 0x1C))
#define USART_ICR(b)        (*(volatile uint32_t *)((b) + 0x20))

#define USART_CR1_UE        (1u << 0)
#define USART_CR1_RE        (1u << 2)
#define USART_CR1_TE        (1u << 3)
#define USART_CR1_RXNEIE    (1u << 5)
#define USART_CR1_TCIE      (1u << 6)
#define USART_ISR_RXNE      (1u << 5)
#define USART_ISR_TXE       (1u << 7)
#define USART_ISR_TC        (1u << 6)
#define USART_ISR_ORE       (1u << 3)

/* ---- ADC ----------------------------------------------------------- */
#define ADC1_BASE           0x40022000u
#define ADC3_BASE           0x58026000u
#define ADC_CR(b)           (*(volatile uint32_t *)((b) + 0x00))
#define ADC_ISR(b)          (*(volatile uint32_t *)((b) + 0x04))
#define ADC_CR1(b)          (*(volatile uint32_t *)((b) + 0x08))
#define ADC_SQR1(b)         (*(volatile uint32_t *)((b) + 0x30))
#define ADC_SQR2(b)         (*(volatile uint32_t *)((b) + 0x34))
#define ADC_DR1(b)          (*(volatile uint32_t *)((b) + 0x40))
#define ADC_CR_ADEN         (1u << 0)
#define ADC_CR_ADSTART      (1u << 2)
#define ADC_ISR_ADRDY       (1u << 0)
#define ADC_ISR_EOC         (1u << 2)

/* ---- DMA ----------------------------------------------------------- */
#define DMA1_BASE           0x40020000u
#define DMA1_STR0_CR(b)     (*(volatile uint32_t *)((b) + 0x10))
#define DMA1_STR0_PAR(b)    (*(volatile uint32_t *)((b) + 0x18))
#define DMA1_STR0_M0AR(b)   (*(volatile uint32_t *)((b) + 0x20))
#define DMA1_STR0_NDTR(b)   (*(volatile uint32_t *)((b) + 0x28))

/* ---- Flash --------------------------------------------------------- */
#define FLASH_BASE          0x52002000u
#define FLASH_KEYR         (*(volatile uint32_t *)(FLASH_BASE + 0x08))
#define FLASH_SR           (*(volatile uint32_t *)(FLASH_BASE + 0x10))
#define FLASH_CR           (*(volatile uint32_t *)(FLASH_BASE + 0x14))
#define FLASH_AR           (*(volatile uint32_t *)(FLASH_BASE + 0x20))

/* ---- Watchdog ------------------------------------------------------ */
#define IWDG_BASE           0x58004800u
#define IWDG_KR             (*(volatile uint32_t *)(IWDG_BASE + 0x00))
#define IWDG_PR             (*(volatile uint32_t *)(IWDG_BASE + 0x04))
#define IWDG_RLR            (*(volatile uint32_t *)(IWDG_BASE + 0x08))
#define IWDG_SR             (*(volatile uint32_t *)(IWDG_BASE + 0x0C))

#endif /* RADARPHANTOM_REGISTERS_H */