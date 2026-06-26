/*
 * registers.h — STM32H743 register base addresses & bit definitions
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Minimal hand-maintained register map (no CMSIS dependency) for SideProbe.
 * Only the peripherals SideProbe actually uses are defined.
 */
#ifndef SIDEPROBE_REGISTERS_H
#define SIDEPROBE_REGISTERS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Base addresses ---- */
#define RCC_BASE        0x58024400u
#define PWR_BASE        0x58024800u
#define FLASH_BASE_REG 0x52002000u   /* FLASH controller, not to be confused with flash memory */
#define GPIOA_BASE      0x58020000u
#define GPIOB_BASE      0x58020400u
#define GPIOC_BASE      0x58020800u
#define GPIOD_BASE      0x58020C00u
#define GPIOE_BASE      0x58021000u
#define GPIOH_BASE      0x58021C00u
#define SYSCFG_BASE      0x58000400u
#define EXTI_BASE        0x58000D00u
#define DMA1_BASE        0x40020000u
#define DMA2_BASE        0x40020400u
#define DMAMUX1_BASE     0x40020800u
#define USART1_BASE      0x40011000u
#define USART2_BASE      0x40004400u
#define SPI1_BASE        0x40013000u
#define SPI2_BASE        0x40003800u
#define I2C1_BASE        0x40005400u
#define SDMMC1_BASE      0x52007000u
#define USB1_OTG_BASE    0x40080000u
#define FMC_BASE         0x52004000u
#define ADC1_BASE        0x40022000u
#define ADC2_BASE        0x40022300u
#define ADC12_COMMON     0x40023000u
#define HSEM_BASE        0x48002400u

/* ---- RCC bitfield helpers ---- */
#define RCC_CR            (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_CFGR          (*(volatile uint32_t *)(RCC_BASE + 0x10))
#define RCC_PLL1CFGR      (*(volatile uint32_t *)(RCC_BASE + 0x14))
#define RCC_PLLCKSELR     (*(volatile uint32_t *)(RCC_BASE + 0x28))
#define RCC_PLL1DIVR      (*(volatile uint32_t *)(RCC_BASE + 0x2C))
#define RCC_AHB3ENR       (*(volatile uint32_t *)(RCC_BASE + 0xD4))  /* FMC, SDMMC */
#define RCC_AHB1ENR       (*(volatile uint32_t *)(RCC_BASE + 0xD8))  /* GPIOA..E, DMA1/2 */
#define RCC_AHB2ENR       (*(volatile uint32_t *)(RCC_BASE + 0xDC))  /* USB OTG */
#define RCC_APB1ENR       (*(volatile uint32_t *)(RCC_BASE + 0xE0))  /* USART2/I2C1/SPI2 */
#define RCC_APB2ENR       (*(volatile uint32_t *)(RCC_BASE + 0xE4))  /* USART1/SPI1 */
#define RCC_APB4ENR       (*(volatile uint32_t *)(RCC_BASE + 0xEC))

/* RCC enable bits */
#define RCC_AHB3ENR_FMC    (1u << 4)
#define RCC_AHB3ENR_SDMMC1 (1u << 16)
#define RCC_AHB1ENR_GPIOA  (1u << 0)
#define RCC_AHB1ENR_GPIOB  (1u << 1)
#define RCC_AHB1ENR_GPIOC  (1u << 2)
#define RCC_AHB1ENR_GPIOD  (1u << 3)
#define RCC_AHB1ENR_GPIOE  (1u << 4)
#define RCC_AHB1ENR_DMA1   (1u << 8)
#define RCC_AHB1ENR_DMA2   (1u << 9)
#define RCC_AHB2ENR_OTG    (1u << 12)
#define RCC_APB1ENR_USART2 (1u << 17)
#define RCC_APB1ENR_I2C1   (1u << 21)
#define RCC_APB1ENR_SPI2   (1u << 14)
#define RCC_APB2ENR_USART1 (1u << 4)
#define RCC_APB2ENR_SPI1   (1u << 12)

/* ---- GPIO registers (per-bank) ---- */
#define GPIO_MODER(b)   (*(volatile uint32_t *)((b) + 0x00))
#define GPIO_OTYPER(b)  (*(volatile uint32_t *)((b) + 0x04))
#define GPIO_OSPEEDR(b) (*(volatile uint32_t *)((b) + 0x08))
#define GPIO_PUPDR(b)   (*(volatile uint32_t *)((b) + 0x0C))
#define GPIO_IDR(b)     (*(volatile uint32_t *)((b) + 0x10))
#define GPIO_ODR(b)     (*(volatile uint32_t *)((b) + 0x14))
#define GPIO_BSRR(b)    (*(volatile uint32_t *)((b) + 0x18))
#define GPIO_AFRL(b)    (*(volatile uint32_t *)((b) + 0x20))
#define GPIO_AFRH(b)    (*(volatile uint32_t *)((b) + 0x24))

/* GPIO modes */
#define GPIO_MODE_IN    0u
#define GPIO_MODE_OUT   1u
#define GPIO_MODE_AF    2u
#define GPIO_MODE_AN    3u

/* ---- USART registers ---- */
#define USART_CR1(b)    (*(volatile uint32_t *)((b) + 0x00))
#define USART_CR2(b)    (*(volatile uint32_t *)((b) + 0x04))
#define USART_CR3(b)    (*(volatile uint32_t *)((b) + 0x08))
#define USART_BRR(b)   (*(volatile uint32_t *)((b) + 0x0C))
#define USART_ISR(b)   (*(volatile uint32_t *)((b) + 0x14))
#define USART_RDR(b)   (*(volatile uint32_t *)((b) + 0x18))
#define USART_TDR(b)   (*(volatile uint32_t *)((b) + 0x1C))

#define USART_CR1_UE     (1u << 0)
#define USART_CR1_TE     (1u << 3)
#define USART_CR1_RE     (1u << 2)
#define USART_ISR_TXE    (1u << 7)
#define USART_ISR_RXNE   (1u << 5)

/* ---- SPI registers ---- */
#define SPI_CR1(b)       (*(volatile uint32_t *)((b) + 0x00))
#define SPI_CR2(b)       (*(volatile uint32_t *)((b) + 0x04))
#define SPI_SR(b)        (*(volatile uint32_t *)((b) + 0x08))
#define SPI_DR(b)        (*(volatile uint32_t *)((b) + 0x0C))

#define SPI_CR1_SPE      (1u << 6)
#define SPI_CR1_MSTR     (1u << 1)
#define SPI_CR1_CPHA     (1u << 0)
#define SPI_CR1_CPOL     (1u << 1)
#define SPI_SR_TXP       (1u << 1)
#define SPI_SR_RXNE      (1u << 0)
#define SPI_SR_BSY       (1u << 7)

/* ---- DMA stream registers (simplified) ---- */
typedef struct {
    volatile uint32_t CR;     /* 0x00 */
    volatile uint32_t NDTR;   /* 0x04 */
    volatile uint32_t PAR;    /* 0x08 */
    volatile uint32_t M0AR;   /* 0x0C */
    volatile uint32_t M1AR;   /* 0x10 */
    volatile uint32_t FCR;    /* 0x14 */
    volatile uint32_t RESERVED;
} dma_stream_t;

#define DMA_STREAM(base, n)  ((volatile dma_stream_t *)((base) + 0x10 + (n) * 0x18))

#define DMA_CR_EN       (1u << 0)
#define DMA_CR_TCIE     (1u << 4)
#define DMA_CR_MINC     (1u << 10)
#define DMA_CR_DIR_P2M  (0u << 6)
#define DMA_CR_DIR_M2P  (1u << 6)
#define DMA_CR_M2M      (2u << 6)
#define DMA_CR_CHSEL(c) ((c) << 25)
#define DMA_CR_PL(p)    ((p) << 16)
#define DMA_CR_PSIZE_16 (1u << 11)
#define DMA_CR_MSIZE_16 (1u << 13)

/* ---- FMC SDRAM register offsets ---- */
#define FMC_SDCR1       (*(volatile uint32_t *)(FMC_BASE + 0x40))
#define FMC_SDCR2       (*(volatile uint32_t *)(FMC_BASE + 0x44))
#define FMC_SDTR1       (*(volatile uint32_t *)(FMC_BASE + 0x48))
#define FMC_SDTR2       (*(volatile uint32_t *)(FMC_BASE + 0x4C))
#define FMC_SDCMR       (*(volatile uint32_t *)(FMC_BASE + 0x50))
#define FMC_SDSR       (*(volatile uint32_t *)(FMC_BASE + 0x54))

/* ---- EXTI ---- */
#define EXTI_IMR1       (*(volatile uint32_t *)(EXTI_BASE + 0x00))
#define EXTI_RTSR1      (*(volatile uint32_t *)(EXTI_BASE + 0x08))
#define EXTI_FTSR1      (*(volatile uint32_t *)(EXTI_BASE + 0x0C))
#define EXTI_PR1        (*(volatile uint32_t *)(EXTI_BASE + 0x3C))

/* ---- PWR ---- */
#define PWR_CR3         (*(volatile uint32_t *)(PWR_BASE + 0x08))
#define PWR_D3CR        (*(volatile uint32_t *)(PWR_BASE + 0x18))
#define PWR_SR          (*(volatile uint32_t *)(PWR_BASE + 0x04))
#define PWR_CR3_SCUEN   (1u << 2)

/* ---- Flash controller (wait states) ---- */
#define FLASH_ACR       (*(volatile uint32_t *)(FLASH_BASE_REG + 0x00))
#define FLASH_ACR_LATENCY(ws) ((ws) & 0x0F)
#define FLASH_ACR_WRHIGHFREQ_MASK (0x3u << 4)

/* ---- Misc helpers ---- */
static inline void sp_delay_us(volatile uint32_t us) {
    /* rough busy-wait at 480 MHz: ~6 cycles per us per iteration */
    while (us--) {
        for (volatile int i = 0; i < 80; ++i) __asm__ volatile("nop");
    }
}

static inline void sp_delay_ms(volatile uint32_t ms) {
    sp_delay_us(ms * 1000);
}

#ifdef __cplusplus
}
#endif
#endif /* SIDEPROBE_REGISTERS_H */