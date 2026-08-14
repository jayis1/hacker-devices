/*
 * registers.h — STM32G474 register-level definitions
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Deliberately NOT a HAL: we define only the registers/bitfields the firmware
 * touches, as named structs + bit helpers. This keeps the source readable,
 * dependency-free, and small, and lets a reviewer see exactly which silicon
 * bits are being programmed.
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

/* ---- Base addresses ---- */
#define PERIPH_BASE      0x40000000u
#define AHB1_BASE        (PERIPH_BASE + 0x00020000u)
#define AHB2_BASE        (PERIPH_BASE + 0x08000000u)
#define APB1_BASE        (PERIPH_BASE + 0x00000000u)
#define APB2_BASE        (PERIPH_BASE + 0x00010000u)

#define RCC_BASE         (AHB1_BASE + 0x2100u)
#define GPIOA_BASE       (AHB2_BASE + 0x0000u)
#define GPIOB_BASE       (AHB2_BASE + 0x0400u)
#define GPIOC_BASE       (AHB2_BASE + 0x0800u)
#define SPI1_BASE        (APB2_BASE + 0x3000u)
#define TIM6_BASE        (APB1_BASE + 0x1000u)
#define TIM7_BASE        (APB1_BASE + 0x1400u)
#define USB_BASE         (APB1_BASE + 0x5C00u)
#define PWR_BASE         (APB1_BASE + 0x4000u)
#define I2C1_BASE        (APB1_BASE + 0x5400u)
#define EXTI_BASE        (APB2_BASE + 0x0F00u)

/* ---- Generic register accessor ---- */
#define REG32(addr) (*(volatile uint32_t *)(addr))

/* ---- RCC ---- */
struct rcc {
    volatile uint32_t CR;       /* 0x00 */
    volatile uint32_t ICSCR;    /* 0x04 */
    volatile uint32_t CFGR;     /* 0x08 */
    volatile uint32_t CIR;       /* 0x0C */
    volatile uint32_t RESERVED0[2];
    volatile uint32_t PLLCFGR;  /* 0x14 */
    volatile uint32_t RESERVED1;
    volatile uint32_t CIA;       /* 0x1C */
    volatile uint32_t RESERVED2[7];
    volatile uint32_t CCIPR;     /* 0x58 */
    volatile uint32_t RESERVED3;
    volatile uint32_t CCIPR2;    /* 0x60 */
    volatile uint32_t RESERVED4[3];
    volatile uint32_t AHB1ENR;   /* 0x48 — note: offset in real G4 */
    volatile uint32_t AHB2ENR;   /* 0x4C */
    volatile uint32_t AHB3ENR;   /* 0x50 */
    volatile uint32_t RESERVED5;
    volatile uint32_t APB1ENR1;  /* 0x58 */
    volatile uint32_t APB1ENR2;  /* 0x5C */
    volatile uint32_t APB2ENR;   /* 0x60 */
};
#define RCC ((struct rcc *)RCC_BASE)

/* RCC bit helpers we use */
#define RCC_CR_HSION        (1u<<8)
#define RCC_CR_HSERDY       (1u<<17)
#define RCC_CR_HSEON        (1u<<16)
#define RCC_CR_PLLON        (1u<<24)
#define RCC_CR_PLLRDY       (1u<<25)

#define RCC_CFGR_SW_HSI     0u
#define RCC_CFGR_SW_HSE     1u
#define RCC_CFGR_SW_PLL     2u
#define RCC_CFGR_SWS_SHIFT   2

#define RCC_AHB1ENR_DMA1     (1u<<0)
#define RCC_AHB2ENR_GPIOA    (1u<<0)
#define RCC_AHB2ENR_GPIOB    (1u<<1)
#define RCC_AHB2ENR_GPIOC    (1u<<2)
#define RCC_APB1ENR1_TIM6    (1u<<4)
#define RCC_APB1ENR1_TIM7    (1u<<5)
#define RCC_APB1ENR1_I2C1    (1u<<21)
#define RCC_APB1ENR1_USB     (1u<<23)
#define RCC_APB1ENR2_TIM16   (1u<<17)
#define RCC_APB2ENR_SPI1     (1u<<12)
#define RCC_APB2ENR_SYSCFG   (1u<<0)

/* ---- GPIO ---- */
struct gpio {
    volatile uint32_t MODER;   /* 0x00: 00 in, 01 out, 10 alt, 11 analog */
    volatile uint32_t OTYPER;  /* 0x04: 0 push-pull, 1 open-drain */
    volatile uint32_t OSPEEDR; /* 0x08: 00 low .. 11 very high */
    volatile uint32_t PUPDR;   /* 0x0C: 00 none, 01 pull-up, 10 pull-down */
    volatile uint32_t IDR;     /* 0x10 */
    volatile uint32_t ODR;     /* 0x14 */
    volatile uint32_t BSRR;    /* 0x18: set[15:0]/reset[31:16] */
    volatile uint32_t LCKR;     /* 0x1C */
    volatile uint32_t AFRL;    /* 0x20 */
    volatile uint32_t AFRH;    /* 0x24 */
    volatile uint32_t BRR;     /* 0x28 */
};
#define GPIOA ((struct gpio *)GPIOA_BASE)
#define GPIOB ((struct gpio *)GPIOB_BASE)
#define GPIOC ((struct gpio *)GPIOC_BASE)

/* GPIO bit helpers */
#define GPIO_OUT_PP(p,n) do { (p)->MODER  &= ~(3u<<((n)*2)); \
                              (p)->MODER  |=  (1u<<((n)*2)); \
                              (p)->OTYPER &= ~(1u<<(n));      } while (0)
#define GPIO_IN_PUP(p,n) do { (p)->MODER &= ~(3u<<((n)*2)); \
                              (p)->PUPDR &= ~(3u<<((n)*2)); \
                              (p)->PUPDR |=  (1u<<((n)*2)); } while (0)
#define GPIO_IN_PDN(p,n) do { (p)->MODER &= ~(3u<<((n)*2)); \
                              (p)->PUPDR &= ~(3u<<((n)*2)); \
                              (p)->PUPDR |=  (2u<<((n)*2)); } while (0)
#define GPIO_IN_FLT(p,n) do { (p)->MODER &= ~(3u<<((n)*2)); \
                              (p)->PUPDR &= ~(3u<<((n)*2)); } while (0)
#define GPIO_AF(p,n,af) do {  if ((n)<8) { (p)->AFRL &= ~(0xFu<<((n)*4)); \
                                            (p)->AFRL |=  ((af)<<((n)*4)); } \
                              else {        (p)->AFRH &= ~(0xFu<<(((n)-8)*4)); \
                                            (p)->AFRH |=  ((af)<<(((n)-8)*4)); } } while (0)
#define GPIO_ALT(p,n,af) do { (p)->MODER &= ~(3u<<((n)*2)); \
                              (p)->MODER |=  (2u<<((n)*2)); \
                              GPIO_AF((p),(n),(af)); } while (0)
#define GPIO_SET(p,n)   ((p)->BSRR = (1u<<(n)))
#define GPIO_CLR(p,n)   ((p)->BSRR = (1u<<((n)+16)))
#define GPIO_GET(p,n)   (((p)->IDR >> (n)) & 1u)

/* ---- SPI1 (master to FPGA) ---- */
struct spi {
    volatile uint32_t CR1;   /* 0x00 */
    volatile uint32_t CR2;   /* 0x04 */
    volatile uint32_t SR;    /* 0x08 */
    volatile uint32_t DR;    /* 0x0C */
    volatile uint32_t CRCPR; /* 0x10 */
    volatile uint32_t RXCRC; /* 0x14 */
    volatile uint32_t TXCRC; /* 0x18 */
};
#define SPI1 ((struct spi *)SPI1_BASE)

#define SPI_CR1_CPHA     (1u<<0)
#define SPI_CR1_CPOL     (1u<<1)
#define SPI_CR1_MSTR     (1u<<2)
#define SPI_CR1_BR_MASK  0x38u
#define SPI_CR1_BR_SHIFT 3
#define SPI_CR1_SPE     (1u<<6)
#define SPI_CR1_LSBFIRST (1u<<7)
#define SPI_CR1_SSI     (1u<<8)
#define SPI_CR1_SSM     (1u<<9)
#define SPI_CR1_BIDIMODE (1u<<15)
#define SPI_CR2_RXNEIE  (1u<<6)
#define SPI_CR2_TXEIE   (1u<<7)
#define SPI_CR2_DS_MASK 0xFu
#define SPI_CR2_DS_SHIFT 8
#define SPI_CR2_FRXTH   (1u<<12)
#define SPI_SR_RXNE     (1u<<0)
#define SPI_SR_TXE      (1u<<1)
#define SPI_SR_BSY      (1u<<7)
#define SPI_SR_OVR      (1u<<6)
#define SPI_SR_CRCERR   (1u<<4)

/* ---- TIM6 (1 ms tick) ---- */
struct tim6 {
    volatile uint32_t CR1;   /* 0x00 */
    volatile uint32_t CR2;   /* 0x04 */
    volatile uint32_t DIER;  /* 0x0C — note: reserved 0x08 */
    volatile uint32_t SR;    /* 0x10 */
    volatile uint32_t EGR;   /* 0x14 */
    volatile uint32_t CNT;   /* 0x24 — reserved gap 0x18..0x20 */
    volatile uint32_t PSC;   /* 0x28 */
    volatile uint32_t ARR;   /* 0x2C */
};
#define TIM6 ((struct tim6 *)TIM6_BASE)
#define TIM6_CR1_CEN    (1u<<0)
#define TIM6_DIER_UIE   (1u<<0)
#define TIM6_SR_UIF     (1u<<0)

/* ---- USB FS ---- */
struct usb {
    volatile uint32_t EP0R;
    volatile uint32_t RESERVED[15];
    volatile uint32_t CNTR;
    volatile uint32_t ISTR;
    volatile uint32_t FNR;
    volatile uint32_t DADDR;
    volatile uint32_t BTABLE;
};
#define USB ((struct usb *)USB_BASE)

/* ---- EXTI ---- */
struct exti {
    volatile uint32_t IMR1;   /* 0x00 */
    volatile uint32_t EMR1;   /* 0x04 */
    volatile uint32_t RTSR1;  /* 0x08 */
    volatile uint32_t FTSR1;  /* 0x0C */
    volatile uint32_t SWIER1; /* 0x10 */
    volatile uint32_t PR1;    /* 0x14 */
};
#define EXTI ((struct exti *)EXTI_BASE)

/* ---- PWR ---- */
#define PWR ((volatile uint32_t *)PWR_BASE)
#define PWR_CR1_DBP (1u<<8)

/* ---- NVIC ---- */
#define NVIC_BASE  (0xE000E100u)
#define NVIC_ISER   (*(volatile uint32_t *)(NVIC_BASE + 0x00))
#define NVIC_ICPR   (*(volatile uint32_t *)(NVIC_BASE + 0x180))
#define NVIC_IPR(n) (*(volatile uint32_t *)(NVIC_BASE + 0x300 + 4*((n)/4))

/* ---- IRQ numbers we use ---- */
#define IRQ_TIM6      17u
#define IRQ_SPI1      35u
#define IRQ_EXTI10_15 40u   /* EXTI[15:10] combined on G4 */

/* ---- Bit helpers ---- */
#define BIT(n)  (1u<<(n))
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

#endif /* REGISTERS_H */