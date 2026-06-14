/*
 * registers.h — STM32F407VGT6 peripheral register definitions
 * CAN Bus Infiltrator firmware
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

/* ========== Cortex-M4 Core Registers ========== */
#define SCB_BASE        0xE000ED00UL
#define NVIC_BASE       0xE000E100UL
#define SCS_BASE        0xE000E000UL
#define SysTick_BASE    0xE000E010UL

typedef struct {
    volatile uint32_t CPUID;
    volatile uint32_t ICSR;
    volatile uint32_t VTOR;
    volatile uint32_t AIRCR;
    volatile uint32_t SCR;
    volatile uint32_t CCR;
    volatile uint32_t SHPR[3];
    volatile uint32_t SHCSR;
    volatile uint32_t CFSR;
    volatile uint32_t HFSR;
    volatile uint32_t DFSR;
    volatile uint32_t MMFAR;
    volatile uint32_t BFAR;
    volatile uint32_t AFSR;
    volatile uint32_t ID_PFR[2];
    volatile uint32_t ID_DFR;
    volatile uint32_t ID_AFR;
    volatile uint32_t ID_MMFR[4];
    volatile uint32_t ID_ISAR[5];
    volatile uint32_t CPACR;
} SCB_TypeDef;

#define SCB ((SCB_TypeDef *)SCB_BASE)

typedef struct {
    volatile uint32_t ISER[8];
    volatile uint32_t RESERVED0[24];
    volatile uint32_t ICER[8];
    volatile uint32_t RESERVED1[24];
    volatile uint32_t ISPR[8];
    volatile uint32_t RESERVED2[24];
    volatile uint32_t ICPR[8];
    volatile uint32_t RESERVED3[24];
    volatile uint32_t IABR[8];
    volatile uint32_t RESERVED4[56];
    volatile uint32_t IPR[60];
    volatile uint32_t RESERVED5[644];
    volatile uint32_t STIR;
} NVIC_TypeDef;

#define NVIC ((NVIC_TypeDef *)NVIC_BASE)

typedef struct {
    volatile uint32_t CSR;
    volatile uint32_t RVR;
    volatile uint32_t CVR;
    volatile uint32_t CALIB;
} SysTick_TypeDef;

#define SysTick ((SysTick_TypeDef *)SysTick_BASE)

/* ========== RCC Registers ========== */
#define RCC_BASE    0x40023800UL

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t AHB1ENR;
    volatile uint32_t AHB2ENR;
    volatile uint32_t AHB3ENR;
    volatile uint32_t RESERVED0;
    volatile uint32_t APB1ENR;
    volatile uint32_t APB2ENR;
    volatile uint32_t RESERVED1[2];
    volatile uint32_t AHB1LPENR;
    volatile uint32_t AHB2LPENR;
    volatile uint32_t AHB3LPENR;
    volatile uint32_t RESERVED2;
    volatile uint32_t APB1LPENR;
    volatile uint32_t APB2LPENR;
    volatile uint32_t RESERVED3[2];
    volatile uint32_t BDCR;
    volatile uint32_t CSR;
    volatile uint32_t RESERVED4[2];
    volatile uint32_t SSCGR;
    volatile uint32_t PLLI2SCFGR;
    volatile uint32_t PLLSAICFGR;
    volatile uint32_t DCKCFGR;
} RCC_TypeDef;

#define RCC ((RCC_TypeDef *)RCC_BASE)

#define RCC_APB1ENR_CAN1EN     (1 << 25)
#define RCC_APB1ENR_CAN2EN     (1 << 26)
#define RCC_APB1ENR_SPI3EN     (1 << 15)
#define RCC_APB1ENR_I2C1EN     (1 << 21)
#define RCC_APB1ENR_TIM2EN     (1 << 0)
#define RCC_APB2ENR_SYSCFGEN   (1 << 14)
#define RCC_AHB1ENR_GPIOA       (1 << 0)
#define RCC_AHB1ENR_GPIOB       (1 << 1)
#define RCC_AHB1ENR_GPIOC       (1 << 2)
#define RCC_AHB1ENR_GPIOD       (1 << 3)
#define RCC_AHB1ENR_GPIOE       (1 << 4)
#define RCC_AHB1ENR_DMA2EN      (1 << 22)
#define RCC_AHB1ENR_BKPSRAMEN   (1 << 18)

/* ========== GPIO Registers ========== */
#define GPIOA_BASE  0x40020000UL
#define GPIOB_BASE  0x40020400UL
#define GPIOC_BASE  0x40020800UL
#define GPIOD_BASE  0x40020C00UL
#define GPIOE_BASE  0x40021000UL

typedef struct {
    volatile uint32_t MODER;      /* Port mode register */
    volatile uint32_t OTYPER;    /* Port output type register */
    volatile uint32_t OSPEEDR;   /* Port output speed register */
    volatile uint32_t PUPDR;     /* Port pull-up/pull-down register */
    volatile uint32_t IDR;       /* Port input data register */
    volatile uint32_t ODR;       /* Port output data register */
    volatile uint32_t BSRR;     /* Port bit set/reset register */
    volatile uint32_t LCKR;     /* Port configuration lock register */
    volatile uint32_t AFR[2];   /* Alternate function registers */
} GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD ((GPIO_TypeDef *)GPIOD_BASE)
#define GPIOE ((GPIO_TypeDef *)GPIOE_BASE)

/* ========== TIM Registers ========== */
#define TIM2_BASE   0x40000000UL

typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SMCR;
    volatile uint32_t DIER;
    volatile uint32_t SR;
    volatile uint32_t EGR;
    volatile uint32_t CCMR1;
    volatile uint32_t CCMR2;
    volatile uint32_t CCER;
    volatile uint32_t CNT;
    volatile uint32_t PSC;
    volatile uint32_t ARR;
    volatile uint32_t RCR;
    volatile uint32_t CCR1;
    volatile uint32_t CCR2;
    volatile uint32_t CCR3;
    volatile uint32_t CCR4;
    volatile uint32_t RESERVED0;
    volatile uint32_t DCR;
    volatile uint32_t DMAR;
    volatile uint32_t OR;
} TIM_TypeDef;

#define TIM2 ((TIM_TypeDef *)TIM2_BASE)

/* ========== SPI Registers ========== */
#define SPI3_BASE   0x40003C00UL

typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t CRCPR;
    volatile uint32_t RXCRCR;
    volatile uint32_t TXCRCR;
} SPI_TypeDef;

#define SPI3 ((SPI_TypeDef *)SPI3_BASE)

/* ========== I2C Registers ========== */
#define I2C1_BASE   0x40005400UL

typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t OAR1;
    volatile uint32_t OAR2;
    volatile uint32_t DR;
    volatile uint32_t SR1;
    volatile uint32_t SR2;
    volatile uint32_t CCR;
    volatile uint32_t TRISE;
} I2C_TypeDef;

#define I2C1 ((I2C_TypeDef *)I2C1_BASE)

/* ========== CAN Registers ========== */
#define CAN1_BASE   0x40006400UL
#define CAN2_BASE   0x40006800UL

typedef struct {
    volatile uint32_t MCR;      /* 0x00 */
    volatile uint32_t MSR;      /* 0x04 */
    volatile uint32_t TSR;      /* 0x08 */
    volatile uint32_t RF0R;     /* 0x0C */
    volatile uint32_t RF1R;     /* 0x10 */
    volatile uint32_t IER;      /* 0x14 */
    volatile uint32_t ESR;      /* 0x18 */
    volatile uint32_t BTR;      /* 0x1C */
    volatile uint32_t RESERVED0[88]; /* 0x20-0x17C */
    volatile uint32_t sFIFOMailBox[3][8]; /* 0x180-0x1DC */
    volatile uint32_t RESERVED1[16]; /* 0x1E0-0x21C */
    volatile uint32_t FMR;      /* 0x200 */
    volatile uint32_t FM1R;    /* 0x204 */
    volatile uint32_t RESERVED2; /* 0x208 */
    volatile uint32_t FS1R;    /* 0x20C */
    volatile uint32_t RESERVED3; /* 0x210 */
    volatile uint32_t FFA1R;   /* 0x214 */
    volatile uint32_t RESERVED4; /* 0x218 */
    volatile uint32_t FA1R;    /* 0x21C */
    volatile uint32_t RESERVED5[8]; /* 0x220-0x23C */
    volatile uint32_t sFilterRegister[28][4]; /* 0x240-0x3BC */
} CAN_TypeDef;

#define CAN1 ((CAN_TypeDef *)CAN1_BASE)
#define CAN2 ((CAN_TypeDef *)CAN2_BASE)

/* CAN MCR bits */
#define CAN_MCR_INRQ     (1 << 0)
#define CAN_MCR_SLEEP    (1 << 1)
#define CAN_MCR_TXFP     (1 << 2)
#define CAN_MCR_RFLM     (1 << 3)
#define CAN_MCR_NART     (1 << 4)
#define CAN_MCR_AWUM     (1 << 5)
#define CAN_MCR_ABOM     (1 << 6)
#define CAN_MCR_TTCM     (1 << 7)
#define CAN_MCR_RESET    (1 << 15)

/* CAN BTR bits */
#define CAN_BTR_BRP(x)   ((x) << 0)
#define CAN_BTR_TS1(x)   ((x) << 16)
#define CAN_BTR_TS2(x)   ((x) << 20)
#define CAN_BTR_SJW(x)   ((x) << 24)
#define CAN_BTR_LBKM     (1 << 30)
#define CAN_BTR_SILM     (1 << 31)

/* CAN TSR bits */
#define CAN_TSR_RQCP0    (1 << 0)
#define CAN_TSR_RQCP1    (1 << 8)
#define CAN_TSR_RQCP2    (1 << 16)
#define CAN_TSR_TME0     (1 << 26)
#define CAN_TSR_TME1     (1 << 27)
#define CAN_TSR_TME2     (1 << 28)

/* CAN IER bits */
#define CAN_IER_TMEIE    (1 << 0)
#define CAN_IER_FMPIE0   (1 << 1)
#define CAN_IER_FMPIE1   (1 << 4)
#define CAN_IER_ERRIE    (1 << 2)
#define CAN_IER_EWGIE    (1 << 3)
#define CAN_IER_EPVIE    (1 << 10)
#define CAN_IER_BOFIE    (1 << 9)

/* CAN ESR bits */
#define CAN_ESR_EWGF     (1 << 0)
#define CAN_ESR_EPVF     (1 << 1)
#define CAN_ESR_BOFF     (1 << 2)

/* ========== SDIO Registers ========== */
#define SDIO_BASE   0x40012C00UL

typedef struct {
    volatile uint32_t POWER;
    volatile uint32_t CLKCR;
    volatile uint32_t ARG;
    volatile uint32_t CMD;
    volatile uint32_t RESPCMD;
    volatile uint32_t RESP1;
    volatile uint32_t RESP2;
    volatile uint32_t RESP3;
    volatile uint32_t RESP4;
    volatile uint32_t DTIMER;
    volatile uint32_t DLEN;
    volatile uint32_t DCTRL;
    volatile uint32_t DCOUNT;
    volatile uint32_t STA;
    volatile uint32_t ICR;
    volatile uint32_t MASK;
    volatile uint32_t RESERVED0[2];
    volatile uint32_t FIFOCNT;
    volatile uint32_t RESERVED1[13];
    volatile uint32_t FIFO;
} SDIO_TypeDef;

#define SDIO ((SDIO_TypeDef *)SDIO_BASE)

/* ========== FLASH Registers ========== */
#define FLASH_BASE  0x40023C00UL

typedef struct {
    volatile uint32_t ACR;
    volatile uint32_t KEYR;
    volatile uint32_t OPTKEYR;
    volatile uint32_t SR;
    volatile uint32_t CR;
    volatile uint32_t OPTCR;
    volatile uint32_t OPTCR1;
} FLASH_TypeDef;

#define FLASH ((FLASH_TypeDef *)FLASH_BASE)

/* ========== DMA2 Registers ========== */
#define DMA2_BASE   0x40026400UL

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t NDTR;
    volatile uint32_t PAR;
    volatile uint32_t M0AR;
    volatile uint32_t M1AR;
    volatile uint32_t FCR;
} DMA_Stream_TypeDef;

#define DMA2_Stream3 ((DMA_Stream_TypeDef *)(DMA2_BASE + 0x58))

/* ========== NVIC Interrupt Numbers ========== */
#define CAN1_RX0_IRQn     20
#define CAN1_RX1_IRQn     21
#define CAN2_RX0_IRQn     63
#define SPI3_IRQn         51
#define I2C1_EV_IRQn      31
#define SDIO_IRQn         49
#define OTG_FS_IRQn       67
#define TIM2_IRQn         28

/* ========== IWDG Registers ========== */
#define IWDG_BASE   0x40003000UL

typedef struct {
    volatile uint32_t KR;
    volatile uint32_t PR;
    volatile uint32_t RLR;
    volatile uint32_t SR;
} IWDG_TypeDef;

#define IWDG ((IWDG_TypeDef *)IWDG_BASE)

#endif /* REGISTERS_H */