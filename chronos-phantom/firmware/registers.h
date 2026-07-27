/*
 * registers.h — STM32H753 register definitions for Chronos-Phantom
 * Minimal hand-written register map (no CMSIS dependency for portability).
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef CHRONOS_PHANTOM_REGISTERS_H
#define CHRONOS_PHANTOM_REGISTERS_H

#include <stdint.h>

/* --------------------------------------------------------------------- */
/*  Base addresses                                                        */
/* --------------------------------------------------------------------- */
#define PERIPH_BASE        0x40000000UL
#define PERIPH_BASE_APB2   0x40010000UL
#define PERIPH_BASE_AHB1   0x40018000UL
#define PERIPH_BASE_AHB2   0x48000000UL

/* GPIO ports */
#define GPIOA_BASE         (PERIPH_BASE_AHB2 + 0x0000)
#define GPIOB_BASE         (PERIPH_BASE_AHB2 + 0x0400)
#define GPIOC_BASE         (PERIPH_BASE_AHB2 + 0x0800)
#define GPIOD_BASE         (PERIPH_BASE_AHB2 + 0x0C00)

/* RCC */
#define RCC_BASE           (PERIPH_BASE_AHB1 + 0x0000)

/* PWR */
#define PWR_BASE           (PERIPH_BASE_APB1 + 0x7000)

/* USART1 (APB2) */
#define USART1_BASE        (PERIPH_BASE_APB2 + 0x1000)
#define USART3_BASE       (PERIPH_BASE_APB1 + 0x4800)

/* I2C2 (APB1) */
#define I2C2_BASE          (PERIPH_BASE_APB1 + 0x5800)

/* Ethernet MAC */
#define ETH_BASE           (PERIPH_BASE_AHB1 + 0x2000)

/* DAC1 */
#define DAC1_BASE          (PERIPH_BASE_AHB1 + 0x7400)

/* DMA1 */
#define DMA1_BASE          (PERIPH_BASE_AHB1 + 0x6000)

/* TIM3/TIM4/TIM8 */
#define TIM3_BASE          (PERIPH_BASE_APB1 + 0x0400)
#define TIM4_BASE          (PERIPH_BASE_APB1 + 0x0800)
#define TIM8_BASE          (PERIPH_BASE_APB2 + 0x3400)

/* SYSCFG */
#define SYSCFG_BASE        (PERIPH_BASE_APB2 + 0x0000)

/* --------------------------------------------------------------------- */
/*  GPIO register layout                                                  */
/* --------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t MODER;      /* 0x00 mode                            */
    volatile uint32_t OTYPER;     /* 0x04 output type                     */
    volatile uint32_t OSPEEDR;    /* 0x08 output speed                    */
    volatile uint32_t PUPDR;      /* 0x0C pull-up/down                    */
    volatile uint32_t IDR;        /* 0x10 input data                      */
    volatile uint32_t ODR;        /* 0x14 output data                     */
    volatile uint32_t BSRR;       /* 0x18 bit set/reset                   */
    volatile uint32_t LCKR;       /* 0x1C lock                            */
    volatile uint32_t AFRL;       /* 0x20 alt function low                */
    volatile uint32_t AFRH;       /* 0x24 alt function high               */
} gpio_t;

#define GPIOA   ((gpio_t *)GPIOA_BASE)
#define GPIOB   ((gpio_t *)GPIOB_BASE)
#define GPIOC   ((gpio_t *)GPIOC_BASE)
#define GPIOD   ((gpio_t *)GPIOD_BASE)

/* GPIO mode constants */
#define GPIO_MODE_INPUT     0x00
#define GPIO_MODE_OUTPUT    0x01
#define GPIO_MODE_AF        0x02
#define GPIO_MODE_ANALOG    0x03

#define GPIO_OTYPE_PP       0x00
#define GPIO_OTYPE_OD       0x01

#define GPIO_OSPEED_LOW     0x00
#define GPIO_OSPEED_MED     0x01
#define GPIO_OSPEED_HIGH    0x02
#define GPIO_OSPEED_VHIGH   0x03

#define GPIO_PUPD_NONE      0x00
#define GPIO_PUPD_PULLUP    0x01
#define GPIO_PUPD_PULLDN    0x02

/* --------------------------------------------------------------------- */
/*  RCC register layout (subset)                                          */
/* --------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t CR;          /* 0x00                                 */
    volatile uint32_t HSICFGR;     /* 0x04                                 */
    volatile uint32_t CRRCR;       /* 0x08                                 */
    volatile uint32_t CSICFGR;     /* 0x0C                                 */
    volatile uint32_t CFGR;        /* 0x10                                 */
    volatile uint32_t RESERVED1;    /* 0x14                                 */
    volatile uint32_t D1CFGR;      /* 0x18 (D1 Domain clock config)        */
    volatile uint32_t D2CFGR;      /* 0x1C (D2 Domain clock config)        */
    volatile uint32_t D3CFGR;      /* 0x20                                 */
    volatile uint32_t RESERVED2;   /* 0x24                                 */
    volatile uint32_t CKCFGR;      /* 0x28                                 */
    volatile uint32_t D1CCIPR;     /* 0x2C                                 */
    volatile uint32_t D2CCIP1R;    /* 0x30                                 */
    volatile uint32_t D2CCIP2R;    /* 0x34                                 */
    volatile uint32_t D3CCIPR;     /* 0x38                                 */
    volatile uint32_t RESERVED3;   /* 0x3C                                 */
    volatile uint32_t BDCR;        /* 0x40                                 */
    volatile uint32_t CSR;         /* 0x44                                 */
    volatile uint32_t RESERVED4;   /* 0x48-0x4C                           */
    volatile uint32_t AHB3RSTR;    /* 0x50                                 */
    volatile uint32_t AHB1RSTR;    /* 0x54                                 */
    volatile uint32_t AHB2RSTR;    /* 0x58                                 */
    volatile uint32_t AHB4RSTR;    /* 0x5C                                 */
    volatile uint32_t APB1RSTR;    /* 0x60                                 */
    volatile uint32_t APB1LRSTR;   /* 0x60 (low)                          */
    volatile uint32_t APB1HRSTR;   /* 0x64 (high)                         */
    volatile uint32_t APB2RSTR;    /* 0x68                                 */
    volatile uint32_t RESERVED5;   /* 0x6C-0x7C                           */
    volatile uint32_t AHB3ENR;     /* 0x80                                 */
    volatile uint32_t AHB1ENR;     /* 0x84                                 */
    volatile uint32_t AHB2ENR;     /* 0x88                                 */
    volatile uint32_t AHB4ENR;     /* 0x8C                                 */
    volatile uint32_t APB1LENR;    /* 0x90 (low)                          */
    volatile uint32_t APB1HENR;    /* 0x94 (high)                         */
    volatile uint32_t APB2ENR;     /* 0x98                                 */
} rcc_t;

#define RCC     ((rcc_t *)RCC_BASE)

/* RCC enable bits */
#define RCC_AHB1ENR_DMA1EN       (1 << 0)
#define RCC_AHB1ENR_DMA2EN       (1 << 1)
#define RCC_AHB1ENR_ETHMACEN     (1 << 15)
#define RCC_AHB1ENR_ETHMACTXEN   (1 << 16)
#define RCC_AHB1ENR_ETHMACRXEN   (1 << 17)
#define RCC_AHB1ENR_DAC1EN       (1 << 16) /* placeholder */

#define RCC_AHB2ENR_GPIOAEN      (1 << 0)
#define RCC_AHB2ENR_GPIOBEN      (1 << 1)
#define RCC_AHB2ENR_GPIOCEN      (1 << 2)
#define RCC_AHB2ENR_GPIODEN      (1 << 3)

#define RCC_APB1LENR_USART3EN    (1 << 18)
#define RCC_APB1LENR_I2C2EN      (1 << 22)
#define RCC_APB1LENR_TIM3EN      (1 << 1)
#define RCC_APB1LENR_TIM4EN      (1 << 2)

#define RCC_APB2ENR_USART1EN     (1 << 4)
#define RCC_APB2ENR_TIM8EN       (1 << 1)

/* --------------------------------------------------------------------- */
/*  USART register layout                                                  */
/* --------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t CR1;   /* 0x00 control 1                            */
    volatile uint32_t CR2;   /* 0x04 control 2                            */
    volatile uint32_t CR3;   /* 0x08 control 3                            */
    volatile uint32_t BRR;   /* 0x0C baud rate                            */
    volatile uint32_t GTPR;  /* 0x10 guard time / prescaler              */
    volatile uint32_t RTOR;  /* 0x14 receiver timeout                    */
    volatile uint32_t RQR;   /* 0x18 request                              */
    volatile uint32_t ISR;  /* 0x1C interrupt status                    */
    volatile uint32_t ICR;  /* 0x20 interrupt clear                     */
    volatile uint32_t RDR;  /* 0x24 receive data                        */
    volatile uint32_t TDR;  /* 0x28 transmit data                       */
} usart_t;

#define USART1 ((usart_t *)USART1_BASE)
#define USART3 ((usart_t *)USART3_BASE)

/* USART CR1 bits */
#define USART_CR1_UE          (1 << 0)
#define USART_CR1_RE          (1 << 2)
#define USART_CR1_TE          (1 << 3)
#define USART_CR1_RXNEIE      (1 << 5)
#define USART_CR1_TCIE        (1 << 6)

/* USART ISR bits */
#define USART_ISR_RXNE        (1 << 5)
#define USART_ISR_TXE         (1 << 7)
#define USART_ISR_TC         (1 << 6)

/* --------------------------------------------------------------------- */
/*  Ethernet MAC register layout (subset)                                 */
/* --------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t MACCR;     /* 0x00                                    */
    volatile uint32_t MACFFR;     /* 0x04 frame filter                       */
    volatile uint32_t MACHTHR;    /* 0x08 hash table high                    */
    volatile uint32_t MACHTLR;    /* 0x0C hash table low                      */
    volatile uint32_t MACMIIAR;   /* 0x10 MII address                        */
    volatile uint32_t MACMIIDR;   /* 0x14 MII data                           */
    volatile uint32_t MACFCR;     /* 0x18 flow control                       */
    volatile uint32_t RESERVED1;  /* 0x1C                                    */
    volatile uint32_t MACVLANTR;  /* 0x20 VLAN tag                           */
    volatile uint32_t RESERVED2[3]; /* 0x24-0x2C                            */
    volatile uint32_t MACRWUFFR;  /* 0x30 remote wake-up filter              */
    volatile uint32_t MACPMTCSR;  /* 0x34 PMT control/status                  */
    volatile uint32_t RESERVED3;  /* 0x38                                    */
    volatile uint32_t MACISR;     /* 0x3C interrupt status                    */
    volatile uint32_t MACIMR;     /* 0x40 interrupt mask                      */
    volatile uint32_t MACA0HR;   /* 0x40 (placeholder) — MAC addr 0 high    */
    volatile uint32_t MACA0LR;   /* 0x44 MAC addr 0 low                     */
    /* ... (many more registers omitted for brevity) ...                  */
    volatile uint32_t MACSTNR;    /* 0x4C system time nanoseconds (PTP)     */
    volatile uint32_t MACSTSR;    /* 0x50 system time seconds (PTP)          */
    volatile uint32_t MACSTSUR;   /* 0x54 system time seconds update         */
    volatile uint32_t MACSTNUR;   /* 0x58 system time nanoseconds update     */
    volatile uint32_t MACPPSCR;   /* 0x60 PPS control                         */
} eth_mac_t;

#define ETH_MAC  ((eth_mac_t *)(ETH_BASE + 0x0000))

/* Ethernet DMA */
typedef struct {
    volatile uint32_t DMABMR;    /* 0x1000 bus mode                          */
    volatile uint32_t DMATPDR;    /* 0x1004 TX poll demand                    */
    volatile uint32_t DMARPDR;    /* 0x1008 RX poll demand                    */
    volatile uint32_t DMARDLAR;   /* 0x100C RX descriptor list addr           */
    volatile uint32_t DMATDLAR;   /* 0x1010 TX descriptor list addr           */
    volatile uint32_t DMASR;     /* 0x1014 DMA status                        */
    volatile uint32_t DMAOMR;    /* 0x1018 operation mode                    */
    volatile uint32_t DMAIER;   /* 0x101C interrupt enable                  */
    volatile uint32_t DMAMFBOCR; /* 0x1020 missed frame & buffer overflow    */
    /* ... (PTP timestamp registers) ...                                   */
    volatile uint32_t DMACHTDR;  /* 0x1048 current host TX desc              */
    volatile uint32_t DMACHRDR;  /* 0x104C current host RX desc              */
    volatile uint32_t DMACHTBAR; /* 0x1050 current host TX buffer addr       */
    volatile uint32_t DMACHRBAR; /* 0x1054 current host RX buffer addr       */
} eth_dma_t;

#define ETH_DMA  ((eth_dma_t *)(ETH_BASE + 0x1000))

/* ETH DMA descriptor flags */
#define ETH_DMARXDESC_OWN      (1 << 31)
#define ETH_DMARXDESC_FS       (1 << 9)
#define ETH_DMARXDESC_LS       (1 << 8)
#define ETH_DMATXDESC_OWN      (1 << 31)
#define ETH_DMATXDESC_FS       (1 << 29)
#define ETH_DMATXDESC_LS       (1 << 30)
#define ETH_DMATXDESC_IC       (1 << 14)

/* MACCR bits */
#define ETH_MACCR_FES          (1 << 14)   /* fast speed 100M            */
#define ETH_MACCR_DM          (1 << 11)   /* duplex mode                */
#define ETH_MACCR_RE          (1 << 2)    /* receive enable             */
#define ETH_MACCR_TE          (1 << 3)    /* transmit enable            */

/* DMAOMR bits */
#define ETH_DMAOMR_SR         (1 << 1)    /* start receive              */
#define ETH_DMAOMR_ST         (1 << 13)   /* start transmit             */

/* --------------------------------------------------------------------- */
/*  DAC register layout                                                    */
/* --------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t CR;        /* 0x00 control                            */
    volatile uint32_t SWTRIGR;    /* 0x04 software trigger                   */
    volatile uint32_t DHR12R1;   /* 0x08 12-bit right-aligned ch1          */
    volatile uint32_t DHR12L1;   /* 0x0C 12-bit left-aligned ch1            */
    volatile uint32_t DHR8R1;    /* 0x10 8-bit ch1                          */
    volatile uint32_t RESERVED1;
    volatile uint32_t DHR12R2;   /* 0x14 12-bit ch2                         */
    volatile uint32_t RESERVED2[3];
    volatile uint32_t DOR1;      /* 0x20 ch1 output                          */
    volatile uint32_t DOR2;      /* 0x24 ch2 output                          */
    volatile uint32_t RESERVED3;
    volatile uint32_t SR;        /* 0x34 status                             */
} dac_t;

#define DAC1  ((dac_t *)DAC1_BASE)

/* DAC CR bits */
#define DAC_CR_EN1            (1 << 0)
#define DAC_CR_TSEL1_SW       (7 << 3)
#define DAC_CR_DMAEN1         (1 << 12)

/* --------------------------------------------------------------------- */
/*  I2C register layout (subset)                                          */
/* --------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t CR1;     /* 0x00 control 1                            */
    volatile uint32_t CR2;     /* 0x04 control 2                            */
    volatile uint32_t OAR1;    /* 0x08 own address 1                        */
    volatile uint32_t OAR2;    /* 0x0C own address 2                        */
    volatile uint32_t TIMINGR; /* 0x10 timing                               */
    volatile uint32_t TIMEOUTR;/* 0x14 timeout                              */
    volatile uint32_t ISR;     /* 0x18 interrupt & status                   */
    volatile uint32_t ICR;     /* 0x1C interrupt clear                      */
    volatile uint32_t PECR;    /* 0x20 PEC                                   */
    volatile uint32_t RXDR;    /* 0x24 receive data                         */
    volatile uint32_t TXDR;    /* 0x28 transmit data                         */
} i2c_t;

#define I2C2   ((i2c_t *)I2C2_BASE)

/* I2C CR2 bits */
#define I2C_CR2_START         (1 << 13)
#define I2C_CR2_STOP          (1 << 14)
#define I2C_CR2_NBYTES_SHIFT  16
#define I2C_CR2_NACK          (1 << 15)
#define I2C_CR2_RD_WRN        (1 << 10)
#define I2C_CR2_AUTOEND       (1 << 25)

/* I2C ISR bits */
#define I2C_ISR_TXIS          (1 << 1)
#define I2C_ISR_RXNE          (1 << 2)
#define I2C_ISR_TC           (1 << 6)
#define I2C_ISR_TCR          (1 << 7)
#define I2C_ISR_BUSY         (1 << 15)
#define I2C_ISR_NACKF        (1 << 13)

/* --------------------------------------------------------------------- */
/*  TIM register layout (subset for PWM)                                  */
/* --------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t CR1;     /* 0x00 control 1                            */
    volatile uint32_t CR2;     /* 0x04 control 2                            */
    volatile uint32_t SMCR;    /* 0x08 slave mode                           */
    volatile uint32_t DIER;    /* 0x0C DMA/interrupt enable                 */
    volatile uint32_t SR;      /* 0x10 status                                */
    volatile uint32_t EGR;     /* 0x14 event generation                      */
    volatile uint32_t CCMR1;   /* 0x18 capture/compare mode 1              */
    volatile uint32_t CCMR2;   /* 0x1C capture/compare mode 2              */
    volatile uint32_t CCER;    /* 0x20 capture/compare enable               */
    volatile uint32_t CNT;     /* 0x24 counter                              */
    volatile uint32_t PSC;     /* 0x28 prescaler                            */
    volatile uint32_t ARR;     /* 0x2C auto-reload                           */
    volatile uint32_t RCR;     /* 0x30 repetition counter                   */
    volatile uint32_t CCR1;    /* 0x34 capture/compare 1                    */
    volatile uint32_t CCR2;    /* 0x38 capture/compare 2                    */
    volatile uint32_t CCR3;    /* 0x3C capture/compare 3                    */
    volatile uint32_t CCR4;    /* 0x40 capture/compare 4                    */
    volatile uint32_t BDTR;    /* 0x44 break / deadtime                      */
} tim_t;

#define TIM3   ((tim_t *)TIM3_BASE)
#define TIM4   ((tim_t *)TIM4_BASE)
#define TIM8   ((tim_t *)TIM8_BASE)

/* TIM CR1 bits */
#define TIM_CR1_CEN            (1 << 0)
#define TIM_CR1_ARPE           (1 << 15)

/* TIM CCER bits */
#define TIM_CCER_CCxE(ch)     (1 << ((ch) * 4))

/* TIM CCMR bits (PWM mode 1) */
#define TIM_CCMR_OCxM_PWM1    0x6

/* --------------------------------------------------------------------- */
/*  DMA register layout (subset)                                          */
/* --------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t ISR;     /* 0x00 interrupt status                    */
    volatile uint32_t RESERVED0[4];
    volatile uint32_t S0CR;    /* 0x10 stream 0 control                    */
    volatile uint32_t S0NDTR;  /* 0x14 number of data                       */
    volatile uint32_t S0PAR;   /* 0x18 peripheral address                  */
    volatile uint32_t S0M0AR;  /* 0x1C memory 0 address                    */
    volatile uint32_t S0M1AR;  /* 0x20 memory 1 address                    */
    volatile uint32_t S0FCR;   /* 0x24 FIFO control                         */
    /* ... (streams 1-7 follow) ...                                      */
} dma_t;

#define DMA1   ((dma_t *)DMA1_BASE)

/* --------------------------------------------------------------------- */
/*  PWR register layout (subset)                                          */
/* --------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t CR1;    /* 0x00 control 1                           */
    volatile uint32_t CSR1;   /* 0x04 control/status 1                    */
    volatile uint32_t CR2;    /* 0x08 control 2                           */
    volatile uint32_t CR3;    /* 0x0C control 3                           */
    volatile uint32_t CPUCR;  /* 0x10 CPU control                         */
    /* ... (many more) ...                                               */
} pwr_t;

#define PWR    ((pwr_t *)PWR_BASE)

/* --------------------------------------------------------------------- */
/*  Interrupt vectors (NVIC) — relevant ones                              */
/* --------------------------------------------------------------------- */
#define NVIC_ETH_IRQn          61
#define NVIC_USART1_IRQn       82
#define NVIC_USART3_IRQn       39
#define NVIC_TIM8_CC_IRQn      52  /* for 1-PPS capture via TIM8_CH2      */

/* NVIC register layout */
#define NVIC_ISER0   (*(volatile uint32_t *)(0xE000E100)
#define NVIC_ICER0   (*(volatile uint32_t *)(0xE000E180)

/* SysTick */
#define SYST_CSR     (*(volatile uint32_t *)(0xE000E010))
#define SYST_RVR     (*(volatile uint32_t *)(0xE000E014))
#define SYST_CVR     (*(volatile uint32_t *)(0xE000E018))

#endif /* CHRONOS_PHANTOM_REGISTERS_H */