/*
 * tpm-phantom — registers.h
 * MMIO register definitions for STM32H743VIT6 target.
 *
 * Defines the peripheral base addresses, bitfield layouts, and helper
 * macros used by the firmware to drive the GPIO, DMA, SPI, TIM, USB and
 * RNG peripherals that form the TPM bus capture engine.
 *
 * Author: jayis1
 * License: GPL-2.0
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef TPM_PHANTOM_REGISTERS_H
#define TPM_PHANTOM_REGISTERS_H

#include <stdint.h>

/* ===================================================================
 * Memory map — STM32H743 (Cortex-M7, RM0433 rev 7)
 * =================================================================== */
#define PERIPH_BASE         (0x40000000UL)
#define PERIPH_AHB1_BASE    (PERIPH_BASE + 0x00020000UL)
#define PERIPH_AHB4_BASE    (0x58020000UL)
#define PERIPH_APB1_BASE    (PERIPH_BASE + 0x00010000UL)
#define PERIPH_APB2_BASE    (0x40010000UL)
#define PERIPH_APB4_BASE    (0x58000000UL)

/* ----- RCC ----- */
#define RCC_BASE            (PERIPH_AHB4_BASE + 0x4400UL)
#define RCC_CR              (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_CFGR            (*(volatile uint32_t *)(RCC_BASE + 0x10))
#define RCC_PLLCFGR         (*(volatile uint32_t *)(RCC_BASE + 0x14))
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0xD8))
#define RCC_AHB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0xDC))
#define RCC_AHB3ENR         (*(volatile uint32_t *)(RCC_BASE + 0xE0))
#define RCC_AHB4ENR         (*(volatile uint32_t *)(RCC_BASE + 0xE4))
#define RCC_APB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0xE8))
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0xF0))
#define RCC_APB3ENR         (*(volatile uint32_t *)(RCC_BASE + 0xEC))
#define RCC_APB4ENR         (*(volatile uint32_t *)(RCC_BASE + 0xF8))
#define RCC_D1CFGR          (*(volatile uint32_t *)(RCC_BASE + 0x18))
#define RCC_D2CFGR          (*(volatile uint32_t *)(RCC_BASE + 0x1C))
#define RCC_D3CFGR          (*(volatile uint32_t *)(RCC_BASE + 0x20))

/* RCC bit helpers */
#define RCC_CR_HSION        (1U << 0)
#define RCC_CR_HSEON        (1U << 16)
#define RCC_CR_HSERDY       (1U << 17)
#define RCC_CR_PLL1ON       (1U << 24)
#define RCC_CR_PLL1RDY      (1U << 25)
#define RCC_CR_CSION        (1U << 7)
#define RCC_CR_CSIRDY       (1U << 8)

/* ----- PWR ----- */
#define PWR_BASE            (PERIPH_APB1_BASE + 0x00005000UL)
#define PWR_CR3            (*(volatile uint32_t *)(PWR_BASE + 0x04))
#define PWR_D3CR           (*(volatile uint32_t *)(PWR_BASE + 0x14))
#define PWR_CR3_LDOEN       (1U << 11)
#define PWR_D3CR_VOS_SHIFT  3
#define PWR_D3CR_VOS_MASK   (3U << 3)

/* ----- Flash ----- */
#define FLASH_ACR           (*(volatile uint32_t *)(PERIPH_AHB3_BASE + 0x0000UL + 0x00))
#define FLASH_ACR_LATENCY_MASK  (0xFU)

/* ----- GPIO ----- */
typedef struct {
    volatile uint32_t MODER;    /* 0x00 mode */
    volatile uint32_t OTYPER;   /* 0x04 output type */
    volatile uint32_t OSPEEDR;  /* 0x08 output speed */
    volatile uint32_t PUPDR;    /* 0x0C pull-up/down */
    volatile uint32_t IDR;      /* 0x10 input data */
    volatile uint32_t ODR;      /* 0x14 output data */
    volatile uint32_t BSRR;     /* 0x18 set/reset */
    volatile uint32_t LCKR;     /* 0x1C lock */
    volatile uint32_t AFRL;    /* 0x20 alt func low */
    volatile uint32_t AFRH;    /* 0x24 alt func high */
    volatile uint32_t BRR;     /* 0x28 reset */
} GPIO_TypeDef;

#define GPIOA_BASE          (PERIPH_AHB4_BASE + 0x0000UL)
#define GPIOB_BASE          (PERIPH_AHB4_BASE + 0x0400UL)
#define GPIOC_BASE          (PERIPH_AHB4_BASE + 0x0800UL)
#define GPIOD_BASE          (PERIPH_AHB4_BASE + 0x0C00UL)
#define GPIOE_BASE          (PERIPH_AHB4_BASE + 0x1000UL)
#define GPIOF_BASE          (PERIPH_AHB4_BASE + 0x1400UL)
#define GPIOG_BASE          (PERIPH_AHB4_BASE + 0x1800UL)
#define GPIOH_BASE          (PERIPH_AHB4_BASE + 0x1C00UL)

#define GPIOA               ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB               ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC               ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD               ((GPIO_TypeDef *)GPIOD_BASE)
#define GPIOE               ((GPIO_TypeDef *)GPIOE_BASE)
#define GPIOF               ((GPIO_TypeDef *)GPIOF_BASE)
#define GPIOG               ((GPIO_TypeDef *)GPIOG_BASE)
#define GPIOH               ((GPIO_TypeDef *)GPIOH_BASE)

#define GPIO_MODE_INPUT      0x0
#define GPIO_MODE_OUTPUT     0x1
#define GPIO_MODE_AF         0x2
#define GPIO_MODE_ANALOG     0x3

#define GPIO_OTYPE_PP        0x0
#define GPIO_OTYPE_OD        0x1

#define GPIO_SPEED_LOW       0x0
#define GPIO_SPEED_MED      0x1
#define GPIO_SPEED_HIGH      0x2
#define GPIO_SPEED_VHIGH     0x3

#define GPIO_PUPD_NONE       0x0
#define GPIO_PUPD_PU         0x1
#define GPIO_PUPD_PD         0x2

/* ----- SYSCFG ----- */
#define SYSCFG_BASE          (PERIPH_APB4_BASE + 0x0400UL)
#define SYSCFG_PMCR         (*(volatile uint32_t *)(SYSCFG_BASE + 0x04))
#define SYSCFG_EXTICR1      (*(volatile uint32_t *)(SYSCFG_BASE + 0x08))

/* ----- DMA1 ----- */
#define DMA1_BASE            (PERIPH_AHB1_BASE + 0x6000UL)
typedef struct {
    volatile uint32_t CR;       /* 0x00 stream config */
    volatile uint32_t NDTR;     /* 0x04 count */
    volatile uint32_t PAR;      /* 0x08 peripheral addr */
    volatile uint32_t M0AR;     /* 0x0C mem addr 0 */
    volatile uint32_t M1AR;     /* 0x10 mem addr 1 */
    volatile uint32_t FCR;       /* 0x14 FIFO ctrl */
} DMA_Stream_TypeDef;

typedef struct {
    volatile uint32_t LISR;      /* low interrupt status */
    volatile uint32_t HISR;      /* high interrupt status */
    volatile uint32_t LIFCR;     /* low flag clear */
    volatile uint32_t HIFCR;     /* high flag clear */
} DMA_Common_TypeDef;

#define DMA1                ((DMA_Stream_TypeDef *)(DMA1_BASE + 0x10UL))
#define DMA1_S0             (*((DMA_Stream_TypeDef *)(DMA1_BASE + 0x10UL)))
#define DMA1_S1             (*((DMA_Stream_TypeDef *)(DMA1_BASE + 0x28UL)))
#define DMA1_S2             (*((DMA_Stream_TypeDef *)(DMA1_BASE + 0x40UL)))
#define DMA1_S3             (*((DMA_Stream_TypeDef *)(DMA1_BASE + 0x58UL)))
#define DMA1_COMMON         (*((DMA_Common_TypeDef *)DMA1_BASE))
#define DMA2_BASE           (PERIPH_AHB1_BASE + 0x7000UL)
#define DMA2_COMMON         (*((DMA_Common_TypeDef *)DMA2_BASE))
#define DMA2_S0             (*((DMA_Stream_TypeDef *)(DMA2_BASE + 0x10UL)))

/* DMA stream bits */
#define DMA_CR_EN            (1U << 0)
#define DMA_CR_DMEIE         (1U << 2)
#define DMA_CR_TCIE         (1U << 4)
#define DMA_CR_HTIE         (1U << 3)
#define DMA_CR_TEIE         (1U << 2)
#define DMA_CR_DIR_MASK     (3U << 6)
#define DMA_CR_DIR_P2M      (0U << 6)
#define DMA_CR_DIR_M2P      (1U << 6)
#define DMA_CR_DIR_M2M      (2U << 6)
#define DMA_CR_MINC         (1U << 10)
#define DMA_CR_PINC         (1U << 9)
#define DMA_CR_PSIZE_8      (0U << 11)
#define DMA_CR_PSIZE_16     (1U << 11)
#define DMA_CR_PSIZE_32     (2U << 11)
#define DMA_CR_MSIZE_8      (0U << 13)
#define DMA_CR_MSIZE_16     (1U << 13)
#define DMA_CR_MSIZE_32     (2U << 13)
#define DMA_CR_CIRC         (1U << 8)
#define DMA_CR_DBM          (1U << 18)
#define DMA_CR_CT           (1U << 19)
#define DMA_CR_PL_LOW       (0U << 16)
#define DMA_CR_PL_MED       (1U << 16)
#define DMA_CR_PL_HIGH      (2U << 16)
#define DMA_CR_PL_VHIGH     (3U << 16)
#define DMA_CR_CHSEL_SHIFT  25
#define DMA_CR_CHSEL_MASK   (7U << 25)

/* ----- SPI (slave for SPI TPM capture) ----- */
#define SPI1_BASE            (PERIPH_APB2_BASE + 0x3000UL)
#define SPI2_BASE            (PERIPH_APB1_BASE + 0x3800UL)
#define SPI3_BASE            (PERIPH_APB1_BASE + 0x3C00UL)
typedef struct {
    volatile uint32_t CR1;      /* 0x00 */
    volatile uint32_t CR2;      /* 0x04 */
    volatile uint32_t CFG1;     /* 0x08 */
    volatile uint32_t CFG2;     /* 0x0C */
    volatile uint32_t IER;      /* 0x10 */
    volatile uint32_t SR;       /* 0x14 */
    volatile uint32_t IFCR;     /* 0x18 */
    volatile uint32_t TXDR;     /* 0x20 (byte access) */
    volatile uint32_t RXDR;     /* 0x30 (byte access) */
    volatile uint32_t I2SCFGR;  /* 0x40 */
} SPI_TypeDef;

#define SPI1                ((SPI_TypeDef *)SPI1_BASE)
#define SPI2                ((SPI_TypeDef *)SPI2_BASE)
#define SPI3                ((SPI_TypeDef *)SPI3_BASE)

#define SPI_CR1_SPE          (1U << 0)
#define SPI_CR1_CSTART       (1U << 9)
#define SPI_CR1_MSTR         (1U << 1)
#define SPI_CR1_SSI          (1U << 12)
#define SPI_CFG1_RXDMAEN     (1U << 0)
#define SPI_CFG1_TXDMAEN     (1U << 1)
#define SPI_CFG1_FTHLV_SHIFT 5
#define SPI_CFG1_DSIZE_SHIFT 0
#define SPI_CFG1_MBR_SHIFT   28
#define SPI_CFG2_CPOL        (1U << 0)
#define SPI_CFG2_CPHA        (1U << 1)
#define SPI_CFG2_LSBFRST     (1U << 2)
#define SPI_CFG2_MASTER      (1U << 4)
#define SPI_CFG2_SSOE        (1U << 5)
#define SPI_CFG2_COMM_FULL   (0U << 17)
#define SPI_CFG2_SLAVE       (0U << 4)
#define SPI_SR_RXP           (1U << 0)
#define SPI_SR_TXP           (1U << 1)
#define SPI_SR_RXWNE         (1U << 6)
#define SPI_SR_EOT           (1U << 3)
#define SPI_SR_OVR           (1U << 6)
#define SPI_SR_MODF          (1U << 5)

/* ----- TIM (LPC clock sampler, SPI clock, heartbeat) ----- */
#define TIM2_BASE            (PERIPH_APB1_BASE + 0x0000UL)
#define TIM3_BASE            (PERIPH_APB1_BASE + 0x0400UL)
#define TIM4_BASE            (PERIPH_APB1_BASE + 0x0800UL)
#define TIM6_BASE            (PERIPH_APB1_BASE + 0x1000UL)
#define TIM7_BASE            (PERIPH_APB1_BASE + 0x1400UL)
typedef struct {
    volatile uint32_t CR1;       /* 0x00 */
    volatile uint32_t CR2;      /* 0x04 */
    volatile uint32_t SMCR;     /* 0x08 slave mode */
    volatile uint32_t DIER;     /* 0x0C DMA/interrupt enable */
    volatile uint32_t SR;       /* 0x10 status */
    volatile uint32_t EGR;      /* 0x14 event gen */
    volatile uint32_t CCMR1;    /* 0x18 */
    volatile uint32_t CCMR2;    /* 0x1C */
    volatile uint32_t CCER;     /* 0x20 */
    volatile uint32_t CNT;      /* 0x24 */
    volatile uint32_t PSC;      /* 0x28 prescaler */
    volatile uint32_t ARR;      /* 0x2C autoreload */
    volatile uint32_t CCR1;     /* 0x34 */
    volatile uint32_t CCR2;     /* 0x38 */
    volatile uint32_t CCR3;     /* 0x3C */
    volatile uint32_t CCR4;     /* 0x40 */
} TIM_TypeDef;

#define TIM2                ((TIM_TypeDef *)TIM2_BASE)
#define TIM3                ((TIM_TypeDef *)TIM3_BASE)
#define TIM4                ((TIM_TypeDef *)TIM4_BASE)
#define TIM6                ((TIM_TypeDef *)TIM6_BASE)
#define TIM7                ((TIM_TypeDef *)TIM7_BASE)

#define TIM_CR1_CEN          (1U << 0)
#define TIM_CR1_ARPE         (1U << 7)
#define TIM_DIER_UDE         (1U << 8)
#define TIM_DIER_UIE         (1U << 0)
#define TIM_SR_UIF           (1U << 0)
#define TIM_SMCR_SMS_SHIFT  0
#define TIM_SMCR_SMS_MASK    (7U << 0)
#define TIM_SMCR_TS_SHIFT    4
#define TIM_SMCR_TS_MASK     (7U << 4)
#define TIM_CCMR1_CC1S_SHIFT 0
#define TIM_CCER_CC1E        (1U << 0)
#define TIM_CCER_CC1P        (1U << 1)
#define TIM_DIER_TDE         (1U << 14)

/* ----- NVIC ----- */
#define NVIC_BASE            (0xE000E100UL)
#define NVIC_ISER0           (*(volatile uint32_t *)(NVIC_BASE + 0x00))
#define NVIC_ICER0           (*(volatile uint32_t *)(NVIC_BASE + 0x80))
#define NVIC_IPR_BASE        (0xE000E400UL)

/* ----- USB OTG HS (FS physical, used for CDC) ----- */
#define USB_OTG_HS_BASE     (0x40040000UL)
#define USB_OTG_GOTGCTL     (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x000))
#define USB_OTG_GOTGINT     (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x004))
#define USB_OTG_GAHBCFG     (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x008))
#define USB_OTG_GUSBCFG     (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x00C))
#define USB_OTG_GRSTCTL     (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x010))
#define USB_OTG_GINTSTS     (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x014))
#define USB_OTG_GINTMSK     (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x018))
#define USB_OTG_GRXFSIZ     (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x01C))
#define USB_OTG_DIEPCTL0    (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x900))
#define USB_OTG_DOEPCTL0    (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0xB00))
#define USB_OTG_DAINTMSK    (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x818))

/* ----- RNG ----- */
#define RNG_BASE             (PERIPH_AHB2_BASE + 0x0800UL)
#define RNG_CR               (*(volatile uint32_t *)(RNG_BASE + 0x00))
#define RNG_SR               (*(volatile uint32_t *)(RNG_BASE + 0x04))
#define RNG_DR               (*(volatile uint32_t *)(RNG_BASE + 0x08))
#define RNG_CR_RNGEN         (1U << 2)
#define RNG_CR_IE            (1U << 3)
#define RNG_SR_DRDY          (1U << 0)
#define RNG_SR_SEIS          (1U << 6)
#define RNG_SR_CEIS          (1U << 5)

/* ----- UART (BLE bridge, debug) ----- */
#define USART3_BASE          (PERIPH_APB1_BASE + 0x4800UL)
#define UART4_BASE           (PERIPH_APB1_BASE + 0x4C00UL)
typedef struct {
    volatile uint32_t CR1;       /* 0x00 */
    volatile uint32_t CR2;      /* 0x04 */
    volatile uint32_t CR3;      /* 0x08 */
    volatile uint32_t BRR;      /* 0x0C */
    volatile uint32_t GTDR;      /* 0x10 guard time + prescaler */
    volatile uint32_t RTOR;      /* 0x14 receiver timeout */
    volatile uint32_t RQR;       /* 0x18 request */
    volatile uint32_t ISR;       /* 0x1C interrupt status */
    volatile uint32_t ICR;       /* 0x20 interrupt clear */
    volatile uint32_t RD;        /* 0x24 RX data */
    volatile uint32_t TD;        /* 0x28 TX data */
} USART_TypeDef;

#define USART3               ((USART_TypeDef *)USART3_BASE)
#define UART4                ((USART_TypeDef *)UART4_BASE)

#define USART_CR1_UE          (1U << 0)
#define USART_CR1_TE          (1U << 3)
#define USART_CR1_RE          (1U << 2)
#define USART_CR1_RXNEIE      (1U << 5)
#define USART_CR1_TCIE       (1U << 6)
#define USART_ISR_TXE         (1U << 7)
#define USART_ISR_TC          (1U << 6)
#define USART_ISR_RXNE        (1U << 5)
#define USART_ISR_RXFT        (1U << 25)

/* ----- IWDG (watchdog) ----- */
#define IWDG_BASE             (0x40003000UL)
#define IWDG_KR              (*(volatile uint32_t *)(IWDG_BASE + 0x00))
#define IWDG_PR              (*(volatile uint32_t *)(IWDG_BASE + 0x04))
#define IWDG_RLR             (*(volatile uint32_t *)(IWDG_BASE + 0x08))
#define IWDG_SR              (*(volatile uint32_t *)(IWDG_BASE + 0x0C))

/* ----- SDMMC (MicroSD capture storage) ----- */
#define SDMMC1_BASE           (PERIPH_APB2_BASE + 0x2800UL)
#define SDMMC1_POWER         (*(volatile uint32_t *)(SDMMC1_BASE + 0x00))
#define SDMMC1_CLKCR         (*(volatile uint32_t *)(SDMMC1_BASE + 0x04))
#define SDMMC1_ARG           (*(volatile uint32_t *)(SDMMC1_BASE + 0x08))
#define SDMMC1_CMD           (*(volatile uint32_t *)(SDMMC1_BASE + 0x0C))
#define SDMMC1_RESP1         (*(volatile uint32_t *)(SDMMC1_BASE + 0x30))
#define SDMMC1_DTIMER        (*(volatile uint32_t *)(SDMMC1_BASE + 0x40))
#define SDMMC1_DLEN          (*(volatile uint32_t *)(SDMMC1_BASE + 0x44))
#define SDMMC1_DCTRL         (*(volatile uint32_t *)(SDMMC1_BASE + 0x48))
#define SDMMC1_DCOUNT        (*(volatile uint32_t *)(SDMMC1_BASE + 0x4C))
#define SDMMC1_STA           (*(volatile uint32_t *)(SDMMC1_BASE + 0x34))
#define SDMMC1_ICR           (*(volatile uint32_t *)(SDMMC1_BASE + 0x38))

/* ----- Interrupt numbers ----- */
#define IRQ_DMA1_STREAM0      11
#define IRQ_DMA1_STREAM1      12
#define IRQ_DMA1_STREAM2      13
#define IRQ_DMA1_STREAM3      14
#define IRQ_DMA2_STREAM0      56
#define IRQ_SPI1              35
#define IRQ_TIM2              28
#define IRQ_USART3            80
#define IRQ_OTG_HS            77

/* ----- Helper macros ----- */
#define BIT(n)                (1UL << (n))
#define READ_REG(r)           ((volatile uint32_t)(r))
#define SET_BIT(r, b)         ((r) |= (b))
#define CLEAR_BIT(r, b)       ((r) &= ~(b))
#define WRITE_REG(r, v)       ((r) = (volatile uint32_t)(v))

#define UNUSED(x)             ((void)(x))

#endif /* TPM_PHANTOM_REGISTERS_H */