/*
 * registers.h — STM32H563 register definitions for NVMe-Phantom firmware
 *
 * Author:  jayis1
 * License: GPL-2.0
 *
 * Minimal set of register definitions used by the bare-metal firmware.
 * The STM32H563 is a Cortex-M33 part in the STM32H5 family; its register
 * map is largely compatible with STM32H7 with some differences in the
 * RCC and USB peripherals.  Only the registers we actually touch are
 * defined here — we do not pull in the full ST HAL (keeps Flash footprint
 * small and avoids HAL licence / version churn).
 */

#ifndef NVME_PHANTOM_REGISTERS_H
#define NVME_PHANTOM_REGISTERS_H

#include <stdint.h>

/* ---- Base addresses ---------------------------------------------------- */

#define PERIPH_BASE        0x40000000UL
#define AHB1_BASE          (PERIPH_BASE + 0x00000)
#define AHB2_BASE          (PERIPH_BASE + 0x08000)
#define AHB3_BASE          (PERIPH_BASE + 0x10000)
#define APB1_BASE          (PERIPH_BASE + 0x10000)  /* legacy alias */
#define APB1_GRP1_BASE     (PERIPH_BASE + 0x10000)
#define APB1_GRP2_BASE     (PERIPH_BASE + 0x18000)
#define APB2_BASE          (PERIPH_BASE + 0x20000)
#define APB3_BASE          (PERIPH_BASE + 0x28000)

/* Cortex-M33 system peripherals */
#define SCB_BASE           0xE000ED00UL
#define SYSTICK_BASE       0xE000E010UL
#define NVIC_BASE          0xE000E100UL
#define MPU_BASE           0xE000ED90UL

/* STM32H5 peripherals we use */
#define RCC_BASE           0x44020C00UL   /* Reset & Clock Control       */
#define PWR_BASE           0x44020800UL
#define GPIOA_BASE         0x42020000UL
#define GPIOB_BASE         0x42020400UL
#define GPIOC_BASE         0x42020800UL
#define GPIOD_BASE         0x42020C00UL
#define SPI1_BASE          0x40013000UL
#define SPI3_BASE          0x40003C00UL
#define I2C1_BASE          0x40005400UL
#define I2C2_BASE          0x40005800UL
#define UART4_BASE         0x40004C00UL
#define SDMMC1_BASE        0x40012800UL
#define USB_BASE           0x40016000UL
#define ADC1_BASE          0x40022000UL
#define DMAMUX1_BASE       0x40020800UL
#define DMA1_BASE          0x40020000UL

/* ---- Register access macros -------------------------------------------- */

#define REG32(addr)        (*(volatile uint32_t *)(addr))
#define REG16(addr)        (*(volatile uint16_t *)(addr))
#define REG8(addr)         (*(volatile uint8_t  *)(addr))
#define SET_BIT(reg, bit)      do { (reg) |=  (bit); } while (0)
#define CLEAR_BIT(reg, bit)    do { (reg) &= ~(bit); } while (0)
#define READ_BIT(reg, bit)     ((reg) & (bit))
#define WAIT_WHILE(cond)       do { } while (cond)

/* ---- RCC (selected) ---------------------------------------------------- */

#define RCC_CR              REG32(RCC_BASE + 0x00)
#define RCC_CFGR            REG32(RCC_BASE + 0x10)
#define RCC_PLL1CFGR        REG32(RCC_BASE + 0x20)
#define RCC_PLL1DIVR        REG32(RCC_BASE + 0x24)
#define RCC_AHB1ENR         REG32(RCC_BASE + 0x80)
#define RCC_AHB2ENR         REG32(RCC_BASE + 0x84)
#define RCC_AHB3ENR         REG32(RCC_BASE + 0x88)
#define RCC_APB1LENR        REG32(RCC_BASE + 0x90)
#define RCC_APB1HENR        REG32(RCC_BASE + 0x94)
#define RCC_APB2ENR         REG32(RCC_BASE + 0x98)
#define RCC_APB3ENR         REG32(RCC_BASE + 0x9C)

#define RCC_CR_HSION        (1U << 0)
#define RCC_CR_HSIRDY       (1U << 1)
#define RCC_CR_HSEON        (1U << 8)
#define RCC_CR_HSERDY       (1U << 9)
#define RCC_CR_PLL1ON       (1U << 24)
#define RCC_CR_PLL1RDY      (1U << 25)

#define RCC_AHB1ENR_GPIOA   (1U << 0)
#define RCC_AHB1ENR_GPIOB   (1U << 1)
#define RCC_AHB1ENR_GPIOC   (1U << 2)
#define RCC_AHB1ENR_GPIOD   (1U << 3)
#define RCC_AHB1ENR_DMA1    (1U << 21)
#define RCC_AHB1ENR_DMAMUX1 (1U << 22)
#define RCC_AHB2ENR_ADC1    (1U << 5)
#define RCC_AHB3ENR_USB     (1U << 0)
#define RCC_APB1LENR_SPI3   (1U << 15)
#define RCC_APB1LENR_UART4  (1U << 19)
#define RCC_APB1LENR_I2C1   (1U << 21)
#define RCC_APB1LENR_I2C2   (1U << 22)
#define RCC_APB1LENR_SDMMC1 (1U << 28)
#define RCC_APB2ENR_SPI1    (1U << 12)

/* ---- GPIO (STM32H5 uses MODER, OTYPER, OSPEEDR, PUPDR, IDR, ODR, AFRL/H) */

#define GPIO_MODER_OFF      0x00
#define GPIO_OTYPER_OFF     0x04
#define GPIO_OSPEEDR_OFF    0x08
#define GPIO_PUPDR_OFF      0x0C
#define GPIO_IDR_OFF        0x10
#define GPIO_ODR_OFF        0x14
#define GPIO_AFRL_OFF       0x20
#define GPIO_AFRH_OFF       0x24
#define GPIO_SECCFGR_OFF    0x30   /* security config (TrustZone) */

#define GPIO_MODE_INPUT     0x0
#define GPIO_MODE_OUTPUT    0x1
#define GPIO_MODE_AF        0x2
#define GPIO_MODE_ANALOG    0x3
#define GPIO_SPEED_LOW      0x0
#define GPIO_SPEED_MED      0x1
#define GPIO_SPEED_HIGH     0x2
#define GPIO_SPEED_VHIGH    0x3
#define GPIO_PUPD_NONE      0x0
#define GPIO_PUPD_PULLUP    0x1
#define GPIO_PUPD_PULLDN    0x2

#define GPIOA_MODER         REG32(GPIOA_BASE + GPIO_MODER_OFF)
#define GPIOA_OTYPER        REG32(GPIOA_BASE + GPIO_OTYPER_OFF)
#define GPIOA_OSPEEDR       REG32(GPIOA_BASE + GPIO_OSPEEDR_OFF)
#define GPIOA_PUPDR         REG32(GPIOA_BASE + GPIO_PUPDR_OFF)
#define GPIOA_IDR           REG32(GPIOA_BASE + GPIO_IDR_OFF)
#define GPIOA_ODR           REG32(GPIOA_BASE + GPIO_ODR_OFF)
#define GPIOA_AFRL          REG32(GPIOA_BASE + GPIO_AFRL_OFF)
#define GPIOA_AFRH          REG32(GPIOA_BASE + GPIO_AFRH_OFF)
/* (GPIOB/C/D likewise — expanded in each driver as needed) */

/* ---- SPI (selected) ---------------------------------------------------- */

#define SPI_CR1             0x00
#define SPI_CR2             0x04
#define SPI_SR              0x08
#define SPI_DR              0x0C
#define SPI_CR1_SPE         (1U << 6)
#define SPI_CR1_MSTR        (1U << 2)
#define SPI_CR1_CPHA        (1U << 0)
#define SPI_CR1_CPOL        (1U << 1)
#define SPI_CR1_BR_DIV2     (0U << 3)
#define SPI_CR1_BR_DIV4     (1U << 3)
#define SPI_CR1_BR_DIV8     (2U << 3)
#define SPI_CR1_BR_DIV16    (3U << 3)
#define SPI_CR1_BR_DIV32    (4U << 3)
#define SPI_CR1_BR_DIV256   (6U << 3)
#define SPI_SR_RXNE         (1U << 0)
#define SPI_SR_TXE          (1U << 1)
#define SPI_SR_BSY          (1U << 7)
#define SPI_SR_MODF         (1U << 4)
#define SPI_SR_OVR          (1U << 6)

/* ---- I2C (selected, STM32H5 I2C v2 peripheral) ------------------------- */

#define I2C_CR1             0x00
#define I2C_CR2             0x04
#define I2C_OAR1            0x08
#define I2C_DR              0x18   /* simplified; real I2C v2 uses TXDR/RXDR */
#define I2C_TXDR            0x28
#define I2C_RXDR            0x24
#define I2C_SR1             0x10   /* legacy alias for ISR low bits */
#define I2C_ISR             0x14
#define I2C_ICR             0x1C
#define I2C_CR1_PE          (1U << 0)
#define I2C_CR2_START       (1U << 13)
#define I2C_CR2_STOP        (1U << 14)
#define I2C_CR2_NACK        (1U << 15)
#define I2C_ISR_BUSY        (1U << 15)
#define I2C_ISR_TXE         (1U << 0)
#define I2C_ISR_RXNE        (1U << 2)
#define I2C_ISR_TC          (1U << 6)
#define I2C_ISR_NACKF       (1U << 4)

/* ---- USART (selected) -------------------------------------------------- */

#define USART_CR1           0x00
#define USART_CR2           0x04
#define USART_CR3           0x08
#define USART_BRR           0x0C
#define USART_RDR           0x24
#define USART_TDR           0x28
#define USART_ISR           0x1C
#define USART_CR1_UE        (1U << 0)
#define USART_CR1_TE        (1U << 3)
#define USART_CR1_RE        (1U << 2)
#define USART_CR1_RXNEIE    (1U << 5)
#define USART_CR1_TCIE      (1U << 6)
#define USART_ISR_RXNE      (1U << 5)
#define USART_ISR_TXE       (1U << 7)
#define USART_ISR_TC        (1U << 6)

/* ---- SDMMC (selected) -------------------------------------------------- */

#define SDMMC_POWER         0x00
#define SDMMC_CLKCR         0x04
#define SDMMC_ARG           0x08
#define SDMMC_CMD           0x0C
#define SDMMC_RESP1         0x10
#define SDMMC_DTIMER        0x40
#define SDMMC_DLEN          0x44
#define SDMMC_DCTRL         0x48
#define SDMMC_DCOUNT        0x4C
#define SDMMC_STA           0x34
#define SDMMC_ICR           0x38
#define SDMMC_MASK          0x3C
#define SDMMC_FIFO          0x80
#define SDMMC_POWER_PWRON   0x03
#define SDMMC_STA_CMDREND   (1U << 6)
#define SDMMC_STA_DATAEND   (1U << 8)
#define SDMMC_STA_RXDAVL    (1U << 21)
#define SDMMC_STA_TXDAVL    (1U << 22)

/* ---- USB (selected, STM32H5 USB FS) ------------------------------------ */

#define USB_CNTR            0x40
#define USB_ISTR            0x44
#define USB_FNR             0x48
#define USB_DADDR           0x4C
#define USB_BTABLE          0x50
#define USB_EP0R            0x00
#define USB_EP1R            0x04
#define USB_EP2R            0x08
#define USB_EP3R            0x0C
#define USB_ISTR_RESET      (1U << 10)
#define USB_ISTR_SUSP       (1U << 11)
#define USB_ISTR_CTR        (1U << 15)
#define USB_CNTR_PDWN       (1U << 1)
#define USB_CNTR_USBPUE     (1U << 3)
#define USB_DADDR_ADD       (1U << 7)

/* ---- ADC (selected) ---------------------------------------------------- */

#define ADC_ISR             0x00
#define ADC_CR              0x08
#define ADC_CFGR            0x0C
#define ADC_SQR1            0x30
#define ADC_DR              0x40
#define ADC_CR_ADVREGEN     (1U << 28)
#define ADC_CR_ADEN         (1U << 0)
#define ADC_CR_ADSTART      (1U << 2)
#define ADC_ISR_ADRDY       (1U << 0)
#define ADC_ISR_EOC         (1U << 2)

/* ---- SysTick ----------------------------------------------------------- */

#define SYST_CSR            REG32(SYSTICK_BASE + 0x00)
#define SYST_RVR            REG32(SYSTICK_BASE + 0x04)
#define SYST_CVR            REG32(SYSTICK_BASE + 0x08)
#define SYST_CSR_ENABLE     (1U << 0)
#define SYST_CSR_TICKINT    (1U << 1)
#define SYST_CSR_CLKSOURCE  (1U << 2)

/* ---- NVIC helpers ------------------------------------------------------ */

#define NVIC_ISER0          REG32(NVIC_BASE + 0x000)
#define NVIC_ICER0          REG32(NVIC_BASE + 0x080)
#define NVIC_IPR_BASE       (NVIC_BASE + 0x300)

/* IRQ numbers used (selected) */
#define IRQ_SPI1            35
#define IRQ_SPI3            51
#define IRQ_I2C1_EV         31
#define IRQ_UART4           44
#define IRQ_SDMMC1          49
#define IRQ_USB_FS          33
#define IRQ_DMA1_CH1        11

/* ---- Timing helpers ---------------------------------------------------- */

#define BOARD_TICKS_PER_US  (BOARD_SYSCLK_HZ / 1000000UL)  /* 250 */
#define BOARD_TICKS_PER_MS  (BOARD_SYSCLK_HZ / 1000UL)     /* 250000 */

static inline void board_delay_us(volatile uint32_t us)
{
    volatile uint32_t end = us * BOARD_TICKS_PER_US;
    while (end--) { __asm volatile ("nop"); }
}

static inline void board_delay_ms(volatile uint32_t ms)
{
    while (ms--) { board_delay_us(1000); }
}

#endif /* NVME_PHANTOM_REGISTERS_H */