/*
 * registers.h — STM32F401 MMIO register definitions for BLE Phantom
 *
 * Base addresses and register offsets for all peripherals used
 * by the BLE Phantom firmware.
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

/* ============================================================
 * Peripheral Base Addresses
 * ============================================================ */

#define PERIPH_BASE         0x40000000U
#define APB1PERIPH_BASE     (PERIPH_BASE + 0x00000000U)
#define APB2PERIPH_BASE     (PERIPH_BASE + 0x00010000U)
#define AHB1PERIPH_BASE     (PERIPH_BASE + 0x00020000U)
#define AHB2PERIPH_BASE     (PERIPH_BASE + 0x10000000U)

/* ============================================================
 * GPIO Registers
 * ============================================================ */

#define GPIOA_BASE          (AHB1PERIPH_BASE + 0x0000U)
#define GPIOB_BASE          (AHB1PERIPH_BASE + 0x0400U)
#define GPIOC_BASE          (AHB1PERIPH_BASE + 0x0800U)

typedef struct {
    volatile uint32_t MODER;      /* 0x00: Port mode */
    volatile uint32_t OTYPER;     /* 0x04: Output type */
    volatile uint32_t OSPEEDR;   /* 0x08: Output speed */
    volatile uint32_t PUPDR;     /* 0x0C: Pull-up/pull-down */
    volatile uint32_t IDR;       /* 0x10: Input data */
    volatile uint32_t ODR;       /* 0x14: Output data */
    volatile uint32_t BSRR;      /* 0x18: Bit set/reset */
    volatile uint32_t LCKR;      /* 0x1C: Lock */
    volatile uint32_t AFR[2];    /* 0x20-0x24: Alternate function */
} GPIO_TypeDef;

#define GPIOA   ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB   ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC   ((GPIO_TypeDef *)GPIOC_BASE)

/* GPIO bit definitions */
#define GPIO_MODE_INPUT       0x00U
#define GPIO_MODE_OUTPUT      0x01U
#define GPIO_MODE_ALTERNATE   0x02U
#define GPIO_MODE_ANALOG     0x03U

#define GPIO_OTYPE_PP         0x00U
#define GPIO_OTYPE_OD         0x01U

#define GPIO_SPEED_LOW        0x00U
#define GPIO_SPEED_MEDIUM     0x01U
#define GPIO_SPEED_FAST       0x02U
#define GPIO_SPEED_VERY_HIGH  0x03U

#define GPIO_PULL_NO          0x00U
#define GPIO_PULL_UP          0x01U
#define GPIO_PULL_DOWN        0x02U

/* ============================================================
 * RCC Registers
 * ============================================================ */

#define RCC_BASE             (AHB1PERIPH_BASE + 0x3800U)

typedef struct {
    volatile uint32_t CR;         /* 0x00: Clock control */
    volatile uint32_t PLLCFGR;   /* 0x04: PLL configuration */
    volatile uint32_t CFGR;      /* 0x08: Clock configuration */
    volatile uint32_t CIR;       /* 0x0C: Clock interrupt */
    volatile uint32_t AHB1RSTR;  /* 0x10: AHB1 reset */
    volatile uint32_t AHB2RSTR;  /* 0x14: AHB2 reset */
    volatile uint32_t AHB3RSTR;  /* 0x18: AHB3 reset */
    volatile uint32_t RESERVED0;
    volatile uint32_t APB1RSTR;  /* 0x20: APB1 reset */
    volatile uint32_t APB2RSTR;  /* 0x24: APB2 reset */
    volatile uint32_t RESERVED1[2];
    volatile uint32_t AHB1ENR;   /* 0x30: AHB1 enable */
    volatile uint32_t AHB2ENR;   /* 0x34: AHB2 enable */
    volatile uint32_t AHB3ENR;   /* 0x38: AHB3 enable */
    volatile uint32_t RESERVED2;
    volatile uint32_t APB1ENR;   /* 0x40: APB1 enable */
    volatile uint32_t APB2ENR;   /* 0x44: APB2 enable */
} RCC_TypeDef;

#define RCC   ((RCC_TypeDef *)RCC_BASE)

/* RCC CR bits */
#define RCC_CR_HSION        (1U << 0)
#define RCC_CR_HSEON        (1U << 16)
#define RCC_CR_HSERDY       (1U << 17)
#define RCC_CR_PLLON        (1U << 24)
#define RCC_CR_PLLRDY       (1U << 25)

/* RCC PLLCFGR bits */
#define RCC_PLLCFGR_PLLSRC_HSE  (1U << 22)

/* RCC CFGR bits */
#define RCC_CFGR_SW_HSI     0x00U
#define RCC_CFGR_SW_PLL     0x02U
#define RCC_CFGR_SWS_SHIFT  2U

/* RCC AHB1ENR bits */
#define RCC_AHB1ENR_GPIOAEN   (1U << 0)
#define RCC_AHB1ENR_GPIOBEN   (1U << 1)
#define RCC_AHB1ENR_GPIOCEN   (1U << 2)
#define RCC_AHB1ENR_DMA1EN    (1U << 21)
#define RCC_AHB1ENR_DMA2EN    (1U << 22)

/* RCC AHB2ENR bits */
#define RCC_AHB2ENR_OTGFSEN   (1U << 7)

/* RCC APB1ENR bits */
#define RCC_APB1ENR_SPI2EN    (1U << 14)
#define RCC_APB1ENR_I2C1EN    (1U << 21)
#define RCC_APB1ENR_USART2EN  (1U << 17)
#define RCC_APB1ENR_PWREN     (1U << 28)

/* RCC APB2ENR bits */
#define RCC_APB2ENR_SPI1EN    (1U << 12)
#define RCC_APB2ENR_SYSCFGEN  (1U << 14)

/* ============================================================
 * SPI Registers
 * ============================================================ */

#define SPI1_BASE            (APB2PERIPH_BASE + 0x3000U)
#define SPI2_BASE            (APB1PERIPH_BASE + 0x3800U)

typedef struct {
    volatile uint32_t CR1;       /* 0x00: Control 1 */
    volatile uint32_t CR2;       /* 0x04: Control 2 */
    volatile uint32_t SR;       /* 0x08: Status */
    volatile uint32_t DR;       /* 0x0C: Data */
    volatile uint32_t CRCPR;    /* 0x10: CRC polynomial */
    volatile uint32_t RXCRCR;   /* 0x14: RX CRC */
    volatile uint32_t TXCRCR;   /* 0x18: TX CRC */
} SPI_TypeDef;

#define SPI1   ((SPI_TypeDef *)SPI1_BASE)
#define SPI2   ((SPI_TypeDef *)SPI2_BASE)

/* SPI CR1 bits */
#define SPI_CR1_CPHA        (1U << 0)
#define SPI_CR1_CPOL        (1U << 1)
#define SPI_CR1_MSTR        (1U << 2)
#define SPI_CR1_BR_SHIFT    3U
#define SPI_CR1_SPE         (1U << 6)
#define SPI_CR1_LSBFIRST    (1U << 7)
#define SPI_CR1_SSI         (1U << 8)
#define SPI_CR1_SSM         (1U << 9)
#define SPI_CR1_RXONLY      (1U << 10)
#define SPI_CR1_DFF         (1U << 11)
#define SPI_CR1_CRCNEXT     (1U << 12)
#define SPI_CR1_CRCEN       (1U << 13)
#define SPI_CR1_BIDIMODE    (1U << 15)

/* SPI SR bits */
#define SPI_SR_RXNE         (1U << 0)
#define SPI_SR_TXE          (1U << 1)
#define SPI_SR_BSY          (1U << 7)

/* ============================================================
 * I2C Registers
 * ============================================================ */

#define I2C1_BASE            (APB1PERIPH_BASE + 0x5400U)

typedef struct {
    volatile uint32_t CR1;       /* 0x00: Control 1 */
    volatile uint32_t CR2;       /* 0x04: Control 2 */
    volatile uint32_t SR1;      /* 0x08: Status 1 */
    volatile uint32_t SR2;      /* 0x0C: Status 2 */
    volatile uint32_t CCR;      /* 0x10: Clock control */
    volatile uint32_t TRISE;    /* 0x14: Rise time */
} I2C_TypeDef;

#define I2C1   ((I2C_TypeDef *)I2C1_BASE)

/* ============================================================
 * USART Registers
 * ============================================================ */

#define USART2_BASE           (APB1PERIPH_BASE + 0x4400U)

typedef struct {
    volatile uint32_t SR;        /* 0x00: Status */
    volatile uint32_t DR;       /* 0x04: Data */
    volatile uint32_t BRR;      /* 0x08: Baud rate */
    volatile uint32_t CR1;      /* 0x0C: Control 1 */
    volatile uint32_t CR2;      /* 0x10: Control 2 */
    volatile uint32_t CR3;      /* 0x14: Control 3 */
} USART_TypeDef;

#define USART2   ((USART_TypeDef *)USART2_BASE)

/* ============================================================
 * DMA Registers
 * ============================================================ */

#define DMA1_BASE             (AHB1PERIPH_BASE + 0x6000U)

typedef struct {
    volatile uint32_t LISR;     /* 0x00: Low interrupt status */
    volatile uint32_t HISR;    /* 0x04: High interrupt status */
    volatile uint32_t LIFCR;   /* 0x08: Low interrupt flag clear */
    volatile uint32_t HIFCR;    /* 0x0C: High interrupt flag clear */
} DMA_TypeDef;

typedef struct {
    volatile uint32_t CR;       /* 0x00: Configuration */
    volatile uint32_t NDTR;    /* 0x04: Number of data */
    volatile uint32_t PAR;      /* 0x08: Peripheral address */
    volatile uint32_t M0AR;    /* 0x0C: Memory 0 address */
    volatile uint32_t M1AR;    /* 0x10: Memory 1 address */
    volatile uint32_t FCR;      /* 0x14: FIFO control */
} DMA_Stream_TypeDef;

#define DMA1   ((DMA_TypeDef *)DMA1_BASE)
#define DMA1_STREAM3 ((DMA_Stream_TypeDef *)(DMA1_BASE + 0x08CU + 0x18 * 3))
#define DMA1_STREAM5 ((DMA_Stream_TypeDef *)(DMA1_BASE + 0x08CU + 0x18 * 5))

/* DMA CR bits */
#define DMA_SxCR_CHSEL_SHIFT 25U
#define DMA_SxCR_MBURST_INC4  (0x01U << 23)
#define DMA_SxCR_PBURST_INC4  (0x01U << 21)
#define DMA_SxCR_CT          (1U << 19)
#define DMA_SxCR_DBM         (1U << 18)
#define DMA_SxCR_PL_SHIFT    16U
#define DMA_SxCR_PINC        (1U << 9)
#define DMA_SxCR_MINC        (1U << 10)
#define DMA_SxCR_PSIZE_8     0x00U
#define DMA_SxCR_PSIZE_16    0x01U << 11
#define DMA_SxCR_MSIZE_8     0x00U
#define DMA_SxCR_MSIZE_16    0x01U << 13
#define DMA_SxCR_DIR_P2M     0x00U
#define DMA_SxCR_DIR_M2P     0x01U << 6
#define DMA_SxCR_DIR_M2M     0x02U << 6
#define DMA_SxCR_TCIE        (1U << 4)
#define DMA_SxCR_EN          (1U << 0)

/* ============================================================
 * EXTI Registers
 * ============================================================ */

#define EXTI_BASE             (PERIPH_BASE + 0x3C00U)

typedef struct {
    volatile uint32_t IMR;      /* 0x00: Interrupt mask */
    volatile uint32_t EMR;      /* 0x04: Event mask */
    volatile uint32_t RTSR;     /* 0x08: Rising trigger select */
    volatile uint32_t FTSR;     /* 0x0C: Falling trigger select */
    volatile uint32_t SWIER;   /* 0x10: Software interrupt */
    volatile uint32_t PR;       /* 0x14: Pending */
} EXTI_TypeDef;

#define EXTI   ((EXTI_TypeDef *)EXTI_BASE)

/* ============================================================
 * SYSCFG Registers
 * ============================================================ */

#define SYSCFG_BASE            (APB2PERIPH_BASE + 0x3800U)

typedef struct {
    volatile uint32_t MEMRMP;   /* 0x00: Memory remap */
    volatile uint32_t PMC;      /* 0x04: Peripheral mode */
    volatile uint32_t EXTICR[4]; /* 0x08-0x14: EXTI config */
    volatile uint32_t CMPCR;    /* 0x20: Compensation */
} SYSCFG_TypeDef;

#define SYSCFG   ((SYSCFG_TypeDef *)SYSCFG_BASE)

/* ============================================================
 * USB OTG FS Registers
 * ============================================================ */

#define OTG_FS_GLOBAL_BASE     0x50000000U
#define OTG_FS_DEVICE_BASE     0x50000800U
#define OTG_FS_FIFO_BASE(n)    (0x50002000U + ((n) * 0x1000U))

typedef struct {
    volatile uint32_t GOTGCTL;  /* 0x000: OTG control */
    volatile uint32_t GOTGINT;  /* 0x004: OTG interrupt */
    volatile uint32_t GAHBCFG;  /* 0x008: AHB config */
    volatile uint32_t GUSBCFG;  /* 0x00C: USB config */
    volatile uint32_t GRSTCTL;  /* 0x010: Reset */
    volatile uint32_t GINTSTS;  /* 0x014: Interrupt status */
    volatile uint32_t GINTMSK;  /* 0x018: Interrupt mask */
    volatile uint32_t GRXSTSR;  /* 0x01C: RX status read */
    volatile uint32_t GRXSTSP;  /* 0x020: RX status pop */
    volatile uint32_t GRXFSIZ;  /* 0x024: RX FIFO size */
    volatile uint32_t DIEPTXF0; /* 0x28: EP0 TX FIFO size */
    volatile uint32_t DIEPTXF1; /* 0x104: EP1 TX FIFO size */
    volatile uint32_t DIEPTXF2; /* 0x108: EP2 TX FIFO size */
    volatile uint32_t DIEPTXF3; /* 0x10C: EP3 TX FIFO size */
} OTG_FS_GLOBAL_TypeDef;

typedef struct {
    volatile uint32_t DCFG;     /* 0x00: Device config */
    volatile uint32_t DCTL;     /* 0x04: Device control */
    volatile uint32_t DSTS;     /* 0x08: Device status */
    volatile uint32_t RESERVED0;
    volatile uint32_t DIEPMSK;  /* 0x10: IN EP interrupt mask */
    volatile uint32_t DOEPMSK;  /* 0x14: OUT EP interrupt mask */
    volatile uint32_t DAINT;    /* 0x18: Device interrupt */
    volatile uint32_t DAINTMSK; /* 0x1C: Device interrupt mask */
    volatile uint32_t RESERVED1;
    volatile uint32_t DVBUSDIS; /* 0x28: VBUS discharge */
    volatile uint32_t DVBUSPULSE; /* 0x2C: VBUS pulse */
    volatile uint32_t RESERVED2[2];
    volatile uint32_t DIEPCTL0; /* 0x100: EP0 IN control */
    volatile uint32_t RESERVED3;
    volatile uint32_t DIEPINT0; /* 0x108: EP0 IN interrupt */
    volatile uint32_t RESERVED4;
    volatile uint32_t DIEPTSIZ0; /* 0x110: EP0 IN transfer size */
    volatile uint32_t RESERVED5[3];
    volatile uint32_t DIEPCTL1; /* 0x120: EP1 IN control */
    volatile uint32_t RESERVED6;
    volatile uint32_t DIEPINT1; /* 0x128: EP1 IN interrupt */
    volatile uint32_t RESERVED7;
    volatile uint32_t DIEPTSIZ1; /* 0x130: EP1 IN transfer size */
    volatile uint32_t RESERVED8[3];
    volatile uint32_t DIEPCTL2; /* 0x140: EP2 IN control */
    volatile uint32_t RESERVED9;
    volatile uint32_t DIEPINT2; /* 0x148: EP2 IN interrupt */
    volatile uint32_t RESERVED10;
    volatile uint32_t DIEPTSIZ2; /* 0x150: EP2 IN transfer size */
    volatile uint32_t RESERVED11[3];
    volatile uint32_t DIEPCTL3; /* 0x160: EP3 IN control */
} OTG_FS_DEVICE_TypeDef;

#define OTG_FS_GLOBAL   ((OTG_FS_GLOBAL_TypeDef *)OTG_FS_GLOBAL_BASE)
#define OTG_FS_DEVICE   ((OTG_FS_DEVICE_TypeDef *)OTG_FS_DEVICE_BASE)

/* OTG FS GUSBCFG bits */
#define OTG_GUSBCFG_FDMOD      (1U << 30)
#define OTG_GUSBCFG_TRDT(n)    ((n) << 10)
#define OTG_GUSBCFG_PHYSEL     (1U << 6)

/* OTG FS GRSTCTL bits */
#define OTG_GRSTCTL_CSRST      (1U << 0)

/* OTG FS GINTSTS bits */
#define OTG_GINTSTS_USBRST     (1U << 12)
#define OTG_GINTSTS_ENUMDNE    (1U << 13)
#define OTG_GINTSTS_RXFLVL     (1U << 4)
#define OTG_GINTSTS_IEPINT     (1U << 18)
#define OTG_GINTSTS_OEPINT     (1U << 19)

/* OTG FS GINTMSK bits */
#define OTG_GINTMSK_USBRST     (1U << 12)
#define OTG_GINTMSK_ENUMDNE    (1U << 13)
#define OTG_GINTMSK_RXFLVL     (1U << 4)
#define OTG_GINTMSK_IEPINT     (1U << 18)
#define OTG_GINTMSK_OEPINT     (1U << 19)

/* OTG FS GAHBCFG bits */
#define OTG_GAHBCFG_GINT      (1U << 0)

/* OTG FS DCFG bits */
#define OTG_DCFG_DSPD_FULL    (3U << 0)
#define OTG_DCFG_NZLSOHSK     (1U << 2)

/* OTG FS DCTL bits */
#define OTG_DCTL_SDIS         (1U << 1)
#define OTG_DCTL_RWUSIG       (1U << 0)

/* OTG FS DIEPCTL bits */
#define OTG_DIEPCTL_EPENA     (1U << 15)
#define OTG_DIEPINT_TXFE      (1U << 7)

/* ============================================================
 * FLASH Registers
 * ============================================================ */

#define FLASH_BASE            0x40023C00U

typedef struct {
    volatile uint32_t ACR;       /* 0x00: Access control */
    volatile uint32_t KEYR;     /* 0x04: Key */
    volatile uint32_t OPTKEYR;  /* 0x08: Option key */
    volatile uint32_t SR;       /* 0x0C: Status */
    volatile uint32_t CR;       /* 0x10: Control */
} FLASH_TypeDef;

#define FLASH   ((FLASH_TypeDef *)FLASH_BASE)

#define FLASH_ACR_LATENCY_0WS   0x00U
#define FLASH_ACR_LATENCY_1WS   0x01U
#define FLASH_ACR_LATENCY_2WS   0x02U
#define FLASH_ACR_ICEN          (1U << 9)
#define FLASH_ACR_DCEN          (1U << 10)

/* ============================================================
 * NVIC Registers (Cortex-M4)
 * ============================================================ */

#define NVIC_BASE              0xE000E100U

typedef struct {
    volatile uint32_t ISER[8];   /* 0x00-0x1C: Interrupt set enable */
    volatile uint32_t RESERVED0[24];
    volatile uint32_t ICER[8];   /* 0x80-0x9C: Interrupt clear enable */
    volatile uint32_t RESERVED1[24];
    volatile uint32_t ISPR[8];   /* 0x100-0x11C: Interrupt set pending */
    volatile uint32_t RESERVED2[24];
    volatile uint32_t ICPR[8];   /* 0x180-0x19C: Interrupt clear pending */
    volatile uint32_t RESERVED3[24];
    volatile uint32_t IABR[8];   /* 0x200-0x21C: Interrupt active bit */
    volatile uint32_t RESERVED4[56];
    volatile uint32_t IP[240];   /* 0x300-0x6BC: Interrupt priority */
} NVIC_TypeDef;

#define NVIC   ((NVIC_TypeDef *)NVIC_BASE)

/* IRQ Numbers */
#define EXTI0_IRQn        6
#define EXTI1_IRQn        7
#define SPI1_IRQn         35
#define SPI2_IRQn         36
#define OTG_FS_IRQn       67

/* ============================================================
 * SysTick Registers
 * ============================================================ */

#define SYSTICK_BASE          0xE000E010U

typedef struct {
    volatile uint32_t CSR;      /* 0x00: Control and status */
    volatile uint32_t RVR;      /* 0x04: Reload value */
    volatile uint32_t CVR;      /* 0x08: Current value */
} SysTick_TypeDef;

#define SYSTICK   ((SysTick_TypeDef *)SYSTICK_BASE)

#define SYSTICK_CSR_ENABLE    (1U << 0)
#define SYSTICK_CSR_TICKINT   (1U << 1)
#define SYSTICK_CSR_CLKSOURCE (1U << 2)

#endif /* REGISTERS_H */