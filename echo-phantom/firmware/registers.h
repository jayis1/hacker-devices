/*
 * registers.h — STM32H743 Register Definitions for ECHO-Phantom
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Minimal register definitions for the STM32H743VIT6 MCU used in
 * ECHO-Phantom. Only the peripherals needed by the firmware are
 * defined: SAI1 (audio), SDMMC1 (SD card), FMC (SDRAM), USART1
 * (BLE), USB OTG1 (CDC), GPIO, RCC, PWR, DMA, and timers.
 *
 * This file intentionally avoids the full ST HAL — all register
 * accesses are explicit for educational transparency.
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>
#include <stddef.h>

/* ========================================================================
 *  Memory Map (STM32H743)
 * ======================================================================== */

#define PERIPH_BASE         0x40000000U
#define PERIPH_APB1_BASE    (PERIPH_BASE + 0x00000000U)
#define PERIPH_APB2_BASE    (PERIPH_BASE + 0x00010000U)
#define PERIPH_AHB1_BASE    (PERIPH_BASE + 0x00020000U)
#define PERIPH_AHB2_BASE    (PERIPH_BASE + 0x08000000U)
#define PERIPH_AHB3_BASE    (PERIPH_BASE + 0x10000000U)
#define PERIPH_AHB4_BASE    (PERIPH_BASE + 0x18000000U)

/* ========================================================================
 *  Register Access Helpers
 * ======================================================================== */

#define REG32(addr)        (*(volatile uint32_t *)(addr))
#define REG16(addr)        (*(volatile uint16_t *)(addr))
#define REG8(addr)         (*(volatile uint8_t  *)(addr))

#define BIT(n)             (1U << (n))

#define SET_BIT(reg, mask)    ((reg) |= (mask))
#define CLR_BIT(reg, mask)    ((reg) &= ~(mask))
#define RD_BIT(reg, mask)     (((reg) & (mask)) != 0)
#define MOD_REG(reg, clr, set) do { (reg) = ((reg) & ~(clr)) | (set); } while(0)

/* ========================================================================
 *  Reset and Clock Control (RCC)
 * ======================================================================== */

#define RCC_BASE        (PERIPH_AHB1_BASE + 0x1000U)

#define RCC_CR          REG32(RCC_BASE + 0x00U)
#define RCC_CFGR        REG32(RCC_BASE + 0x10U)
#define RCC_CFGR2       REG32(RCC_BASE + 0x50U)
#define RCC_D1CFGR      REG32(RCC_BASE + 0x18U)
#define RCC_D2CFGR      REG32(RCC_BASE + 0x1CU)
#define RCC_D3CFGR      REG32(RCC_BASE + 0x20U)
#define RCC_PLLCKSELR   REG32(RCC_BASE + 0x28U)
#define RCC_PLL1DIVR    REG32(RCC_BASE + 0x2CU)
#define RCC_PLL1FRACR   REG32(RCC_BASE + 0x30U)
#define RCC_PLL2DIVR    REG32(RCC_BASE + 0x38U)
#define RCC_PLL3DIVR    REG32(RCC_BASE + 0x44U)

#define RCC_AHB1ENR     REG32(RCC_BASE + 0xD8U)
#define RCC_AHB2ENR     REG32(RCC_BASE + 0xDCU)
#define RCC_AHB3ENR     REG32(RCC_BASE + 0xE0U)
#define RCC_AHB4ENR     REG32(RCC_BASE + 0xE4U)
#define RCC_APB1ENR     REG32(RCC_BASE + 0xE8U)
#define RCC_APB1LENR    REG32(RCC_BASE + 0xE8U)
#define RCC_APB1HENR    REG32(RCC_BASE + 0xECU)

/* TIM2 clock enable is in APB1LENR */
#define RCC_APB1LENR_TIM2EN  BIT(0)
#define RCC_APB2ENR     REG32(RCC_BASE + 0xF0U)
#define RCC_APB3ENR     REG32(RCC_BASE + 0xF4U)
#define RCC_APB4ENR     REG32(RCC_BASE + 0xF8U)

/* RCC bit definitions */
#define RCC_CR_HSION        BIT(0)
#define RCC_CR_HSIRDY       BIT(1)
#define RCC_CR_HSEON        BIT(2)
#define RCC_CR_HSERDY       BIT(3)
#define RCC_CR_PLL1ON       BIT(24)
#define RCC_CR_PLL1RDY      BIT(25)

#define RCC_AHB1ENR_DMA1EN  BIT(0)
#define RCC_AHB1ENR_DMA2EN  BIT(1)
#define RCC_AHB1ENR_SDMMC1EN BIT(16)
#define RCC_AHB3ENR_FMCEN    BIT(4)

#define RCC_AHB4ENR_GPIOAEN  BIT(0)
#define RCC_AHB4ENR_GPIOBEN  BIT(1)
#define RCC_AHB4ENR_GPIOCEN  BIT(2)
#define RCC_AHB4ENR_GPIODEN  BIT(3)
#define RCC_AHB4ENR_GPIOEEN  BIT(4)
#define RCC_AHB4ENR_GPIOFEN  BIT(5)
#define RCC_AHB4ENR_GPIOGEN  BIT(6)

#define RCC_APB1ENR_USART1EN BIT(0)
#define RCC_APB1ENR_TIM2EN   BIT(0)
#define RCC_APB2ENR_SAI1EN   BIT(17)
#define RCC_APB2ENR_TIM1EN   BIT(0)
#define RCC_APB2ENR_TIM8EN   BIT(1)
#define RCC_APB2ENR_SYSCFGEN BIT(0)

/* ========================================================================
 *  Power Controller (PWR)
 * ======================================================================== */

#define PWR_BASE       (PERIPH_APB1_BASE + 0x4000U)
#define PWR_CR1        REG32(PWR_BASE + 0x00U)
#define PWR_CR3        REG32(PWR_BASE + 0x08U)
#define PWR_SR1        REG32(PWR_BASE + 0x10U)

#define PWR_CR3_SCUEN  BIT(2)

/* ========================================================================
 *  GPIO
 * ======================================================================== */

#define GPIOA_BASE     (PERIPH_AHB4_BASE + 0x0000U)
#define GPIOB_BASE     (PERIPH_AHB4_BASE + 0x0400U)
#define GPIOC_BASE     (PERIPH_AHB4_BASE + 0x0800U)
#define GPIOD_BASE     (PERIPH_AHB4_BASE + 0x0C00U)
#define GPIOE_BASE     (PERIPH_AHB4_BASE + 0x1000U)
#define GPIOF_BASE     (PERIPH_AHB4_BASE + 0x1400U)
#define GPIOG_BASE     (PERIPH_AHB4_BASE + 0x1800U)

#define GPIO_MODER(b)  REG32((b) + 0x00U)
#define GPIO_OTYPER(b) REG32((b) + 0x04U)
#define GPIO_OSPEEDR(b) REG32((b) + 0x08U)
#define GPIO_PUPDR(b) REG32((b) + 0x0CU)
#define GPIO_IDR(b)   REG32((b) + 0x10U)
#define GPIO_ODR(b)   REG32((b) + 0x14U)
#define GPIO_BSRR(b)  REG32((b) + 0x18U)
#define GPIO_AFRL(b)  REG32((b) + 0x20U)
#define GPIO_AFRH(b)  REG32((b) + 0x24U)
#define GPIO_ASR(b)   REG32((b) + 0x2CU)
#define GPIO_HSLR(b)  REG32((b) + 0x30U)

/* GPIO modes */
#define GPIO_MODE_INPUT   0x0
#define GPIO_MODE_OUTPUT  0x1
#define GPIO_MODE_AF      0x2
#define GPIO_MODE_ANALOG  0x3

/* GPIO speeds */
#define GPIO_SPEED_LOW    0x0
#define GPIO_SPEED_MED    0x1
#define GPIO_SPEED_HIGH   0x2
#define GPIO_SPEED_VHIGH  0x3

/* GPIO pull */
#define GPIO_PUPD_NONE    0x0
#define GPIO_PUPD_PU      0x1
#define GPIO_PUPD_PD      0x2

/* AF numbers */
#define AF0   0
#define AF1   1
#define AF2   2
#define AF3   3
#define AF4   4
#define AF5   5
#define AF6   6
#define AF7   7
#define AF8   8
#define AF9   9
#define AF10  10
#define AF11  11
#define AF12  12
#define AF13  13

/* ========================================================================
 *  Serial Audio Interface (SAI1)
 * ======================================================================== */

#define SAI1_BASE      (PERIPH_AHB2_BASE + 0x4000U)

#define SAI1_GCR       REG32(SAI1_BASE + 0x00U)

/* Block A registers (offset 0x04) */
#define SAI1_CLKA      REG32(SAI1_BASE + 0x04U)
#define SAI1_CR1A      REG32(SAI1_BASE + 0x08U)
#define SAI1_CR2A      REG32(SAI1_BASE + 0x0CU)
#define SAI1_FRCRA     REG32(SAI1_BASE + 0x10U)
#define SAI1_SLOTA      REG32(SAI1_BASE + 0x14U)
#define SAI1_IMA       REG32(SAI1_BASE + 0x18U)
#define SAI1_SR        REG32(SAI1_BASE + 0x1CU)
#define SAI1_CLRFR     REG32(SAI1_BASE + 0x20U)
#define SAI1_DR        REG32(SAI1_BASE + 0x28U)

/* Block B registers (offset 0x40) */
#define SAI1_CLKB      REG32(SAI1_BASE + 0x44U)
#define SAI1_CR1B      REG32(SAI1_BASE + 0x48U)
#define SAI1_CR2B      REG32(SAI1_BASE + 0x4CU)
#define SAI1_FRCRB     REG32(SAI1_BASE + 0x50U)
#define SAI1_SLOTB     REG32(SAI1_BASE + 0x54U)
#define SAI1_IMB       REG32(SAI1_BASE + 0x58U)
#define SAI1_SRB       REG32(SAI1_BASE + 0x5CU)
#define SAI1_CLRFB     REG32(SAI1_BASE + 0x60U)
#define SAI1_DB        REG32(SAI1_BASE + 0x68U)

/* SAI CR1 bits */
#define SAI_CR1_EN        BIT(0)
#define SAI_CR1_MCKEN     BIT(27)
#define SAI_CR1_NODIV     BIT(19)
#define SAI_CR1_OUTDRIV   BIT(18)
#define SAI_CR1_MCKDIV_Pos 20
#define SAI_CR1_SYNCEN_Pos 12
#define SAI_CR1_CKSTR     BIT(8)
#define SAI_CR1_LSBFIRST  BIT(9)
#define SAI_CR1_DS_Pos    5
#define SAI_CR1_DS_MASK   (0xF << 5)
#define SAI_CR1_MCKSTR    BIT(28)
#define SAI_CR1_SAIEN     BIT(16)
#define SAI_CR1_FREE_OPC  BIT(14)
#define SAI_CR1_MODE_Pos  0
#define SAI_CR1_MODE_MASK 0x3

/* SAI modes */
#define SAI_MODE_MASTER_TX  0x0
#define SAI_MODE_MASTER_RX  0x1
#define SAI_MODE_SLAVE_TX   0x2
#define SAI_MODE_SLAVE_RX   0x3

/* SAI data sizes */
#define SAI_DS_8BIT    0x2
#define SAI_DS_16BIT   0x4
#define SAI_DS_24BIT   0x6
#define SAI_DS_32BIT   0x8

/* SAI CR2 bits */
#define SAI_CR2_FFLUSH   BIT(3)
#define SAI_CR2_TRIS     BIT(4)
#define SAI_CR2_MUTE     BIT(5)
#define SAI_CR2_COMP_Pos 14

/* SAI FRCR bits */
#define SAI_FRCR_FSALL_Pos 0
#define SAI_FRCR_FRL_Pos 16
#define SAI_FRCR_FSDEF    BIT(0)
#define SAI_FRCR_FSPOL    BIT(4)
#define SAI_FRCR_FSOFF    BIT(5)

/* SAI SLOTR bits */
#define SAI_SLOTR_NBSLOT_Pos 0
#define SAI_SLOTR_FBOFF_Pos 16
#define SAI_SLOTR_SLOTEN_Pos 0

/* SAI SR bits */
#define SAI_SR_FREQ       BIT(0)
#define SAI_SR_FLVL_Pos   16
#define SAI_SR_CPLD       BIT(16)
#define SAI_SR_WCKCFG     BIT(2)

/* ========================================================================
 *  DMA (DMA1 — stream 0 for SAI1_A RX, stream 1 for SAI1_B TX)
 * ======================================================================== */

#define DMA1_BASE      (PERIPH_AHB1_BASE + 0x0000U)

/* DMA stream registers (each 0x18 bytes apart) */
#define DMA_SCR(s)     REG32(DMA1_BASE + (s) * 0x18U + 0x00U)
#define DMA_SNDTR(s)   REG32(DMA1_BASE + (s) * 0x18U + 0x04U)
#define DMA_SPAR(s)    REG32(DMA1_BASE + (s) * 0x18U + 0x08U)
#define DMA_SM0AR(s)   REG32(DMA1_BASE + (s) * 0x18U + 0x0CU)
#define DMA_SM1AR(s)   REG32(DMA1_BASE + (s) * 0x18U + 0x10U)
#define DMA_SFCR(s)    REG32(DMA1_BASE + (s) * 0x18U + 0x14U)

/* DMA interrupt registers */
#define DMA_LISR       REG32(DMA1_BASE + 0x000CU)
#define DMA_HISR       REG32(DMA1_BASE + 0x004CU)
#define DMA_LIFCR      REG32(DMA1_BASE + 0x0008U)
#define DMA_HIFCR      REG32(DMA1_BASE + 0x0048U)

/* DMA SCR bits */
#define DMA_SCR_EN     BIT(0)
#define DMA_SCR_DMEIE  BIT(2)
#define DMA_SCR_TEIE   BIT(3)
#define DMA_SCR_HTIE   BIT(4)
#define DMA_SCR_TCIE   BIT(5)
#define DMA_SCR_PFCTRL BIT(1)
#define DMA_SCR_DIR_Pos 4
#define DMA_SCR_PSIZE_Pos 11
#define DMA_SCR_MSIZE_Pos 13
#define DMA_SCR_MINC  BIT(10)
#define DMA_SCR_PINC  BIT(9)
#define DMA_SCR_CIRC  BIT(8)
#define DMA_SCR_DBM   BIT(18)
#define DMA_SCR_CT     BIT(19)
#define DMA_SCR_CHSEL_Pos 25

#define DMA_SCR_CHSEL_SAI1_A 6
#define DMA_SCR_CHSEL_SAI1_B 7

/* DMA transfer direction */
#define DMA_DIR_P2M  0x2
#define DMA_DIR_M2P  0x1
#define DMA_DIR_M2M  0x0

/* ========================================================================
 *  USART1 (BLE C2 link)
 * ======================================================================== */

#define USART1_BASE    (PERIPH_APB2_BASE + 0x0000U)

#define USART_CR1      REG32(USART1_BASE + 0x00U)
#define USART_CR2      REG32(USART1_BASE + 0x04U)
#define USART_CR3      REG32(USART1_BASE + 0x08U)
#define USART_BRR      REG32(USART1_BASE + 0x0CU)
#define USART_GTPR     REG32(USART1_BASE + 0x10U)
#define USART_RTOR     REG32(USART1_BASE + 0x14U)
#define USART_RQR      REG32(USART1_BASE + 0x18U)
#define USART_ISR      REG32(USART1_BASE + 0x1CU)
#define USART_ICR      REG32(USART1_BASE + 0x20U)
#define USART_RDR      REG32(USART1_BASE + 0x24U)
#define USART_TDR      REG32(USART1_BASE + 0x28U)

#define USART_CR1_UE      BIT(0)
#define USART_CR1_RE      BIT(2)
#define USART_CR1_TE      BIT(3)
#define USART_CR1_RXNEIE  BIT(5)
#define USART_CR1_TCIE    BIT(6)
#define USART_CR1_OVER8   BIT(15)

#define USART_ISR_RXNE     BIT(5)
#define USART_ISR_TC      BIT(6)
#define USART_ISR_TXE     BIT(7)

/* ========================================================================
 *  Timer — TIM2 (for BCLK frequency measurement)
 * ======================================================================== */

#define TIM2_BASE      (PERIPH_APB1_BASE + 0x0000U)

#define TIM2_CR1       REG32(TIM2_BASE + 0x00U)
#define TIM2_CR2       REG32(TIM2_BASE + 0x04U)
#define TIM2_SMCR      REG32(TIM2_BASE + 0x08U)
#define TIM2_DIER      REG32(TIM2_BASE + 0x0CU)
#define TIM2_SR        REG32(TIM2_BASE + 0x10U)
#define TIM2_EGR       REG32(TIM2_BASE + 0x14U)
#define TIM2_CCMR1     REG32(TIM2_BASE + 0x18U)
#define TIM2_CCMR2     REG32(TIM2_BASE + 0x1CU)
#define TIM2_CCER      REG32(TIM2_BASE + 0x20U)
#define TIM2_CNT       REG32(TIM2_BASE + 0x24U)
#define TIM2_PSC       REG32(TIM2_BASE + 0x28U)
#define TIM2_ARR       REG32(TIM2_BASE + 0x2CU)
#define TIM2_CCR1      REG32(TIM2_BASE + 0x34U)
#define TIM2_CCR2      REG32(TIM2_BASE + 0x38U)

#define TIM_CR1_CEN     BIT(0)
#define TIM_CR1_UDIS    BIT(1)

#define TIM_SMCR_SMS_Pos 0
#define TIM_SMCR_SMS_ECM1 0x4  /* External Clock Mode 1 */

#define TIM_CCER_CC1E   BIT(0)

/* ========================================================================
 *  Flash Controller (for inject clip storage)
 * ======================================================================== */

#define FLASH_BASE     (PERIPH_AHB1_BASE + 0x2000U)

#define FLASH_ACR      REG32(FLASH_BASE + 0x00U)
#define FLASH_KEYR     REG32(FLASH_BASE + 0x04U)
#define FLASH_OPTKEYR  REG32(FLASH_BASE + 0x08U)
#define FLASH_CR       REG32(FLASH_BASE + 0x0CU)
#define FLASH_SR       REG32(FLASH_BASE + 0x10U)
#define FLASH_CCR      REG32(FLASH_BASE + 0x14U)

/* ========================================================================
 *  External Interrupt / Event Controller (EXTI)
 * ======================================================================== */

#define EXTI_BASE      (PERIPH_AHB1_BASE + 0x3C00U)

#define EXTI_IMR1      REG32(EXTI_BASE + 0x00U)
#define EXTI_EMR1      REG32(EXTI_BASE + 0x04U)
#define EXTI_RTSR1     REG32(EXTI_BASE + 0x08U)
#define EXTI_FTSR1     REG32(EXTI_BASE + 0x0CU)
#define EXTI_SWIER1    REG32(EXTI_BASE + 0x10U)
#define EXTI_PR1       REG32(EXTI_BASE + 0x14U)

/* ========================================================================
 *  NVIC
 * ======================================================================== */

#define SCS_BASE       0xE000E000U
#define NVIC_ISER(n)   REG32(SCS_BASE + 0x100U + (n) * 4U)
#define NVIC_ICER(n)   REG32(SCS_BASE + 0x180U + (n) * 4U)
#define NVIC_IPR(n)    REG8(SCS_BASE + 0x300U + (n))
#define NVIC_ICPR(n)   REG32(SCS_BASE + 0x240U + (n) * 4U)

/* IRQ numbers */
#define IRQ_DMA1_STR0  11   /* DMA1 stream 0 (SAI1_A RX) */
#define IRQ_DMA1_STR1  12   /* DMA1 stream 1 (SAI1_B TX) */
#define IRQ_USART1     37
#define IRQ_TIM2       28
#define IRQ_SAI1       85

/* ========================================================================
 *  SDRAM controller via FMC
 * ======================================================================== */

#define FMC_BASE       (PERIPH_AHB3_BASE + 0x4000U)

#define FMC_SDCR1      REG32(FMC_BASE + 0x40U)
#define FMC_SDCR2      REG32(FMC_BASE + 0x44U)
#define FMC_SDTR1      REG32(FMC_BASE + 0x48U)
#define FMC_SDTR2      REG32(FMC_BASE + 0x4CU)
#define FMC_SDCMR      REG32(FMC_BASE + 0x50U)
#define FMC_SDSR       REG32(FMC_BASE + 0x58U)

#define SDRAM_BASE_ADDR 0xD0000000U

/* FMC SDRAM command bits */
#define FMC_SDCMR_MODE_Pos 0
#define FMC_SDCMR_CTB2     BIT(4)
#define FMC_SDCMR_MODE_CLK 0x1
#define FMC_SDCMR_MODE_PALL 0x2
#define FMC_SDCMR_MODE_AUTO 0x3
#define FMC_SDCMR_MODE_LOAD 0x4

/* ========================================================================
 *  SDMMC1
 * ======================================================================== */

#define SDMMC1_BASE    (PERIPH_AHB2_BASE + 0x8000U)

#define SDMMC1_CLKCR   REG32(SDMMC1_BASE + 0x00U)
#define SDMMC1_DCTRL   REG32(SDMMC1_BASE + 0x2CU)
#define SDMMC1_DLENR   REG32(SDMMC1_BASE + 0x28U)
#define SDMMC1_DTIMER  REG32(SDMMC1_BASE + 0x30U)
#define SDMMC1_DLEN    REG32(SDMMC1_BASE + 0x28U)
#define SDMMC1_DCNT    REG32(SDMMC1_BASE + 0x34U)
#define SDMMC1_STA     REG32(SDMMC1_BASE + 0x34U)
#define SDMMC1_ICR     REG32(SDMMC1_BASE + 0x38U)
#define SDMMC1_MASK    REG32(SDMMC1_BASE + 0x3CU)
#define SDMMC1_FIFOCNT REG32(SDMMC1_BASE + 0x48U)
#define SDMMC1_FIFO    REG32(SDMMC1_BASE + 0x80U)

/* ========================================================================
 *  System Configuration (SYSCFG)
 * ======================================================================== */

#define SYSCFG_BASE    (PERIPH_APB2_BASE + 0x1000U)
#define SYSCFG_MEMRMP  REG32(SYSCFG_BASE + 0x00U)
#define SYSCFG_EXTICR1 REG32(SYSCFG_BASE + 0x08U)

/* ========================================================================
 *  Unique device ID and flash size
 * ======================================================================== */

#define UID_BASE       0x1FF1E800U
#define FLASHSIZE_BASE 0x1FF1E880U

#endif /* REGISTERS_H */