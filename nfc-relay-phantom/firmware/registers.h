/*
 * NFC Relay Phantom — Register Definitions
 * STM32L4S5VIT6 MMIO registers and PN5180/EM4095 registers
 *
 * Copyright (c) 2026 Hacker Devices. Licensed under GPL-2.0.
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

/* ======================================================================
 * ARM Cortex-M4 Core Registers
 * ====================================================================== */
#define SCB_BASE           0xE000ED00UL
#define SCB_CPUID           (*(volatile uint32_t *)(SCB_BASE + 0x00))
#define SCB_ICSR            (*(volatile uint32_t *)(SCB_BASE + 0x04))
#define SCB_VTOR            (*(volatile uint32_t *)(SCB_BASE + 0x08))
#define SCB_AIRCR           (*(volatile uint32_t *)(SCB_BASE + 0x0C))
#define SCB_SCR             (*(volatile uint32_t *)(SCB_BASE + 0x10))
#define SCB_CCR             (*(volatile uint32_t *)(SCB_BASE + 0x14))
#define SCB_SHPR1           (*(volatile uint32_t *)(SCB_BASE + 0x18))
#define SCB_SHPR2           (*(volatile uint32_t *)(SCB_BASE + 0x1C))
#define SCB_SHPR3           (*(volatile uint32_t *)(SCB_BASE + 0x20))

#define NVIC_BASE           0xE000E100UL
#define NVIC_ISER0          (*(volatile uint32_t *)(NVIC_BASE + 0x00))
#define NVIC_ISER1          (*(volatile uint32_t *)(NVIC_BASE + 0x04))
#define NVIC_ISER2          (*(volatile uint32_t *)(NVIC_BASE + 0x08))
#define NVIC_ICER0          (*(volatile uint32_t *)(NVIC_BASE + 0x80))
#define NVIC_ICER1          (*(volatile uint32_t *)(NVIC_BASE + 0x84))
#define NVIC_ISPR0          (*(volatile uint32_t *)(NVIC_BASE + 0x100))
#define NVIC_ISPR1          (*(volatile uint32_t *)(NVIC_BASE + 0x104))

#define SYSTICK_BASE        0xE000E010UL
#define SYSTICK_CSR         (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYSTICK_RVR         (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYSTICK_CVR         (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))

/* ======================================================================
 * STM32L4 RCC Registers
 * ====================================================================== */
#define RCC_BASE            0x40021000UL
#define RCC_CR              (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_ICSCR           (*(volatile uint32_t *)(RCC_BASE + 0x04))
#define RCC_CFGR            (*(volatile uint32_t *)(RCC_BASE + 0x08))
#define RCC_PLLCFGR         (*(volatile uint32_t *)(RCC_BASE + 0x0C))

#define RCC_CR_HSION        (1 << 0)
#define RCC_CR_HSIRDY       (1 << 2)
#define RCC_CR_HSEON        (1 << 16)
#define RCC_CR_HSERDY       (1 << 17)
#define RCC_CR_PLLON        (1 << 24)
#define RCC_CR_PLLRDY       (1 << 25)

#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x48))
#define RCC_AHB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x4C))
#define RCC_AHB3ENR         (*(volatile uint32_t *)(RCC_BASE + 0x50))

#define RCC_AHB2ENR_GPIOAEN  (1 << 0)
#define RCC_AHB2ENR_GPIOBEN  (1 << 1)
#define RCC_AHB2ENR_GPIOCEN  (1 << 2)
#define RCC_AHB2ENR_GPIODEN  (1 << 3)
#define RCC_AHB2ENR_GPIOEEN  (1 << 4)

#define RCC_APB1ENR1        (*(volatile uint32_t *)(RCC_BASE + 0x58))
#define RCC_APB1ENR2        (*(volatile uint32_t *)(RCC_BASE + 0x5C))
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x60))

#define RCC_APB1ENR1_SPI2EN  (1 << 14)
#define RCC_APB1ENR1_USART1EN (1 << 0)
#define RCC_APB1ENR1_USART4EN (1 << 19)
#define RCC_APB1ENR1_I2C1EN   (1 << 21)
#define RCC_APB2ENR_SPI1EN    (1 << 12)
#define RCC_APB2ENR_SYSCFGEN (1 << 0)

/* ======================================================================
 * STM32L4 GPIO Registers (typedef struct style)
 * ====================================================================== */
typedef struct {
    volatile uint32_t MODER;      /* 0x00: Port mode */
    volatile uint32_t OTYPER;     /* 0x04: Output type */
    volatile uint32_t OSPEEDR;    /* 0x08: Output speed */
    volatile uint32_t PUPDR;      /* 0x0C: Pull-up/pull-down */
    volatile uint32_t IDR;        /* 0x10: Input data */
    volatile uint32_t ODR;        /* 0x14: Output data */
    volatile uint32_t BSRR;       /* 0x18: Bit set/reset */
    volatile uint32_t LCKR;       /* 0x1C: Lock */
    volatile uint32_t AFR[2];     /* 0x20-0x24: Alternate function */
    volatile uint32_t BRR;        /* 0x28: Bit reset */
    volatile uint32_t ASCR;       /* 0x2C: Analog switch control */
} GPIO_TypeDef;

#define GPIOA                ((GPIO_TypeDef *)0x40020000UL)
#define GPIOB                ((GPIO_TypeDef *)0x40020400UL)
#define GPIOC                ((GPIO_TypeDef *)0x40020800UL)
#define GPIOD                ((GPIO_TypeDef *)0x40020C00UL)
#define GPIOE                ((GPIO_TypeDef *)0x40021000UL)

/* ======================================================================
 * STM32L4 SPI Registers
 * ====================================================================== */
typedef struct {
    volatile uint32_t CR1;        /* 0x00: Control 1 */
    volatile uint32_t CR2;        /* 0x04: Control 2 */
    volatile uint32_t SR;         /* 0x08: Status */
    volatile uint32_t DR;         /* 0x0C: Data */
    volatile uint32_t CRCPR;     /* 0x10: CRC polynomial */
    volatile uint32_t RXCRCR;    /* 0x14: RX CRC */
    volatile uint32_t TXCRCR;    /* 0x18: TX CRC */
} SPI_TypeDef;

#define SPI1                 ((SPI_TypeDef *)0x40013000UL)
#define SPI2                 ((SPI_TypeDef *)0x40003800UL)

#define SPI_CR1_SPE          (1 << 6)
#define SPI_CR1_MSTR         (1 << 2)
#define SPI_CR1_BR_Pos       3
#define SPI_CR1_CPOL         (1 << 0)
#define SPI_CR1_CPHA         (1 << 1)
#define SPI_CR1_SSM          (1 << 9)
#define SPI_CR1_SSI          (1 << 8)

#define SPI_CR2_DS_Pos       8
#define SPI_CR2_DS_8BIT      (7 << SPI_CR2_DS_Pos)
#define SPI_CR2_RXNEIE       (1 << 6)
#define SPI_CR2_TXEIE        (1 << 7)
#define SPI_CR2_DMAEN       (1 << 0)

#define SPI_SR_RXNE          (1 << 0)
#define SPI_SR_TXE           (1 << 1)
#define SPI_SR_BSY           (1 << 7)
#define SPI_SR_OVR           (1 << 6)

/* ======================================================================
 * STM32L4 USART Registers
 * ====================================================================== */
typedef struct {
    volatile uint32_t CR1;        /* 0x00 */
    volatile uint32_t CR2;        /* 0x04 */
    volatile uint32_t CR3;        /* 0x08 */
    volatile uint32_t BRR;        /* 0x0C */
    volatile uint32_t GTPR;       /* 0x10 (reserved) */
    volatile uint32_t RTOR;       /* 0x14 */
    volatile uint32_t RQR;        /* 0x18 */
    volatile uint32_t ISR;        /* 0x1C */
    volatile uint32_t ICR;        /* 0x20 */
    volatile uint32_t RD_TDR;     /* 0x24: RX/TX data (alias) */
    volatile uint32_t TDR;        /* 0x28 */
} USART_TypeDef;

#define USART1               ((USART_TypeDef *)0x40011000UL)
#define USART4               ((USART_TypeDef *)0x40004C00UL)

#define USART_CR1_UE         (1 << 0)
#define USART_CR1_TE         (1 << 3)
#define USART_CR1_RE         (1 << 2)
#define USART_CR1_RXNEIE     (1 << 5)
#define USART_CR1_TCIE       (1 << 6)

#define USART_ISR_TXE        (1 << 7)
#define USART_ISR_TC         (1 << 6)
#define USART_ISR_RXNE       (1 << 5)
#define USART_ISR_ORE        (1 << 3)

/* ======================================================================
 * STM32L4 I2C Registers
 * ====================================================================== */
typedef struct {
    volatile uint32_t CR1;        /* 0x00 */
    volatile uint32_t CR2;        /* 0x04 */
    volatile uint32_t OAR1;       /* 0x08 */
    volatile uint32_t OAR2;       /* 0x0C */
    volatile uint32_t TIMINGR;    /* 0x10 */
    volatile uint32_t TIMEOUTR;   /* 0x14 */
    volatile uint32_t ISR;        /* 0x18 */
    volatile uint32_t ICR;        /* 0x1C */
    volatile uint32_t PECR;       /* 0x20 */
    volatile uint32_t RXDR;       /* 0x24 */
    volatile uint32_t TXDR;       /* 0x28 */
} I2C_TypeDef;

#define I2C1                 ((I2C_TypeDef *)0x40005400UL)

#define I2C_CR1_PE           (1 << 0)
#define I2C_CR1_TXIE         (1 << 1)
#define I2C_CR1_RXIE         (1 << 2)
#define I2C_CR1_NACKIE       (1 << 4)
#define I2C_CR1_STOPIE      (1 << 5)

#define I2C_CR2_START        (1 << 13)
#define I2C_CR2_STOP         (1 << 14)
#define I2C_CR2_RD_WRN      (1 << 10)
#define I2C_CR2_NBYTES_Pos   16

#define I2C_ISR_TXIS         (1 << 1)
#define I2C_ISR_RXNE         (1 << 2)
#define I2C_ISR_STOPF        (1 << 5)
#define I2C_ISR_NACKF        (1 << 2)
#define I2C_ISR_BUSY         (1 << 15)

/* ======================================================================
 * STM32L4 DMA Registers
 * ====================================================================== */
typedef struct {
    volatile uint32_t ISR;        /* 0x00: Interrupt status */
    uint32_t reserved0;
    volatile uint32_t CCR[8];     /* 0x08+: Channel config */
    volatile uint32_t CNDTR[8];   /* 0x28+: Number of data */
    volatile uint32_t CPAR[8];    /* 0x48+: Peripheral address */
    volatile uint32_t CMAR[8];    /* 0x68+: Memory address */
} DMA_TypeDef;

#define DMA1                 ((DMA_TypeDef *)0x40020000UL)

#define DMA_CCR_EN           (1 << 0)
#define DMA_CCR_TCIE         (1 << 1)
#define DMA_CCR_HTIE         (1 << 2)
#define DMA_CCR_DIR          (1 << 4)
#define DMA_CCR_CIRC         (1 << 5)
#define DMA_CCR_PINC         (1 << 6)
#define DMA_CCR_MINC         (1 << 10)
#define DMA_CCR_PSIZE_8BIT   (0 << 8)
#define DMA_CCR_PSIZE_16BIT  (1 << 8)
#define DMA_CCR_MSIZE_8BIT   (0 << 12)
#define DMA_CCR_MSIZE_16BIT  (1 << 12)

/* ======================================================================
 * STM32L4 Flash Registers
 * ====================================================================== */
#define FLASH_BASE           0x40022000UL
#define FLASH_ACR            (*(volatile uint32_t *)(FLASH_BASE + 0x00))
#define FLASH_ACR_LATENCY_Pos 0
#define FLASH_ACR_ICEN       (1 << 9)
#define FLASH_ACR_DCEN       (1 << 10)
#define FLASH_ACR_PRFTEN     (1 << 8)

/* ======================================================================
 * PN5180 Register Map
 * ====================================================================== */
#define PN5180_REG_SYSTEM_CONFIG       0x00U
#define PN5180_REG_IRQ_STATUS          0x04U
#define PN5180_REG_IRQ_ENABLE          0x08U
#define PN5180_REG_TX_DATA_CFG         0x14U
#define PN5180_REG_TX_MOD_DEPTH        0x18U
#define PN5180_REG_RX_CFG              0x1CU
#define PN5180_REG_RX_STATUS           0x20U
#define PN5180_REG_FIELD_STATUS        0x24U
#define PN5180_REG_FIELD_ON_TIME        0x28U
#define PN5180_REG_FIELD_OFF_TIME       0x2CU
#define PN5180_REG_CRC_CFG             0x30U
#define PN5180_REG_MISC_CFG            0x34U
#define PN5180_REG_ANAT_CONFIG         0x38U
#define PN5180_REG_RF_STATUS           0x3CU

/* PN5180 Commands */
#define PN5180_CMD_ACTIVATE_TX          0x01U
#define PN5180_CMD_DEACTIVATE_TX        0x02U
#define PN5180_CMD_ACTIVATE_RX          0x03U
#define PN5180_CMD_DEACTIVATE_RX        0x04U
#define PN5180_CMD_TRANSCEIVE           0x05U
#define PN5180_CMD_WRITE_DATA           0x08U
#define PN5180_CMD_READ_DATA            0x09U
#define PN5180_CMD_LOAD_RF_CONFIG       0x0AU

/* PN5180 IRQ Flags */
#define PN5180_IRQ_TX_DONE             (1 << 0)
#define PN5180_IRQ_RX_DONE             (1 << 1)
#define PN5180_IRQ_COLLISION            (1 << 2)
#define PN5180_IRQ_CRC_ERROR            (1 << 3)
#define PN5180_IRQ_FIELD_DET            (1 << 4)
#define PN5180_IRQ_GENERAL_ERR          (1 << 5)
#define PN5180_IRQ_RF_DET               (1 << 6)

/* ======================================================================
 * EM4095 Register-like Constants
 * (No MMIO — controlled via UART and GPIO)
 * ====================================================================== */
#define EM4095_SHD_ACTIVE    0    /* SHD=LOW → chip active */
#define EM4095_SHD_STANDBY   1    /* SHD=HIGH → standby */
#define EM4095_MOD_MANCHESTER 0   /* MOD=LOW → normal RX */
#define EM4095_MOD_DIRECT    1    /* MOD=HIGH → direct modulation */

/* ======================================================================
 * W25Q128 Commands
 * ====================================================================== */
#define W25Q_CMD_WRITE_ENABLE       0x06U
#define W25Q_CMD_WRITE_DISABLE      0x04U
#define W25Q_CMD_READ_STATUS1       0x05U
#define W25Q_CMD_WRITE_STATUS1      0x01U
#define W25Q_CMD_PAGE_PROGRAM       0x02U
#define W25Q_CMD_SECTOR_ERASE       0x20U
#define W25Q_CMD_BLOCK_ERASE_32K    0x52U
#define W25Q_CMD_BLOCK_ERASE_64K    0xD8U
#define W25Q_CMD_CHIP_ERASE         0xC7U
#define W25Q_CMD_READ_DATA          0x03U
#define W25Q_CMD_FAST_READ          0x0BU
#define W25Q_CMD_READ_ID            0x9FU
#define W25Q_CMD_READ_UID           0x4BU
#define W25Q_CMD_ENTER_QPI          0x38U
#define W25Q_CMD_EXIT_QPI           0xFFU

/* W25Q Status Bits */
#define W25Q_SR1_BUSY               (1 << 0)
#define W25Q_SR1_WEL                (1 << 1)
#define W25Q_SR1_BP_SHIFT           2
#define W25Q_SR1_SRWD               (1 << 7)

/* ======================================================================
 * BQ25896 I2C Registers
 * ====================================================================== */
#define BQ25896_REG_EN_CTRL          0x03U
#define BQ25896_REG_CHARGE_CTRL     0x04U
#define BQ25896_REG_INPUT_CTRL      0x00U
#define BQ25896_REG_SYS_CTRL        0x07U
#define BQ25896_REG_FAULT           0x0CU
#define BQ25896_REG_STATUS           0x0BU

/* ======================================================================
 * SSD1306 Commands
 * ====================================================================== */
#define SSD1306_CMD_DISPLAY_OFF      0xAEU
#define SSD1306_CMD_DISPLAY_ON       0xAFU
#define SSD1306_CMD_SET_MUX_RATIO    0xA8U
#define SSD1306_CMD_SET_DISPLAY_OFF  0xA5U
#define SSD1306_CMD_SET_DISPLAY_ON   0xA4U
#define SSD1306_CMD_SET_SEG_REMAP    0xA1U
#define SSD1306_CMD_SET_COM_SCAN     0xC8U
#define SSD1306_CMD_SET_COM_PINS     0xDAU
#define SSD1306_CMD_SET_CONTRAST     0x81U
#define SSD1306_CMD_SET_PRECHARGE    0xD9U
#define SSD1306_CMD_SET_VCOM         0xDBU
#define SSD1306_CMD_SET_OSC_FREQ     0xD5U
#define SSD1306_CMD_SET_CHARGE_PUMP  0x8DU
#define SSD1306_CMD_SET_COL_ADDR     0x21U
#define SSD1306_CMD_SET_PAGE_ADDR    0x22U
#define SSD1306_CMD_SET_START_LINE   0x40U
#define SSD1306_CMD_SET_MEMORY_MODE  0x20U

#endif /* REGISTERS_H */