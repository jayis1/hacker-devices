/*
 * registers.h — STM32H743 MMIO Register Definitions
 * PhantomBridge PoE Network Implant
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

/* ===== ARM Cortex-M7 Core Registers ===== */
#define SCB_CPUID          (*(volatile uint32_t *)0xE000ED00UL)
#define SCB_ICSR           (*(volatile uint32_t *)0xE000ED04UL)
#define SCB_VTOR           (*(volatile uint32_t *)0xE000ED08UL)
#define SCB_AIRCR          (*(volatile uint32_t *)0xE000ED0CUL)
#define SCB_SCR            (*(volatile uint32_t *)0xE000ED10UL)
#define SCB_CCR            (*(volatile uint32_t *)0xE000ED14UL)
#define SCB_SHPR1          (*(volatile uint32_t *)0xE000ED18UL)
#define SCB_SHPR2          (*(volatile uint32_t *)0xE000ED1CUL)
#define SCB_SHPR3          (*(volatile uint32_t *)0xE000ED20UL)

#define SysTick_CTRL       (*(volatile uint32_t *)0xE000E010UL)
#define SysTick_LOAD       (*(volatile uint32_t *)0xE000E014UL)
#define SysTick_VAL        (*(volatile uint32_t *)0xE000E018UL)
#define SysTick_CALIB      (*(volatile uint32_t *)0xE000E01CUL)

#define NVIC_ISER0         (*(volatile uint32_t *)0xE000E100UL)
#define NVIC_ICER0         (*(volatile uint32_t *)0xE000E180UL)
#define NVIC_ISPR0         (*(volatile uint32_t *)0xE000E200UL)
#define NVIC_ICPR0         (*(volatile uint32_t *)0xE000E280UL)

/* ===== RCC Registers ===== */
#define RCC_CR             (*(volatile uint32_t *)0x58024400UL)
#define RCC_HSICFGR        (*(volatile uint32_t *)0x58024404UL)
#define RCC_D1CFGR        (*(volatile uint32_t *)0x5802440CUL)
#define RCC_D2CFGR        (*(volatile uint32_t *)0x58024410UL)
#define RCC_D3CFGR        (*(volatile uint32_t *)0x58024414UL)
#define RCC_PLLCKSELR      (*(volatile uint32_t *)0x58024428UL)
#define RCC_PLLCFGR        (*(volatile uint32_t *)0x5802442CUL)
#define RCC_PLL1DIVR       (*(volatile uint32_t *)0x58024430UL)
#define RCC_PLL1FRACR      (*(volatile uint32_t *)0x58024434UL)
#define RCC_PLL2DIVR       (*(volatile uint32_t *)0x58024438UL)
#define RCC_PLL2FRACR      (*(volatile uint32_t *)0x5802443CUL)
#define RCC_PLL3DIVR       (*(volatile uint32_t *)0x58024440UL)
#define RCC_PLL3FRACR      (*(volatile uint32_t *)0x58024444UL)
#define RCC_D1CCIPR       (*(volatile uint32_t *)0x5802448CUL)
#define RCC_D2CCIP1R      (*(volatile uint32_t *)0x58024490UL)
#define RCC_D2CCIP2R      (*(volatile uint32_t *)0x58024494UL)
#define RCC_D3CCIPR       (*(volatile uint32_t *)0x58024498UL)
#define RCC_AHB3ENR        (*(volatile uint32_t *)0x580244D4UL)
#define RCC_AHB1ENR        (*(volatile uint32_t *)0x580244E0UL)
#define RCC_AHB2ENR        (*(volatile uint32_t *)0x580244E4UL)
#define RCC_AHB4ENR        (*(volatile uint32_t *)0x580244E8UL)
#define RCC_APB1LENR       (*(volatile uint32_t *)0x580244ECUL)
#define RCC_APB1HENR       (*(volatile uint32_t *)0x580244F0UL)
#define RCC_APB2ENR        (*(volatile uint32_t *)0x580244F4UL)
#define RCC_APB3ENR        (*(volatile uint32_t *)0x580244F8UL)
#define RCC_APB4ENR        (*(volatile uint32_t *)0x580244FCUL)

/* RCC CR bits */
#define RCC_CR_HSION       (1 << 0)
#define RCC_CR_HSIRDY      (1 << 2)
#define RCC_CR_HSEON       (1 << 16)
#define RCC_CR_HSERDY      (1 << 17)
#define RCC_CR_PLL1ON      (1 << 24)
#define RCC_CR_PLL1RDY     (1 << 25)
#define RCC_CR_PLL2ON      (1 << 26)
#define RCC_CR_PLL2RDY     (1 << 27)
#define RCC_CR_PLL3ON      (1 << 28)
#define RCC_CR_PLL3RDY     (1 << 29)
#define RCC_CR_CSSON       (1 << 19)

/* ===== GPIO Registers (Base + offset per port) ===== */
#define GPIOA_BASE         0x58020000UL
#define GPIOB_BASE         0x58020400UL
#define GPIOC_BASE         0x58020800UL
#define GPIOD_BASE         0x58020C00UL
#define GPIOE_BASE         0x58021000UL

#define GPIO_MODER(base)   (*(volatile uint32_t *)(base + 0x00))
#define GPIO_OTYPER(base)  (*(volatile uint32_t *)(base + 0x04))
#define GPIO_OSPEEDR(base) (*(volatile uint32_t *)(base + 0x08))
#define GPIO_PUPDR(base)   (*(volatile uint32_t *)(base + 0x0C))
#define GPIO_IDR(base)     (*(volatile uint32_t *)(base + 0x10))
#define GPIO_ODR(base)     (*(volatile uint32_t *)(base + 0x14))
#define GPIO_BSRR(base)    (*(volatile uint32_t *)(base + 0x18))
#define GPIO_LCKR(base)    (*(volatile uint32_t *)(base + 0x1C))
#define GPIO_AFRL(base)    (*(volatile uint32_t *)(base + 0x20))
#define GPIO_AFRH(base)    (*(volatile uint32_t *)(base + 0x24))

/* GPIO Mode values */
#define GPIO_MODE_INPUT     0
#define GPIO_MODE_OUTPUT    1
#define GPIO_MODE_AF        2
#define GPIO_MODE_ANALOG    3

/* GPIO Speed values */
#define GPIO_SPEED_LOW      0
#define GPIO_SPEED_MED      1
#define GPIO_SPEED_HIGH     2
#define GPIO_SPEED_VHIGH    3

/* GPIO Pull values */
#define GPIO_PULL_NONE      0
#define GPIO_PULL_UP        1
#define GPIO_PULL_DOWN      2

/* Helper macros */
#define GPIO_PIN_MODE(base, pin, mode) \
    GPIO_MODER(base) = (GPIO_MODER(base) & ~(3 << (pin*2))) | ((mode) << (pin*2))

#define GPIO_PIN_AF(base, pin, af) \
    do { \
        if ((pin) < 8) \
            GPIO_AFRL(base) = (GPIO_AFRL(base) & ~(0xF << (pin*4))) | ((af) << (pin*4)); \
        else \
            GPIO_AFRH(base) = (GPIO_AFRH(base) & ~(0xF << ((pin-8)*4))) | ((af) << ((pin-8)*4)); \
    } while(0)

#define GPIO_PIN_SPEED(base, pin, speed) \
    GPIO_OSPEEDR(base) = (GPIO_OSPEEDR(base) & ~(3 << (pin*2))) | ((speed) << (pin*2))

#define GPIO_PIN_PULL(base, pin, pull) \
    GPIO_PUPDR(base) = (GPIO_PUPDR(base) & ~(3 << (pin*2))) | ((pull) << (pin*2))

#define GPIO_SET(base, pin)   GPIO_BSRR(base) = (1 << (pin))
#define GPIO_CLR(base, pin)   GPIO_BSRR(base) = (1 << ((pin) + 16))
#define GPIO_READ(base, pin)  ((GPIO_IDR(base) >> (pin)) & 1)

/* ===== ETH MAC Registers ===== */
#define ETH_BASE            0x40028000UL
#define ETH_MACCR           (*(volatile uint32_t *)(ETH_BASE + 0x0000))
#define ETH_MACECR          (*(volatile uint32_t *)(ETH_BASE + 0x0004))
#define ETH_MACPFR          (*(volatile uint32_t *)(ETH_BASE + 0x0008))
#define ETH_MACWTR          (*(volatile uint32_t *)(ETH_BASE + 0x000C))
#define ETH_MACHTR          (*(volatile uint32_t *)(ETH_BASE + 0x0010))
#define ETH_MACHTLR         (*(volatile uint32_t *)(ETH_BASE + 0x0014))
#define ETH_MACVTR          (*(volatile uint32_t *)(ETH_BASE + 0x0058))
#define ETH_MACVHTR         (*(volatile uint32_t *)(ETH_BASE + 0x005C))
#define ETH_MACIVIR         (*(volatile uint32_t *)(ETH_BASE + 0x0060))
#define ETH_MACIVHR         (*(volatile uint32_t *)(ETH_BASE + 0x0064))
#define ETH_MACA0HR         (*(volatile uint32_t *)(ETH_BASE + 0x0400))
#define ETH_MACA0LR         (*(volatile uint32_t *)(ETH_BASE + 0x0404))
#define ETH_MACA1HR         (*(volatile uint32_t *)(ETH_BASE + 0x0408))
#define ETH_MACA1LR         (*(volatile uint32_t *)(ETH_BASE + 0x040C))
#define ETH_MMCCR           (*(volatile uint32_t *)(ETH_BASE + 0x0500))
#define ETH_MMCRIR          (*(volatile uint32_t *)(ETH_BASE + 0x0504))
#define ETH_MMCTIR          (*(volatile uint32_t *)(ETH_BASE + 0x0508))

/* ETH DMA Registers (offset from ETH_BASE + 0x1000 for STM32H7) */
#define ETH_DMA_BASE        (ETH_BASE + 0x1000UL)
#define ETH_DMAMR           (*(volatile uint32_t *)(ETH_DMA_BASE + 0x0000))
#define ETH_DMASBMR         (*(volatile uint32_t *)(ETH_DMA_BASE + 0x0004))
#define ETH_DMAISR          (*(volatile uint32_t *)(ETH_DMA_BASE + 0x0028))
#define ETH_DMAIER          (*(volatile uint32_t *)(ETH_DMA_BASE + 0x002C))
#define ETH_DMACRXCR        (*(volatile uint32_t *)(ETH_DMA_BASE + 0x0050))
#define ETH_DMACRDLAR       (*(volatile uint32_t *)(ETH_DMA_BASE + 0x0054))
#define ETH_DMACRDTAR       (*(volatile uint32_t *)(ETH_DMA_BASE + 0x0058))
#define ETH_DMACRDRLR       (*(volatile uint32_t *)(ETH_DMA_BASE + 0x005C))
#define ETH_DMACRXDTR       (*(volatile uint32_t *)(ETH_DMA_BASE + 0x0060))
#define ETH_DMACCTXCR       (*(volatile uint32_t *)(ETH_DMA_BASE + 0x0070))
#define ETH_DMACTDLAR       (*(volatile uint32_t *)(ETH_DMA_BASE + 0x0074))
#define ETH_DMACTDTAR       (*(volatile uint32_t *)(ETH_DMA_BASE + 0x0078))

/* ETH MACCR bits */
#define ETH_MACCR_RE        (1 << 0)   /* Receiver enable */
#define ETH_MACCR_TE        (1 << 1)   /* Transmitter enable */
#define ETH_MACCR_DM        (1 << 13)  /* Duplex mode: 1=full */
#define ETH_MACCR_FES       (1 << 14)  /* Speed: 1=1Gbps */
#define ETH_MACCR_GPSLCE    (1 << 16)  /* Giant packet limit */

/* ETH DMA mode bits */
#define ETH_DMAMR_SWR       (1 << 0)   /* Software reset */
#define ETH_DMAMR_INTM      (1 << 12)  /* Interrupt mode */
#define ETH_DMAMR_TXPR      (1 << 11)  /* Transmit priority */

/* ETH DMA RX control bits */
#define ETH_DMACRXCR_SR     (1 << 0)   /* Start receive */
#define ETH_DMACRXCR_RBSZ   (14 << 1) /* Buffer size: 1536 = (1536/4) << 1 */

/* ETH DMA TX control bits */
#define ETH_DMACTXCR_ST     (1 << 0)   /* Start transmit */

/* ===== ETH DMA Descriptor Format (STM32H7) ===== */
typedef struct {
    volatile uint32_t tdes0;   /* Own(31), Last(30), First(29), Len(15:0) */
    volatile uint32_t tdes1;   /* Buffer 1 address (low 32 bits) */
    volatile uint32_t tdes2;   /* Buffer 2 address or next desc */
    volatile uint32_t tdes3;   /* Buffer 1 length (low 16), Buffer 2 length (high 16) */
} eth_tx_desc_t;

typedef struct {
    volatile uint32_t rdes0;   /* Own(31), Last(30), First(29), Error(15), Len(14:0) */
    volatile uint32_t rdes1;   /* Buffer 1 address (low 32 bits) */
    volatile uint32_t rdes2;   /* Buffer 2 address or next desc */
    volatile uint32_t rdes3;   /* Buffer 1 length (low 16), Buffer 2 length (high 16) */
} eth_rx_desc_t;

/* Descriptor own bits */
#define ETH_DESC_OWN        (1UL << 31)
#define ETH_DESC_LAST       (1UL << 30)
#define ETH_DESC_FIRST      (1UL << 29)
#define ETH_DESC_ERR        (1UL << 15)

/* ===== FMC SDRAM Registers ===== */
#define FMC_BASE            0xA0000000UL
#define FMC_BCR5            (*(volatile uint32_t *)0x58021440UL)
#define FMC_BTR5            (*(volatile uint32_t *)0x58021444UL)
#define FMC_SDCR1           (*(volatile uint32_t *)0x580214C0UL)
#define FMC_SDCR2           (*(volatile uint32_t *)0x580214C4UL)
#define FMC_SDTR1           (*(volatile uint32_t *)0x580214E0UL)
#define FMC_SDTR2           (*(volatile uint32_t *)0x580214E4UL)
#define FMC_SDCMR           (*(volatile uint32_t *)0x58021500UL)
#define FMC_SDRTR           (*(volatile uint32_t *)0x58021504UL)
#define FMC_SDSDR           (*(volatile uint32_t *)0x58021508UL)

/* FMC SDRAM Command Mode bits */
#define FMC_SDCMR_MODE      (0x7UL << 0)
#define FMC_SDCMR_MODE_NORMAL     0
#define FMC_SDCMR_MODE_CLK_EN    1
#define FMC_SDCMR_MODE_PALL      2
#define FMC_SDCMR_MODE_AUTOREF   3
#define FMC_SDCMR_MODE_LOAD_MR   4
#define FMC_SDCMR_MODE_SELFREF   5
#define FMC_SDCMR_MODE_POWERDOWN 6
#define FMC_SDCMR_BANK_1         (1UL << 4)
#define FMC_SDCMR_BANK_2         (1UL << 5)
#define FMC_SDCMR_MRD(val)       ((val) << 9)

/* SDRAM Mode Register value */
/* CAS=3, Sequential burst, Full page burst, Write burst length = single */
#define SDRAM_MR_VALUE     0x0023  /* CAS latency 3, burst length full page */

/* ===== SPI2 Registers ===== */
#define SPI2_BASE            0x40003800UL
#define SPI2_CR1             (*(volatile uint32_t *)(SPI2_BASE + 0x00))
#define SPI2_CR2             (*(volatile uint32_t *)(SPI2_BASE + 0x04))
#define SPI2_SR              (*(volatile uint32_t *)(SPI2_BASE + 0x08))
#define SPI2_DR              (*(volatile uint32_t *)(SPI2_BASE + 0x0C))
#define SPI2_CRCPR           (*(volatile uint32_t *)(SPI2_BASE + 0x10))
#define SPI2_RXCRCR          (*(volatile uint32_t *)(SPI2_BASE + 0x14))
#define SPI2_TXCRCR          (*(volatile uint32_t *)(SPI2_BASE + 0x18))

/* SPI CR1 bits */
#define SPI_CR1_SPE         (1 << 0)
#define SPI_CR1_MSTR        (1 << 2)
#define SPI_CR1_BR          (7 << 3)
#define SPI_CR1_CPOL        (1 << 0)  /* In CR1 */
#define SPI_CR1_CPHA        (1 << 0)  /* In CR1 */

/* SPI SR bits */
#define SPI_SR_RXNE         (1 << 0)
#define SPI_SR_TXE          (1 << 1)
#define SPI_SR_BSY          (1 << 7)

/* ===== UART4 Registers ===== */
#define UART4_BASE           0x40004C00UL
#define UART4_CR1            (*(volatile uint32_t *)(UART4_BASE + 0x00))
#define UART4_CR2            (*(volatile uint32_t *)(UART4_BASE + 0x04))
#define UART4_CR3            (*(volatile uint32_t *)(UART4_BASE + 0x08))
#define UART4_BRR            (*(volatile uint32_t *)(UART4_BASE + 0x0C))
#define UART4_RQR            (*(volatile uint32_t *)(UART4_BASE + 0x18))
#define UART4_ISR            (*(volatile uint32_t *)(UART4_BASE + 0x1C))
#define UART4_ICR            (*(volatile uint32_t *)(UART4_BASE + 0x20))
#define UART4_RDR            (*(volatile uint32_t *)(UART4_BASE + 0x24))
#define UART4_TDR            (*(volatile uint32_t *)(UART4_BASE + 0x28))

/* UART CR1 bits */
#define UART_CR1_UE         (1 << 0)
#define UART_CR1_TE         (1 << 3)
#define UART_CR1_RE         (1 << 2)
#define UART_CR1_RXNEIE     (1 << 5)

/* ===== I2C1 Registers ===== */
#define I2C1_BASE            0x40005400UL
#define I2C1_CR1             (*(volatile uint32_t *)(I2C1_BASE + 0x00))
#define I2C1_CR2             (*(volatile uint32_t *)(I2C1_BASE + 0x04))
#define I2C1_OAR1            (*(volatile uint32_t *)(I2C1_BASE + 0x08))
#define I2C1_OAR2            (*(volatile uint32_t *)(I2C1_BASE + 0x0C))
#define I2C1_TIMINGR         (*(volatile uint32_t *)(I2C1_BASE + 0x10))
#define I2C1_SR              (*(volatile uint32_t *)(I2C1_BASE + 0x18))
#define I2C1_ISR             (*(volatile uint32_t *)(I2C1_BASE + 0x1C))
#define I2C1_ICR             (*(volatile uint32_t *)(I2C1_BASE + 0x20))
#define I2C1_PECR            (*(volatile uint32_t *)(I2C1_BASE + 0x24))
#define I2C1_RXDR            (*(volatile uint32_t *)(I2C1_BASE + 0x28))
#define I2C1_TXDR            (*(volatile uint32_t *)(I2C1_BASE + 0x2C))

/* I2C CR1 bits */
#define I2C_CR1_PE           (1 << 0)
#define I2C_CR1_TXIE         (1 << 1)
#define I2C_CR1_RXIE         (1 << 2)
#define I2C_CR1_STOPIE       (1 << 4)
#define I2C_CR1_NACKIE       (1 << 5)

/* I2C ISR bits */
#define I2C_ISR_TXE          (1 << 0)
#define I2C_ISR_TXIS         (1 << 1)
#define I2C_ISR_RXNE         (1 << 2)
#define I2C_ISR_STOPF        (1 << 5)
#define I2C_ISR_NACKF        (1 << 4)
#define I2C_ISR_TC           (1 << 6)

/* ===== DMA Registers (BDMA for H7) ===== */
#define BDMA_BASE            0x58025800UL
#define BDMA_ISR             (*(volatile uint32_t *)(BDMA_BASE + 0x00))
#define BDMA_IFCR            (*(volatile uint32_t *)(BDMA_BASE + 0x04))

/* ===== MPU Registers ===== */
#define MPU_TYPE             (*(volatile uint32_t *)0xE000ED90UL)
#define MPU_CTRL             (*(volatile uint32_t *)0xE000ED94UL)
#define MPU_RNR              (*(volatile uint32_t *)0xE000ED98UL)
#define MPU_RBAR             (*(volatile uint32_t *)0xE000ED9CUL)
#define MPU_RLAR             (*(volatile uint32_t *)0xE000EDA0UL)

/* MPU CTRL bits */
#define MPU_CTRL_ENABLE      (1 << 0)
#define MPU_CTRL_HFNMIENA    (1 << 1)
#define MPU_CTRL_PRIVDEFENA  (1 << 2)

/* MPU Region Attributes */
#define MPU_RBAR_BASE(val)   ((val) & 0xFFFFFFE0UL)
#define MPU_RLAR_LIMIT(val)  ((val) & 0xFFFFFFE0UL)
#define MPU_ATTR_NORMAL      0x04ULL  /* Normal, write-back, read/write */
#define MPU_ATTR_DEVICE       0x00ULL  /* Device, non-shareable */
#define MPU_ATTR_STRONG_ORD   0x00ULL  /* Strongly ordered */

#endif /* REGISTERS_H */