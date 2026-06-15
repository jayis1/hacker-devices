/*
 * registers.h — STM32H743 register definitions for Infrared Phantom
 * Direct MMIO register access, no HAL dependency
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

/* ========================================
 * Base addresses
 * ======================================== */

#define RCC_BASE                0x40040400U
#define PWR_BASE                0x40007000U
#define FLASH_BASE_REG          0x52002000U
#define GPIOA_BASE              0x58020000U
#define GPIOB_BASE              0x58020400U
#define GPIOC_BASE              0x58020800U
#define GPIOD_BASE              0x58020C00U
#define GPIOE_BASE              0x58021000U
#define DMA1_BASE               0x40020000U
#define DMA2_BASE               0x40020400U
#define DMAMUX1_BASE            0x40020800U
#define SPI1_BASE               0x40013000U
#define SPI2_BASE               0x40003800U
#define SPI3_BASE               0x40003C00U
#define USART1_BASE             0x40011000U
#define SDMMC1_BASE             0x40016000U
#define TIM2_BASE               0x40000000U
#define TIM3_BASE               0x40000400U
#define ADC1_BASE               0x40022000U
#define ADC12_COMMON_BASE       0x40022300U
#define USB_OTG_HS_BASE         0x40040000U
#define SYSCFG_BASE             0x58000400U
#define NVIC_BASE               0xE000E100U
#define SCB_BASE                0xE000ED00U
#define CORTEX_M7_DCB_BASE     0xE000EDF0U

/* ========================================
 * RCC registers
 * ======================================== */

typedef struct {
    volatile uint32_t CR;           /* 0x00: Clock control register */
    volatile uint32_t HSICFGR;     /* 0x04: HSI configuration */
    volatile uint32_t CRRCR;       /* 0x08: RC clocks configuration */
    volatile uint32_t CSR;         /* 0x0C: Clock security & recovery */
    volatile uint32_t PLLCKSELR;  /* 0x10: PLL clock selection */
    volatile uint32_t PLLCFGR;    /* 0x14: PLL configuration */
    volatile uint32_t PLL1DIVR;   /* 0x18: PLL1 dividers */
    volatile uint32_t PLL2DIVR;   /* 0x1C: PLL2 dividers */
    volatile uint32_t PLL3DIVR;   /* 0x20: PLL3 dividers */
    volatile uint32_t PLL1FRACR;  /* 0x24: PLL1 fractional divider */
    volatile uint32_t PLL2FRACR;  /* 0x28: PLL2 fractional divider */
    volatile uint32_t PLL3FRACR;  /* 0x2C: PLL3 fractional divider */
    volatile uint32_t RESERVED0;   /* 0x30 */
    volatile uint32_t RESERVED1;   /* 0x34 */
    volatile uint32_t D1CFGR;     /* 0x38: Domain 1 configuration */
    volatile uint32_t D2CFGR;     /* 0x3C: Domain 2 configuration */
    volatile uint32_t D3CFGR;     /* 0x40: Domain 3 configuration */
    volatile uint32_t RESERVED2;   /* 0x44 */
    volatile uint32_t RESERVED3;   /* 0x48 */
    volatile uint32_t CKGR;       /* 0x4C: Clock gating (legacy) */
    volatile uint32_t RESERVED4[3];/* 0x50-0x58 */
    volatile uint32_t D1CCIPR;    /* 0x5C: Domain 1 peripheral clock sel */
    volatile uint32_t D2CCIP1R;   /* 0x60: Domain 2 periph clock sel 1 */
    volatile uint32_t D2CCIP2R;   /* 0x64: Domain 2 periph clock sel 2 */
    volatile uint32_t RESERVED5;   /* 0x68 */
    volatile uint32_t D3CCIPR;    /* 0x6C: Domain 3 periph clock sel */
    volatile uint32_t RESERVED6;   /* 0x70 */
    volatile uint32_t RESERVED7;   /* 0x74 */
    volatile uint32_t RESERVED8;   /* 0x78 */
    volatile uint32_t RESERVED9;   /* 0x7C */
    volatile uint32_t RESERVED10[4];/* 0x80-0x8C */
    volatile uint32_t RESERVED11[4];/* 0x90-0x9C */
    volatile uint32_t RESERVED12[4];/* 0xA0-0xAC */
    volatile uint32_t RESERVED13[4];/* 0xB0-0xBC */
    volatile uint32_t RESERVED14[4];/* 0xC0-0xCC */
    volatile uint32_t GCR;         /* 0xD0: Global control register */
    volatile uint32_t RESERVED15[3];/* 0xD4-0xDC */
    volatile uint32_t AHB1ENR;    /* 0xE0: AHB1 peripheral clock enable */
    volatile uint32_t AHB2ENR;    /* 0xE4: AHB2 peripheral clock enable */
    volatile uint32_t AHB3ENR;    /* 0xE8: AHB3 peripheral clock enable */
    volatile uint32_t AHB4ENR;    /* 0xEC: AHB4 peripheral clock enable */
    volatile uint32_t RESERVED16;
    volatile uint32_t APB1LENR;   /* 0xF4: APB1L peripheral clock enable */
    volatile uint32_t APB1HENR;   /* 0xF8: APB1H peripheral clock enable */
    volatile uint32_t APB2ENR;     /* 0xFC: APB2 peripheral clock enable */
    volatile uint32_t APB3ENR;     /* 0x100: APB3 peripheral clock enable */
    volatile uint32_t RESERVED17;
    volatile uint32_t APB4ENR;     /* 0x104: APB4 peripheral clock enable */
} RCC_TypeDef;

#define RCC     ((RCC_TypeDef *)RCC_BASE)

/* RCC CR bits */
#define RCC_CR_HSEON           (1U << 16)
#define RCC_CR_HSERDY          (1U << 17)
#define RCC_CR_PLL1ON           (1U << 24)
#define RCC_CR_PLL1RDY          (1U << 25)
#define RCC_CR_PLL2ON           (1U << 26)
#define RCC_CR_PLL2RDY          (1U << 27)

/* RCC PLLCKSELR bits */
#define RCC_PLLCKSELR_DIVM1_Pos    0U
#define RCC_PLLCKSELR_PLL1SRC_Pos  0U
#define RCC_PLLCKSELR_PLL1SRC_HSE  (3U << 0)

/* RCC AHB4ENR bits */
#define RCC_AHB4ENR_GPIOAEN    (1U << 0)
#define RCC_AHB4ENR_GPIOBEN    (1U << 1)
#define RCC_AHB4ENR_GPIOCEN    (1U << 2)
#define RCC_AHB4ENR_GPIODEN    (1U << 3)
#define RCC_AHB4ENR_GPIOEEN    (1U << 4)

/* RCC APB1LENR bits */
#define RCC_APB1LENR_TIM2EN    (1U << 0)
#define RCC_APB1LENR_TIM3EN    (1U << 1)
#define RCC_APB1LENR_SPI3EN    (1U << 15)
#define RCC_APB1LENR_USART1EN  (1U << 16)

/* RCC APB2ENR bits */
#define RCC_APB2ENR_SPI1EN     (1U << 12)
#define RCC_APB2ENR_SPI2EN     (1U << 13)
#define RCC_APB2ENR_USART1EN  (1U << 4)
#define RCC_APB2ENR_ADC1EN     (1U << 16)

/* RCC APB4ENR bits */
#define RCC_APB4ENR_SYSCFGEN   (1U << 0)
#define RCC_APB4ENR_PWREN       (1U << 1)

/* ========================================
 * PWR registers
 * ======================================== */

typedef struct {
    volatile uint32_t CR1;        /* 0x00 */
    volatile uint32_t CR2;        /* 0x04 */
    volatile uint32_t CR3;        /* 0x08 */
    volatile uint32_t CR4;        /* 0x0C */
    volatile uint32_t CR5;        /* 0x10 */
    volatile uint32_t RESERVED0;
    volatile uint32_t RESERVED1;
    volatile uint32_t SR1;        /* 0x1C */
    volatile uint32_t SR2;        /* 0x20 */
    volatile uint32_t RESERVED2[2];
    volatile uint32_t CSR1;       /* 0x2C */
    volatile uint32_t CSR2;       /* 0x30 */
    volatile uint32_t RESERVED3[2];
    volatile uint32_t WKUPEFR;    /* 0x3C */
    volatile uint32_t RESERVED4[2];
    volatile uint32_t WKUPCR;     /* 0x48 */
} PWR_TypeDef;

#define PWR     ((PWR_TypeDef *)PWR_BASE)

#define PWR_CR3_VOS_Pos        0U
#define PWR_CR3_VOS_0          (1U << 0)   /* VOS = 1 (SCALE1) */
#define PWR_CSR1_VOSRDY        (1U << 13)

/* ========================================
 * GPIO registers
 * ======================================== */

typedef struct {
    volatile uint32_t MODER;      /* 0x00: Port mode register */
    volatile uint32_t OTYPER;     /* 0x04: Port output type register */
    volatile uint32_t OSPEEDR;    /* 0x08: Port output speed register */
    volatile uint32_t PUPDR;      /* 0x0C: Port pull-up/pull-down register */
    volatile uint32_t IDR;        /* 0x10: Port input data register */
    volatile uint32_t ODR;        /* 0x14: Port output data register */
    volatile uint32_t BSRR;       /* 0x18: Port bit set/reset register */
    volatile uint32_t LCKR;       /* 0x1C: Port configuration lock register */
    volatile uint32_t AFR[2];     /* 0x20-0x24: Alternate function registers */
    volatile uint32_t BRR;        /* 0x28: Port bit reset register */
} GPIO_TypeDef;

#define GPIOA   ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB   ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC   ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD   ((GPIO_TypeDef *)GPIOD_BASE)
#define GPIOE   ((GPIO_TypeDef *)GPIOE_BASE)

/* GPIO mode values */
#define GPIO_MODE_INPUT          0U
#define GPIO_MODE_OUTPUT         1U
#define GPIO_MODE_AF             2U
#define GPIO_MODE_ANALOG         3U

/* GPIO speed values */
#define GPIO_SPEED_LOW           0U
#define GPIO_SPEED_MEDIUM        1U
#define GPIO_SPEED_HIGH          2U
#define GPIO_SPEED_VERY_HIGH    3U

/* GPIO pull values */
#define GPIO_PULL_NONE           0U
#define GPIO_PULL_UP             1U
#define GPIO_PULL_DOWN           2U

/* ========================================
 * TIM registers (TIM2, TIM3)
 * ======================================== */

typedef struct {
    volatile uint32_t CR1;        /* 0x00: Control register 1 */
    volatile uint32_t CR2;        /* 0x04: Control register 2 */
    volatile uint32_t SMCR;       /* 0x08: Slave mode control register */
    volatile uint32_t DIER;       /* 0x0C: DMA/interrupt enable register */
    volatile uint32_t SR;         /* 0x10: Status register */
    volatile uint32_t EGR;        /* 0x14: Event generation register */
    volatile uint32_t CCMR1;      /* 0x18: Capture/compare mode 1 */
    volatile uint32_t CCMR2;      /* 0x1C: Capture/compare mode 2 */
    volatile uint32_t CCER;       /* 0x20: Capture/compare enable register */
    volatile uint32_t CNT;        /* 0x24: Counter */
    volatile uint32_t PSC;        /* 0x28: Prescaler */
    volatile uint32_t ARR;        /* 0x2C: Auto-reload register */
    volatile uint32_t RESERVED0;
    volatile uint32_t CCR1;       /* 0x34: Capture/compare 1 */
    volatile uint32_t CCR2;       /* 0x38: Capture/compare 2 */
    volatile uint32_t CCR3;       /* 0x3C: Capture/compare 3 */
    volatile uint32_t CCR4;       /* 0x40: Capture/compare 4 */
    volatile uint32_t RESERVED1;
    volatile uint32_t DCR;        /* 0x48: DMA control register */
    volatile uint32_t DMAR;       /* 0x4C: DMA address for transfer */
} TIM_TypeDef;

#define TIM2    ((TIM_TypeDef *)TIM2_BASE)
#define TIM3    ((TIM_TypeDef *)TIM3_BASE)

/* TIM CR1 bits */
#define TIM_CR1_CEN             (1U << 0)
#define TIM_CR1_UDIS            (1U << 1)
#define TIM_CR1_URS             (1U << 2)
#define TIM_CR1_ARPE            (1U << 7)

/* TIM DIER bits */
#define TIM_DIER_UIE            (1U << 0)
#define TIM_DIER_CC1IE          (1U << 1)
#define TIM_DIER_CC2IE          (1U << 2)
#define TIM_DIER_DMAEN          (1U << 8)
#define TIM_DIER_CC1DE          (1U << 9)
#define TIM_DIER_TDE            (1U << 14)

/* TIM CCMR1 bits */
#define TIM_CCMR1_CC1S_Pos      0U
#define TIM_CCMR1_CC1S_INPUT_TI1 (1U << 0)
#define TIM_CCMR1_CC1S_INPUT_TI2 (2U << 0)
#define TIM_CCMR1_IC1F_Pos      4U
#define TIM_CCMR1_OC1M_Pos      4U
#define TIM_CCMR1_OC1M_PWM1     (6U << 4)
#define TIM_CCMR1_OC1PE          (1U << 3)
#define TIM_CCMR1_CC2S_Pos      8U
#define TIM_CCMR1_OC2M_Pos      12U
#define TIM_CCMR1_OC2M_PWM1     (6U << 12)
#define TIM_CCMR1_OC2PE          (1U << 11)

/* TIM CCER bits */
#define TIM_CCER_CC1E           (1U << 0)
#define TIM_CCER_CC1P           (1U << 1)
#define TIM_CCER_CC2E           (1U << 4)
#define TIM_CCER_CC2P           (1U << 5)

/* ========================================
 * ADC registers
 * ======================================== */

typedef struct {
    volatile uint32_t ISR;        /* 0x00: Interrupt & status register */
    volatile uint32_t IER;       /* 0x04: Interrupt enable register */
    volatile uint32_t CR;         /* 0x08: Control register */
    volatile uint32_t CFGR;       /* 0x0C: Configuration register */
    volatile uint32_t CFGR2;      /* 0x10: Configuration register 2 */
    volatile uint32_t SMPR1;      /* 0x14: Sample time register 1 */
    volatile uint32_t SMPR2;      /* 0x18: Sample time register 2 */
    volatile uint32_t RESERVED0;
    volatile uint32_t TR1;        /* 0x20: Watchdog threshold 1 */
    volatile uint32_t TR2;        /* 0x24: Watchdog threshold 2 */
    volatile uint32_t TR3;        /* 0x28: Watchdog threshold 3 */
    volatile uint32_t RESERVED1[4];
    volatile uint32_t JDR1;       /* 0x3C: Injected data 1 */
    volatile uint32_t JDR2;       /* 0x40: Injected data 2 */
    volatile uint32_t RESERVED2;
    volatile uint32_t DR;         /* 0x48: Regular data */
    volatile uint32_t RESERVED3[2];
    volatile uint32_t AWD2CR;     /* 0x58: Analog watchdog 2 config */
    volatile uint32_t AWD3CR;     /* 0x5C: Analog watchdog 3 config */
    volatile uint32_t RESERVED4;
    volatile uint32_t JSQR;       /* 0x64: Injected sequence */
    volatile uint32_t RESERVED5[2];
    volatile uint32_t CFGR;       /* 0x70: (alternate) */
    volatile uint32_t RESERVED6;
    volatile uint32_t PCSEL;      /* 0x78: Pre-selection */
} ADC_TypeDef;

#define ADC1    ((ADC_TypeDef *)ADC1_BASE)

/* ADC CR bits */
#define ADC_CR_ADEN             (1U << 0)
#define ADC_CR_ADDIS            (1U << 1)
#define ADC_CR_ADSTART          (1U << 2)
#define ADC_CR_ADSTP            (1U << 4)
#define ADC_CR_ADCAL            (1U << 16)
#define ADC_CR_ADVREGEN         (1U << 28)
#define ADC_CR_CALVREFSEL_Pos   30U

/* ADC CFGR bits */
#define ADC_CFGR_DMAEN          (1U << 0)
#define ADC_CFGR_DMACFG         (1U << 1)
#define ADC_CFGR_CONT           (1U << 13)
#define ADC_CFGR_OVRDIS         (1U << 12)
#define ADC_CFGR_RES_Pos        2U
#define ADC_CFGR_ALIGN          (1U << 15)

/* ADC ISR bits */
#define ADC_ISR_ADRDY           (1U << 0)
#define ADC_ISR_EOC             (1U << 2)
#define ADC_ISR_OVR             (1U << 4)

/* ========================================
 * SPI registers
 * ======================================== */

typedef struct {
    volatile uint32_t CR1;        /* 0x00: Control register 1 */
    volatile uint32_t CR2;        /* 0x04: Control register 2 */
    volatile uint32_t SR;         /* 0x08: Status register */
    volatile uint32_t DR;         /* 0x0C: Data register */
    volatile uint32_t CRCPR;      /* 0x10: CRC polynomial register */
    volatile uint32_t RXCRCR;     /* 0x14: RX CRC register */
    volatile uint32_t TXCRCR;     /* 0x18: TX CRC register */
} SPI_TypeDef;

#define SPI1    ((SPI_TypeDef *)SPI1_BASE)
#define SPI2    ((SPI_TypeDef *)SPI2_BASE)

/* SPI CR1 bits */
#define SPI_CR1_CPHA            (1U << 0)
#define SPI_CR1_CPOL            (1U << 1)
#define SPI_CR1_MSTR            (1U << 2)
#define SPI_CR1_BR_Pos          3U
#define SPI_CR1_SPE             (1U << 6)
#define SPI_CR1_LSBFIRST        (1U << 7)
#define SPI_CR1_SSI             (1U << 8)
#define SPI_CR1_SSM             (1U << 9)
#define SPI_CR1_RXONLY          (1U << 10)

/* SPI SR bits */
#define SPI_SR_RXNE             (1U << 0)
#define SPI_SR_TXE              (1U << 1)
#define SPI_SR_BSY              (1U << 7)

/* ========================================
 * USART registers
 * ======================================== */

typedef struct {
    volatile uint32_t CR1;        /* 0x00: Control register 1 */
    volatile uint32_t CR2;        /* 0x04: Control register 2 */
    volatile uint32_t CR3;        /* 0x08: Control register 3 */
    volatile uint32_t BRR;        /* 0x0C: Baud rate register */
    volatile uint32_t GTPR;       /* 0x10: Guard time and prescaler */
    volatile uint32_t RTOR;       /* 0x14: Receiver timeout register */
    volatile uint32_t RQR;        /* 0x18: Request register */
    volatile uint32_t ISR;        /* 0x1C: Interrupt & status register */
    volatile uint32_t ICR;        /* 0x20: Interrupt flag clear register */
    volatile uint32_t RDR;        /* 0x24: Receive data register */
    volatile uint32_t TDR;        /* 0x28: Transmit data register */
} USART_TypeDef;

#define USART1  ((USART_TypeDef *)USART1_BASE)

/* USART CR1 bits */
#define USART_CR1_UE            (1U << 0)
#define USART_CR1_TE            (1U << 3)
#define USART_CR1_RE            (1U << 2)
#define USART_CR1_RXNEIE        (1U << 5)
#define USART_CR1_TCIE          (1U << 6)

/* USART ISR bits */
#define USART_ISR_TXE           (1U << 7)
#define USART_ISR_RXNE          (1U << 5)
#define USART_ISR_TC            (1U << 6)

/* ========================================
 * W25Q128 flash commands
 * ======================================== */

#define W25Q_JEDEC_ID           0x9FU
#define W25Q_READ                0x03U
#define W25Q_FAST_READ           0x0BU
#define W25Q_WRITE_ENABLE        0x06U
#define W25Q_WRITE_DISABLE       0x04U
#define W25Q_READ_SR1            0x05U
#define W25Q_READ_SR2            0x35U
#define W25Q_READ_SR3            0x15U
#define W25Q_WRITE_SR1           0x01U
#define W25Q_WRITE_SR2           0x31U
#define W25Q_PAGE_PROGRAM        0x02U
#define W25Q_SECTOR_ERASE        0x20U
#define W25Q_BLOCK_ERASE_32K     0x52U
#define W25Q_BLOCK_ERASE_64K     0xD8U
#define W25Q_CHIP_ERASE          0xC7U
#define W25Q_RELEASE_PWRDOWN     0xABU

/* Expected JEDEC ID for W25Q128JVSIQ */
#define W25Q_JEDEC_EXPECTED      0xEF4018U

/* ========================================
 * SSD1306 OLED commands
 * ======================================== */

#define SSD1306_DISPLAY_OFF          0xAEU
#define SSD1306_DISPLAY_ON          0xAFU
#define SSD1306_SET_MUX_RATIO       0xA8U
#define SSD1306_SET_DISPLAY_OFFSET  0xD3U
#define SSD1306_SET_START_LINE      0x40U
#define SSD1306_SET_SEGMENT_REMAP   0xA1U
#define SSD1306_SET_COM_SCAN_DIR    0xC8U
#define SSD1306_SET_COM_PINS        0xDAU
#define SSD1306_SET_CONTRAST        0x81U
#define SSD1306_SET_PRECHARGE       0xD9U
#define SSD1306_SET_VCOM_DETECT     0xDBU
#define SSD1306_SET_CLOCK_DIV       0xD5U
#define SSD1306_CHARGE_PUMP         0x8DU
#define SSD1306_SET_MEMORY_MODE     0x20U
#define SSD1306_SET_COL_ADDR        0x21U
#define SSD1306_SET_PAGE_ADDR       0x22U
#define SSD1306_NORMAL_DISPLAY      0xA6U
#define SSD1306_INVERT_DISPLAY      0xA7U
#define SSD1306_ALL_ON              0xA5U
#define SSD1306_ALL_OFF             0xA4U

/* OLED dimensions */
#define SSD1306_WIDTH           128U
#define SSD1306_HEIGHT           32U
#define SSD1306_PAGES            4U   /* 32 / 8 */

/* ========================================
 * DMA register structure (STM32H7 simplified)
 * ======================================== */

typedef struct {
    volatile uint32_t CR;         /* 0x00: Stream configuration */
    volatile uint32_t NDTR;       /* 0x04: Number of data items */
    volatile uint32_t PAR;        /* 0x08: Peripheral address */
    volatile uint32_t M0AR;       /* 0x0C: Memory 0 address */
    volatile uint32_t M1AR;       /* 0x10: Memory 1 address */
    volatile uint32_t FCR;        /* 0x14: FIFO control register */
} DMA_Stream_TypeDef;

/* DMA stream offsets: each stream is 0x18 bytes apart */
#define DMA1_STREAM0    ((DMA_Stream_TypeDef *)(DMA1_BASE + 0x10))
#define DMA1_STREAM1    ((DMA_Stream_TypeDef *)(DMA1_BASE + 0x28))
#define DMA1_STREAM2    ((DMA_Stream_TypeDef *)(DMA1_BASE + 0x40))
#define DMA1_STREAM3    ((DMA_Stream_TypeDef *)(DMA1_BASE + 0x58))
#define DMA1_STREAM4    ((DMA_Stream_TypeDef *)(DMA1_BASE + 0x70))
#define DMA1_STREAM5    ((DMA_Stream_TypeDef *)(DMA1_BASE + 0x88))
#define DMA1_STREAM6    ((DMA_Stream_TypeDef *)(DMA1_BASE + 0xA0))
#define DMA1_STREAM7    ((DMA_Stream_TypeDef *)(DMA1_BASE + 0xB8))

/* DMA CR bits */
#define DMA_CR_EN                (1U << 0)
#define DMA_CR_TCIE             (1U << 4)
#define DMA_CR_HTIE             (1U << 3)
#define DMA_CR_TEIE             (1U << 2)
#define DMA_CR_DMEIE            (1U << 1)
#define DMA_CR_CIRC             (1U << 8)
#define DMA_CR_PINC             (1U << 9)
#define DMA_CR_MINC             (1U << 10)
#define DMA_CR_PSIZE_8B         (0U << 11)
#define DMA_CR_PSIZE_16B        (1U << 11)
#define DMA_CR_PSIZE_32B        (2U << 11)
#define DMA_CR_MSIZE_8B         (0U << 13)
#define DMA_CR_MSIZE_16B        (1U << 13)
#define DMA_CR_MSIZE_32B        (2U << 13)
#define DMA_CR_DIR_P2M          (0U << 6)
#define DMA_CR_DIR_M2P          (1U << 6)
#define DMA_CR_DIR_M2M          (2U << 6)

/* ========================================
 * SCB and NVIC (Cortex-M7 core)
 * ======================================== */

typedef struct {
    volatile uint32_t ISER[8];    /* Interrupt Set Enable */
    volatile uint32_t RESERVED0[24];
    volatile uint32_t ICER[8];    /* Interrupt Clear Enable */
    volatile uint32_t RESERVED1[24];
    volatile uint32_t ISPR[8];    /* Interrupt Set Pending */
    volatile uint32_t RESERVED2[24];
    volatile uint32_t ICPR[8];    /* Interrupt Clear Pending */
} NVIC_TypeDef;

#define NVIC    ((NVIC_TypeDef *)NVIC_BASE)

typedef struct {
    volatile uint32_t CPUID;      /* 0x00 */
    volatile uint32_t ICSR;       /* 0x04 */
    volatile uint32_t VTOR;       /* 0x08 */
    volatile uint32_t AIRCR;      /* 0x0C */
    volatile uint32_t SCR;        /* 0x10 */
    volatile uint32_t CCR;        /* 0x14 */
    volatile uint32_t SHPR[3];    /* 0x18-0x20 */
    volatile uint32_t SHCSR;      /* 0x24 */
} SCB_TypeDef;

#define SCB     ((SCB_TypeDef *)SCB_BASE)

#endif /* REGISTERS_H */