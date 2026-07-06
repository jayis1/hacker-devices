/*
 * lumen-tap/firmware/registers.h
 * MCU peripheral register definitions and the laser-driver/FPGA register map.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

#ifndef LUMEN_TAP_REGISTERS_H
#define LUMEN_TAP_REGISTERS_H

#include <stdint.h>

/* ===================================================================== */
/*  STM32H743 memory map (subset used by LumenTap)                       */
/* ===================================================================== */

#define PERIPH_BASE         (0x40000000UL)
#define D1_APB1PERIPH_BASE  (PERIPH_BASE + 0x00010000UL) /* not used heavily */
#define D1_AHB1PERIPH_BASE  (PERIPH_BASE + 0x00020000UL)
#define D1_AHB2PERIPH_BASE  (PERIPH_BASE + 0x00030000UL)
#define D3_AHB4PERIPH_BASE  (PERIPH_BASE + 0x58020000UL) /* GPIOx on H7 */

/* GPIO banks on AHB4 (H7) */
#define GPIOA_BASE          (D3_AHB4PERIPH_BASE + 0x0000UL)
#define GPIOB_BASE          (D3_AHB4PERIPH_BASE + 0x0400UL)
#define GPIOC_BASE          (D3_AHB4PERIPH_BASE + 0x0800UL)
#define GPIOD_BASE          (D3_AHB4PERIPH_BASE + 0x0C00UL)
#define GPIOE_BASE          (D3_AHB4PERIPH_BASE + 0x1000UL)

/* RCC */
#define RCC_BASE            (0x58024400UL)
#define RCC_CR              (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_PLLCFGR         (*(volatile uint32_t *)(RCC_BASE + 0x0C))
#define RCC_CFGR            (*(volatile uint32_t *)(RCC_BASE + 0x10))
#define RCC_AHB4ENR         (*(volatile uint32_t *)(RCC_BASE + 0xE0))
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0xD8))
#define RCC_AHB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0xDC))
#define RCC_AHB3ENR         (*(volatile uint32_t *)(RCC_BASE + 0xD4))
#define RCC_APB1LENR        (*(volatile uint32_t *)(RCC_BASE + 0xE8))
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0xF0))
#define RCC_AHB4RSTR        (*(volatile uint32_t *)(RCC_BASE + 0x7C))

/* PWR */
#define PWR_BASE            (0x58024800UL)
#define PWR_CR3             (*(volatile uint32_t *)(PWR_BASE + 0x00))
#define PWR_D3CR            (*(volatile uint32_t *)(PWR_BASE + 0x18))

/* Flash controller */
#define FLASH_BASE          (0x52002000UL)
#define FLASH_ACR           (*(volatile uint32_t *)(FLASH_BASE + 0x00))

/* SysCfg */
#define SYSCFG_BASE         (0x58002400UL)
#define SYSCFG_PMCR         (*(volatile uint32_t *)(SYSCFG_BASE + 0x04))

/* ---- GPIO register layout ------------------------------------------- */
typedef struct {
    volatile uint32_t MODER;   /* 0x00 */
    volatile uint32_t OTYPER;  /* 0x04 */
    volatile uint32_t OSPEEDR; /* 0x08 */
    volatile uint32_t PUPDR;   /* 0x0C */
    volatile uint32_t IDR;     /* 0x10 */
    volatile uint32_t ODR;     /* 0x14 */
    volatile uint32_t BSRR;    /* 0x18 */
    volatile uint32_t LCKR;    /* 0x1C */
    volatile uint32_t AFRL;    /* 0x20 */
    volatile uint32_t AFRH;    /* 0x24 */
    volatile uint32_t BRR;     /* 0x28 */
} ltm_gpio_regs_t;

#define GPIOA ((ltm_gpio_regs_t *)GPIOA_BASE)
#define GPIOB ((ltm_gpio_regs_t *)GPIOB_BASE)
#define GPIOC ((ltm_gpio_regs_t *)GPIOC_BASE)
#define GPIOD ((ltm_gpio_regs_t *)GPIOD_BASE)

/* ---- ADC1 (optical return) ------------------------------------------ */
#define ADC1_BASE           (0x40022000UL)
#define ADC2_BASE           (0x40022100UL)
#define ADC12_COMMON_BASE   (0x40022300UL)

typedef struct {
    volatile uint32_t ISR;     /* 0x00 */
    volatile uint32_t IER;     /* 0x04 */
    volatile uint32_t CR;      /* 0x08 */
    volatile uint32_t CFGR;    /* 0x0C */
    volatile uint32_t CFGR2;   /* 0x10 */
    volatile uint32_t SMPR1;   /* 0x14 */
    volatile uint32_t SMPR2;   /* 0x18 */
    volatile uint32_t PCSEL;   /* 0x1C */
    volatile uint32_t LFSR;    /* 0x20 */
    volatile uint32_t RESSEL;  /* 0x24 (H7: AWDxyz regs replaced) */
    uint32_t        RESERVED0[2];
    volatile uint32_t DR;      /* 0x2C — regular data */
    uint32_t        RESERVED1[28];
    volatile uint32_t JSQR;    /* 0xA0 */
    volatile uint32_t OFR[4];  /* 0xA4.. */
    volatile uint32_t JDR1;    /* etc. (abridged) */
} ltm_adc_t;

#define ADC1 ((ltm_adc_t *)ADC1_BASE)

/* ADC12 common */
typedef struct {
    volatile uint32_t CSR;     /* 0x00 */
    volatile uint32_t CCR;     /* 0x08 (offset adjusted on H7) */
    volatile uint32_t CDR;     /* 0x0C dual data */
} ltm_adc12_t;
#define ADC12_COMMON ((ltm_adc12_t *)ADC12_COMMON_BASE)

/* ADC ISR flags */
#define ADC_ISR_ADRDY        (1U << 0)
#define ADC_ISR_EOC          (1U << 2)
#define ADC_ISR_OVR          (1U << 4)
#define ADC_ISR_JEOC         (1U << 6)

/* ADC CR bits */
#define ADC_CR_ADEN          (1U << 0)
#define ADC_CR_ADDIS         (1U << 1)
#define ADC_CR_ADCAL         (1U << 1) /* on older; on H7 calibration reg */
#define ADC_CR_ADSTART       (1U << 2)
#define ADC_CR_BOOST         (1U << 8) /* H7 boost mode */

/* ADC CFGR bits */
#define ADC_CFGR_DMNGT_MASK  (0x3 << 0)
#define ADC_CFGR_DMNGT_DMA   (0x1 << 0)
#define ADC_CFGR_CONT        (1U << 2)
#define ADC_CFGR_OVRMOD      (1U << 4)
#define ADC_CFGR_RES_16BIT   (0x0 << 3) /* H7 RES[2:0] 000=16-bit */
#define ADC_CFGR_EXTEN_MASK  (0x3 << 10)
#define ADC_CFGR_EXTSEL_MASK (0xF << 6) /* H7 EXTSEL[3:0] */

/* ---- DMA1 stream 0 (ADC1) ------------------------------------------- */
#define DMA1_BASE           (0x48026000UL)
#define DMA1_STREAM0_BASE   (DMA1_BASE + 0x10UL)

typedef struct {
    volatile uint32_t CR;      /* 0x00 (NDT etc.) — H7 layout differs */
    volatile uint32_t NDTR;    /* 0x04 */
    volatile uint32_t PAR;     /* 0x08 */
    volatile uint32_t M0AR;    /* 0x0C */
    volatile uint32_t M1AR;    /* 0x10 */
    volatile uint32_t FCR;     /* 0x14 */
} ltm_dma_stream_t;

#define DMA1_S0 ((ltm_dma_stream_t *)DMA1_STREAM0_BASE)

#define DMA_SCR_EN           (1U << 0)
#define DMA_SCR_DMEIE        (1U << 2)
#define DMA_SCR_TEIE         (1U << 3)
#define DMA_SCR_HTIE         (1U << 4)
#define DMA_SCR_TCIE         (1U << 5)
#define DMA_SCR_PFCTRL       (1U << 5)
#define DMA_SCR_DIR_MASK     (0x3 << 6)
#define DMA_SCR_DIR_P2M      (0x2 << 6)
#define DMA_SCR_CIRC         (1U << 8)
#define DMA_SCR_MINC         (1U << 10)
#define DMA_SCR_PSIZE_MASK   (0x3 << 8) /* H7: MSIZE/PSIZE at different bits */
#define DMA_SCR_MSIZE_16     (0x1 << 14)
#define DMA_SCR_PSIZE_16     (0x1 << 12)
#define DMA_SCR_PL_HI        (0x2 << 16)

/* DMA1 ISR / IFR */
#define DMA1_LISR            (*(volatile uint32_t *)(DMA1_BASE + 0x00))
#define DMA1_HISR            (*(volatile uint32_t *)(DMA1_BASE + 0x04))
#define DMA1_LIFCR           (*(volatile uint32_t *)(DMA1_BASE + 0x08))
#define DMA1_HIFCR           (*(volatile uint32_t *)(DMA1_BASE + 0x0C))

/* ---- TIM1 (laser PWM) ----------------------------------------------- */
#define TIM1_BASE           (0x40010000UL)
typedef struct {
    volatile uint32_t CR1;    /* 0x00 */
    volatile uint32_t CR2;    /* 0x04 */
    volatile uint32_t SMCR;   /* 0x08 */
    volatile uint32_t DIER;   /* 0x0C */
    volatile uint32_t SR;     /* 0x10 */
    volatile uint32_t EGR;    /* 0x14 */
    volatile uint32_t CCMR1;  /* 0x18 */
    volatile uint32_t CCMR2;  /* 0x1C */
    volatile uint32_t CCER;   /* 0x20 */
    volatile uint32_t CNT;    /* 0x24 */
    volatile uint32_t PSC;    /* 0x28 */
    volatile uint32_t ARR;    /* 0x2C */
    volatile uint32_t RCR;    /* 0x30 */
    volatile uint32_t CCR1;   /* 0x34 */
    volatile uint32_t CCR2;   /* 0x38 */
    volatile uint32_t CCR3;   /* 0x3C */
    volatile uint32_t CCR4;   /* 0x40 */
} ltm_tim_t;
#define TIM1 ((ltm_tim_t *)TIM1_BASE)

#define TIM_CR1_CEN          (1U << 0)
#define TIM_CR1_ARPE         (1U << 7)
#define TIM_CCMR1_OC1M_PWM1  (0x6 << 4)
#define TIM_CCMR1_OC1PE      (1U << 3)
#define TIM_CCER_CC1E        (1U << 0)

/* ---- USB OTG HS (used in FS mode without ULPI) ---------------------- */
#define USB_OTG_HS_BASE      (0x40040000UL)
/* Full register set is large; we expose only the offsets we touch. */
#define OTG_HS_GOTGCTL       (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x000))
#define OTG_HS_GOTGINT       (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x004))
#define OTG_HS_GAHBCFG       (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x008))
#define OTG_HS_GUSBCFG       (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x00C))
#define OTG_HS_GRSTCTL       (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x010))
#define OTG_HS_GINTSTS       (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x014))
#define OTG_HS_GINTMSK       (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x018))
#define OTG_HS_GRXFSIZ       (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x024))
#define OTG_HS_DIEPTXF0_HNPTXFSIZ (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x028))
#define OTG_HS_HNPTXSTS      (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x02C))
#define OTG_HS_DCFG          (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x800))
#define OTG_HS_DCTL          (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x804))
#define OTG_HS_DSTS          (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x808))
#define OTG_HS_DIEPMSK       (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x810))
#define OTG_HS_DOEPMSK       (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x814))
#define OTG_HS_DAINT         (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x818))
#define OTG_HS_DAINTMSK      (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x81C))

/* Device IN endpoint 0 control */
#define OTG_HS_DIEPCTL0      (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x900))
#define OTG_HS_DIEPCTL1      (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x920))
#define OTG_HS_DIEPCTL2      (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x940))
#define OTG_HS_DOEPCTL0      (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0xB00))
#define OTG_HS_DOEPCTL1      (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0xB20))

/* OTG interrupt bits we care about */
#define OTG_GINTSTS_USBRST   (1U << 12)
#define OTG_GINTSTS_ENUMDONE (1U << 13)
#define OTG_GINTSTS_IEPINT   (1U << 18)
#define OTG_GINTSTS_OEPINT   (1U << 19)
#define OTG_GINTSTS_RXFLVL   (1U << 4)
#define OTG_GINTSTS_SOF      (1U << 3)

/* ---- I2C1 (TSL257 ambient sensor + CS4271 codec) -------------------- */
#define I2C1_BASE            (0x40005400UL)
typedef struct {
    volatile uint32_t CR1;    /* 0x00 */
    volatile uint32_t CR2;    /* 0x04 */
    volatile uint32_t OAR1;   /* 0x08 */
    volatile uint32_t OAR2;   /* 0x0C */
    volatile uint32_t TIMINGR;/* 0x10 */
    volatile uint32_t TIMEOUTR; /* 0x14 */
    volatile uint32_t ISR;    /* 0x18 */
    volatile uint32_t ICR;    /* 0x1C */
    volatile uint32_t PECR;   /* 0x20 */
    volatile uint32_t RXDR;   /* 0x24 */
    volatile uint32_t TXDR;   /* 0x28 */
} ltm_i2c_t;
#define I2C1 ((ltm_i2c_t *)I2C1_BASE)

#define I2C_ISR_TXE          (1U << 0)
#define I2C_ISR_RXNE         (1U << 2)
#define I2C_ISR_TC           (1U << 6)
#define I2C_ISR_NACKF        (1U << 10)
#define I2C_CR1_PE           (1U << 0)
#define I2C_CR2_START        (1U << 13)
#define I2C_CR2_STOP         (1U << 14)
#define I2C_CR2_RD_WRN       (1U << 10)

/* ---- SDMMC1 --------------------------------------------------------- */
#define SDMMC1_BASE          (0x58007000UL)
/* We mostly drive SD via a bit-banged/SDK layer; only expose base. */
#define SDMMC1_POWER         (*(volatile uint32_t *)(SDMMC1_BASE + 0x00))
#define SDMMC1_CLKCR         (*(volatile uint32_t *)(SDMMC1_BASE + 0x04))
#define SDMMC1_ARG           (*(volatile uint32_t *)(SDMMC1_BASE + 0x08))
#define SDMMC1_CMD           (*(volatile uint32_t *)(SDMMC1_BASE + 0x0C))
#define SDMMC1_RESP1         (*(volatile uint32_t *)(SDMMC1_BASE + 0x10))
#define SDMMC1_DTIMER        (*(volatile uint32_t *)(SDMMC1_BASE + 0x14))
#define SDMMC1_DLEN          (*(volatile uint32_t *)(SDMMC1_BASE + 0x18))
#define SDMMC1_DCTRL         (*(volatile uint32_t *)(SDMMC1_BASE + 0x1C))
#define SDMMC1_DCOUNT        (*(volatile uint32_t *)(SDMMC1_BASE + 0x20))
#define SDMMC1_STA           (*(volatile uint32_t *)(SDMMC1_BASE + 0x24))
#define SDMMC1_ICR           (*(volatile uint32_t *)(SDMMC1_BASE + 0x28))

/* SDMMC1 status bits */
#define SDMMC_STA_CMDREND    (1U << 6)
#define SDMMC_STA_DATAEND    (1U << 8)
#define SDMMC_STA_DTIMEOUT   (1U << 2)
#define SDMMC_STA_CTIMEOUT   (1U << 0)

/* ---- I2C device addresses (7-bit) ----------------------------------- */
#define TSL257_I2C_ADDR      (0x29)   /* ambient light sensor */
#define CS4271_I2C_ADDR      (0x4A)   /* optional audio codec */

/* ---- TSL257 register map (abridged) --------------------------------- */
#define TSL257_REG_ENABLE    (0x00 | 0x80)  /* command + reg 0 */
#define TSL257_REG_ATIME     (0x01 | 0x80)
#define TSL257_REG_WTIME     (0x03 | 0x80)
#define TSL257_REG_AILT      (0x04 | 0x80)
#define TSL257_REG_AIHT      (0x06 | 0x80)
#define TSL257_REG_PERS      (0x0C | 0x80)
#define TSL257_REG_CONTROL   (0x0F | 0x80)
#define TSL257_REG_ID        (0x12 | 0x80)
#define TSL257_REG_STATUS    (0x13 | 0x80)
#define TSL257_REG_C0DATA    (0x14 | 0x80)
#define TSL257_REG_C0DATAH   (0x15 | 0x80)
#define TSL257_REG_C1DATA    (0x16 | 0x80)
#define TSL257_REG_C1DATAH   (0x17 | 0x80)

/* ---- NVIC ----------------------------------------------------------- */
#define NVIC_ISER0           (*(volatile uint32_t *)0xE000E100UL)
#define NVIC_ICER0           (*(volatile uint32_t *)0xE000E180UL)
#define NVIC_ISPR0           (*(volatile uint32_t *)0xE000E200UL)
#define NVIC_ICPR0           (*(volatile uint32_t *)0xE000E280UL)
#define NVIC_IPR_BASE        ((volatile uint8_t *)0xE000E400UL)

/* IRQ numbers (H7) */
#define DMA1_Stream0_IRQn    11
#define DMA1_Stream1_IRQn    12
#define ADC1_IRQn            18
#define TIM1_UP_IRQn         25
#define USB_OTG_HS_IRQn      77  /* shifted into IPR index 77-32 etc. */
#define I2C1_EV_IRQn         31
#define EXTI9_5_IRQn         23

/* ---- EXTI ----------------------------------------------------------- */
#define EXTI_BASE            (0x5800D000UL) /* H7 EXTI on D3 AHB */
#define EXTI_IMR1            (*(volatile uint32_t *)(EXTI_BASE + 0x00))
#define EXTI_RTSR1           (*(volatile uint32_t *)(EXTI_BASE + 0x08))
#define EXTI_FTSR1           (*(volatile uint32_t *)(EXTI_BASE + 0x0C))
#define EXTI_PR1             (*(volatile uint32_t *)(EXTI_BASE + 0x14))
#define EXTI_C1_BASE         (EXTI_BASE + 0x80UL)
#define EXTI_C1_IMR1         (*(volatile uint32_t *)(EXTI_C1_BASE + 0x00))

/* ---- SysTick -------------------------------------------------------- */
#define SYSTICK_BASE         (0xE000E010UL)
#define SYSTICK_CSR          (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYSTICK_RVR          (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYSTICK_CVR          (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))
#define SYSTICK_CSR_ENABLE   (1U << 0)
#define SYSTICK_CSR_TICKINT  (1U << 1)
#define SYSTICK_CSR_CLKSRC   (1U << 2)

/* ---- SCB / vector table --------------------------------------------- */
#define SCB_VTOR             (*(volatile uint32_t *)0xE000ED08UL)
#define SCB_AIRCR            (*(volatile uint32_t *)0xE000ED0CUL)
#define SCB_AIRCR_VECTKEY    (0x5FAU << 16)

#endif /* LUMEN_TAP_REGISTERS_H */