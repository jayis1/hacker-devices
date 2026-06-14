/*
 * registers.h — CC1352P Hardware Register Definitions
 * Substation Sub-GHz IoT Gateway Implant
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================
 * Memory-Mapped Register Access Macros
 * ============================================ */
#define REG32(addr)    (*(volatile uint32_t *)(addr))
#define REG16(addr)    (*(volatile uint16_t *)(addr))
#define REG8(addr)     (*(volatile uint8_t  *)(addr))

#define BIT(n)         (1UL << (n))

/* ============================================
 * Clock Control Registers
 * ============================================ */
#define SCLK_HF_MCLK          REG32(0x400C0000)
#define SCLK_MF_CTL            REG32(0x400C0004)
#define SCLK_LF_CTL            REG32(0x400C0008)
#define DPLL_MCLK_CFG0         REG32(0x400C0010)
#define DPLL_MCLK_CFG1         REG32(0x400C0014)
#define DPLL_MCLK_STATUS       REG32(0x400C0018)
#define XOSC_HF_CTRL           REG32(0x400C0020)
#define XOSC_LF_CTRL           REG32(0x400C0024)

/* Clock control bit fields */
#define XOSC_HF_ENABLE         BIT(0)
#define XOSC_HF_RDY            BIT(1)
#define XOSC_HF_XOSC_HF_FAST_START  BIT(4)
#define SCLK_HF_SRC_DPLL      (2 << 0)
#define SCLK_HF_SRC_XOSC      (1 << 0)
#define SCLK_MF_SRC_XOSC_HF   BIT(0)
#define SCLK_MF_DIV(n)         ((n) << 8)
#define SCLK_LF_SRC_XOSC_LF   BIT(0)
#define DPLL_MCLK_ENABLE       BIT(0)
#define DPLL_MCLK_LOCKED       BIT(0)
#define DPLL_MCLK_REF_DIV(n)   ((n) << 16)
#define DPLL_MCLK_MULT(n)      ((n) << 0)

/* ============================================
 * GPIO Registers
 * ============================================ */
#define GPIO_DOE0              REG32(0x40022000)
#define GPIO_DOE1              REG32(0x40022004)
#define GPIO_DOUT0             REG32(0x40022008)
#define GPIO_DOUT1             REG32(0x4002200C)
#define GPIO_DIN0              REG32(0x40022010)
#define GPIO_DIN1              REG32(0x40022014)
#define GPIO_EV0               REG32(0x40022018)
#define GPIO_EV1               REG32(0x4002201C)

/* GPIO Control Registers (per-pin) */
#define GPIO_CTRL(n)           REG32(0x40022080 + ((n) * 4))

/* GPIO Control bit fields */
#define GPIO_MODE_INPUT        (0 << 0)
#define GPIO_MODE_OUTPUT       (1 << 0)
#define GPIO_MODE_PERIPHERAL   (2 << 0)
#define GPIO_PULL_UP           BIT(3)
#define GPIO_PULL_DOWN         BIT(4)
#define GPIO_SLEW_FAST        BIT(5)
#define GPIO_HYSTERESIS        BIT(6)
#define GPIO_OPEN_DRAIN        BIT(7)

/* ============================================
 * SSI (SPI) Registers — Per-instance macros
 * ============================================ */
#define SSI_CR0(base)          REG32((base) + 0x000)
#define SSI_CR1(base)          REG32((base) + 0x004)
#define SSI_DR(base)           REG32((base) + 0x008)
#define SSI_SR(base)           REG32((base) + 0x00C)
#define SSI_CPSR(base)         REG32((base) + 0x010)
#define SSI_IM(base)           REG32((base) + 0x014)
#define SSI_RIS(base)          REG32((base) + 0x018)
#define SSI_MIS(base)          REG32((base) + 0x01C)
#define SSI_ICR(base)          REG32((base) + 0x020)
#define SSI_DMACR(base)        REG32((base) + 0x024)

/* SSI CR0 bit fields */
#define SSI_CR0_SPH            BIT(4)
#define SSI_CR0_SPO            BIT(3)
#define SSI_CR0_FRF_SPI        (0 << 5)
#define SSI_CR0_DSS_8BIT       (7 << 0)
#define SSI_CR0_DSS_16BIT      (15 << 0)

/* SSI CR1 bit fields */
#define SSI_CR1_SSE            BIT(1)
#define SSI_CR1_MS             BIT(2)
#define SSI_CR1_LBM            BIT(0)

/* SSI SR bit fields */
#define SSI_SR_TFE             BIT(0)   /* TX FIFO empty */
#define SSI_SR_TNF             BIT(1)   /* TX FIFO not full */
#define SSI_SR_RNE             BIT(2)   /* RX FIFO not empty */
#define SSI_SR_RFF             BIT(3)   /* RX FIFO full */
#define SSI_SR_BSY             BIT(4)   /* SSI busy */

/* Instance addresses */
#define SSI0_CR0               SSI_CR0(SSI0_BASE)
#define SSI0_CR1               SSI_CR1(SSI0_BASE)
#define SSI0_DR                SSI_DR(SSI0_BASE)
#define SSI0_SR                SSI_SR(SSI0_BASE)
#define SSI0_CPSR              SSI_CPSR(SSI0_BASE)

#define SSI1_CR0               SSI_CR0(SSI1_BASE)
#define SSI1_CR1               SSI_CR1(SSI1_BASE)
#define SSI1_DR                SSI_DR(SSI1_BASE)
#define SSI1_SR                SSI_SR(SSI1_BASE)

/* ============================================
 * UART Registers
 * ============================================ */
#define UART_DR(base)          REG32((base) + 0x000)
#define UART_RSR(base)         REG32((base) + 0x004)
#define UART_FR(base)          REG32((base) + 0x018)
#define UART_IBRD(base)        REG32((base) + 0x024)
#define UART_FBRD(base)        REG32((base) + 0x028)
#define UART_LCRH(base)        REG32((base) + 0x02C)
#define UART_CTL(base)         REG32((base) + 0x030)
#define UART_IFLS(base)        REG32((base) + 0x034)
#define UART_IM(base)          REG32((base) + 0x038)
#define UART_RIS(base)         REG32((base) + 0x03C)
#define UART_MIS(base)         REG32((base) + 0x040)
#define UART_ICR(base)         REG32((base) + 0x044)

/* UART bit fields */
#define UART_CTL_UARTEN        BIT(0)
#define UART_CTL_TXE           BIT(8)
#define UART_CTL_RXE           BIT(9)
#define UART_LCRH_FEN          BIT(4)
#define UART_LCRH_8BIT         (3 << 5)
#define UART_FR_TXFF           BIT(5)
#define UART_FR_RXFE           BIT(4)

/* ============================================
 * I2C Registers
 * ============================================ */
#define I2C_SA                  REG32(I2C0_BASE + 0x000)
#define I2C_PV                  REG32(I2C0_BASE + 0x004)
#define I2C_MSA                 REG32(I2C0_BASE + 0x008)
#define I2C_MDR                 REG32(I2C0_BASE + 0x00C)
#define I2C_MCR                 REG32(I2C0_BASE + 0x010)
#define I2C_MCLKOCNT            REG32(I2C0_BASE + 0x014)
#define I2C_MBMON               REG32(I2C0_BASE + 0x01C)

/* I2C MCR bit fields */
#define I2C_MCR_MFE            BIT(0)
#define I2C_MCR_LPBK           BIT(1)

/* I2C MDR bit fields */
#define I2C_MDR_DATA_MSK       0xFF
#define I2C_MDR_RS             BIT(10)
#define I2C_MDR_ACK            BIT(12)
#define I2C_MDR_STOP           BIT(0)
#define I2C_MDR_START          BIT(1)
#define I2C_MDR_RUN            BIT(2)

/* ============================================
 * RF Core Registers
 * ============================================ */
#define RFC_DBELL_CMDR          REG32(0x40037004)
#define RFC_DBELL_CMDSTA        REG32(0x40037008)
#define RFC_DBELL_CMDARG        REG32(0x4003700C)
#define RFC_DBELL_RFCPEIFG      REG32(0x40037014)

/* RF Core Doorbell bit fields */
#define RFC_DBELL_CMD_TRIGGER   BIT(0)
#define RFC_DBELL_CMD_DONE      BIT(0)

/* RF Power control */
#define RFC_PWR_CLKEN           BIT(0)
#define RFC_PWR_BLE_CLKEN      BIT(1)

/* ============================================
 * RF Command Structures
 * ============================================ */
typedef struct __attribute__((packed)) {
    uint16_t commandNo;
    uint16_t status;
    void    *pNextOp;
    uint32_t startTime;
    struct {
        uint8_t triggerType;
        uint8_t pastTrig;
        uint8_t futureTrig;
        uint8_t en;
    } startTrigger;
} rfc_command_t;

typedef struct __attribute__((packed)) {
    rfc_command_t header;
    uint8_t  mode;
    struct {
        uint8_t frontEndMode;
        uint8_t biasMode;
    } config;
    uint8_t loDivider;
} rfc_CMD_RADIO_SETUP_t;

typedef struct __attribute__((packed)) {
    rfc_command_t header;
    uint8_t  rule;
    uint8_t  maxPktLen;
    uint8_t  pktLen;
    uint32_t syncWord;
    uint8_t  syncWordLen;
} rfc_CMD_PROP_RX_t;

typedef struct __attribute__((packed)) {
    rfc_command_t header;
    uint8_t  pktLen;
    void    *pPkt;
} rfc_CMD_PROP_TX_t;

typedef struct __attribute__((packed)) {
    rfc_command_t header;
} rfc_CMD_ABORT_t;

/* ============================================
 * USB Registers
 * ============================================ */
#define USB_CTRL                REG32(USB_BASE + 0x000)
#define USB_FADDR               REG32(USB_BASE + 0x004)
#define USB_DPS                 REG32(USB_BASE + 0x008)
#define USB_EPIDX               REG32(USB_BASE + 0x00C)

/* USB Control bit fields */
#define USB_CTRL_USBEN          BIT(0)
#define USB_CTRL_PLLEN          BIT(1)
#define USB_CTRL_PHYEN          BIT(2)

/* ============================================
 * DMA (uDMA) Registers
 * ============================================ */
#define UDMA_STAT               REG32(0x4000F000)
#define UDMA_CFG                REG32(0x4000F004)
#define UDMA_CTLBASE            REG32(0x4000F008)
#define UDMA_ALTBASE            REG32(0x4000F00C)
#define UDMA_SWREQ              REG32(0x4000F020)

/* DMA channel assignments */
#define UDMA_CH8_SSI0RX         8   /* SDRAM SPI RX */
#define UDMA_CH9_SSI0TX         9   /* SDRAM SPI TX */

/* ============================================
 * Timer Registers
 * ============================================ */
#define GPT0_CFG                REG32(TIMER0_BASE + 0x000)
#define GPT0_TAMR               REG32(TIMER0_BASE + 0x004)
#define GPT0_TBMR               REG32(TIMER0_BASE + 0x008)
#define GPT0_TAILR              REG32(TIMER0_BASE + 0x048)
#define GPT0_TBILR              REG32(TIMER0_BASE + 0x04C)
#define GPT0_TAPR               REG32(TIMER0_BASE + 0x038)
#define GPT0_CTL                REG32(TIMER0_BASE + 0x00C)
#define GPT0_ICR                REG32(TIMER0_BASE + 0x024)
#define GPT0_IMR                REG32(TIMER0_BASE + 0x018)

/* ============================================
 * Watchdog Registers
 * ============================================ */
#define WDT_LOAD               REG32(WDT_BASE + 0x000)
#define WDT_VALUE              REG32(WDT_BASE + 0x004)
#define WDT_CTL                REG32(WDT_BASE + 0x008)
#define WDT_ICR                REG32(WDT_BASE + 0x00C)
#define WDT_RIS                REG32(WDT_BASE + 0x010)
#define WDT_MIS                REG32(WDT_BASE + 0x014)
#define WDT_TEST               REG32(WDT_BASE + 0x418)
#define WDT_LOCK               REG32(WDT_BASE + 0xC00)

/* Watchdog bit fields */
#define WDT_CTL_INTEN          BIT(0)
#define WDT_CTL_RESEN          BIT(1)
#define WDT_LOCK_UNLOCK        0x1ACCE551

/* ============================================
 * Flash Controller Registers
 * ============================================ */
#define FCFG_ICEPICK_DEVICE_ID  REG32(0x50001000)
#define FCFG_USER_ID            REG32(0x50001254)
#define FCFG_FLASH_SIZE         REG32(0x5000102C)

/* ============================================
 * NVIC Registers (Cortex-M4F)
 * ============================================ */
#define NVIC_ISER0              REG32(0xE000E100)
#define NVIC_ISER1              REG32(0xE000E104)
#define NVIC_ISER2              REG32(0xE000E108)
#define NVIC_ICER0              REG32(0xE000E180)
#define NVIC_ICER1              REG32(0xE000E184)
#define NVIC_ICER2              REG32(0xE000E188)
#define NVIC_ISPR0              REG32(0xE000E200)
#define NVIC_ISPR1              REG32(0xE000E204)
#define NVIC_ISPR2              REG32(0xE000E208)
#define NVIC_ICPR0              REG32(0xE000E280)
#define NVIC_ICPR1              REG32(0xE000E284)
#define NVIC_ICPR2              REG32(0xE000E288)
#define NVIC_IABR0              REG32(0xE000E300)
#define NVIC_IABR1              REG32(0xE000E304)
#define NVIC_IPR0                REG32(0xE000E400)

/* ============================================
 * Interrupt Numbers (CC1352P-specific)
 * ============================================ */
#define IRQ_RESET               0
#define IRQ_NMI                 1
#define IRQ_HARD_FAULT          2
#define IRQ_MEM_MANAGE          3
#define IRQ_BUS_FAULT           4
#define IRQ_USAGE_FAULT         5
#define IRQ_SVCALL              6
#define IRQ_PENDSV              7
#define IRQ_SYSTICK             8
#define IRQ_GPIO                9
#define UART0_IRQ               10
#define UART1_IRQ               11
#define SSI0_IRQ                24
#define SSI1_IRQ                25
#define I2C0_IRQ                26
#define SSI3_IRQ                27
#define RFC_IRQ                 48
#define USB_IRQ                 35
#define GPT0A_IRQ               36
#define GPT0B_IRQ               37
#define GPT1A_IRQ               38
#define GPT1B_IRQ               39
#define WDT_IRQ                 40
#define DMA_IRQ                 41

/* ============================================
 * System Control Registers
 * ============================================ */
#define SCB_CPACR               REG32(0xE000ED88)
#define SCB_VTOR                REG32(0xE000ED08)

/* FPU enable */
#define SCB_CPACR_FPU_ENABLE   (0xF << 20)

#ifdef __cplusplus
}
#endif

#endif /* REGISTERS_H */