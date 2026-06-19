/*
 * registers.h — STM32H743 peripheral base addresses + QCA7420 SPI register map
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef POWERLINE_REAPER_REGISTERS_H
#define POWERLINE_REAPER_REGISTERS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ===================== STM32H743 peripheral bases ===================== */
/* (Abbreviated to the peripherals actually used; full RM0455 for the rest.) */

#define RCC_BASE                0x58024400UL
#define PWR_BASE                 0x58024800UL
#define GPIOA_BASE              0x58020000UL
#define GPIOB_BASE              0x58020400UL
#define GPIOC_BASE              0x58020800UL
#define GPIOD_BASE              0x58020C00UL
#define GPIOE_BASE              0x58021000UL
#define GPIOH_BASE              0x58021C00UL

#define SPI2_BASE                0x5800B800UL
#define SDMMC2_BASE             0x58022400UL
#define USART3_BASE             0x58000400UL
#define I2C1_BASE                0x58005400UL
#define ADC1_BASE                0x58024000UL
#define TIM2_BASE                0x40000000UL
#define QUADSPI_BASE            0x58003000UL
#define USB1_OTG_HS_BASE        0x40040000UL
#define DMAMUX1_BASE            0x58020800UL /* alias, see RM */
#define DMA1_BASE               0x40020000UL
#define DMA1_STREAM0_BASE       0x40020010UL
#define DMA1_STREAM1_BASE       0x40020028UL
#define DMA1_STREAM2_BASE       0x40020040UL
#define DMA1_STREAM3_BASE       0x40020058UL
#define DMA1_STREAM4_BASE       0x40020070UL
#define DMA1_STREAM5_BASE       0x40020088UL
#define DMA1_STREAM6_BASE       0x400200A0UL
#define DMA1_STREAM7_BASE       0x400200B8UL

/* EXTI / SYSCFG */
#define SYSCFG_BASE             0x58000400UL
#define EXTI_BASE               0x58000D80UL

/* ---- GPIO register layout (offsets) ---- */
typedef struct {
    volatile uint32_t MODER;    /* 0x00 */
    volatile uint32_t OTYPER;   /* 0x04 */
    volatile uint32_t OSPEEDR;   /* 0x08 */
    volatile uint32_t PUPDR;    /* 0x0C */
    volatile uint32_t IDR;       /* 0x10 */
    volatile uint32_t ODR;       /* 0x14 */
    volatile uint32_t BSRR;      /* 0x18 */
    volatile uint32_t LCKR;      /* 0x1C */
    volatile uint32_t AFRL;     /* 0x20 */
    volatile uint32_t AFRH;     /* 0x24 */
    volatile uint32_t BRR;       /* 0x28 */
    uint32_t reserved[0x31];
} gpio_t;

/* ---- RCC enable bit helpers (simplified) ---- */
#define RCC_AHB1ENR   (*(volatile uint32_t *)(RCC_BASE + 0xD8))
#define RCC_AHB2ENR   (*(volatile uint32_t *)(RCC_BASE + 0xDC))
#define RCC_AHB3ENR   (*(volatile uint32_t *)(RCC_BASE + 0xE0))
#define RCC_AHB4ENR   (*(volatile uint32_t *)(RCC_BASE + 0xE4))
#define RCC_APB1ENR   (*(volatile uint32_t *)(RCC_BASE + 0xE8))
#define RCC_APB2ENR   (*(volatile uint32_t *)(RCC_BASE + 0xEC))
#define RCC_APB3ENR   (*(volatile uint32_t *)(RCC_BASE + 0xF0))
#define RCC_APB4ENR   (*(volatile uint32_t *)(RCC_BASE + 0xF4))

/* GPIOx bit positions in AHB4ENR */
#define RCC_AHB4ENR_GPIOA    (1U << 0)
#define RCC_AHB4ENR_GPIOB    (1U << 1)
#define RCC_AHB4ENR_GPIOC    (1U << 2)
#define RCC_AHB4ENR_GPIOD    (1U << 3)
#define RCC_AHB4ENR_GPIOE    (1U << 4)
#define RCC_AHB4ENR_GPIOH    (1U << 7)

/* SPI2 bit pos in APB1ENR */
#define RCC_APB1ENR_SPI2     (1U << 14)
/* SDMMC2 bit pos in APB4ENR */
#define RCC_APB4ENR_SDMMC2   (1U << 9)
/* USART3 bit pos in APB1ENR1 */
#define RCC_APB1ENR1_USART3  (1U << 18)
/* I2C1 bit pos in APB1ENR1 */
#define RCC_APB1ENR1_I2C1    (1U << 21)
/* TIM2 bit pos in APB1LENR */
#define RCC_APB1LENR_TIM2    (1U << 0)
/* QSPI bit pos in AHB3ENR */
#define RCC_AHB3ENR_QSPI     (1U << 14)
/* USB1 OTG HS bit pos in AHB1ENR */
#define RCC_AHB1ENR_USB1OTGHS (1U << 29)
/* ADC1 bit pos in AHB2ENR */
#define RCC_AHB2ENR_ADC12    (1U << 24)
/* DMA1 bit pos in AHB1ENR */
#define RCC_AHB1ENR_DMA1     (1U << 0)
/* DMAMUX1 bit pos in AHB1ENR */
#define RCC_AHB1ENR_DMAMUX1  (1U << 2)
/* SYSCFG bit pos in APB4ENR */
#define RCC_APB4ENR_SYSCFG   (1U << 1)

/* ---- SPI register layout (offsets; SPI2 etc identical) ---- */
typedef struct {
    volatile uint32_t CR1;       /* 0x00 */
    volatile uint32_t CR2;       /* 0x04 */
    volatile uint32_t CFG1;      /* 0x08 */
    volatile uint32_t CFG2;      /* 0x0C */
    volatile uint32_t IER;       /* 0x10 */
    volatile uint32_t SR;        /* 0x14 */
    volatile uint32_t IFCR;      /* 0x18 */
    volatile uint32_t TXDR;      /* 0x20 (8/16/32-bit access) */
    volatile uint32_t RXDR;      /* 0x24 */
    volatile uint32_t RD_PIO;    /* 0x28 */
    uint32_t reserved[7];
} spi_t;

#define SPI_CR1_CSTART      (1U << 3)
#define SPI_CR1_SPE          (1U << 1)
#define SPI_CR1_MASRX       (1U << 0)
#define SPI_CFG1_DSIZE_8    (7U << 0)
#define SPI_CFG1_DSIZE_16   (15U << 0)
#define SPI_CFG1_MBR_DIV8   (2U << 28)   /* 240/8 = 30 MHz max */
#define SPI_CFG2_MASTER     (1U << 22)
#define SPI_CFG2_SSOE       (1U << 17)
#define SPI_CFG2_CPOL_LOW   (0U << 16)
#define SPI_CFG2_CPHA_1EDGE (0U << 15)
#define SPI_SR_RXP          (1U << 0)
#define SPI_SR_TXP          (1U << 1)
#define SPI_SR_EOT          (1U << 3)
#define SPI_SR_TSERF        (1U << 4)
#define SPI_SR_BUSY         (1U << 7)
#define SPI_IFCR_EOTC       (1U << 3)
#define SPI_IFCR_TXTFC      (1U << 1)
#define SPI_IFCR_RXBFC      (1U << 0)

/* ---- USART register layout (offsets) ---- */
typedef struct {
    volatile uint32_t CR1;       /* 0x00 */
    volatile uint32_t CR2;       /* 0x04 */
    volatile uint32_t CR3;       /* 0x08 */
    volatile uint32_t BRR;       /* 0x0C */
    volatile uint32_t GTPR;      /* 0x10 */
    volatile uint32_t RTOR;      /* 0x14 */
    volatile uint32_t RQR;       /* 0x18 */
    volatile uint32_t ISR;       /* 0x1C */
    volatile uint32_t ICR;       /* 0x20 */
    volatile uint32_t RDR;       /* 0x24 */
    volatile uint32_t TDR;       /* 0x28 */
} usart_t;

#define USART_CR1_UE         (1U << 0)
#define USART_CR1_RE         (1U << 2)
#define USART_CR1_TE         (1U << 3)
#define USART_CR1_RXNEIE     (1U << 5)
#define USART_CR1_TCIE       (1U << 6)
#define USART_CR3_CTSE       (1U << 9)
#define USART_CR3_RTSE       (1U << 8)
#define USART_ISR_RXNE      (1U << 5)
#define USART_ISR_TC         (1U << 6)
#define USART_ISR_BUSY       (1U << 16)

/* ---- I2C register layout (offsets) ---- */
typedef struct {
    volatile uint32_t CR1;       /* 0x00 */
    volatile uint32_t CR2;       /* 0x04 */
    volatile uint32_t OAR1;      /* 0x08 */
    volatile uint32_t OAR2;      /* 0x0C */
    volatile uint32_t TIMINGR;   /* 0x10 */
    volatile uint32_t TIMEOUTR;  /* 0x14 */
    volatile uint32_t ISR;       /* 0x18 */
    volatile uint32_t ICR;       /* 0x1C */
    volatile uint32_t PECR;      /* 0x20 */
    volatile uint32_t RXDR;      /* 0x24 */
    volatile uint32_t TXDR;      /* 0x28 */
} i2c_t;

#define I2C_CR1_PE          (1U << 0)
#define I2C_CR2_START       (1U << 13)
#define I2C_CR2_STOP        (1U << 14)
#define I2C_CR2_RD_WRN      (1U << 10)
#define I2C_CR2_NBYTES(sh)  (((sh) & 0xFF) << 16)
#define I2C_ISR_RXNE        (1U << 2)
#define I2C_ISR_TXIS        (1U << 1)
#define I2C_ISR_TC          (1U << 6)
#define I2C_ISR_BUSY        (1U << 15)

/* ===================== QCA7420 SPI register map ===================== */
/* The QCA7420 exposes a windowed SPI register interface. Two 8-bit
 * register accesses (write addr, read/write data) form each transaction.
 * Key registers (from the QCA7420 Programming Guide):
 */

#define QCA7420_SPI_CMD_READ    0x00
#define QCA7420_SPI_CMD_WRITE   0x40
#define QCA7420_SPI_CMD_WR_INC  0x60

#define QCA7420_REG_SCR         0x8F  /* SPI Control Register */
#define QCA7420_REG_SFR         0x90  /* SPI Status / FIFO     */
#define QCA7420_REG_DFW         0x91  /* Data FIFO Write       */
#define QCA7420_REG_DFR         0x92  /* Data FIFO Read        */
#define QCA7420_REG_IODIR       0x93  /* GPIO direction         */
#define QCA7420_REG_IODAT       0x94  /* GPIO data              */

/* MAC MIB registers (memory-mapped via SPI window) */
#define QCA7420_MIB_RXCTL      0x0000  /* RX control: promisc bit 7 */
#define QCA7420_MIB_TXCTL      0x0004
#define QCA7420_MIB_MACADDR    0x0010  /* 6 bytes                  */
#define QCA7420_MIB_NMK        0x0020  /* 16 bytes Network Membership Key */
#define QCA7420_MIB_NEK        0x0030  /* 16 bytes Network Encryption Key  */
#define QCA7420_MIB_NETID      0x0040  /* 7 bytes Network ID string       */
#define QCA7420_MIB_CCO_MAC    0x0050
#define QCA7420_MIB_CCO_PRI    0x0056
#define QCA7420_MIB_TEI        0x0058  /* this station's TEI             */
#define QCA7420_MIB_PROMISC    0x0060  /* PromiscuousMode flag           */

/* MIB_RXCTL / TXCTL bits */
#define QCA7420_RXCTL_PROMISC  (1U << 7)
#define QCA7420_RXCTL_RXEN     (1U << 0)
#define QCA7420_TXCTL_TXEN      (1U << 0)
#define QCA7420_TXCTL_BEACON    (1U << 4)
#define QCA7420_TXCTL_MGMT      (1U << 5)

/* MAC management message opcodes (IEEE 1901) */
#define QCA7420_OPC_BEACON          0x01
#define QCA7420_OPC_DISCOVER        0x02
#define QCA7420_OPC_ASSOC_REQ       0x03
#define QCA7420_OPC_ASSOC_CNF       0x04
#define QCA7420_OPC_DISASSOC_REQ    0x07
#define QCA7420_OPC_SET_ENCRYPT_KEY 0x0C
#define QCA7420_OPC_GET_NMK         0x0D

/* DMA ring sizes */
#define PLC_RX_RING_LEN      32
#define PLC_TX_RING_LEN       16
#define SD_PCAP_BUF_BYTES  (32 * 1024)
#define EXTERNAL_FLASH_BYTES (16 * 1024 * 1024)
#define INTERNAL_FLASH_BYTES (1 * 1024 * 1024)

/* LED color helpers (TIM2 channels) */
#define LED_PWM_PERIOD       1000   /* 1 kHz tick */

#ifdef __cplusplus
}
#endif

#endif /* POWERLINE_REAPER_REGISTERS_H */