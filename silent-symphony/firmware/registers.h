/*
 * SILENT-SYMPHONY — Ultrasonic Covert Channel Communicator
 * STM32H743VIT6 Register Map
 *
 * Defines MMIO base addresses, register offsets, and bit field
 * definitions for all peripherals used by the device.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

/* =========================================================================
 * MMIO Write/Read helpers
 * ========================================================================= */
static inline void mmio_write32(uintptr_t addr, uint32_t val)
{
    *(volatile uint32_t *)addr = val;
}

static inline uint32_t mmio_read32(uintptr_t addr)
{
    return *(volatile uint32_t *)addr;
}

static inline void mmio_set_bits(uintptr_t addr, uint32_t bits)
{
    mmio_write32(addr, mmio_read32(addr) | bits);
}

static inline void mmio_clear_bits(uintptr_t addr, uint32_t bits)
{
    mmio_write32(addr, mmio_read32(addr) & ~bits);
}

/* =========================================================================
 * System Base Addresses (STM32H743 Reference Manual §2.3)
 * ========================================================================= */
#define FLASH_BASE              0x08000000UL
#define SRAM1_BASE              0x24000000UL
#define SRAM2_BASE              0x24080000UL
#define SRAM3_BASE              0x240C0000UL
#define SRAM4_BASE              0x38000000UL
#define QSPI_ALIAS_BASE         0x90000000UL

/* AHB3 Peripheral Bus */
#define FMC_BASE                0x52000000UL
#define QUADSPI_BASE            0x52005000UL

/* AHB1 Peripheral Bus */
#define GPIOA_BASE              0x58020000UL
#define GPIOB_BASE              0x58020400UL
#define GPIOC_BASE              0x58020800UL
#define GPIOD_BASE              0x58020C00UL
#define GPIOE_BASE              0x58021000UL
#define GPIOF_BASE              0x58021400UL
#define GPIOG_BASE              0x58021800UL
#define DMA1_BASE               0x58025400UL
#define DMA2_BASE               0x58025C00UL
#define ETH_BASE                0x58028000UL
#define USB1_OTG_BASE           0x5800C000UL

/* APB2 Peripheral Bus */
#define TIM1_BASE               0x58010000UL
#define TIM8_BASE               0x58010400UL
#define SPI1_BASE               0x58011000UL
#define SPI4_BASE               0x58011400UL
#define USART1_BASE             0x58011800UL
#define TIM15_BASE              0x58012000UL
#define TIM16_BASE              0x58012400UL
#define TIM17_BASE              0x58012800UL
#define SPI5_BASE               0x58015000UL
#define SAI1_BASE               0x58015800UL
#define SAI2_BASE               0x58015C00UL
#define DFSDM1_BASE             0x58016000UL
#define SPDIFRX_BASE            0x58016400UL
#define MDIOS_BASE              0x58016800UL
#define BDMA_BASE               0x58016C00UL
#define DMAMUX_BASE             0x58020800UL

/* APB1L Peripheral Bus */
#define TIM2_BASE               0x58000000UL
#define TIM3_BASE               0x58000400UL
#define TIM4_BASE               0x58000800UL
#define TIM5_BASE               0x58000C00UL
#define TIM6_BASE               0x58001000UL
#define TIM7_BASE               0x58001400UL
#define I2C1_BASE               0x58005800UL
#define I2C2_BASE               0x58005C00UL
#define I2C3_BASE               0x58006000UL
#define SPI2_BASE               0x58003800UL
#define SPIFI1_BASE             0x58003C00UL
#define USART2_BASE             0x58004400UL
#define USART3_BASE             0x58004800UL
#define USART4_BASE             0x58004C00UL
#define USART5_BASE             0x58005000UL
#define I2C4_BASE               0x58006400UL
#define LPTIM1_BASE             0x58002400UL
#define LPTIM2_BASE             0x58002800UL
#define LPTIM3_BASE             0x58002C00UL
#define LPTIM4_BASE             0x58003000UL
#define LPTIM5_BASE             0x58003400UL
#define WWDG_BASE               0x58002C00UL

/* APB1H Peripheral Bus */
#define LPUART1_BASE            0x58009800UL
#define SPI3_BASE               0x58003C00UL
#define I2S1_BASE               0x58005800UL     /* Shares with I2C1 on APB1L? No, I2Sx are separate IPs */
#define I2S3_BASE               0x58006000UL     /* Shares with I2C3 on APB1L? No, separate */

/* =========================================================================
 * STM32H743 specific peripheral base addresses
 * NOTE: For I2S, the STM32H743 uses SPI as the base IP with I2S mode.
 * I2S1 maps to SPI1 (APB2), I2S2 maps to SPI2 (APB1L), I2S3 maps to SPI3 (APB1H).
 * We use SPI1 as I2S1 (Tx) and SPI3 as I2S3 (Rx).
 * ========================================================================= */
#define I2S_TX_BASE             SPI1_BASE         /* I2S1 via SPI1, APB2 @ 120 MHz */
#define I2S_RX_BASE             SPI3_BASE         /* I2S3 via SPI3, APB1H @ 120 MHz */

/* =========================================================================
 * GPIO Register Offsets (STM32H743 §10.4)
 * ========================================================================= */
#define GPIO_MODER              0x00        /* Mode register (2 bits/pin) */
#define GPIO_OTYPER             0x04        /* Output type */
#define GPIO_OSPEEDR            0x08        /* Output speed */
#define GPIO_PUPDR              0x0C        /* Pull-up/pull-down */
#define GPIO_IDR                0x10        /* Input data */
#define GPIO_ODR                0x14        /* Output data */
#define GPIO_BSRR               0x18        /* Bit set/reset */
#define GPIO_LCKR               0x1C        /* Lock */
#define GPIO_AFRL               0x20        /* Alternate function low */
#define GPIO_AFRH               0x24        /* Alternate function high */

/* GPIO mode values */
#define GPIO_MODE_INPUT         0x0
#define GPIO_MODE_OUTPUT        0x1
#define GPIO_MODE_AF            0x2
#define GPIO_MODE_ANALOG        0x3

/* GPIO output speed */
#define GPIO_SPEED_LOW          0x0
#define GPIO_SPEED_MEDIUM       0x1
#define GPIO_SPEED_HIGH         0x2
#define GPIO_SPEED_VERY_HIGH    0x3

/* GPIO pull modes */
#define GPIO_PULL_NONE          0x0
#define GPIO_PULL_UP            0x1
#define GPIO_PULL_DOWN          0x2

/* =========================================================================
 * SPI/I2S Register Offsets (STM32H743 §45, §46)
 * ========================================================================= */
#define SPI_CR1                 0x00        /* Control register 1 */
#define SPI_CR2                 0x04        /* Control register 2 */
#define SPI_CFG1                0x08        /* Configuration 1 */
#define SPI_CFG2                0x0C        /* Configuration 2 */
#define SPI_IER                 0x10        /* Interrupt enable */
#define SPI_SR                  0x14        /* Status register */
#define SPI_IFCR                0x18        /* Interrupt/flag clear */
#define SPI_TXDR                0x20        /* Tx data register */
#define SPI_RXDR                0x30        /* Rx data register */
#define SPI_I2SCFGR             0x50        /* I2S configuration register */
#define SPI_I2SCFGR_I2SMOD      (1U << 11)  /* I2S mode enable */
#define SPI_I2SCFGR_I2SE        (1U << 10)  /* I2S enable */
#define SPI_I2SCFGR_I2SCFG      (3U << 8)   /* I2S configuration mask */
#define SPI_I2SCFGR_I2SCFG_TX   (1U << 8)   /* I2S Tx mode */
#define SPI_I2SCFGR_I2SCFG_RX   (2U << 8)   /* I2S Rx mode */
#define SPI_I2SCFGR_CKPOL       (1U << 7)   /* Clock polarity */
#define SPI_I2SCFGR_DATLEN      (3U << 5)   /* Data length mask */
#define SPI_I2SCFGR_DATLEN_16   (0U << 5)   /* 16-bit data */
#define SPI_I2SCFGR_DATLEN_24   (1U << 5)   /* 24-bit data */
#define SPI_I2SCFGR_DATLEN_32   (2U << 5)   /* 32-bit data */
#define SPI_I2SCFGR_CHLEN       (1U << 4)   /* Channel length (0=16b, 1=32b) */
#define SPI_I2SCFGR_STD_MASK    (3U << 1)   /* I2S standard mask */
#define SPI_I2SCFGR_STD_PHILIPS (0U << 1)   /* I2S Philips */
#define SPI_I2SCFGR_STD_MSB     (1U << 1)   /* MSB-justified */
#define SPI_I2SCFGR_STD_LSB     (2U << 1)   /* LSB-justified */
#define SPI_I2SCFGR_STD_PCM     (3U << 1)   /* PCM/DSP */
#define SPI_I2SCFGR_I2SDIV      (0xFFU << 0) /* I2S prescaler */

/* SPI control */
#define SPI_CR1_SPE             (1U << 0)   /* SPI enable */
#define SPI_CR1_MSTR            (1U << 2)   /* Master selection */
#define SPI_CR2_TSIZE           (0xFFFFFFFFUL << 0) /* Transfer size */
#define SPI_CR2_TSIZE_OFFSET    0
#define SPI_CFG1_MBR            (7U << 28)  /* Master baud rate */
#define SPI_CFG1_MBR_OFFSET     28
#define SPI_CFG1_FTHLV          (7U << 0)   /* FIFO threshold level */
#define SPI_CFG1_FTHLV_OFFSET   0
#define SPI_CFG2_SSM            (1U << 2)   /* Software slave management */
#define SPI_CFG2_SSI            (1U << 1)   /* Internal slave select */
#define SPI_CFG2_COMM_MASK      (3U << 17)  /* Communication mode */
#define SPI_CFG2_COMM_FULL      (0U << 17)  /* Full duplex */
#define SPI_CFG2_COMM_TX        (1U << 17)  /* Tx only */
#define SPI_CFG2_COMM_RX        (2U << 17)  /* Rx only */

/* SPI status flags */
#define SPI_SR_TXP              (1U << 0)   /* Tx data register empty (with threshold) */
#define SPI_SR_RXP              (1U << 1)   /* Rx data register full (with threshold) */
#define SPI_SR_TXC              (1U << 8)   /* Tx complete */
#define SPI_SR_RXWNE            (1U << 9)   /* Rx FIFO word not empty */
#define SPI_SR_EOT              (1U << 10)  /* End of transfer */
#define SPI_SR_FRLVL_MASK       (3U << 11)  /* FIFO Rx level */
#define SPI_SR_FTLVL_MASK       (3U << 13)  /* FIFO Tx level */

/* I2S prescaler formula: fs = I2S_CLK / (I2SDIV * 8 * (CHLEN+1) * 2) */
/* For 96 kHz: I2SDIV = I2S_CLK / (96k * 8 * 2 * 2) = 120MHz / 3.072M = 39 */
/* For 192 kHz: I2SDIV = I2S_CLK / (192k * 8 * 2 * 2) = 120MHz / 6.144M = 19 */

/* =========================================================================
 * TIM (Timer) Register Offsets (STM32H743 §48)
 * ========================================================================= */
#define TIM_CR1                 0x00        /* Control register 1 */
#define TIM_CR2                 0x04        /* Control register 2 */
#define TIM_SMCR                0x08        /* Slave mode control */
#define TIM_DIER                0x0C        /* DMA/Interrupt enable */
#define TIM_SR                  0x10        /* Status register */
#define TIM_EGR                 0x14        /* Event generation */
#define TIM_CCMR1               0x18        /* Capture/compare mode 1 */
#define TIM_CCMR2               0x1C        /* Capture/compare mode 2 */
#define TIM_CCER                0x20        /* Capture/compare enable */
#define TIM_CNT                 0x24        /* Counter */
#define TIM_PSC                 0x28        /* Prescaler */
#define TIM_ARR                 0x2C        /* Auto-reload */
#define TIM_CCR1                0x34        /* Capture/compare 1 */
#define TIM_CCR2                0x38        /* Capture/compare 2 */
#define TIM_CCR3                0x3C        /* Capture/compare 3 */
#define TIM_CCR4                0x40        /* Capture/compare 4 */

/* TIM control bits */
#define TIM_CR1_CEN             (1U << 0)   /* Counter enable */
#define TIM_CR1_UDIS            (1U << 1)   /* Update disable */
#define TIM_CR1_URS             (1U << 2)   /* Update request source */
#define TIM_CR1_OPM             (1U << 3)   /* One-pulse mode */
#define TIM_CR1_ARPE            (1U << 7)   /* Auto-reload preload */

/* TIM interrupt/DMA enables */
#define TIM_DIER_UIE            (1U << 0)   /* Update interrupt enable */
#define TIM_DIER_CC1IE          (1U << 1)   /* CC1 interrupt enable */
#define TIM_DIER_CC2IE          (1U << 2)   /* CC2 interrupt enable */
#define TIM_DIER_TIE            (1U << 6)   /* Trigger interrupt enable */

/* TIM status flags */
#define TIM_SR_UIF              (1U << 0)   /* Update interrupt flag */
#define TIM_SR_CC1IF            (1U << 1)   /* CC1 interrupt flag */
#define TIM_SR_CC2IF            (1U << 2)   /* CC2 interrupt flag */

/* =========================================================================
 * USART/UART Register Offsets (STM32H743 §51)
 * ========================================================================= */
#define USART_CR1               0x00
#define USART_CR2               0x04
#define USART_CR3               0x08
#define USART_BRR               0x0C
#define USART_RQR               0x18
#define USART_ISR               0x1C
#define USART_ICR               0x20
#define USART_RDR               0x24
#define USART_TDR               0x28
#define USART_PRESC             0x2C

/* USART control bits */
#define USART_CR1_UE            (1U << 0)   /* USART enable */
#define USART_CR1_TE            (1U << 3)   /* Transmitter enable */
#define USART_CR1_RE            (1U << 2)   /* Receiver enable */
#define USART_CR1_RXNEIE        (1U << 5)   /* RX not empty interrupt */
#define USART_CR1_TXEIE         (1U << 7)   /* TX empty interrupt */
#define USART_CR1_M0            (1U << 12)  /* Word length bit 0 */
#define USART_CR1_M1            (1U << 28)  /* Word length bit 1 */
#define USART_CR1_FIFOEN        (1U << 29)  /* FIFO enable */
#define USART_CR1_OVER8         (1U << 15)  /* Oversampling by 8 */

/* USART status flags */
#define USART_ISR_RXNE          (1U << 5)   /* Read data register not empty */
#define USART_ISR_TXE           (1U << 7)   /* Transmit data register empty */
#define USART_ISR_TC            (1U << 6)   /* Transmission complete */
#define USART_ISR_FE            (1U << 1)   /* Framing error */
#define USART_ISR_PE            (1U << 0)   /* Parity error */

/* USART baud rate calculation: BRR = fCK / (16 * baud) for OVER8=0 */

/* =========================================================================
 * RCC (Reset & Clock Control) Offsets (STM32H743 §14)
 * ========================================================================= */
#define RCC_BASE                0x58024400UL
#define RCC_CR                  (RCC_BASE + 0x00)
#define RCC_CFGR                (RCC_BASE + 0x10)
#define RCC_D1CCIPR             (RCC_BASE + 0x4C)
#define RCC_D2CCIP1R            (RCC_BASE + 0x50)
#define RCC_D2CCIP2R            (RCC_BASE + 0x54)
#define RCC_D3CCIPR             (RCC_BASE + 0x58)
#define RCC_AHB1ENR             (RCC_BASE + 0xE0)
#define RCC_AHB2ENR             (RCC_BASE + 0xE4)
#define RCC_AHB3ENR             (RCC_BASE + 0xE8)
#define RCC_AHB4ENR             (RCC_BASE + 0xEC)
#define RCC_APB1LENR            (RCC_BASE + 0x100)
#define RCC_APB1HENR            (RCC_BASE + 0x104)
#define RCC_APB2ENR             (RCC_BASE + 0x108)
#define RCC_APB3ENR             (RCC_BASE + 0x10C)
#define RCC_AHB1LPENR           (RCC_BASE + 0x110)

/* Peripheral clock enables */
#define RCC_AHB4ENR_GPIOA       (1U << 0)
#define RCC_AHB4ENR_GPIOB       (1U << 1)
#define RCC_AHB4ENR_GPIOC       (1U << 2)
#define RCC_AHB4ENR_GPIOD       (1U << 3)
#define RCC_AHB4ENR_GPIOE       (1U << 4)
#define RCC_AHB4ENR_GPIOF       (1U << 5)
#define RCC_AHB4ENR_GPIOG       (1U << 6)

#define RCC_APB2ENR_SPI1        (1U << 12)  /* I2S1 Tx */
#define RCC_APB1HENR_SPI3       (1U << 12)  /* I2S3 Rx */
#define RCC_APB1LENR_USART5     (1U << 20)  /* BLE UART */
#define RCC_APB1LENR_TIM3       (1U << 1)   /* Demod timer */
#define RCC_APB1LENR_TIM4       (1U << 2)   /* Mod timer */
#define RCC_AHB1ENR_DMA1        (1U << 0)
#define RCC_AHB1ENR_DMA2        (1U << 1)
#define RCC_AHB3ENR_QSPI        (1U << 8)

/* Clock source selects */
#define RCC_D2CCIP1R_I2S1SEL   (3U << 2)   /* I2S1 clock source */
#define RCC_D2CCIP1R_I2S3SEL   (3U << 4)   /* I2S3 clock source */

/* =========================================================================
 * DMA Register Offsets (STM32H743 §22)
 * ========================================================================= */
#define DMA_SxCR(x)             (0x10 * (x))       /* Stream x control */
#define DMA_SxNDTR(x)           (0x10 * (x) + 0x04)
#define DMA_SxPAR(x)            (0x10 * (x) + 0x08)
#define DMA_SxM0AR(x)           (0x10 * (x) + 0x0C)
#define DMA_SxM1AR(x)           (0x10 * (x) + 0x10)
#define DMA_SxCR_EN             (1U << 0)
#define DMA_SxCR_TCIE           (1U << 4)   /* Transfer complete interrupt */
#define DMA_SxCR_HTIE           (1U << 5)   /* Half-transfer interrupt */
#define DMA_SxCR_TEIE           (1U << 6)   /* Transfer error interrupt */
#define DMA_SxCR_DIR_OFFSET     6
#define DMA_SxCR_DIR_P2M        (0U << 6)   /* Peripheral to memory */
#define DMA_SxCR_DIR_M2P        (1U << 6)   /* Memory to peripheral */
#define DMA_SxCR_DIR_M2M        (2U << 6)   /* Memory to memory */
#define DMA_SxCR_PINC           (1U << 9)   /* Peripheral increment */
#define DMA_SxCR_MINC           (1U << 10)  /* Memory increment */
#define DMA_SxCR_PSIZE_OFFSET   11
#define DMA_SxCR_PSIZE_8        (0U << 11)
#define DMA_SxCR_PSIZE_16       (1U << 11)
#define DMA_SxCR_PSIZE_32       (2U << 11)
#define DMA_SxCR_MSIZE_OFFSET   13
#define DMA_SxCR_MSIZE_8        (0U << 13)
#define DMA_SxCR_MSIZE_16       (1U << 13)
#define DMA_SxCR_MSIZE_32       (2U << 13)
#define DMA_SxCR_CIRC           (1U << 8)   /* Circular mode */
#define DMA_SxCR_DBM            (1U << 18)  /* Double-buffer mode */
#define DMA_SxCR_PFCTRL         (1U << 5)   /* Peripheral flow controller */
#define DMA_SxCR_PRIO_OFFSET    16
#define DMA_SxCR_PRIO_LOW       (0U << 16)
#define DMA_SxCR_PRIO_MED       (1U << 16)
#define DMA_SxCR_PRIO_HI        (2U << 16)
#define DMA_SxCR_PRIO_VHI       (3U << 16)

/* DMA stream selection */
#define DMA1_STREAM0            0
#define DMA1_STREAM1            1
#define DMA1_STREAM2            2
#define DMA1_STREAM3            3
#define DMA1_STREAM4            4
#define DMA1_STREAM5            5
#define DMA1_STREAM6            6
#define DMA1_STREAM7            7

#define DMA2_STREAM0            8
#define DMA2_STREAM1            9
#define DMA2_STREAM2            10
#define DMA2_STREAM3            11
#define DMA2_STREAM4            12
#define DMA2_STREAM5            13
#define DMA2_STREAM6            14
#define DMA2_STREAM7            15

/* DMA LIFS/LISR register offsets */
#define DMA_LISR                0x00
#define DMA_HISR                0x04
#define DMA_LIFCR               0x08
#define DMA_HIFCR               0x0C
#define DMA_SxCR_BASE(x)        (0x10 * (x) + 0x10)

/* =========================================================================
 * NVIC (Nested Vectored Interrupt Controller)
 * ========================================================================= */
#define NVIC_ISER_BASE          0xE000E100UL
#define NVIC_IPR_BASE           0xE000E400UL

#define NVIC_ISER(n)            (*(volatile uint32_t *)(NVIC_ISER_BASE + (n) * 4))
#define NVIC_IPR(n)             (*(volatile uint32_t *)(NVIC_IPR_BASE + (n) * 4))

/* STM32H743 IRQ numbers */
#define IRQ_DMA1_STREAM0        15
#define IRQ_DMA1_STREAM1        16
#define IRQ_DMA1_STREAM2        17
#define IRQ_DMA1_STREAM3        18
#define IRQ_DMA1_STREAM4        19
#define IRQ_DMA1_STREAM5        20
#define IRQ_DMA1_STREAM6        21
#define IRQ_DMA1_STREAM7        22
#define IRQ_DMA2_STREAM0        56
#define IRQ_DMA2_STREAM1        57
#define IRQ_DMA2_STREAM2        58
#define IRQ_DMA2_STREAM3        59
#define IRQ_DMA2_STREAM4        60
#define IRQ_DMA2_STREAM5        61
#define IRQ_DMA2_STREAM6        62
#define IRQ_DMA2_STREAM7        63
#define IRQ_TIM3                46
#define IRQ_TIM4                47
#define IRQ_USART5              53
#define IRQ_SPI1                56
#define IRQ_SPI3                56  /* Actually SPI3 is separate but same prio group */

/* Priority: 0 = highest, 15 = lowest (4-bit priority, no sub-priority) */

/* =========================================================================
 * USB OTG FS Register Offsets (STM32H743 §55)
 * ========================================================================= */
#define USB_OTG_GOTGCTL         0x000
#define USB_OTG_GOTGINT         0x004
#define USB_OTG_GAHBCFG         0x008
#define USB_OTG_GUSBCFG         0x00C
#define USB_OTG_GRSTCTL         0x010
#define USB_OTG_GINTSTS         0x014
#define USB_OTG_GINTMSK         0x018
#define USB_OTG_GRXSTSR         0x01C
#define USB_OTG_GRXSTSP         0x020
#define USB_OTG_GRXFSIZ         0x024
#define USB_OTG_GNPTXFSIZ       0x028
#define USB_OTG_GNPTXSTS        0x02C
#define USB_OTG_DCFG            0x400
#define USB_OTG_DCTL            0x404
#define USB_OTG_DSTS            0x408
#define USB_OTG_DIEPMSK         0x410
#define USB_OTG_DOEPMSK         0x414
#define USB_OTG_DAINT           0x418
#define USB_OTG_DAINTMSK        0x41C
#define USB_OTG_DIEPCTL(n)      (0x500 + 0x20 * (n))
#define USB_OTG_DOEPCTL(n)      (0x600 + 0x20 * (n))
#define USB_OTG_DIEPTSIZ(n)     (0x510 + 0x20 * (n))
#define USB_OTG_DOEPTSIZ(n)     (0x610 + 0x20 * (n))
#define USB_OTG_DIEPDMA(n)      (0x514 + 0x20 * (n))
#define USB_OTG_DOEPDMA(n)      (0x614 + 0x20 * (n))

/* =========================================================================
 * SAI Register Offsets (STM32H743 §47, if needed for debug)
 * ========================================================================= */
#define SAI1_BASE_              0x58015800UL
#define SAI_CR1                 0x00
#define SAI_CR2                 0x04
#define SAI_FRCR                0x08
#define SAI_SLOTR               0x10
#define SAI_IM                  0x14
#define SAI_SR                  0x18
#define SAI_CLRFR               0x20
#define SAI_DR                  0x24

/* =========================================================================
 * WWDG (Windowed Watchdog)
 * ========================================================================= */
#define WWDG_BASE               0x58002C00UL
#define WWDG_CR                 0x00
#define WWDG_CFR                0x04
#define WWDG_SR                 0x08

/* =========================================================================
 * PWR (Power Controller)
 * ========================================================================= */
#define PWR_BASE                0x58024800UL
#define PWR_CR1                 (PWR_BASE + 0x00)
#define PWR_CSR1                (PWR_BASE + 0x04)
#define PWR_CR2                 (PWR_BASE + 0x08)
#define PWR_CR3                 (PWR_BASE + 0x0C)
#define PWR_CPUCR               (PWR_BASE + 0x10)
#define PWR_D3CR                (PWR_BASE + 0x18)
#define PWR_WKUPCR              (PWR_BASE + 0x20)
#define PWR_WKUPFR              (PWR_BASE + 0x28)

/* =========================================================================
 * FLASH Controller
 * ========================================================================= */
#define FLASH_ACR               0x00        /* Flash access control register */
#define FLASH_KEYR              0x04        /* Flash key register */
#define FLASH_KEY1              0x45670123UL
#define FLASH_KEY2              0xCDEF89ABUL
#define FLASH_OPTKEYR           0x08
#define FLASH_OPTKEY1           0x08192A3BUL
#define FLASH_OPTKEY2           0x4C5D6E7FUL

/* =========================================================================
 * STM32H743 System Control Block
 * ========================================================================= */
#define SCB_BASE                0xE000ED00UL
#define SCB_CPUID               (SCB_BASE + 0x00)
#define SCB_ICSR                (SCB_BASE + 0x04)
#define SCB_VTOR                (SCB_BASE + 0x08)
#define SCB_AIRCR               (SCB_BASE + 0x0C)
#define SCB_SCR                 (SCB_BASE + 0x10)
#define SCB_CCR                 (SCB_BASE + 0x14)
#define SCB_SHPR1               (SCB_BASE + 0x18)
#define SCB_SHPR2               (SCB_BASE + 0x1C)
#define SCB_SHPR3               (SCB_BASE + 0x20)

/* System control */
#define SCB_AIRCR_VECTKEY       (0x5FAU << 16) /* KEY must be written */
#define SCB_AIRCR_SYSRESETREQ   (1U << 2)   /* System reset request */
#define SCB_SCR_SLEEPDEEP       (1U << 2)   /* Deep sleep */
#define SCB_CCR_DIV_0_TRP       (1U << 4)   /* Trap on divide by 0 */

/* =========================================================================
 * CORTEX-M7 System Timer (SysTick)
 * ========================================================================= */
#define SYSTICK_BASE            0xE000E010UL
#define SYSTICK_CSR             (SYSTICK_BASE + 0x00)
#define SYSTICK_RVR             (SYSTICK_BASE + 0x04)
#define SYSTICK_CVR             (SYSTICK_BASE + 0x08)
#define SYSTICK_CALIB           (SYSTICK_BASE + 0x0C)

#define SYSTICK_CSR_ENABLE      (1U << 0)
#define SYSTICK_CSR_TICKINT     (1U << 1)
#define SYSTICK_CSR_CLKSOURCE   (1U << 2)
#define SYSTICK_CSR_COUNTFLAG   (1U << 16)

/* =========================================================================
 * QuadSPI (QUADSPI) Register Offsets (STM32H743 §24)
 * ========================================================================= */
#define QSPI_CR                 0x00        /* Control register */
#define QSPI_DCR                0x04        /* Device configuration */
#define QSPI_SR                 0x08        /* Status register */
#define QSPI_FCR                0x0C        /* Flag clear register */
#define QSPI_DLR                0x10        /* Data length */
#define QSPI_AR                 0x14        /* Address register */
#define QSPI_ABR                0x18        /* Alternate bytes */
#define QSPI_DR                 0x20        /* Data register */
#define QSPI_PSMKR              0x24        /* Polling status mask */
#define QSPI_PSMAR              0x28        /* Polling status match */
#define QSPI_PIR                0x2C        /* Polling interval */
#define QSPI_LPTR               0x30        /* Low-power timeout */
#define QSPI_CCR                0x100       /* Communication control register (indirect mode) */

/* QSPI control bits */
#define QSPI_CR_EN              (1U << 0)
#define QSPI_CR_ABORT           (1U << 1)
#define QSPI_CR_DMAEN           (1U << 2)
#define QSPI_CR_TCEN            (1U << 3)
#define QSPI_CR_FTHRES          (7U << 8)   /* FIFO threshold */
#define QSPI_CR_TEIE            (1U << 16)  /* Transfer error interrupt */
#define QSPI_CR_TCIE            (1U << 17)  /* Transfer complete interrupt */
#define QSPI_CR_FTIE            (1U << 18)  /* FIFO threshold interrupt */
#define QSPI_CR_SMIE            (1U << 19)  /* Status match interrupt */
#define QSPI_CR_TOIE            (1U << 20)  /* Timeout interrupt */

/* QSPI status flags */
#define QSPI_SR_TEF             (1U << 0)   /* Transfer error */
#define QSPI_SR_TCF             (1U << 1)   /* Transfer complete */
#define QSPI_SR_FTF             (1U << 2)   /* FIFO threshold flag */
#define QSPI_SR_SMF             (1U << 3)   /* Status match flag */
#define QSPI_SR_TOF             (1U << 4)   /* Timeout flag */
#define QSPI_SR_BUSY            (1U << 5)   /* Busy */
#define QSPI_SR_FLEVEL          (0x1FUL << 8) /* FIFO level */

/* QSPI configuration selections for CCR */
#define QSPI_CCR_FMODE_MASK     (3U << 26)
#define QSPI_CCR_FMODE_INDIR    (0U << 26)  /* Indirect write */
#define QSPI_CCR_FMODE_INDIRRD  (1U << 26)  /* Indirect read */
#define QSPI_CCR_FMODE_MMIO     (3U << 26)  /* Memory-mapped */
#define QSPI_CCR_DMODE_MASK     (7U << 8)
#define QSPI_CCR_DMODE_NONE     (0U << 8)
#define QSPI_CCR_DMODE_1LINE    (1U << 8)
#define QSPI_CCR_DMODE_2LINES   (3U << 8)
#define QSPI_CCR_DMODE_4LINES   (5U << 8)
#define QSPI_CCR_DCYC_MASK      (0x1FUL << 18)
#define QSPI_CCR_DCYC_OFFSET    18
#define QSPI_CCR_ABSIZE_MASK    (3U << 14)
#define QSPI_CCR_ABSIZE_8       (0U << 14)
#define QSPI_CCR_ABSIZE_16      (1U << 14)
#define QSPI_CCR_ABSIZE_24      (2U << 14)
#define QSPI_CCR_ABSIZE_32      (3U << 14)
#define QSPI_CCR_ABMODE_MASK    (7U << 10)
#define QSPI_CCR_ABMODE_NONE    (0U << 10)
#define QSPI_CCR_ABMODE_1LINE   (1U << 10)
#define QSPI_CCR_ABMODE_4LINES  (5U << 10)
#define QSPI_CCR_ADMODE_MASK    (7U << 4)
#define QSPI_CCR_ADMODE_NONE    (0U << 4)
#define QSPI_CCR_ADMODE_1LINE   (1U << 4)
#define QSPI_CCR_ADMODE_4LINES  (5U << 4)
#define QSPI_CCR_IMODE_MASK     (7U << 0)
#define QSPI_CCR_IMODE_NONE     (0U << 0)
#define QSPI_CCR_IMODE_1LINE    (1U << 0)
#define QSPI_CCR_IMODE_4LINES   (5U << 0)

/* =========================================================================
 * PSRAM (FMC) Register Offsets — FMC NAND/PC Card Controller (STM32H743 §38)
 * We use FMC Bank 3 for PSRAM (16-bit multiplexed).
 * ========================================================================= */
#define FMC_PCR_BANK3           0x80
#define FMC_PMEM_BANK3          0x88
#define FMC_PATT_BANK3          0x8C
#define FMC_PIO_BANK3           0x94
#define FMC_PCR_PWAITEN         (1U << 1)
#define FMC_PCR_PWID_16         (1U << 4)
#define FMC_PCR_TCLR_OFFSET     9
#define FMC_PCR_TCLR_MASK       (0x0FUL << 9)
#define FMC_PCR_TAR_OFFSET      13
#define FMC_PCR_TAR_MASK        (0x0FUL << 13)
#define FMC_PCR_ECCEN           (1U << 6)
#define FMC_PCR_PBKEN           (1U << 2)

/* FMC memory controller base (via FMC_BANK3_BASE for PSRAM) */
#define FMC_BANK3_BASE          0x68000000UL

#endif /* REGISTERS_H */