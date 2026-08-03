/*
 * registers.h — STM32G474 register base addresses & bit definitions
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Minimal hand-maintained register map (no CMSIS dependency) for WattPhantom.
 * Only the peripherals WattPhantom actually uses are defined.
 */
#ifndef WATTPHANTOM_REGISTERS_H
#define WATTPHANTOM_REGISTERS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Base addresses (STM32G474, Cortex-M4F) ---- */
#define RCC_BASE        0x40021000u
#define PWR_BASE        0x40007000u
#define FLASH_BASE_REG  0x40022000u
#define GPIOA_BASE      0x48000000u
#define GPIOB_BASE      0x48000400u
#define GPIOC_BASE      0x48000800u
#define GPIOD_BASE      0x48000C00u
#define GPIOE_BASE      0x48001000u
#define GPIOF_BASE      0x48001400u
#define GPIOG_BASE      0x48001800u
#define SYSCFG_BASE     0x40010000u
#define EXTI_BASE       0x40010400u
#define DMA1_BASE       0x40020000u
#define DMA2_BASE       0x40020400u
#define DMAMUX1_BASE    0x40020800u

/* APB1 peripherals */
#define I2C1_BASE       0x40005400u
#define I2C2_BASE       0x40005800u
#define I2C3_BASE       0x40007800u
#define I2C4_BASE       0x40008400u
#define SPI2_BASE       0x40003800u
#define SPI3_BASE       0x40003C00u
#define USART2_BASE     0x40004400u
#define USART3_BASE     0x40004800u
#define UART4_BASE      0x40004C00u
#define LPUART1_BASE    0x40008000u

/* APB2 peripherals */
#define USART1_BASE     0x40013800u
#define SPI1_BASE       0x40013000u
#define TIM1_BASE       0x40012C00u
#define TIM8_BASE       0x40013400u
#define ADC1_BASE       0x50000000u
#define ADC2_BASE       0x50000080u

/* UCPD (USB-C Power Delivery) peripheral */
#define UCPD1_BASE      0x4000DC00u
#define UCPD2_BASE      0x4000E000u

/* ---- RCC bit definitions ---- */
#define RCC_CR_HSION    (1u << 8)
#define RCC_CR_HSEON    (1u << 16)
#define RCC_CR_HSERDY   (1u << 17)
#define RCC_CR_PLLON    (1u << 24)
#define RCC_CR_PLLRDY   (1u << 25)

#define RCC_CFGR_SW_HSI     0u
#define RCC_CFGR_SW_HSE     1u
#define RCC_CFGR_SW_PLL     2u
#define RCC_CFGR_SW_MASK    3u
#define RCC_CFGR_SWS_SHIFT  2u

/* APB1ENR1 peripheral enables */
#define RCC_APB1ENR1_I2C1EN  (1u << 21)
#define RCC_APB1ENR1_I2C2EN  (1u << 22)
#define RCC_APB1ENR1_SPI2EN  (1u << 14)
#define RCC_APB1ENR1_USART2EN (1u << 17)
#define RCC_APB1ENR1_USART3EN (1u << 18)
#define RCC_APB1ENR1_UART4EN  (1u << 19)
#define RCC_APB1ENR1_LPUART1EN (1u << 0)
#define RCC_APB1ENR1_UCPD1EN  (1u << 23)
#define RCC_APB1ENR1_TIM2EN   (1u << 0)

/* APB2ENR peripheral enables */
#define RCC_APB2ENR_USART1EN (1u << 14)
#define RCC_APB2ENR_SPI1EN   (1u << 12)
#define RCC_APB2ENR_TIM1EN   (1u << 11)
#define RCC_APB2ENR_ADC1EN   (1u << 20)
#define RCC_APB2ENR_SYSCFGEN (1u << 0)

/* AHB1ENR */
#define RCC_AHB1ENR_DMA1EN  (1u << 0)
#define RCC_AHB1ENR_DMA2EN  (1u << 1)
#define RCC_AHB1ENR_DMAMUX1EN (1u << 2)

/* AHB2ENR */
#define RCC_AHB2ENR_GPIOAEN (1u << 0)
#define RCC_AHB2ENR_GPIOBEN (1u << 1)
#define RCC_AHB2ENR_GPIOCEN (1u << 2)
#define RCC_AHB2ENR_GPIODEN (1u << 3)
#define RCC_AHB2ENR_GPIOEEN (1u << 4)
#define RCC_AHB2ENR_GPIOFEN (1u << 5)
#define RCC_AHB2ENR_GPIOGEN (1u << 6)
#define RCC_AHB2ENR_ADC12EN (1u << 13)

/* ---- GPIO register offsets ---- */
#define GPIO_MODER_OFFSET   0x00u
#define GPIO_OTYPER_OFFSET   0x04u
#define GPIO_OSPEEDR_OFFSET  0x08u
#define GPIO_PUPDR_OFFSET    0x0Cu
#define GPIO_IDR_OFFSET      0x10u
#define GPIO_ODR_OFFSET      0x14u
#define GPIO_BSRR_OFFSET     0x18u
#define GPIO_LCKR_OFFSET     0x1Cu
#define GPIO_AFRL_OFFSET     0x20u
#define GPIO_AFRH_OFFSET     0x24u
#define GPIO_BRR_OFFSET      0x28u

#define GPIO_REG(base, off)  (*(volatile uint32_t *)((base) + (off)))

/* GPIO mode values */
#define GPIO_MODE_INPUT      0u
#define GPIO_MODE_OUTPUT     1u
#define GPIO_MODE_AF         2u
#define GPIO_MODE_ANALOG     3u

/* GPIO output type */
#define GPIO_OTYPE_PP        0u
#define GPIO_OTYPE_OD        1u

/* GPIO speed */
#define GPIO_SPEED_LOW       0u
#define GPIO_SPEED_MED       1u
#define GPIO_SPEED_HIGH      2u
#define GPIO_SPEED_VHIGH     3u

/* GPIO pull */
#define GPIO_PUPD_NONE       0u
#define GPIO_PUPD_UP         1u
#define GPIO_PUPD_DOWN       2u

/* ---- I2C register offsets ---- */
#define I2C_CR1_OFFSET       0x00u
#define I2C_CR2_OFFSET       0x04u
#define I2C_OAR1_OFFSET      0x08u
#define I2C_OAR2_OFFSET      0x0Cu
#define I2C_TIMINGR_OFFSET   0x10u
#define I2C_ISR_OFFSET       0x18u
#define I2C_ICR_OFFSET       0x1Cu
#define I2C_RXDR_OFFSET      0x24u
#define I2C_TXDR_OFFSET      0x28u

#define I2C_CR1_PE           (1u << 0)
#define I2C_CR1_TXIE         (1u << 1)
#define I2C_CR1_RXIE         (1u << 2)
#define I2C_CR1_STOPIE       (1u << 4)
#define I2C_CR1_NACKIE       (1u << 3)
#define I2C_ISR_TXE          (1u << 0)
#define I2C_ISR_RXNE         (1u << 2)
#define I2C_ISR_TC           (1u << 6)
#define I2C_ISR_STOPF        (1u << 5)
#define I2C_ISR_NACKF        (1u << 3)
#define I2C_CR2_NBYTES_SHIFT 16u
#define I2C_CR2_NACK         (1u << 15)
#define I2C_CR2_STOP         (1u << 14)
#define I2C_CR2_START        (1u << 13)
#define I2C_CR2_RD_WRN       (1u << 10)
#define I2C_CR2_SADD_SHIFT   0u

/* ---- USART register offsets ---- */
#define USART_CR1_OFFSET     0x00u
#define USART_CR2_OFFSET     0x04u
#define USART_CR3_OFFSET     0x08u
#define USART_BRR_OFFSET     0x0Cu
#define USART_ISR_OFFSET     0x1Cu
#define USART_RDR_OFFSET     0x24u
#define USART_TDR_OFFSET     0x28u

#define USART_CR1_UE         (1u << 0)
#define USART_CR1_RE         (1u << 2)
#define USART_CR1_TE         (1u << 3)
#define USART_CR1_RXNEIE     (1u << 5)
#define USART_CR1_TCIE       (1u << 6)
#define USART_ISR_RXNE       (1u << 5)
#define USART_ISR_TXE        (1u << 7)
#define USART_ISR_TC         (1u << 6)
#define USART_ISR_BUSY       (1u << 16)

/* ---- UCPD peripheral registers ---- */
#define UCPD_CFGR_OFFSET     0x00u
#define UCPD_CR_OFFSET       0x08u
#define UCPD_IMR_OFFSET      0x10u
#define UCPD_SR_OFFSET       0x14u
#define UCPD_ICR_OFFSET      0x18u
#define UCPD_TX_ORDSET_OFFSET 0x20u
#define UCPD_TX_PAYSZ_OFFSET 0x24u
#define UCPD_TX_PAYDAT_OFFSET 0x28u
#define UCPD_RX_ORDSET_OFFSET 0x30u
#define UCPD_RX_PAYSZ_OFFSET 0x34u
#define UCPD_RX_PAYDAT_OFFSET 0x38u

#define UCPD_CR_UCPDEN       (1u << 0)
#define UCPD_CR_ANAMODE      (1u << 4)
#define UCPD_CR_CCENABLE_SHIFT 8u
#define UCPD_IMR_RXMSGENDIE  (1u << 6)
#define UCPD_IMR_TXMSGDISIE  (1u << 10)
#define UCPD_IMR_TXMSGSENTIE (1u << 11)
#define UCPD_SR_RXMSGEND     (1u << 6)
#define UCPD_SR_TXMSGDISC    (1u << 10)
#define UCPD_SR_TXMSGSENT    (1u << 11)
#define UCPD_SR_RXORDDET     (1u << 5)

/* ---- FUSB302B I²C addresses ---- */
#define FUSB302B_SRC_ADDR    0x44u   /* Source port CC PHY (7-bit: 0x22) */
#define FUSB302B_SNK_ADDR    0x46u   /* Sink port CC PHY (7-bit: 0x23) */

/* FUSB302B register map */
#define FUSB302_DEVICE_ID    0x01u
#define FUSB302_SWITCHES0    0x02u
#define FUSB302_SWITCHES1    0x03u
#define FUSB302_MEASURE      0x04u
#define FUSB302_CONTROL0     0x06u
#define FUSB302_CONTROL1     0x07u
#define FUSB302_CONTROL2     0x08u
#define FUSB302_CONTROL3     0x09u
#define FUSB302_CONTROL4     0x0Au
#define FUSB302_CONTROL5     0x0Bu
#define FUSB302_POWER        0x0Cu
#define FUSB302_RESET        0x0Du
#define FUSB302_MASK         0x0Eu
#define FUSB302_MASKA        0x0Fu
#define FUSB302_MASKB        0x10u
#define FUSB302_STATUS0A     0x3Cu
#define FUSB302_STATUS1A     0x3Du
#define FUSB302_INTERRUPTA   0x3Eu
#define FUSB302_INTERRUPTB   0x3Fu
#define FUSB302_STATUS0      0x40u
#define FUSB302_STATUS1      0x41u
#define FUSB302_FIFOS        0x43u

/* FUSB302 SWITCHES0 bits */
#define FUSB302_SW0_PU_EN_CC1  (1u << 0)
#define FUSB302_SW0_PU_EN_CC2  (1u << 1)
#define FUSB302_SW0_PD_EN_CC1  (1u << 2)
#define FUSB302_SW0_PD_EN_CC2  (1u << 3)
#define FUSB302_SW0_MEAS_CC1   (1u << 4)
#define FUSB302_SW0_MEAS_CC2   (1u << 5)
#define FUSB302_SW0_AUTO_CRC   (1u << 6)

/* FUSB302 CONTROL0 bits */
#define FUSB302_CTRL0_TX_FLUSH (1u << 0)
#define FUSB302_CTRL0_RX_FLUSH (1u << 1)
#define FUSB302_CTRL0_INT_REC  (1u << 2)
#define FUSB302_CTRL0_HOST_CUR_SHIFT 4u

/* ---- INA226 I²C ---- */
#define INA226_ADDR_SRC     0x40u   /* Source port current monitor */
#define INA226_ADDR_SNK     0x41u   /* Sink port current monitor */
#define INA226_REG_CONFIG   0x00u
#define INA226_REG_SHUNT_V  0x01u
#define INA226_REG_BUS_V    0x02u
#define INA226_REG_POWER    0x03u
#define INA226_REG_CURRENT  0x04u
#define INA226_REG_CALIB    0x05u
#define INA226_REG_MASK     0x06u
#define INA226_REG_ALERT    0x07u

/* ---- TMUX2512 GPIO control ---- */
/* Controlled via GPIO, not I²C. See board.h for pin assignments. */

/* ---- SSD1306 OLED ---- */
#define SSD1306_ADDR        0x3Cu
#define SSD1306_WIDTH       128u
#define SSD1306_HEIGHT      64u

/* ---- MAX17048 fuel gauge ---- */
#define MAX17048_ADDR       0x36u
#define MAX17048_REG_VCELL  0x02u
#define MAX17048_REG_SOC    0x04u
#define MAX17048_REG_VERSION 0x08u
#define MAX17048_REG_HIBRT  0x0Au
#define MAX17048_REG_CONFIG 0x0Cu
#define MAX17048_REG_VALRT  0x1Au
#define MAX17048_REG_CRATE  0x16u

/* ---- NVIC ---- */
#define NVIC_ISER0          (*(volatile uint32_t *)0xE000E100u)
#define NVIC_ICER0          (*(volatile uint32_t *)0xE000E180u)
#define NVIC_IPR_BASE        0xE000E400u

/* ---- SysTick ---- */
#define SYST_CSR             (*(volatile uint32_t *)0xE000E010u)
#define SYST_RVR             (*(volatile uint32_t *)0xE000E014u)
#define SYST_CVR             (*(volatile uint32_t *)0xE000E018u)
#define SYSTICK_ENABLE       (1u << 0)
#define SYSTICK_TICKINT      (1u << 1)
#define SYSTICK_CLKSOURCE    (1u << 2)

/* ---- Flash wait states ---- */
#define FLASH_ACR_OFFSET     0x00u
#define FLASH_ACR_LATENCY_MASK 0x07u
#define FLASH_ACR_PRFTEN     (1u << 8)
#define FLASH_ACR_ICEN       (1u << 9)
#define FLASH_ACR_DCEN       (1u << 10)

#ifdef __cplusplus
}
#endif

#endif /* WATTPHANTOM_REGISTERS_H */