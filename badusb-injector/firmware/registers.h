/*
 * PHANTOM — RP2040 Register Definitions
 * Copyright (C) 2024 Hacker Devices
 * Licensed under GPL-2.0
 *
 * Register definitions for RP2040 peripherals used by PHANTOM firmware.
 * Based on RP2040 datasheet (RP2040-RP2040_Datasheet.pdf)
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

/* Base addresses */
#define RESETS_BASE          0x4000C000
#define CLOCKS_BASE          0x40008000
#define PADS_BANK_0_BASE     0x4001C000
#define PADS_QSPI_BASE       0x40020000
#define IO_BANK_0_BASE       0x40014000
#define IO_QSPI_BASE         0x40018000
#define SIO_BASE             0xD0000000
#define SPI0_BASE            0x4003C000
#define SPI1_BASE            0x40040000
#define UART0_BASE           0x40034000
#define UART1_BASE           0x40038000
#define I2C0_BASE            0x40044000
#define I2C1_BASE            0x40048000
#define ADC_BASE             0x4004C000
#define PWM_BASE             0x40050000
#define TIMER_BASE           0x40054000
#define WATCHDOG_BASE        0x40058000
#define XIP_CTRL_BASE        0x50000000
#define XIP_SSI_BASE         0x5000C000
#define DMA_BASE             0x50001000
#define USB_BASE             0x50110000
#define PIO0_BASE            0x50200000
#define PIO1_BASE            0x50300000
#define PPB_BASE             0xE0000000

/* Reset registers */
#define RESETS_RESET         (*(volatile uint32_t *)(RESETS_BASE + 0x00))
#define RESETS_RESET_DONE    (*(volatile uint32_t *)(RESETS_BASE + 0x08))

/* Reset bits */
#define RESETS_RESET_USB         (1 << 24)
#define RESETS_RESET_UART0       (1 << 22)
#define RESETS_RESET_SPI0        (1 << 18)
#define RESETS_RESET_I2C0        (1 << 15)
#define RESETS_RESET_ADC         (1 << 13)
#define RESETS_RESET_PWM         (1 << 11)
#define RESETS_RESET_PIO0        (1 << 5)
#define RESETS_RESET_PIO1        (1 << 6)
#define RESETS_RESET_IO_BANK0    (1 << 0)

/* Clock registers */
#define CLOCKS_CLK_REF_CTRL      (*(volatile uint32_t *)(CLOCKS_BASE + 0x00))
#define CLOCKS_CLK_REF_DIV       (*(volatile uint32_t *)(CLOCKS_BASE + 0x04))
#define CLOCKS_CLK_SYS_CTRL      (*(volatile uint32_t *)(CLOCKS_BASE + 0x08))
#define CLOCKS_CLK_SYS_DIV       (*(volatile uint32_t *)(CLOCKS_BASE + 0x0C))
#define CLOCKS_CLK_PERI_CTRL     (*(volatile uint32_t *)(CLOCKS_BASE + 0x10))
#define CLOCKS_CLK_USB_CTRL      (*(volatile uint32_t *)(CLOCKS_BASE + 0x14))
#define CLOCKS_CLK_USB_DIV       (*(volatile uint32_t *)(CLOCKS_BASE + 0x18))
#define CLOCKS_CLK_ADC_CTRL      (*(volatile uint32_t *)(CLOCKS_BASE + 0x1C))
#define CLOCKS_CLK_ADC_DIV       (*(volatile uint32_t *)(CLOCKS_BASE + 0x20))

/* Clock source selections */
#define CLOCKS_CLK_REF_CTRL_SRC_ROSC     0x0
#define CLOCKS_CLK_REF_CTRL_SRC_XOSC     0x2
#define CLOCKS_CLK_SYS_CTRL_SRC_PLL_SYS  0x1

/* PLL registers (SYS PLL) */
#define PLL_SYS_BASE              0x40028000
#define PLL_SYS_CS                (*(volatile uint32_t *)(PLL_SYS_BASE + 0x00))
#define PLL_SYS_PWR              (*(volatile uint32_t *)(PLL_SYS_BASE + 0x04))
#define PLL_SYS_FBDIV_INT        (*(volatile uint32_t *)(PLL_SYS_BASE + 0x08))
#define PLL_SYS_PRIM             (*(volatile uint32_t *)(PLL_SYS_BASE + 0x0C))

/* PLL registers (USB PLL) */
#define PLL_USB_BASE              0x4002C000
#define PLL_USB_CS                (*(volatile uint32_t *)(PLL_USB_BASE + 0x00))
#define PLL_USB_PWR              (*(volatile uint32_t *)(PLL_USB_BASE + 0x04))
#define PLL_USB_FBDIV_INT        (*(volatile uint32_t *)(PLL_USB_BASE + 0x08))
#define PLL_USB_PRIM             (*(volatile uint32_t *)(PLL_USB_BASE + 0x0C))

/* XOSC registers */
#define XOSC_BASE                 0x40024000
#define XOSC_CTRL                 (*(volatile uint32_t *)(XOSC_BASE + 0x00))
#define XOSC_STATUS               (*(volatile uint32_t *)(XOSC_BASE + 0x04))
#define XOSC_STARTUP              (*(volatile uint32_t *)(XOSC_BASE + 0x08))

#define XOSC_CTRL_ENABLE_VALUE    0x0D1AA1  /* XOSC enable magic */
#define XOSC_STATUS_STABLE        (1 << 31)

/* GPIO registers (SIO) */
#define SIO_GPIO_IN               (*(volatile uint32_t *)(SIO_BASE + 0x004))
#define SIO_GPIO_IN_HI            (*(volatile uint32_t *)(SIO_BASE + 0x008))
#define SIO_GPIO_OUT              (*(volatile uint32_t *)(SIO_BASE + 0x010))
#define SIO_GPIO_OUT_SET          (*(volatile uint32_t *)(SIO_BASE + 0x014))
#define SIO_GPIO_OUT_CLR          (*(volatile uint32_t *)(SIO_BASE + 0x018))
#define SIO_GPIO_OUT_XOR          (*(volatile uint32_t *)(SIO_BASE + 0x01C))
#define SIO_GPIO_OE               (*(volatile uint32_t *)(SIO_BASE + 0x020))
#define SIO_GPIO_OE_SET           (*(volatile uint32_t *)(SIO_BASE + 0x024))
#define SIO_GPIO_OE_CLR           (*(volatile uint32_t *)(SIO_BASE + 0x028))

/* IO_BANK_0 registers (GPIO function select) */
#define IO_BANK_0_GPIO_CTRL(n)    (*(volatile uint32_t *)(IO_BANK_0_BASE + 0x004 + (n) * 8))
#define IO_BANK_0_GPIO_INTE(n)    (*(volatile uint32_t *)(IO_BANK_0_BASE + 0x0F0 + (n) * 4))

/* Pad control registers */
#define PADS_BANK_0_GPIO(n)       (*(volatile uint32_t *)(PADS_BANK_0_BASE + 0x004 + (n) * 4))

/* SPI0 registers */
#define SPI0_SSPCR0              (*(volatile uint32_t *)(SPI0_BASE + 0x00))
#define SPI0_SSPCR1              (*(volatile uint32_t *)(SPI0_BASE + 0x04))
#define SPI0_SSPDR               (*(volatile uint32_t *)(SPI0_BASE + 0x08))
#define SPI0_SSPSR               (*(volatile uint32_t *)(SPI0_BASE + 0x0C))
#define SPI0_SSPCPSR             (*(volatile uint32_t *)(SPI0_BASE + 0x10))
#define SPI0_SSPDMACR            (*(volatile uint32_t *)(SPI0_BASE + 0x14))

/* SPI status bits */
#define SPI_SSPSR_TFE            (1 << 0)  /* TX FIFO empty */
#define SPI_SSPSR_TNF            (1 << 1)  /* TX FIFO not full */
#define SPI_SSPSR_RNE            (1 << 2)  /* RX FIFO not empty */
#define SPI_SSPSR_RFF            (1 << 3)  /* RX FIFO full */
#define SPI_SSPSR_BSY            (1 << 4)  /* Busy */

/* SPI control bits */
#define SPI_SSPCR0_DSS_8BIT      (0x07 << 0)  /* 8-bit data size */
#define SPI_SSPCR0_FRF_SPI       (0x00 << 4)  /* SPI frame format */
#define SPI_SSPCR0_SPO           (1 << 6)      /* Clock polarity (CPOL) */
#define SPI_SSPCR0_SPH           (1 << 7)      /* Clock phase (CPHA) */
#define SPI_SSPCR1_SSE           (1 << 1)      /* SPI enable */
#define SPI_SSPCR1_MS            (1 << 2)      /* Master mode */

/* UART0 registers */
#define UART0_DR                  (*(volatile uint32_t *)(UART0_BASE + 0x00))
#define UART0_RSR                 (*(volatile uint32_t *)(UART0_BASE + 0x04))
#define UART0_FR                  (*(volatile uint32_t *)(UART0_BASE + 0x18))
#define UART0_IBRD                (*(volatile uint32_t *)(UART0_BASE + 0x24))
#define UART0_FBRD                (*(volatile uint32_t *)(UART0_BASE + 0x28))
#define UART0_LCR_H              (*(volatile uint32_t *)(UART0_BASE + 0x2C))
#define UART0_CR                  (*(volatile uint32_t *)(UART0_BASE + 0x30))
#define UART0_IFLS                (*(volatile uint32_t *)(UART0_BASE + 0x34))
#define UART0_IMSC                (*(volatile uint32_t *)(UART0_BASE + 0x38))
#define UART0_ICR                 (*(volatile uint32_t *)(UART0_BASE + 0x44))

/* UART flags */
#define UART_FR_TXFF              (1 << 5)  /* TX FIFO full */
#define UART_FR_RXFE              (1 << 4)  /* RX FIFO empty */
#define UART_FR_BUSY              (1 << 3)  /* UART busy */

/* UART control */
#define UART_CR_UARTEN            (1 << 0)  /* UART enable */
#define UART_CR_TXE               (1 << 8)  /* TX enable */
#define UART_CR_RXE               (1 << 9)  /* RX enable */

/* UART line control */
#define UART_LCR_H_FEN            (1 << 4)  /* FIFO enable */
#define UART_LCR_H_WLEN_8        (0x03 << 5)  /* 8-bit word */

/* I2C0 registers */
#define I2C0_IC_CON               (*(volatile uint32_t *)(I2C0_BASE + 0x00))
#define I2C0_IC_TAR              (*(volatile uint32_t *)(I2C0_BASE + 0x04))
#define I2C0_IC_DATA_CMD         (*(volatile uint32_t *)(I2C0_BASE + 0x10))
#define I2C0_IC_STATUS            (*(volatile uint32_t *)(I2C0_BASE + 0x18))
#define I2C0_IC_FS_SCL_HCNT      (*(volatile uint32_t *)(I2C0_BASE + 0x1C))
#define I2C0_IC_FS_SCL_LCNT      (*(volatile uint32_t *)(I2C0_BASE + 0x20))
#define I2C0_IC_ENABLE            (*(volatile uint32_t *)(I2C0_BASE + 0x6C))

/* I2C control bits */
#define I2C_IC_CON_MASTER_MODE    (1 << 0)
#define I2C_IC_CON_SPEED_FAST     (1 << 2)
#define I2C_IC_CON_RESTART_EN     (1 << 5)
#define I2C_IC_CON_IC_ENABLE      (1 << 0)

/* I2C data commands */
#define I2C_IC_DATA_CMD_WRITE     (0 << 8)
#define I2C_IC_DATA_CMD_READ      (1 << 8)
#define I2C_IC_DATA_CMD_STOP      (1 << 9)
#define I2C_IC_DATA_CMD_RESTART   (1 << 10)

/* ADC registers */
#define ADC_CS                    (*(volatile uint32_t *)(ADC_BASE + 0x00))
#define ADC_RESULT                (*(volatile uint32_t *)(ADC_BASE + 0x04))
#define ADC_FCS                   (*(volatile uint32_t *)(ADC_BASE + 0x08))
#define ADC_FIFO                  (*(volatile uint32_t *)(ADC_BASE + 0x0C))

/* ADC control bits */
#define ADC_CS_EN                 (1 << 0)   /* ADC enable */
#define ADC_CS_START_ONCE         (1 << 2)   /* Start one conversion */
#define ADC_CS_START_MANY         (1 << 3)   /* Start continuous */
#define ADC_CS_RROBIN_SHIFT       16        /* Round-robin bits */

/* USB device controller registers */
#define USB_MAIN_CTRL             (*(volatile uint32_t *)(USB_BASE + 0x40))
#define USB_SIE_CTRL              (*(volatile uint32_t *)(USB_BASE + 0x50))
#define USB_SIE_STATUS            (*(volatile uint32_t *)(USB_BASE + 0x54))
#define USB_BUFF_STATUS           (*(volatile uint32_t *)(USB_BASE + 0x58))
#define USB_BUFF_CPU_SHOULD_HANDLE (*(volatile uint32_t *)(USB_BASE + 0x5C))
#define USB_EP0_BUF_CTRL         (*(volatile uint32_t *)(USB_BASE + 0x80))
#define USB_EP_BUF_CTRL(n)       (*(volatile uint32_t *)(USB_BASE + 0x80 + (n) * 4))
#define USB_INT_EP_CTRL          (*(volatile uint32_t *)(USB_BASE + 0x100))
#define USB_INTS                  (*(volatile uint32_t *)(USB_BASE + 0x108))
#define USB_INTE                  (*(volatile uint32_t *)(USB_BASE + 0x10C))
#define USB_INTF                  (*(volatile uint32_t *)(USB_BASE + 0x110))
#define USB_SETUP_PACKET         (*(volatile uint32_t *)(USB_BASE + 0x114))

/* USB control bits */
#define USB_MAIN_CTRL_SIM_CONN    (1 << 31)  /* Simulate connection */
#define USB_MAIN_CTRL_CONTROLLER_EN (1 << 0) /* Controller enable */

#define USB_SIE_CTRL_EP0_INT_ACK  (1 << 0)
#define USB_SIE_CTRL_EP0_INT_PEND (1 << 1)
#define USB_SIE_CTRL_PULLUP_EN    (1 << 13)  /* Enable pull-up */
#define USB_SIE_CTRL_BUS_RESET    (1 << 16)  /* Bus reset detected */

/* USB interrupt bits */
#define USB_INTS_SETUP_REQ        (1 << 0)
#define USB_INTS_BUFF_STATUS      (1 << 1)
#define USB_INTS_BUS_RESET        (1 << 2)
#define USB_INTS_DEV_SUSPEND      (1 << 3)
#define USB_INTS_DEV_RESUME       (1 << 4)
#define USB_INTS_EP_STALL_NAK     (1 << 5)
#define USB_INTS_SOF              (1 << 8)

/* Buffer control bits */
#define USB_BUF_CTRL_AVAILABLE    (1 << 10)
#define USB_BUF_CTRL_LENGTH_SHIFT  0
#define USB_BUF_CTRL_FULL         (1 << 15)
#define USB_BUF_CTRL_LAST         (1 << 14)

/* Timer registers */
#define TIMER_TIMEHW              (*(volatile uint32_t *)(TIMER_BASE + 0x00))
#define TIMER_TIMELW              (*(volatile uint32_t *)(TIMER_BASE + 0x04))
#define TIMER_TIMEHR              (*(volatile uint32_t *)(TIMER_BASE + 0x08))
#define TIMER_TIMELR              (*(volatile uint32_t *)(TIMER_BASE + 0x0C))

/* PIO registers (for WS2812B) */
#define PIO0_CTRL                 (*(volatile uint32_t *)(PIO0_BASE + 0x00))
#define PIO0_FJOIN_TX_FIFO        (*(volatile uint32_t *)(PIO0_BASE + 0x04))
#define PIO0_FJOIN_RX_FIFO        (*(volatile uint32_t *)(PIO0_BASE + 0x08))
#define PIO0_FDEBUG               (*(volatile uint32_t *)(PIO0_BASE + 0x10))
#define PIO0_TXF0                 (*(volatile uint32_t *)(PIO0_BASE + 0x20))
#define PIO0_TXF1                 (*(volatile uint32_t *)(PIO0_BASE + 0x24))
#define PIO0_INSTR_MEM(n)         (*(volatile uint32_t *)(PIO0_BASE + 0x40 + (n) * 4))
#define PIO0_SM0_CLKDIV           (*(volatile uint32_t *)(PIO0_BASE + 0xC8))
#define PIO0_SM0_EXECCTRL         (*(volatile uint32_t *)(PIO0_BASE + 0xCC))
#define PIO0_SM0_SHIFTCTRL        (*(volatile uint32_t *)(PIO0_BASE + 0xD0))
#define PIO0_SM0_ADDR             (*(volatile uint32_t *)(PIO0_BASE + 0xD4))
#define PIO0_SM0_INSTR            (*(volatile uint32_t *)(PIO0_BASE + 0xD8))
#define PIO0_SM0_PINCTRL          (*(volatile uint32_t *)(PIO0_BASE + 0xDC))

/* NVIC registers (Cortex-M0+) */
#define NVIC_ISER                 (*(volatile uint32_t *)(PPB_BASE + 0xE100))
#define NVIC_ICER                 (*(volatile uint32_t *)(PPB_BASE + 0xE180))
#define NVIC_ISPR                 (*(volatile uint32_t *)(PPB_BASE + 0xE200))
#define NVIC_ICPR                 (*(volatile uint32_t *)(PPB_BASE + 0xE280))

/* System Control Block */
#define SCB_VTOR                  (*(volatile uint32_t *)(PPB_BASE + 0xED08))

/* IRQ numbers */
#define TIMER_IRQ_0               0
#define TIMER_IRQ_1               1
#define TIMER_IRQ_2               2
#define TIMER_IRQ_3               3
#define PWM_IRQ_WRAP              4
#define USBCTRL_IRQ               5
#define XIP_IRQ                   6
#define PIO0_IRQ_0                7
#define PIO0_IRQ_1                8
#define PIO1_IRQ_0                9
#define PIO1_IRQ_1                10
#define DMA_IRQ_0                 11
#define DMA_IRQ_1                 12
#define IO_IRQ_BANK0              13
#define IO_IRQ_QSPI               14
#define SIO_IRQ_PROC0             15
#define SIO_IRQ_PROC1             16
#define CLOCKS_IRQ                17
#define SPI0_IRQ                  18
#define SPI1_IRQ                  19
#define UART0_IRQ                 20
#define UART1_IRQ                 21
#define ADC_IRQ_FIFO              22
#define I2C0_IRQ                  23
#define I2C1_IRQ                  24

/* Utility macros */
#define BIT(n)                    (1U << (n))
#define REG32(addr)               (*(volatile uint32_t *)(addr))
#define REG16(addr)               (*(volatile uint16_t *)(addr))
#define REG8(addr)                (*(volatile uint8_t *)(addr))

#define MHZ(n)                    ((n) * 1000000)
#define KHZ(n)                    ((n) * 1000)

/* Busy wait helpers */
static inline void busy_wait_us(uint32_t us) {
    uint32_t start = TIMER_TIMELR;
    while ((TIMER_TIMELR - start) < us);
}

static inline void busy_wait_ms(uint32_t ms) {
    busy_wait_us(ms * 1000);
}

static inline uint32_t timer_read(void) {
    return TIMER_TIMELR;
}

#endif /* REGISTERS_H */