/*
 * registers.h — MMIO Register Definitions for STM32F401CCU6
 *
 * Complete register map for the STM32F401CCU6 microcontroller
 * used in the RF Transceiver Tool. All base addresses, register
 * offsets, and bit definitions match the STM32F401 reference manual
 * (RM0368, Document DM00091010).
 *
 * Note: This file provides low-level register access. The board.h
 * header provides higher-level convenience macros for the specific
 * pin assignments used in this design.
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

/* ========================================================================
 * Memory Map
 * ======================================================================== */
#define PERIPH_BASE           0x40000000U
#define APB1PERIPH_BASE       PERIPH_BASE
#define APB2PERIPH_BASE       (PERIPH_BASE + 0x00010000U)
#define AHB1PERIPH_BASE       (PERIPH_BASE + 0x00020000U)
#define AHB2PERIPH_BASE       (PERIPH_BASE + 0x10000000U)
#define CORTEX_BASE           0xE000E000U

/* ========================================================================
 * System Control Block (SCB)
 * ======================================================================== */
#define SCB_BASE              (CORTEX_BASE + 0x0D00U)
#define SCB_CPUID             (*(volatile uint32_t *)(SCB_BASE + 0x00U))
#define SCB_ICSR              (*(volatile uint32_t *)(SCB_BASE + 0x04U))
#define SCB_VTOR              (*(volatile uint32_t *)(SCB_BASE + 0x08U))
#define SCB_AIRCR             (*(volatile uint32_t *)(SCB_BASE + 0x0CU))
#define SCB_SCR               (*(volatile uint32_t *)(SCB_BASE + 0x10U))
#define SCB_CCR               (*(volatile uint32_t *)(SCB_BASE + 0x14U))
#define SCB_SHPR1             (*(volatile uint32_t *)(SCB_BASE + 0x18U))
#define SCB_SHPR2             (*(volatile uint32_t *)(SCB_BASE + 0x1CU))
#define SCB_SHPR3             (*(volatile uint32_t *)(SCB_BASE + 0x20U))

/* ========================================================================
 * Nested Vectored Interrupt Controller (NVIC)
 * ======================================================================== */
#define NVIC_BASE             (CORTEX_BASE + 0x0100U)
#define NVIC_ISER0            (*(volatile uint32_t *)(NVIC_BASE + 0x00U))
#define NVIC_ISER1            (*(volatile uint32_t *)(NVIC_BASE + 0x04U))
#define NVIC_ICER0            (*(volatile uint32_t *)(NVIC_BASE + 0x80U))
#define NVIC_ICER1            (*(volatile uint32_t *)(NVIC_BASE + 0x84U))
#define NVIC_ISPR0            (*(volatile uint32_t *)(NVIC_BASE + 0x100U))
#define NVIC_ISPR1            (*(volatile uint32_t *)(NVIC_BASE + 0x104U))
#define NVIC_ICPR0            (*(volatile uint32_t *)(NVIC_BASE + 0x180U))
#define NVIC_ICPR1            (*(volatile uint32_t *)(NVIC_BASE + 0x184U))
#define NVIC_IPR_BASE         (NVIC_BASE + 0x300U)

/* ========================================================================
 * SysTick Timer
 * ======================================================================== */
#define SYSTICK_BASE          (CORTEX_BASE + 0x0010U)
#define SYSTICK_CSR            (*(volatile uint32_t *)(SYSTICK_BASE + 0x00U))
#define SYSTICK_RVR            (*(volatile uint32_t *)(SYSTICK_BASE + 0x04U))
#define SYSTICK_CVR            (*(volatile uint32_t *)(SYSTICK_BASE + 0x08U))
#define SYSTICK_CALIB          (*(volatile uint32_t *)(SYSTICK_BASE + 0x0CU))

#define SYSTICK_CSR_ENABLE     (1U << 0)
#define SYSTICK_CSR_TICKINT    (1U << 1)
#define SYSTICK_CSR_CLKSOURCE  (1U << 2)
#define SYSTICK_CSR_COUNTFLAG  (1U << 16)

/* ========================================================================
 * Reset and Clock Control (RCC)
 * ======================================================================== */
#define RCC_BASE              0x40023800U

#define RCC_CR                (*(volatile uint32_t *)(RCC_BASE + 0x00U))
#define RCC_PLLCFGR           (*(volatile uint32_t *)(RCC_BASE + 0x04U))
#define RCC_CFGR              (*(volatile uint32_t *)(RCC_BASE + 0x08U))
#define RCC_CIR               (*(volatile uint32_t *)(RCC_BASE + 0x0CU))
#define RCC_AHB1ENR           (*(volatile uint32_t *)(RCC_BASE + 0x30U))
#define RCC_AHB2ENR           (*(volatile uint32_t *)(RCC_BASE + 0x34U))
#define RCC_AHB3ENR           (*(volatile uint32_t *)(RCC_BASE + 0x38U))
#define RCC_APB1ENR           (*(volatile uint32_t *)(RCC_BASE + 0x40U))
#define RCC_APB2ENR           (*(volatile uint32_t *)(RCC_BASE + 0x44U))

/* RCC_CR bits */
#define RCC_CR_HSION          (1U << 0)
#define RCC_CR_HSIRDY         (1U << 1)
#define RCC_CR_HSEON          (1U << 16)
#define RCC_CR_HSERDY         (1U << 17)
#define RCC_CR_PLLON          (1U << 24)
#define RCC_CR_PLLRDY         (1U << 25)

/* RCC_CFGR bits */
#define RCC_CFGR_SW_MASK      (3U << 0)
#define RCC_CFGR_SW_HSI      (0U << 0)
#define RCC_CFGR_SW_HSE      (1U << 0)
#define RCC_CFGR_SW_PLL      (2U << 0)
#define RCC_CFGR_SWS_MASK     (3U << 2)
#define RCC_CFGR_SWS_PLL      (2U << 2)
#define RCC_CFGR_HPRE_MASK     (0xFU << 4)
#define RCC_CFGR_HPRE_DIV1     (0U << 4)
#define RCC_CFGR_PPRE1_MASK    (7U << 10)
#define RCC_CFGR_PPRE1_DIV2    (4U << 10)
#define RCC_CFGR_PPRE2_MASK    (7U << 13)
#define RCC_CFGR_PPRE2_DIV1    (0U << 13)

/* RCC_PLLCFGR bits */
#define RCC_PLLCFGR_PLLM_SHIFT   0U
#define RCC_PLLCFGR_PLLN_SHIFT   6U
#define RCC_PLLCFGR_PLLP_SHIFT  16U
#define RCC_PLLCFGR_PLLSRC_HSE  (1U << 22)
#define RCC_PLLCFGR_PLLQ_SHIFT  24U

/* RCC_AHB1ENR bits */
#define RCC_AHB1ENR_GPIOAEN    (1U << 0)
#define RCC_AHB1ENR_GPIOBEN    (1U << 1)
#define RCC_AHB1ENR_GPIOCEN    (1U << 2)
#define RCC_AHB1ENR_GPIODEN    (1U << 3)
#define RCC_AHB1ENR_CRCEN      (1U << 12)
#define RCC_AHB1ENR_DMA1EN     (1U << 21)
#define RCC_AHB1ENR_DMA2EN     (1U << 22)

/* RCC_AHB2ENR bits */
#define RCC_AHB2ENR_OTGFSEN    (1U << 7)

/* RCC_APB1ENR bits */
#define RCC_APB1ENR_SPI2EN     (1U << 14)
#define RCC_APB1ENR_I2C1EN     (1U << 21)
#define RCC_APB1ENR_USART2EN   (1U << 17)
#define RCC_APB1ENR_PWREN      (1U << 28)

/* RCC_APB2ENR bits */
#define RCC_APB2ENR_SPI1EN     (1U << 12)
#define RCC_APB2ENR_SYSCFGEN   (1U << 14)
#define RCC_APB2ENR_USART1EN   (1U << 4)

/* ========================================================================
 * GPIO Registers
 * ======================================================================== */
#define GPIOA_BASE             0x40020000U
#define GPIOB_BASE             0x40020400U
#define GPIOC_BASE             0x40020800U
#define GPIOD_BASE             0x40020C00U

#define GPIOx_MODER(b)        (*(volatile uint32_t *)((b) + 0x00U))
#define GPIOx_OTYPER(b)       (*(volatile uint32_t *)((b) + 0x04U))
#define GPIOx_OSPEEDR(b)      (*(volatile uint32_t *)((b) + 0x08U))
#define GPIOx_PUPDR(b)        (*(volatile uint32_t *)((b) + 0x0CU))
#define GPIOx_IDR(b)          (*(volatile uint32_t *)((b) + 0x10U))
#define GPIOx_ODR(b)          (*(volatile uint32_t *)((b) + 0x14U))
#define GPIOx_BSRR(b)         (*(volatile uint32_t *)((b) + 0x18U))
#define GPIOx_LCKR(b)         (*(volatile uint32_t *)((b) + 0x1CU))
#define GPIOx_AFRL(b)         (*(volatile uint32_t *)((b) + 0x20U))
#define GPIOx_AFRH(b)         (*(volatile uint32_t *)((b) + 0x24U))

/* GPIO mode values */
#define GPIO_MODE_INPUT        0U
#define GPIO_MODE_OUTPUT       1U
#define GPIO_MODE_AF           2U
#define GPIO_MODE_ANALOG       3U

/* GPIO speed values */
#define GPIO_SPEED_LOW         0U
#define GPIO_SPEED_MEDIUM      1U
#define GPIO_SPEED_HIGH        2U
#define GPIO_SPEED_VERY_HIGH   3U

/* GPIO pull values */
#define GPIO_PULL_NONE         0U
#define GPIO_PULL_UP           1U
#define GPIO_PULL_DOWN         2U

/* ========================================================================
 * SPI Registers
 * ======================================================================== */
#define SPI1_BASE              0x40013000U
#define SPI2_BASE              0x40003800U

#define SPIx_CR1(b)           (*(volatile uint32_t *)((b) + 0x00U))
#define SPIx_CR2(b)           (*(volatile uint32_t *)((b) + 0x04U))
#define SPIx_SR(b)            (*(volatile uint32_t *)((b) + 0x08U))
#define SPIx_DR(b)            (*(volatile uint32_t *)((b) + 0x0CU))
#define SPIx_CRCPR(b)         (*(volatile uint32_t *)((b) + 0x10U))
#define SPIx_RXCRCR(b)        (*(volatile uint32_t *)((b) + 0x14U))
#define SPIx_TXCRCR(b)        (*(volatile uint32_t *)((b) + 0x18U))

/* SPI CR1 bits */
#define SPI_CR1_CPHA          (1U << 0)
#define SPI_CR1_CPOL          (1U << 1)
#define SPI_CR1_MSTR          (1U << 2)
#define SPI_CR1_BR_SHIFT      3U
#define SPI_CR1_SPE           (1U << 6)
#define SPI_CR1_LSBFIRST      (1U << 7)
#define SPI_CR1_SSI           (1U << 8)
#define SPI_CR1_SSM           (1U << 9)
#define SPI_CR1_RXONLY        (1U << 10)
#define SPI_CR1_DFF           (1U << 11)
#define SPI_CR1_BIDIMODE      (1U << 15)

/* SPI CR2 bits */
#define SPI_CR2_RXDMAEN       (1U << 0)
#define SPI_CR2_TXDMAEN       (1U << 1)
#define SPI_CR2_SSOE          (1U << 2)
#define SPI_CR2_FRF            (1U << 4)
#define SPI_CR2_ERRIE         (1U << 5)
#define SPI_CR2_RXNEIE        (1U << 6)
#define SPI_CR2_TXEIE          (1U << 7)

/* SPI SR bits */
#define SPI_SR_RXNE           (1U << 0)
#define SPI_SR_TXE            (1U << 1)
#define SPI_SR_CHSIDE         (1U << 2)
#define SPI_SR_UDR            (1U << 3)
#define SPI_SR_CRCERR         (1U << 4)
#define SPI_SR_MODF           (1U << 5)
#define SPI_SR_OVR            (1U << 6)
#define SPI_SR_BSY            (1U << 7)
#define SPI_SR_FRE            (1U << 8)

/* ========================================================================
 * I2C Registers
 * ======================================================================== */
#define I2C1_BASE              0x40005400U
#define I2C2_BASE              0x40005800U

#define I2Cx_CR1(b)           (*(volatile uint32_t *)((b) + 0x00U))
#define I2Cx_CR2(b)           (*(volatile uint32_t *)((b) + 0x04U))
#define I2Cx_OAR1(b)          (*(volatile uint32_t *)((b) + 0x08U))
#define I2Cx_OAR2(b)          (*(volatile uint32_t *)((b) + 0x0CU))
#define I2Cx_DR(b)            (*(volatile uint32_t *)((b) + 0x10U))
#define I2Cx_SR1(b)           (*(volatile uint32_t *)((b) + 0x14U))
#define I2Cx_SR2(b)           (*(volatile uint32_t *)((b) + 0x18U))
#define I2Cx_CCR(b)           (*(volatile uint32_t *)((b) + 0x1CU))
#define I2Cx_TRISE(b)         (*(volatile uint32_t *)((b) + 0x20U))

/* I2C CR1 bits */
#define I2C_CR1_PE            (1U << 0)
#define I2C_CR1_SMBUS         (1U << 1)
#define I2C_CR1_SMBTYPE       (1U << 3)
#define I2C_CR1_ENARP         (1U << 4)
#define I2C_CR1_ENPEC         (1U << 5)
#define I2C_CR1_ENGC          (1U << 6)
#define I2C_CR1_NOSTRETCH     (1U << 7)
#define I2C_CR1_START          (1U << 8)
#define I2C_CR1_STOP           (1U << 9)
#define I2C_CR1_ACK            (1U << 10)
#define I2C_CR1_POS             (1U << 11)
#define I2C_CR1_PEC             (1U << 12)
#define I2C_CR1_ALERT           (1U << 13)
#define I2C_CR1_SWRES           (1U << 15)

/* I2C SR1 bits */
#define I2C_SR1_SB             (1U << 0)
#define I2C_SR1_ADDR           (1U << 1)
#define I2C_SR1_BTF            (1U << 2)
#define I2C_SR1_ADD10          (1U << 3)
#define I2C_SR1_STOPF          (1U << 4)
#define I2C_SR1_RXNE           (1U << 6)
#define I2C_SR1_TXE            (1U << 7)
#define I2C_SR1_BERR           (1U << 8)
#define I2C_SR1_ARLO           (1U << 9)
#define I2C_SR1_AF             (1U << 10)
#define I2C_SR1_OVR            (1U << 11)
#define I2C_SR1_PECERR         (1U << 12)
#define I2C_SR1_TIMEOUT        (1U << 14)
#define I2C_SR1_SMBALERT       (1U << 15)

/* I2C CCR bits */
#define I2C_CCR_FS             (1U << 15)
#define I2C_CCR_DUTY           (1U << 14)
#define I2C_CCR_CCR_MASK       0x0FFFU

/* ========================================================================
 * DMA Registers (DMA1 and DMA2)
 * ======================================================================== */
#define DMA1_BASE               0x40026000U
#define DMA2_BASE               0x40026400U

#define DMAx_LISR(b)           (*(volatile uint32_t *)((b) + 0x00U))
#define DMAx_HISR(b)           (*(volatile uint32_t *)((b) + 0x04U))
#define DMAx_LIFCR(b)          (*(volatile uint32_t *)((b) + 0x08U))
#define DMAx_HIFCR(b)          (*(volatile uint32_t *)((b) + 0x0CU))

/* Stream registers: base + 0x10 + 0x18*stream */
#define DMAx_SxCR(b, s)        (*(volatile uint32_t *)((b) + 0x10U + 0x18U * (s)))
#define DMAx_SxNDTR(b, s)     (*(volatile uint32_t *)((b) + 0x14U + 0x18U * (s)))
#define DMAx_SxPAR(b, s)       (*(volatile uint32_t *)((b) + 0x18U + 0x18U * (s)))
#define DMAx_SxM0AR(b, s)      (*(volatile uint32_t *)((b) + 0x1CU + 0x18U * (s)))
#define DMAx_SxM1AR(b, s)      (*(volatile uint32_t *)((b) + 0x20U + 0x18U * (s)))
#define DMAx_SxFCR(b, s)       (*(volatile uint32_t *)((b) + 0x24U + 0x18U * (s)))

/* DMA SxCR bits */
#define DMA_SxCR_EN             (1U << 0)
#define DMA_SxCR_DMEIE          (1U << 1)
#define DMA_SxCR_TEIE           (1U << 2)
#define DMA_SxCR_HTIE           (1U << 3)
#define DMA_SxCR_TCIE           (1U << 4)
#define DMA_SxCR_PFCTRL         (1U << 5)
#define DMA_SxCR_DIR_SHIFT      6U
#define DMA_SxCR_CIRC           (1U << 8)
#define DMA_SxCR_PINC           (1U << 9)
#define DMA_SxCR_MINC           (1U << 10)
#define DMA_SxCR_PSIZE_SHIFT    11U
#define DMA_SxCR_MSIZE_SHIFT    13U
#define DMA_SxCR_CHSEL_SHIFT    25U

/* DMA SxFCR bits */
#define DMA_SxFCR_FTH_SHIFT    0U
#define DMA_SxFCR_DMDIS         (1U << 2)
#define DMA_SxFCR_FS_MASK       (0x07U << 3)
#define DMA_SxFCR_FEIE          (1U << 7)

/* DMA transfer sizes */
#define DMA_PSIZE_BYTE          (0U << 11U)
#define DMA_PSIZE_HALFWORD      (1U << 11U)
#define DMA_PSIZE_WORD          (2U << 11U)
#define DMA_MSIZE_BYTE          (0U << 13U)
#define DMA_MSIZE_HALFWORD      (1U << 13U)
#define DMA_MSIZE_WORD          (2U << 13U)

/* DMA direction */
#define DMA_DIR_PERIPH_TO_MEM   (0U << 6U)
#define DMA_DIR_MEM_TO_PERIPH   (1U << 6U)
#define DMA_DIR_MEM_TO_MEM      (2U << 6U)

/* ========================================================================
 * USB OTG FS Registers (selected)
 * ======================================================================== */
#define OTG_FS_BASE             0x50000000U
#define OTG_FS_GOTGCTL          (*(volatile uint32_t *)(OTG_FS_BASE + 0x000U))
#define OTG_FS_GOTGINT          (*(volatile uint32_t *)(OTG_FS_BASE + 0x004U))
#define OTG_FS_GAHBCFG          (*(volatile uint32_t *)(OTG_FS_BASE + 0x008U))
#define OTG_FS_GUSBCFG          (*(volatile uint32_t *)(OTG_FS_BASE + 0x00CU))
#define OTG_FS_GRSTCTL          (*(volatile uint32_t *)(OTG_FS_BASE + 0x010U))
#define OTG_FS_GINTSTS          (*(volatile uint32_t *)(OTG_FS_BASE + 0x014U))
#define OTG_FS_GINTMSK          (*(volatile uint32_t *)(OTG_FS_BASE + 0x018U))
#define OTG_FS_GRXSTSR          (*(volatile uint32_t *)(OTG_FS_BASE + 0x01CU))
#define OTG_FS_GRXSTSP          (*(volatile uint32_t *)(OTG_FS_BASE + 0x020U))
#define OTG_FS_GRXFSIZ          (*(volatile uint32_t *)(OTG_FS_BASE + 0x024U))
#define OTG_FS_GNPTXFSIZ        (*(volatile uint32_t *)(OTG_FS_BASE + 0x028U))

/* Device mode registers */
#define OTG_FS_DCFG             (*(volatile uint32_t *)(OTG_FS_BASE + 0x800U + 0x04U))
#define OTG_FS_DCTL             (*(volatile uint32_t *)(OTG_FS_BASE + 0x800U + 0x08U))
#define OTG_FS_DSTS             (*(volatile uint32_t *)(OTG_FS_BASE + 0x800U + 0x0CU))
#define OTG_FS_DIEPMSK          (*(volatile uint32_t *)(OTG_FS_BASE + 0x800U + 0x10U))
#define OTG_FS_DOEPMSK          (*(volatile uint32_t *)(OTG_FS_BASE + 0x800U + 0x14U))
#define OTG_FS_DAINT            (*(volatile uint32_t *)(OTG_FS_BASE + 0x800U + 0x18U))
#define OTG_FS_DAINTMSK         (*(volatile uint32_t *)(OTG_FS_BASE + 0x800U + 0x1CU))

/* ========================================================================
 * Flash Registers
 * ======================================================================== */
#define FLASH_BASE              0x40023C00U
#define FLASH_ACR               (*(volatile uint32_t *)(FLASH_BASE + 0x00U))
#define FLASH_KEYR              (*(volatile uint32_t *)(FLASH_BASE + 0x04U))
#define FLASH_OPTKEYR           (*(volatile uint32_t *)(FLASH_BASE + 0x08U))
#define FLASH_SR                (*(volatile uint32_t *)(FLASH_BASE + 0x0CU))
#define FLASH_CR                (*(volatile uint32_t *)(FLASH_BASE + 0x10U))
#define FLASH_OPTCR             (*(volatile uint32_t *)(FLASH_BASE + 0x14U))

/* FLASH_ACR bits */
#define FLASH_ACR_LATENCY_SHIFT  0U
#define FLASH_ACR_PRFTEN         (1U << 8)
#define FLASH_ACR_ICEN           (1U << 9)
#define FLASH_ACR_DCEN           (1U << 10)

/* ========================================================================
 * PWR Registers
 * ======================================================================== */
#define PWR_BASE                0x40007000U
#define PWR_CR                  (*(volatile uint32_t *)(PWR_BASE + 0x00U))
#define PWR_CSR                 (*(volatile uint32_t *)(PWR_BASE + 0x04U))

#define PWR_CR_VOS_MASK         (3U << 14)
#define PWR_CR_VOS_SCALE_2      (2U << 14)
#define PWR_CSR_VOSRDY          (1U << 14)

/* ========================================================================
 * CRC Register
 * ======================================================================== */
#define CRC_BASE                0x40023000U
#define CRC_DR                   (*(volatile uint32_t *)(CRC_BASE + 0x00U))
#define CRC_IDR                  (*(volatile uint32_t *)(CRC_BASE + 0x04U))
#define CRC_CR                   (*(volatile uint32_t *)(CRC_BASE + 0x08U))
#define CRC_CR_RESET             (1U << 0)

/* ========================================================================
 * CC1101 Register Addresses (SPI Command/Address format)
 * ======================================================================== */
/* Write: 0x00 + addr, Read: 0x80 + addr, Burst: 0x40 OR'd */
#define CC1101_IOCFG2           0x00U
#define CC1101_IOCFG1           0x01U
#define CC1101_IOCFG0           0x02U
#define CC1101_FIFOTHR          0x03U
#define CC1101_SYNC1            0x04U
#define CC1101_SYNC0            0x05U
#define CC1101_PKTLEN           0x06U
#define CC1101_PKTCTRL1         0x07U
#define CC1101_PKTCTRL0         0x08U
#define CC1101_ADDR             0x09U
#define CC1101_CHANNR           0x0AU
#define CC1101_FSCTRL1          0x0BU
#define CC1101_FSCTRL0          0x0CU
#define CC1101_FREQ2            0x0DU
#define CC1101_FREQ1            0x0EU
#define CC1101_FREQ0            0x0FU
#define CC1101_MDMCFG4          0x10U
#define CC1101_MDMCFG3          0x11U
#define CC1101_MDMCFG2          0x12U
#define CC1101_MDMCFG1          0x13U
#define CC1101_MDMCFG0          0x14U
#define CC1101_DEVIATN          0x15U
#define CC1101_MCSM2            0x16U
#define CC1101_MCSM1            0x17U
#define CC1101_MCSM0            0x18U
#define CC1101_FREND1           0x19U
#define CC1101_FREND0           0x1AU
#define CC1101_FSCAL3           0x1BU
#define CC1101_FSCAL2           0x1CU
#define CC1101_FSCAL1           0x1DU
#define CC1101_FSCAL0           0x1EU
#define CC1101_TEST2            0x1FU
#define CC1101_TEST1            0x20U
#define CC1101_TEST0            0x21U
#define CC1101_PATABLE          0x3EU

/* CC1101 Command Strobes */
#define CC1101_SRES             0x30U   /* Reset chip */
#define CC1101_SFSTXON          0x31U   /* Enable/calibrate freq synth */
#define CC1101_SXOFF            0x32U   /* Turn off crystal oscillator */
#define CC1101_SCAL             0x33U   /* Calibrate freq synth and PLL */
#define CC1101_SRX              0x34U   /* Enable RX */
#define CC1101_STX             0x35U   /* Enable TX */
#define CC1101_SIDLE            0x36U   /* Exit RX/TX, go to IDLE */
#define CC1101_SWOR             0x38U   /* Start WOR timer */
#define CC1101_SPWD             0x39U   /* Enter power down mode */
#define CC1101_SFRX             0x3AU   /* Flush RX FIFO */
#define CC1101_SFTX             0x3BU   /* Flush TX FIFO */
#define CC1101_SWORRST          0x3CU   /* Reset WOR timer */
#define CC1101_SNOP             0x3DU   /* No operation */

/* CC1101 Status Register (when burst bit OR'd: 0xC0 + addr) */
#define CC1101_PARTNUM          0x30U   /* Read: 0xF0 */
#define CC1101_VERSION          0x31U   /* Read: 0xF1 */
#define CC1101_FREQEST          0x32U   /* Read: 0xF2 */
#define CC1101_LQI              0x33U   /* Read: 0xF3 */
#define CC1101_RSSI             0x34U   /* Read: 0xF4 */
#define CC1101_MARCSTATE        0x35U   /* Read: 0xF5 */
#define CC1101_PKTSTATUS        0x36U   /* Read: 0xF6 */
#define CC1101_VCO_VC_DAC      0x37U   /* Read: 0xF7 */

/* CC1101 SPI access helpers */
#define CC1101_WRITE_BURST      0x40U
#define CC1101_READ_SINGLE      0x80U
#define CC1101_READ_BURST       0xC0U

/* ========================================================================
 * nRF24L01+ Register Addresses
 * ======================================================================== */
#define NRF24_CONFIG            0x00U
#define NRF24_EN_AA             0x01U
#define NRF24_EN_RXADDR         0x02U
#define NRF24_SETUP_AW          0x03U
#define NRF24_SETUP_RETR        0x04U
#define NRF24_RF_CH             0x05U
#define NRF24_RF_SETUP          0x06U
#define NRF24_STATUS            0x07U
#define NRF24_OBSERVE_TX        0x08U
#define NRF24_RPD               0x09U
#define NRF24_RX_ADDR_P0        0x0AU
#define NRF24_RX_ADDR_P1        0x0BU
#define NRF24_RX_ADDR_P2        0x0CU
#define NRF24_RX_ADDR_P3        0x0DU
#define NRF24_RX_ADDR_P4        0x0EU
#define NRF24_RX_ADDR_P5        0x0FU
#define NRF24_TX_ADDR            0x10U
#define NRF24_RX_PW_P0          0x11U
#define NRF24_RX_PW_P1          0x12U
#define NRF24_RX_PW_P2          0x13U
#define NRF24_RX_PW_P3          0x14U
#define NRF24_RX_PW_P4          0x15U
#define NRF24_RX_PW_P5          0x16U
#define NRF24_FIFO_STATUS       0x17U
#define NRF24_DYNPD              0x1CU
#define NRF24_FEATURE            0x1DU

/* nRF24L01+ SPI Commands */
#define NRF24_CMD_R_REGISTER     0x00U
#define NRF24_CMD_W_REGISTER    0x20U
#define NRF24_CMD_R_RX_PAYLOAD  0x61U
#define NRF24_CMD_W_TX_PAYLOAD  0xA0U
#define NRF24_CMD_FLUSH_TX      0xE1U
#define NRF24_CMD_FLUSH_RX      0xE2U
#define NRF24_CMD_REUSE_TX_PL   0xE3U
#define NRF24_CMD_NOP           0xFFU

/* nRF24 CONFIG bits */
#define NRF24_CONFIG_PRIM_RX    (1U << 0)
#define NRF24_CONFIG_PWR_UP     (1U << 1)
#define NRF24_CONFIG_CRCO       (1U << 2)
#define NRF24_CONFIG_EN_CRC     (1U << 3)
#define NRF24_CONFIG_MASK_MAX_RT (1U << 4)
#define NRF24_CONFIG_MASK_TX_DS  (1U << 5)
#define NRF24_CONFIG_MASK_RX_DR  (1U << 6)

/* nRF24 STATUS bits */
#define NRF24_STATUS_TX_DS      (1U << 5)
#define NRF24_STATUS_MAX_RT     (1U << 4)
#define NRF24_STATUS_RX_DR     (1U << 6)

/* nRF24 RF_SETUP bits */
#define NRF24_RF_SETUP_PWR_SHIFT  1U
#define NRF24_RF_SETUP_DR_SHIFT   3U
#define NRF24_RF_SETUP_DR_250K    (1U << 5)
#define NRF24_RF_SETUP_DR_1M      (0U << 3)
#define NRF24_RF_SETUP_DR_2M      (1U << 3)

#endif /* REGISTERS_H */