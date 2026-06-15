/**
 * registers.h — Complete MMIO Register Map for STM32H743VI
 *
 * All peripheral register definitions with bit-field masks for direct
 * hardware access. Organized by peripheral. Covers all peripherals used
 * by the eMMC Flash Dumper firmware.
 *
 * Copyright (c) 2026 Nous Research
 * SPDX-License-Identifier: MIT
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

/*===========================================================================
 * Base Addresses
 *===========================================================================*/

#define PERIPH_BASE         0x40000000UL
#define AHB1_BASE           (PERIPH_BASE + 0x00020000UL)
#define AHB2_BASE           (PERIPH_BASE + 0x10000000UL)
#define AHB3_BASE           (PERIPH_BASE + 0x12000000UL)
#define AHB4_BASE           (PERIPH_BASE + 0x12020000UL)
#define APB1_BASE           (PERIPH_BASE + 0x00000000UL)
#define APB2_BASE           (PERIPH_BASE + 0x00010000UL)
#define APB3_BASE           (PERIPH_BASE + 0x12000000UL)
#define APB4_BASE           (PERIPH_BASE + 0x12020000UL)

/*===========================================================================
 * RCC — Reset and Clock Control
 *===========================================================================*/

#define RCC_BASE            (AHB3_BASE + 0x00004400UL)

typedef struct {
    volatile uint32_t CR;           /* 0x00: Clock control */
    volatile uint32_t ICSCR;        /* 0x04: Internal clock source calibration */
    volatile uint32_t CFGR;         /* 0x10: Clock configuration */
    volatile uint32_t D1CFGR;       /* 0x18: Domain 1 config */
    volatile uint32_t D2CFGR;       /* 0x1C: Domain 2 config */
    volatile uint32_t D3CFGR;       /* 0x20: Domain 3 config */
    volatile uint32_t PLLCKSELR;    /* 0x28: PLL clock source selection */
    volatile uint32_t PLLCFGR;      /* 0x2C: PLL config (legacy) */
    volatile uint32_t PLL1DIVR;     /* 0x30: PLL1 dividers */
    volatile uint32_t PLL1FRACR;    /* 0x34: PLL1 fractional */
    volatile uint32_t PLL2DIVR;     /* 0x38: PLL2 dividers */
    volatile uint32_t PLL2FRACR;    /* 0x3C: PLL2 fractional */
    volatile uint32_t PLL3DIVR;     /* 0x40: PLL3 dividers */
    volatile uint32_t PLL3FRACR;    /* 0x44: PLL3 fractional */
    volatile uint32_t CIER;         /* 0x48: Clock interrupt enable */
    volatile uint32_t CIFR;         /* 0x4C: Clock interrupt flag */
    volatile uint32_t CICR;         /* 0x50: Clock interrupt clear */
    uint32_t _pad1[32];
    volatile uint32_t AHB3ENR;      /* 0xD4: AHB3 peripheral clock enable */
    volatile uint32_t AHB1ENR;      /* 0xD8: AHB1 peripheral clock enable */
    volatile uint32_t AHB2ENR;      /* 0xDC: AHB2 peripheral clock enable */
    volatile uint32_t AHB4ENR;      /* 0xE0: AHB4 peripheral clock enable */
    volatile uint32_t APB1LENR;     /* 0xE4: APB1 low enable */
    volatile uint32_t APB1HENR;     /* 0xE8: APB1 high enable */
    volatile uint32_t APB2ENR;      /* 0xEC: APB2 enable */
    volatile uint32_t APB3ENR;      /* 0xF0: APB3 enable */
    volatile uint32_t APB4ENR;      /* 0xF4: APB4 enable */
} RCC_TypeDef;

#define RCC                 ((RCC_TypeDef *)RCC_BASE)

/* RCC_CR bits */
#define RCC_CR_HSION        BIT(0)
#define RCC_CR_HSIRDY       BIT(2)
#define RCC_CR_HSEON        BIT(16)
#define RCC_CR_HSERDY       BIT(17)
#define RCC_CR_PLL1ON       BIT(24)
#define RCC_CR_PLL1RDY      BIT(25)
#define RCC_CR_PLL2ON       BIT(26)
#define RCC_CR_PLL2RDY      BIT(27)
#define RCC_CR_PLL3ON       BIT(28)
#define RCC_CR_PLL3RDY      BIT(29)

/* RCC_CFGR bits */
#define RCC_CFGR_SW_Pos     0
#define RCC_CFGR_SW_Msk     (0x7UL << RCC_CFGR_SW_Pos)
#define RCC_CFGR_SW_HSI     0
#define RCC_CFGR_SW_HSE     1
#define RCC_CFGR_SW_PLL1    3
#define RCC_CFGR_SWS_Pos    3
#define RCC_CFGR_SWS_Msk    (0x7UL << RCC_CFGR_SWS_Pos)
#define RCC_CFGR_SWS_PLL1   (3UL << RCC_CFGR_SWS_Pos)

/* RCC_PLLCKSELR bits */
#define RCC_PLLCKSELR_DIVM1_Pos  0
#define RCC_PLLCKSELR_DIVM2_Pos  8
#define RCC_PLLCKSELR_DIVM3_Pos  16
#define RCC_PLLCKSELR_PLLSRC_Pos 20
#define RCC_PLLCKSELR_PLLSRC_HSE (2UL << RCC_PLLCKSELR_PLLSRC_Pos)

/* RCC_PLL1DIVR bits */
#define RCC_PLL1DIVR_DIVN1_Pos   0
#define RCC_PLL1DIVR_DIVP1_Pos   9
#define RCC_PLL1DIVR_DIVQ1_Pos   16
#define RCC_PLL1DIVR_DIVR1_Pos   24

/* RCC_D1CFGR bits */
#define RCC_D1CFGR_D1CPRE_Pos    0
#define RCC_D1CFGR_D1CPRE_1      (8UL << RCC_D1CFGR_D1CPRE_Pos)  /* /2 */
#define RCC_D1CFGR_D1PPRE_Pos    4
#define RCC_D1CFGR_D1PPRE_1      (8UL << RCC_D1CFGR_D1PPRE_Pos)  /* /2 */

/* RCC_D2CFGR bits */
#define RCC_D2CFGR_D2PPRE1_Pos   4
#define RCC_D2CFGR_D2PPRE1_3     (8UL << RCC_D2CFGR_D2PPRE1_Pos)  /* /4 */
#define RCC_D2CFGR_D2PPRE2_Pos   8
#define RCC_D2CFGR_D2PPRE2_1     (8UL << RCC_D2CFGR_D2PPRE2_Pos)  /* /2 */

/* RCC_D3CFGR bits */
#define RCC_D3CFGR_D3PPRE_Pos    4
#define RCC_D3CFGR_D3PPRE_1      (8UL << RCC_D3CFGR_D3PPRE_Pos)  /* /2 */

/* RCC_AHB3ENR bits */
#define RCC_AHB3ENR_FMCEN        BIT(0)
#define RCC_AHB3ENR_QSPIEN       BIT(8)
#define RCC_AHB3ENR_SDMMC1EN     BIT(16)
#define RCC_AHB3ENR_SDMMC2EN     BIT(17)
#define RCC_AHB3ENR_MDMAEN       BIT(31)

/* RCC_AHB1ENR bits */
#define RCC_AHB1ENR_DMA1EN       BIT(0)
#define RCC_AHB1ENR_DMA2EN       BIT(1)
#define RCC_AHB1ENR_USB1OTGHSEN  BIT(27)
#define RCC_AHB1ENR_USB2OTGHSEN  BIT(31)

/* RCC_AHB2ENR bits */
#define RCC_AHB2ENR_HASHEN       BIT(5)

/* RCC_AHB4ENR bits */
#define RCC_AHB4ENR_GPIOAEN      BIT(0)
#define RCC_AHB4ENR_GPIOBEN      BIT(1)
#define RCC_AHB4ENR_GPIOCEN      BIT(2)
#define RCC_AHB4ENR_GPIODEN      BIT(3)
#define RCC_AHB4ENR_GPIOEEN      BIT(4)
#define RCC_AHB4ENR_GPIOFEN      BIT(5)
#define RCC_AHB4ENR_GPIOGEN      BIT(6)
#define RCC_AHB4ENR_GPIOHEN      BIT(7)
#define RCC_AHB4ENR_GPIOIEN      BIT(8)

/* RCC_APB1LENR bits */
#define RCC_APB1LENR_TIM2EN      BIT(0)
#define RCC_APB1LENR_I2C1EN      BIT(21)

/* RCC_APB2ENR bits */
#define RCC_APB2ENR_TIM1EN       BIT(0)
#define RCC_APB2ENR_SPI1EN       BIT(12)

/* RCC_APB4ENR bits */
#define RCC_APB4ENR_RTCAPBEN     BIT(16)

/*===========================================================================
 * GPIO — General Purpose I/O
 *===========================================================================*/

typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2];
} GPIO_TypeDef;

#define GPIOA               ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB               ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC               ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD               ((GPIO_TypeDef *)GPIOD_BASE)
#define GPIOE               ((GPIO_TypeDef *)GPIOE_BASE)
#define GPIOF               ((GPIO_TypeDef *)GPIOF_BASE)
#define GPIOG               ((GPIO_TypeDef *)GPIOG_BASE)
#define GPIOH               ((GPIO_TypeDef *)GPIOH_BASE)
#define GPIOI               ((GPIO_TypeDef *)GPIOI_BASE)

/* GPIO_MODER values per pin */
#define GPIO_MODE_INPUT     0x0
#define GPIO_MODE_OUTPUT    0x1
#define GPIO_MODE_AF        0x2
#define GPIO_MODE_ANALOG    0x3

/* GPIO_OTYPER */
#define GPIO_OTYPE_PP       0x0
#define GPIO_OTYPE_OD       0x1

/* GPIO_OSPEEDR */
#define GPIO_SPEED_LOW      0x0
#define GPIO_SPEED_MEDIUM   0x1
#define GPIO_SPEED_HIGH     0x2
#define GPIO_SPEED_VERYHIGH 0x3

/* GPIO_PUPDR */
#define GPIO_PUPD_NONE      0x0
#define GPIO_PUPD_PU        0x1
#define GPIO_PUPD_PD        0x2

/*===========================================================================
 * FMC — Flexible Memory Controller
 *===========================================================================*/

#define FMC_BASE            (AHB3_BASE + 0x00004000UL)

typedef struct {
    uint32_t _pad1[32];             /* 0x00-0x7F: NAND/PC Card registers */
    volatile uint32_t PCR;          /* 0x80: NAND control */
    volatile uint32_t SR;           /* 0x84: NAND status */
    volatile uint32_t PMEM;         /* 0x88: NAND common memory timing */
    volatile uint32_t PATT;         /* 0x8C: NAND attribute memory timing */
    uint32_t _pad2[1];
    volatile uint32_t ECCR;         /* 0x94: NAND ECC result */
    uint32_t _pad3[42];
    volatile uint32_t SDCR1;        /* 0x140: SDRAM control 1 */
    volatile uint32_t SDCR2;        /* 0x144: SDRAM control 2 */
    volatile uint32_t SDTR1;        /* 0x148: SDRAM timing 1 */
    volatile uint32_t SDTR2;        /* 0x14C: SDRAM timing 2 */
    volatile uint32_t SDCMR;        /* 0x150: SDRAM command mode */
    volatile uint32_t SDRTR;        /* 0x154: SDRAM refresh timer */
    volatile uint32_t SDSR;         /* 0x158: SDRAM status */
} FMC_TypeDef;

#define FMC                 ((FMC_TypeDef *)FMC_BASE)

/* FMC_SDCR1 bits */
#define FMC_SDCR1_NC_Pos    0
#define FMC_SDCR1_NC_0      (0UL << FMC_SDCR1_NC_Pos)   /* 8 columns */
#define FMC_SDCR1_NC_1      (1UL << FMC_SDCR1_NC_Pos)   /* 9 columns */
#define FMC_SDCR1_NR_Pos    2
#define FMC_SDCR1_NR_0      (0UL << FMC_SDCR1_NR_Pos)   /* 11 rows */
#define FMC_SDCR1_NR_1      (1UL << FMC_SDCR1_NR_Pos)   /* 12 rows */
#define FMC_SDCR1_NR_2      (2UL << FMC_SDCR1_NR_Pos)   /* 13 rows */
#define FMC_SDCR1_MWID_Pos  4
#define FMC_SDCR1_MWID_0    (0UL << FMC_SDCR1_MWID_Pos)  /* 8-bit */
#define FMC_SDCR1_MWID_1    (1UL << FMC_SDCR1_MWID_Pos)  /* 16-bit */
#define FMC_SDCR1_NB        BIT(6)                       /* 1=internal bank */
#define FMC_SDCR1_CAS_Pos   7
#define FMC_SDCR1_CAS_0     (0UL << FMC_SDCR1_CAS_Pos)   /* CL=1 */
#define FMC_SDCR1_CAS_1     (1UL << FMC_SDCR1_CAS_Pos)   /* CL=2 */
#define FMC_SDCR1_CAS_2     (2UL << FMC_SDCR1_CAS_Pos)   /* CL=3 */
#define FMC_SDCR1_SDCLK_Pos 10
#define FMC_SDCR1_SDCLK_0   (0UL << FMC_SDCR1_SDCLK_Pos) /* HCLK/2 */
#define FMC_SDCR1_SDCLK_1   (1UL << FMC_SDCR1_SDCLK_Pos) /* HCLK */
#define FMC_SDCR1_RBURST    BIT(12)
#define FMC_SDCR1_RPIPE_Pos 13
#define FMC_SDCR1_RPIPE_0   (0UL << FMC_SDCR1_RPIPE_Pos)
#define FMC_SDCR1_RPIPE_1   (1UL << FMC_SDCR1_RPIPE_Pos)
#define FMC_SDCR1_RA        BIT(15)  /* Refresh auto */

/* FMC_SDTR1 bits */
#define FMC_SDTR1_TMRD_Pos  0
#define FMC_SDTR1_TXSR_Pos  4
#define FMC_SDTR1_TRAS_Pos  8
#define FMC_SDTR1_TRC_Pos   12
#define FMC_SDTR1_TWR_Pos   16
#define FMC_SDTR1_TRP_Pos   20
#define FMC_SDTR1_TRCD_Pos  24

/* FMC_SDTR2 bits */
#define FMC_SDTR2_TRAS_Pos  0
#define FMC_SDTR2_TRC_Pos   4
#define FMC_SDTR2_TWR_Pos   8
#define FMC_SDTR2_TRP_Pos   12
#define FMC_SDTR2_TRCD_Pos  16

/* FMC_SDCMR bits */
#define FMC_SDCMR_MODE_Pos  0
#define FMC_SDCMR_MODE_000  (0UL << FMC_SDCMR_MODE_Pos)  /* Normal */
#define FMC_SDCMR_MODE_001  (1UL << FMC_SDCMR_MODE_Pos)  /* Precharge all */
#define FMC_SDCMR_MODE_010  (2UL << FMC_SDCMR_MODE_Pos)  /* Auto-refresh */
#define FMC_SDCMR_MODE_011  (3UL << FMC_SDCMR_MODE_Pos)  /* Load mode reg */
#define FMC_SDCMR_MODE_100  (4UL << FMC_SDCMR_MODE_Pos)  /* Self-refresh */
#define FMC_SDCMR_MODE_101  (5UL << FMC_SDCMR_MODE_Pos)  /* Power-down */
#define FMC_SDCMR_CTB1      BIT(4)
#define FMC_SDCMR_CTB2      BIT(5)
#define FMC_SDCMR_NRFS_Pos  5
#define FMC_SDCMR_MRD_Pos   9

/* FMC_SDRTR bits */
#define FMC_SDRTR_COUNT_Pos 0
#define FMC_SDRTR_CRE       BIT(15)

/*===========================================================================
 * SDMMC2 — eMMC/SD Card Interface
 *===========================================================================*/

#define SDMMC2_BASE         (AHB3_BASE + 0x00007400UL)

typedef struct {
    volatile uint32_t POWER;        /* 0x00 */
    volatile uint32_t CLKCR;        /* 0x04 */
    volatile uint32_t ARG;          /* 0x08 */
    volatile uint32_t CMD;          /* 0x0C */
    volatile const uint32_t RESPCMD;/* 0x10 */
    volatile const uint32_t RESP1;  /* 0x14 */
    volatile const uint32_t RESP2;  /* 0x18 */
    volatile const uint32_t RESP3;  /* 0x1C */
    volatile const uint32_t RESP4;  /* 0x20 */
    volatile uint32_t DTIMER;       /* 0x24 */
    volatile uint32_t DLEN;         /* 0x28 */
    volatile uint32_t DCTRL;        /* 0x2C */
    volatile const uint32_t DCOUNT; /* 0x30 */
    volatile const uint32_t STA;    /* 0x34 */
    volatile uint32_t ICR;          /* 0x38 */
    volatile uint32_t MASK;         /* 0x3C */
    uint32_t _pad1[2];
    volatile const uint32_t FIFOCNT;/* 0x48 */
    uint32_t _pad2[13];
    volatile uint32_t FIFO;         /* 0x80 */
} SDMMC_TypeDef;

#define SDMMC2              ((SDMMC_TypeDef *)SDMMC2_BASE)
#define SDMMC1              ((SDMMC_TypeDef *)(AHB3_BASE + 0x00007000UL))

/* SDMMC_CLKCR bits */
#define SDMMC_CLKCR_CLKDIV_Pos  0
#define SDMMC_CLKCR_CLKEN       BIT(8)
#define SDMMC_CLKCR_PWRSAV      BIT(9)
#define SDMMC_CLKCR_WIDBUS_Pos  14
#define SDMMC_CLKCR_WIDBUS_1    (0UL << SDMMC_CLKCR_WIDBUS_Pos)
#define SDMMC_CLKCR_WIDBUS_4    (1UL << SDMMC_CLKCR_WIDBUS_Pos)
#define SDMMC_CLKCR_WIDBUS_8    (2UL << SDMMC_CLKCR_WIDBUS_Pos)
#define SDMMC_CLKCR_NEGEDGE     BIT(16)
#define SDMMC_CLKCR_HWFC_EN     BIT(17)
#define SDMMC_CLKCR_DDR         BIT(18)
#define SDMMC_CLKCR_BUSSPEED    BIT(19)

/* SDMMC_CMD bits */
#define SDMMC_CMD_CMDINDEX_Pos  0
#define SDMMC_CMD_WAITRESP_Pos  6
#define SDMMC_CMD_WAITRESP_SHORT (1UL << SDMMC_CMD_WAITRESP_Pos)
#define SDMMC_CMD_WAITRESP_LONG  (3UL << SDMMC_CMD_WAITRESP_Pos)
#define SDMMC_CMD_WAITINT       BIT(8)
#define SDMMC_CMD_CPSMEN        BIT(10)
#define SDMMC_CMD_CMDSUSPEND    BIT(11)

/* SDMMC_DCTRL bits */
#define SDMMC_DCTRL_DTEN        BIT(0)
#define SDMMC_DCTRL_DTDIR       BIT(1)
#define SDMMC_DCTRL_DTMODE      BIT(2)
#define SDMMC_DCTRL_DBLOCKSIZE_Pos 4

/* SDMMC_STA bits */
#define SDMMC_STA_CCRCFAIL      BIT(0)
#define SDMMC_STA_DCRCFAIL      BIT(1)
#define SDMMC_STA_CTIMEOUT      BIT(2)
#define SDMMC_STA_DTIMEOUT      BIT(3)
#define SDMMC_STA_TXUNDERR      BIT(4)
#define SDMMC_STA_RXOVERR       BIT(5)
#define SDMMC_STA_CMDREND       BIT(6)
#define SDMMC_STA_CMDSENT       BIT(7)
#define SDMMC_STA_DATAEND       BIT(8)
#define SDMMC_STA_DHOLD         BIT(9)
#define SDMMC_STA_DBCKEND       BIT(10)
#define SDMMC_STA_DABORT        BIT(11)
#define SDMMC_STA_BUSYD0        BIT(20)
#define SDMMC_STA_BUSYD0END     BIT(21)

/*===========================================================================
 * OCTOSPI1 — Octal SPI / Quad SPI Interface
 *===========================================================================*/

#define OCTOSPI1_BASE       (AHB3_BASE + 0x00005000UL)

typedef struct {
    volatile uint32_t CR;           /* 0x00 */
    volatile uint32_t DCR1;         /* 0x08 */
    volatile uint32_t DCR2;         /* 0x0C */
    volatile uint32_t DCR3;         /* 0x10 */
    volatile uint32_t DCR4;         /* 0x14 */
    volatile uint32_t DLR;          /* 0x20 */
    volatile uint32_t AR;           /* 0x30 */
    volatile uint32_t CCR;          /* 0x40 */
    volatile uint32_t TCR;          /* 0x44 */
    volatile uint32_t IR;           /* 0x48 */
    volatile uint32_t ABR;          /* 0x50 */
    volatile const uint32_t SR;     /* 0x50 (overlaps ABR) */
    volatile uint32_t FCR;          /* 0x54 */
    uint32_t _pad1[2];
    volatile uint32_t DR;           /* 0x60 */
    uint32_t _pad2[7];
    volatile uint32_t PSMKR;        /* 0x80 */
    volatile uint32_t PSMAR;        /* 0x84 */
    volatile uint32_t PIR;          /* 0x88 */
    volatile uint32_t LPTR;         /* 0x8C */
} OCTOSPI_TypeDef;

#define OCTOSPI1            ((OCTOSPI_TypeDef *)OCTOSPI1_BASE)

/* OCTOSPI_CR bits */
#define OCTOSPI_CR_EN          BIT(0)
#define OCTOSPI_CR_ABORT       BIT(1)
#define OCTOSPI_CR_DMAEN       BIT(2)
#define OCTOSPI_CR_TCEN        BIT(3)
#define OCTOSPI_CR_FTHRES_Pos  8

/* OCTOSPI_DCR1 bits */
#define OCTOSPI_DCR1_MTYP_Pos  0
#define OCTOSPI_DCR1_MTYP_SPI  (0UL << OCTOSPI_DCR1_MTYP_Pos)
#define OCTOSPI_DCR1_DEVSIZE_Pos 16
#define OCTOSPI_DCR1_CSHT_Pos  8

/* OCTOSPI_CCR bits */
#define OCTOSPI_CCR_IMODE_Pos  0
#define OCTOSPI_CCR_IMODE_NONE (0UL << OCTOSPI_CCR_IMODE_Pos)
#define OCTOSPI_CCR_IMODE_SINGLE (1UL << OCTOSPI_CCR_IMODE_Pos)
#define OCTOSPI_CCR_IMODE_QUAD (3UL << OCTOSPI_CCR_IMODE_Pos)
#define OCTOSPI_CCR_ADMODE_Pos 4
#define OCTOSPI_CCR_ADMODE_NONE (0UL << OCTOSPI_CCR_ADMODE_Pos)
#define OCTOSPI_CCR_ADMODE_SINGLE (1UL << OCTOSPI_CCR_ADMODE_Pos)
#define OCTOSPI_CCR_ADMODE_QUAD (3UL << OCTOSPI_CCR_ADMODE_Pos)
#define OCTOSPI_CCR_ABMODE_Pos 8
#define OCTOSPI_CCR_DMODE_Pos  12
#define OCTOSPI_CCR_DMODE_NONE (0UL << OCTOSPI_CCR_DMODE_Pos)
#define OCTOSPI_CCR_DMODE_SINGLE (1UL << OCTOSPI_CCR_DMODE_Pos)
#define OCTOSPI_CCR_DMODE_QUAD (3UL << OCTOSPI_CCR_DMODE_Pos)
#define OCTOSPI_CCR_FMODE_Pos  16
#define OCTOSPI_CCR_FMODE_INDWR (0UL << OCTOSPI_CCR_FMODE_Pos)
#define OCTOSPI_CCR_FMODE_INDRD (1UL << OCTOSPI_CCR_FMODE_Pos)
#define OCTOSPI_CCR_FMODE_AUTOPOLL (2UL << OCTOSPI_CCR_FMODE_Pos)
#define OCTOSPI_CCR_FMODE_MEMMAP (3UL << OCTOSPI_CCR_FMODE_Pos)

/* OCTOSPI_SR bits */
#define OCTOSPI_SR_TCF         BIT(0)
#define OCTOSPI_SR_SMF         BIT(1)
#define OCTOSPI_SR_TOF         BIT(2)
#define OCTOSPI_SR_BUSY        BIT(3)

/*===========================================================================
 * USB OTG_HS — USB 2.0 High-Speed OTG with ULPI
 *===========================================================================*/

#define USB_OTG_HS_BASE     (AHB1_BASE + 0x00040000UL)

typedef struct {
    volatile uint32_t GOTGCTL;      /* 0x000 */
    volatile uint32_t GOTGINT;      /* 0x004 */
    volatile uint32_t GAHBCFG;      /* 0x008 */
    volatile uint32_t GUSBCFG;      /* 0x00C */
    volatile uint32_t GRSTCTL;      /* 0x010 */
    volatile uint32_t GINTSTS;      /* 0x014 */
    volatile uint32_t GINTMSK;      /* 0x018 */
    volatile const uint32_t GRXSTSR;/* 0x01C */
    volatile const uint32_t GRXSTSP;/* 0x020 */
    volatile uint32_t GRXFSIZ;      /* 0x024 */
    volatile uint32_t DIEPTXF0;     /* 0x028 */
    volatile uint32_t HNPTXSTS;     /* 0x02C */
    uint32_t _pad1[2];
    volatile uint32_t GCCFG;        /* 0x038 */
    volatile uint32_t CID;          /* 0x03C */
    uint32_t _pad2[48];
    volatile uint32_t HPTXFSIZ;     /* 0x100 */
    volatile uint32_t DIEPTXF[15];  /* 0x104-0x13C */
    uint32_t _pad3[176];
    volatile uint32_t DCFG;         /* 0x800 */
    volatile uint32_t DCTL;         /* 0x804 */
    volatile const uint32_t DSTS;   /* 0x808 */
    uint32_t _pad4[1];
    volatile uint32_t DIEPMSK;      /* 0x810 */
    volatile uint32_t DOEPMSK;      /* 0x814 */
    volatile const uint32_t DAINT;  /* 0x818 */
    volatile uint32_t DAINTMSK;     /* 0x81C */
    uint32_t _pad5[56];
    volatile uint32_t DIEPCTL[16];  /* 0x900-0x93C */
    uint32_t _pad6[16];
    volatile uint32_t DIEPTSIZ[16]; /* 0x910-0x94C */
    uint32_t _pad7[16];
    volatile uint32_t DIEPDMA[16];  /* 0x914-0x950 */
    uint32_t _pad8[172];
    volatile uint32_t DOEPCTL[16];  /* 0xB00-0xB3C */
    uint32_t _pad9[16];
    volatile uint32_t DOEPTSIZ[16]; /* 0xB10-0xB4C */
    uint32_t _pad10[16];
    volatile uint32_t DOEPDMA[16];  /* 0xB14-0xB50 */
} USB_OTG_HS_TypeDef;

#define USB_OTG_HS          ((USB_OTG_HS_TypeDef *)USB_OTG_HS_BASE)

/* GAHBCFG bits */
#define USB_GAHBCFG_GINT       BIT(0)
#define USB_GAHBCFG_DMAEN      BIT(5)
#define USB_GAHBCFG_TXFELVL    BIT(7)
#define USB_GAHBCFG_PTXFELVL   BIT(8)

/* GUSBCFG bits */
#define USB_GUSBCFG_TOCAL_Pos  0
#define USB_GUSBCFG_PHYSEL     BIT(6)
#define USB_GUSBCFG_ULPI_UTMI_SEL BIT(4)
#define USB_GUSBCFG_ULPI_CLK_SUSP BIT(11)

/* GRSTCTL bits */
#define USB_GRSTCTL_CSRST      BIT(0)
#define USB_GRSTCTL_TXFFLSH    BIT(5)
#define USB_GRSTCTL_RXFFLSH    BIT(4)

/* GINTSTS/GINTMSK bits */
#define USB_GINTSTS_CMOD       BIT(0)
#define USB_GINTSTS_USBRST     BIT(6)
#define USB_GINTSTS_ENUMDNE    BIT(7)
#define USB_GINTSTS_IEPINT     BIT(18)
#define USB_GINTSTS_OEPINT     BIT(19)
#define USB_GINTSTS_RXFLVL     BIT(4)

/* DCFG bits */
#define USB_DCFG_DSPD_Pos      0
#define USB_DCFG_DSPD_HS       (0UL << USB_DCFG_DSPD_Pos)
#define USB_DCFG_DSPD_FS       (1UL << USB_DCFG_DSPD_Pos)
#define USB_DCFG_NZLSOHSK      BIT(2)
#define USB_DCFG_DAD_Pos       4

/* DCTL bits */
#define USB_DCTL_RWUSIG        BIT(0)
#define USB_DCTL_SDIS          BIT(1)
#define USB_DCTL_GINSTS        BIT(2)
#define USB_DCTL_GONSTS        BIT(3)
#define USB_DCTL_POPRGDNE      BIT(11)

/* DIEPCTL bits */
#define USB_DIEPCTL_MPS_Pos    0
#define USB_DIEPCTL_USBAEP     BIT(15)
#define USB_DIEPCTL_EPENA      BIT(31)
#define USB_DIEPCTL_EPDIS      BIT(30)
#define USB_DIEPCTL_SD0PID     BIT(28)
#define USB_DIEPCTL_SNAK       BIT(27)
#define USB_DIEPCTL_CNAK       BIT(26)
#define USB_DIEPCTL_TXFNUM_Pos 22
#define USB_DIEPCTL_STALL      BIT(21)
#define USB_DIEPCTL_EPTYP_Pos  18
#define USB_DIEPCTL_EPTYP_BULK (2UL << USB_DIEPCTL_EPTYP_Pos)

/* DOEPCTL bits */
#define USB_DOEPCTL_MPS_Pos    0
#define USB_DOEPCTL_USBAEP     BIT(15)
#define USB_DOEPCTL_EPENA      BIT(31)
#define USB_DOEPCTL_EPDIS      BIT(30)
#define USB_DOEPCTL_SD0PID     BIT(28)
#define USB_DOEPCTL_SNAK       BIT(27)
#define USB_DOEPCTL_CNAK       BIT(26)
#define USB_DOEPCTL_STALL      BIT(21)
#define USB_DOEPCTL_EPTYP_Pos  18
#define USB_DOEPCTL_EPTYP_BULK (2UL << USB_DOEPCTL_EPTYP_Pos)

/* DIEPTSIZ bits */
#define USB_DIEPTSIZ_XFRSIZ_Pos 0
#define USB_DIEPTSIZ_PKTCNT_Pos 19
#define USB_DIEPTSIZ_MCNT_Pos   29

/*===========================================================================
 * HASH — Hardware SHA-256 Accelerator
 *===========================================================================*/

#define HASH_BASE           (AHB2_BASE + 0x00060400UL)

typedef struct {
    volatile uint32_t CR;           /* 0x00 */
    volatile uint32_t DIN;          /* 0x04 */
    volatile uint32_t STR;          /* 0x08 */
    uint32_t _pad1[1];
    volatile const uint32_t HR[8];  /* 0x10-0x2C: Digest */
    volatile uint32_t IMR;          /* 0x30 */
    volatile const uint32_t SR;     /* 0x34 */
    uint32_t _pad2[48];
    volatile uint32_t CSR[54];      /* 0xF8-0x1CC: Context swap */
} HASH_TypeDef;

#define HASH                ((HASH_TypeDef *)HASH_BASE)

/* HASH_CR bits */
#define HASH_CR_INIT            BIT(2)
#define HASH_CR_MODE            BIT(3)
#define HASH_CR_ALGO_Pos        7
#define HASH_CR_ALGO_SHA256     BIT(7)
#define HASH_CR_DATATYPE_Pos    4
#define HASH_CR_DATATYPE_32B    (0UL << HASH_CR_DATATYPE_Pos)
#define HASH_CR_DMAE            BIT(9)

/* HASH_STR bits */
#define HASH_STR_NBLW_Pos       0
#define HASH_STR_DCAL            BIT(8)

/* HASH_SR bits */
#define HASH_SR_BUSY            BIT(0)
#define HASH_SR_DMAS            BIT(1)
#define HASH_SR_DCIS            BIT(2)
#define HASH_SR_DINIS           BIT(3)

/*===========================================================================
 * MDMA — Master DMA Controller
 *===========================================================================*/

#define MDMA_BASE           (AHB3_BASE + 0x00020000UL)

typedef struct {
    volatile const uint32_t GISR0;  /* 0x000 */
    uint32_t _pad1[15];
    volatile uint32_t C0CR;         /* 0x040 */
    volatile uint32_t C0TCR;        /* 0x044 */
    volatile uint32_t C0BNDTR;      /* 0x048 */
    volatile uint32_t C0SAR;        /* 0x04C */
    volatile uint32_t C0DAR;        /* 0x050 */
    volatile uint32_t C0BRUR;       /* 0x054 */
    volatile uint32_t C0LAR;        /* 0x058 */
    volatile uint32_t C0TBR;        /* 0x05C */
    uint32_t _pad2[4];
    volatile uint32_t C0MDR;        /* 0x070 */
    volatile uint32_t C0MDR2;       /* 0x074 */
} MDMA_Channel_TypeDef;

typedef struct {
    volatile const uint32_t GISR0;  /* 0x000 */
    uint32_t _pad[63];
    MDMA_Channel_TypeDef CH[8];     /* 0x040-0x0FC per channel */
} MDMA_TypeDef;

#define MDMA                ((MDMA_TypeDef *)MDMA_BASE)
#define MDMA_Channel0       (&MDMA->CH[0])
#define MDMA_Channel1       (&MDMA->CH[1])
#define MDMA_Channel2       (&MDMA->CH[2])
#define MDMA_Channel3       (&MDMA->CH[3])
#define MDMA_Channel4       (&MDMA->CH[4])

/* MDMA_CxCR bits */
#define MDMA_CCR_EN             BIT(0)
#define MDMA_CCR_TEIE           BIT(1)
#define MDMA_CCR_CTCIE          BIT(2)
#define MDMA_CCR_BRTIE          BIT(3)
#define MDMA_CCR_PRIO_Pos       8
#define MDMA_CCR_BURST_Pos      12
#define MDMA_CCR_DIR_Pos        16
#define MDMA_CCR_DIR_MEM2PER    (1UL << MDMA_CCR_DIR_Pos)
#define MDMA_CCR_DIR_PER2MEM    (2UL << MDMA_CCR_DIR_Pos)

/* MDMA_CxTCR bits */
#define MDMA_CTCR_SINC_Pos      0
#define MDMA_CTCR_SINC_0        (0UL << MDMA_CTCR_SINC_Pos)  /* +4 */
#define MDMA_CTCR_DINC_Pos      2
#define MDMA_CTCR_DINC_0        (0UL << MDMA_CTCR_DINC_Pos)  /* +4 */
#define MDMA_CTCR_SWAP_Pos      4
#define MDMA_CTCR_SWAP_0        (0UL << MDMA_CTCR_SWAP_Pos)  /* No swap */
#define MDMA_CTCR_TRG_Pos       16

/* MDMA_GISR0 bits */
#define MDMA_GISR0_TCIF0        BIT(0)
#define MDMA_GISR0_TCIF1        BIT(6)
#define MDMA_GISR0_TCIF2        BIT(12)
#define MDMA_GISR0_TCIF3        BIT(18)
#define MDMA_GISR0_TCIF4        BIT(24)

/*===========================================================================
 * NVIC — Nested Vectored Interrupt Controller
 *===========================================================================*/

#define NVIC_BASE           0xE000E100UL

typedef struct {
    volatile uint32_t ISER[8];      /* 0x000-0x01C: Interrupt set-enable */
    uint32_t _pad1[24];
    volatile uint32_t ICER[8];      /* 0x080-0x09C: Interrupt clear-enable */
    uint32_t _pad2[24];
    volatile uint32_t ISPR[8];      /* 0x100-0x11C: Interrupt set-pending */
    uint32_t _pad3[24];
    volatile uint32_t ICPR[8];      /* 0x180-0x19C: Interrupt clear-pending */
    uint32_t _pad4[24];
    volatile uint32_t IABR[8];      /* 0x200-0x21C: Interrupt active bit */
    uint32_t _pad5[56];
    volatile uint8_t  IP[240];      /* 0x300-0x3EF: Interrupt priority */
    uint32_t _pad6[644];
    volatile uint32_t STIR;         /* 0xE00: Software trigger interrupt */
} NVIC_TypeDef;

#define NVIC                ((NVIC_TypeDef *)NVIC_BASE)

/*===========================================================================
 * SCB — System Control Block
 *===========================================================================*/

#define SCB_BASE            0xE000ED00UL

typedef struct {
    volatile const uint32_t CPUID;  /* 0x00 */
    volatile uint32_t ICSR;         /* 0x04 */
    volatile uint32_t VTOR;         /* 0x08 */
    volatile uint32_t AIRCR;        /* 0x0C */
    volatile uint32_t SCR;          /* 0x10 */
    volatile const uint32_t CCR;    /* 0x14 */
    volatile uint8_t  SHPR[12];     /* 0x18-0x23 */
    volatile uint32_t SHCSR;        /* 0x24 */
    volatile uint32_t CFSR;         /* 0x28 */
    volatile uint32_t HFSR;         /* 0x2C */
    uint32_t _pad1[1];
    volatile uint32_t MMFSR;        /* 0x30 (alias) */
    volatile uint32_t BFSR;         /* 0x30 (alias) */
    volatile uint32_t UFSR;         /* 0x30 (alias) */
    uint32_t _pad2[1];
    volatile uint32_t CPACR;        /* 0x88: Coprocessor access control */
} SCB_TypeDef;

#define SCB                 ((SCB_TypeDef *)SCB_BASE)

/* SCB_CPACR bits */
#define SCB_CPACR_CP10_FULL (3UL << 20)
#define SCB_CPACR_CP11_FULL (3UL << 22)

/*===========================================================================
 * SYSTICK — System Timer
 *===========================================================================*/

#define SYSTICK_BASE        0xE000E010UL

typedef struct {
    volatile uint32_t CSR;          /* 0x00 */
    volatile uint32_t RVR;          /* 0x04 */
    volatile uint32_t CVR;          /* 0x08 */
    volatile const uint32_t CALIB;  /* 0x0C */
} SYSTICK_TypeDef;

#define SYSTICK             ((SYSTICK_TypeDef *)SYSTICK_BASE)

/* SYSTICK_CSR bits */
#define SYSTICK_CSR_ENABLE      BIT(0)
#define SYSTICK_CSR_TICKINT     BIT(1)
#define SYSTICK_CSR_CLKSOURCE   BIT(2)
#define SYSTICK_CSR_COUNTFLAG   BIT(16)

/*===========================================================================
 * EXTI — External Interrupt Controller
 *===========================================================================*/

#define EXTI_BASE           0x52022000UL

typedef struct {
    volatile uint32_t RTSR1;        /* 0x00: Rising trigger selection */
    volatile uint32_t FTSR1;        /* 0x04: Falling trigger selection */
    volatile uint32_t SWIER1;       /* 0x08: Software interrupt event */
    volatile uint32_t PR1;          /* 0x0C: Pending register */
    uint32_t _pad1[4];
    volatile uint32_t RTSR2;        /* 0x20 */
    volatile uint32_t FTSR2;        /* 0x24 */
    volatile uint32_t SWIER2;       /* 0x28 */
    volatile uint32_t PR2;          /* 0x2C */
} EXTI_TypeDef;

#define EXTI                ((EXTI_TypeDef *)EXTI_BASE)

/* EXTI_PR1 bits */
#define EXTI_PR1_PIF6          BIT(6)

/*===========================================================================
 * PWR — Power Control
 *===========================================================================*/

#define PWR_BASE            0x52024000UL

typedef struct {
    volatile uint32_t CR1;          /* 0x00 */
    volatile uint32_t CSR1;         /* 0x04 */
    volatile uint32_t CR2;          /* 0x08 */
    volatile uint32_t CR3;          /* 0x0C */
    volatile uint32_t CPUCR;        /* 0x10 */
    uint32_t _pad1[1];
    volatile uint32_t D3CR;         /* 0x18 */
} PWR_TypeDef;

#define PWR                 ((PWR_TypeDef *)PWR_BASE)

/* PWR_CR3 bits */
#define PWR_CR3_SCUEN           BIT(10)

/* PWR_D3CR bits */
#define PWR_D3CR_VOS_Pos        14
#define PWR_D3CR_VOS_0          BIT(14)
#define PWR_D3CR_VOS_1          BIT(15)
#define PWR_D3CR_VOSRDY         BIT(13)

/*===========================================================================
 * I2C1 — Inter-Integrated Circuit
 *===========================================================================*/

#define I2C1_BASE           (APB1_BASE + 0x00005400UL)

typedef struct {
    volatile uint32_t CR1;          /* 0x00 */
    volatile uint32_t CR2;          /* 0x04 */
    volatile uint32_t OAR1;         /* 0x08 */
    volatile uint32_t OAR2;         /* 0x0C */
    volatile uint32_t TIMINGR;      /* 0x10 */
    volatile uint32_t TIMEOUTR;     /* 0x14 */
    volatile uint32_t ISR;          /* 0x18 */
    volatile uint32_t ICR;          /* 0x1C */
    volatile uint32_t PECR;         /* 0x20 */
    volatile uint32_t RXDR;         /* 0x24 */
    volatile uint32_t TXDR;         /* 0x28 */
} I2C_TypeDef;

#define I2C1                ((I2C_TypeDef *)I2C1_BASE)

/*===========================================================================
 * SPI1 — Serial Peripheral Interface
 *===========================================================================*/

#define SPI1_BASE           (APB2_BASE + 0x00013000UL)

typedef struct {
    volatile uint32_t CR1;          /* 0x00 */
    volatile uint32_t CR2;          /* 0x04 */
    volatile uint32_t SR;           /* 0x08 */
    volatile uint32_t DR;           /* 0x0C */
    volatile uint32_t CRCPR;        /* 0x10 */
    volatile uint32_t RXCRCR;       /* 0x14 */
    volatile uint32_t TXCRCR;       /* 0x18 */
} SPI_TypeDef;

#define SPI1                ((SPI_TypeDef *)SPI1_BASE)

/*===========================================================================
 * TIM1/TIM2 — Timer Peripherals
 *===========================================================================*/

#define TIM1_BASE           (APB2_BASE + 0x00010000UL)
#define TIM2_BASE           (APB1_BASE + 0x00000000UL)

typedef struct {
    volatile uint32_t CR1;          /* 0x00 */
    volatile uint32_t CR2;          /* 0x04 */
    volatile uint32_t SMCR;         /* 0x08 */
    volatile uint32_t DIER;         /* 0x0C */
    volatile uint32_t SR;           /* 0x10 */
    volatile uint32_t EGR;          /* 0x14 */
    volatile uint32_t CCMR1;        /* 0x18 */
    volatile uint32_t CCMR2;        /* 0x1C */
    volatile uint32_t CCER;         /* 0x20 */
    volatile uint32_t CNT;          /* 0x24 */
    volatile uint32_t PSC;          /* 0x28 */
    volatile uint32_t ARR;          /* 0x2C */
    volatile uint32_t RCR;          /* 0x30 */
    volatile uint32_t CCR1;         /* 0x34 */
    volatile uint32_t CCR2;         /* 0x38 */
    volatile uint32_t CCR3;         /* 0x3C */
    volatile uint32_t CCR4;         /* 0x40 */
    volatile uint32_t BDTR;         /* 0x44 */
    volatile uint32_t DCR;          /* 0x48 */
    volatile uint32_t DMAR;         /* 0x4C */
} TIM_TypeDef;

#define TIM1                ((TIM_TypeDef *)TIM1_BASE)
#define TIM2                ((TIM_TypeDef *)TIM2_BASE)

/*===========================================================================
 * RTC — Real-Time Clock
 *===========================================================================*/

#define RTC_BASE            (APB4_BASE + 0x00002800UL)

typedef struct {
    volatile uint32_t TR;           /* 0x00: Time */
    volatile uint32_t DR;           /* 0x04: Date */
    volatile uint32_t CR;           /* 0x08: Control */
    volatile uint32_t ISR;          /* 0x0C: Init/status */
    volatile uint32_t PRER;         /* 0x10: Prescaler */
    volatile uint32_t WUTR;         /* 0x14: Wakeup timer */
    volatile uint32_t CKCFGR;       /* 0x18: Clock config */
    uint32_t _pad1[1];
    volatile uint32_t ALRMAR;       /* 0x20: Alarm A */
    volatile uint32_t ALRMASSR;     /* 0x24 */
    volatile uint32_t ALRMBR;       /* 0x28: Alarm B */
    volatile uint32_t ALRMBSSR;     /* 0x2C */
    volatile uint32_t WPR;          /* 0x30: Write protection */
    volatile uint32_t SSR;          /* 0x34: Sub-second */
    volatile uint32_t SHIFTR;       /* 0x38: Shift control */
    volatile uint32_t TSTR;         /* 0x3C: Timestamp time */
    volatile uint32_t TSDR;         /* 0x40: Timestamp date */
    volatile uint32_t TSSSR;        /* 0x44: Timestamp sub-second */
    volatile uint32_t CALR;         /* 0x48: Calibration */
    volatile uint32_t TAMPCR;       /* 0x4C: Tamper config */
    volatile uint32_t ALRMASSR2;    /* 0x50 */
    volatile uint32_t ALRMBSSR2;    /* 0x54 */
    volatile uint32_t OR;           /* 0x58: Option register */
    volatile uint32_t BKP0R;        /* 0x5C: Backup register 0 */
} RTC_TypeDef;

#define RTC                 ((RTC_TypeDef *)RTC_BASE)

/*===========================================================================
 * NVIC Interrupt Numbers
 *===========================================================================*/

#define NVIC_IRQ_MDMA            0
#define NVIC_IRQ_DMA1_STREAM0    11
#define NVIC_IRQ_DMA2_STREAM0    56
#define NVIC_IRQ_EXTI9_5         23
#define NVIC_IRQ_SDMMC2          65
#define NVIC_IRQ_SDMMC1          66
#define NVIC_IRQ_OCTOSPI1        68
#define NVIC_IRQ_USB_HS          77
#define NVIC_IRQ_HASH            80
#define NVIC_IRQ_I2C1            31
#define NVIC_IRQ_SPI1            35
#define NVIC_IRQ_TIM1_UP         24
#define NVIC_IRQ_TIM2            28
#define NVIC_IRQ_RTC_WKUP        3
#define NVIC_IRQ_SYSTICK         15

#endif /* REGISTERS_H */
