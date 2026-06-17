/*
 * registers.h — RP2040 register definitions for FiberPhantom
 * Author: jayis1
 * Date:   2026-06-17
 *
 * Minimal RP2040 register map (no SDK dependency for this freestanding firmware).
 * Only the registers needed by FiberPhantom drivers are defined.
 */

#ifndef FIBER_PHANTOM_REGISTERS_H
#define FIBER_PHANTOM_REGISTERS_H

#include <stdint.h>

/* ---- Base addresses (RP2040 datasheet, section 2.2) ---- */
#define SIO_BASE        0xd0000000U
#define IO_BANK0_BASE    0x40014000U
#define PADS_BANK0_BASE  0x4001c000U
#define XOSC_BASE        0x40004000U
#define CLOCKS_BASE      0x40008000U
#define PLL_SYS_BASE    0x40020000U
#define PLL_USB_BASE     0x4000c000U
#define WATCHDOG_BASE    0x40058000U
#define PSM_BASE         0x40016000U
#define SPI0_BASE        0x4003c000U
#define SPI1_BASE        0x40040000U
#define UART0_BASE       0x40034000U
#define UART1_BASE       0x40038000U
#define I2C0_BASE        0x40044000U
#define ADC_BASE         0x4000c000U
#define USB_BASE         0x40050000U
#define SYSCFG_BASE      0x40004000U
#define TIMER_BASE       0x40054000U
#define DMA_BASE         0x50000000U

/* ---- SIO registers ---- */
#define SIO_GPIO_IN            (*(volatile uint32_t *)(SIO_BASE + 0x004))
#define SIO_GPIO_HI_IN         (*(volatile uint32_t *)(SIO_BASE + 0x008))
#define SIO_GPIO_OUT           (*(volatile uint32_t *)(SIO_BASE + 0x010))
#define SIO_GPIO_OUT_SET       (*(volatile uint32_t *)(SIO_BASE + 0x014))
#define SIO_GPIO_OUT_CLR       (*(volatile uint32_t *)(SIO_BASE + 0x018))
#define SIO_GPIO_OE            (*(volatile uint32_t *)(SIO_BASE + 0x020))
#define SIO_GPIO_OE_SET        (*(volatile uint32_t *)(SIO_BASE + 0x024))
#define SIO_GPIO_OE_CLR        (*(volatile uint32_t *)(SIO_BASE + 0x028))
#define SIO_GPIO_HI_OE         (*(volatile uint32_t *)(SIO_BASE + 0x030))
#define SIO_GPIO_HI_OE_SET     (*(volatile uint32_t *)(SIO_BASE + 0x034))
#define SIO_GPIO_HI_OE_CLR     (*(volatile uint32_t *)(SIO_BASE + 0x038))

/* ---- IO_BANK0: GPIO control (2 registers per pin: ctrl + status) ---- */
#define IO_BANK0_GPIO_CTRL(n)  (*(volatile uint32_t *)(IO_BANK0_BASE + 0x004 + (n)*8))
#define IO_BANK0_GPIO_STATUS(n) (*(volatile uint32_t *)(IO_BANK0_BASE + 0x000 + (n)*8))

/* GPIO function select values (IO_BANK0_CTRL) */
#define GPIO_FUNC_SPI   1
#define GPIO_FUNC_UART   2
#define GPIO_FUNC_I2C    3
#define GPIO_FUNC_PWM    4
#define GPIO_FUNC_SIO    5
#define GPIO_FUNC_PIO0   6
#define GPIO_FUNC_PIO1   7
#define GPIO_FUNC_USB    0
#define GPIO_FUNC_NULL   0x1f

/* ---- PADS_BANK0: pad configuration (1 register per pin) ---- */
#define PADS_BANK0_GPIO(n)    (*(volatile uint32_t *)(PADS_BANK0_BASE + 0x004 + (n)*4))
#define PAD_OD    (1 << 7)  /* Output disable */
#define PAD_IE    (1 << 6)  /* Input enable */
#define PAD_PUE   (1 << 3)  /* Pull-up enable */
#define PAD_PDE   (1 << 2)  /* Pull-down enable */
#define PAD_SCHMITT (1 << 1) /* Schmitt trigger */
#define PAD_SLEWFAST (1 << 0) /* Fast slew rate */
#define PAD_DRIVE_2MA  (0 << 4)
#define PAD_DRIVE_4MA  (1 << 4)
#define PAD_DRIVE_8MA  (2 << 4)
#define PAD_DRIVE_12MA (3 << 4)

/* ---- XOSC (crystal oscillator) ---- */
#define XOSC_CTRL      (*(volatile uint32_t *)(XOSC_BASE + 0x00))
#define XOSC_STATUS    (*(volatile uint32_t *)(XOSC_BASE + 0x04))
#define XOSC_STARTUP   (*(volatile uint32_t *)(XOSC_BASE + 0x0C))
#define XOSC_ENABLED   0xd0a1 << 0 /* MAGIC */
#define XOSC_FREQ_RANGE_1_15MHZ 0xaa0

/* ---- CLOCKS ---- */
#define CLK_CTRL(id)     (*(volatile uint32_t *)(CLOCKS_BASE + 0x000 + (id)*0x10))
#define CLK_DIV(id)      (*(volatile uint32_t *)(CLOCKS_BASE + 0x004 + (id)*0x10))
#define CLK_SELECTED(id) (*(volatile uint32_t *)(CLOCKS_BASE + 0x008 + (id)*0x10))
#define CLK_FC0_REF_KHZ  (*(volatile uint32_t *)(CLOCKS_BASE + 0x01C))
#define CLK_FC0_MIN_KHZ  (*(volatile uint32_t *)(CLOCKS_BASE + 0x020))
#define CLK_FC0_MAX_KHZ  (*(volatile uint32_t *)(CLOCKS_BASE + 0x024))
#define CLK_FC0_DELAY    (*(volatile uint32_t *)(CLOCKS_BASE + 0x028))
#define CLK_FC0_INTERVAL (*(volatile uint32_t *)(CLOCKS_BASE + 0x02C))
#define CLK_FC0_SRC       (*(volatile uint32_t *)(CLOCKS_BASE + 0x030))
#define CLK_FC0_STATUS    (*(volatile uint32_t *)(CLOCKS_BASE + 0x034))
#define CLK_FC0_RESULT    (*(volatile uint32_t *)(CLOCKS_BASE + 0x038))

/* Clock IDs */
#define CLK_SYS    0
#define CLK_PERI   1
#define CLK_USB    2
#define CLK_ADC    3
#define CLK_RTC    4

/* ---- PLL_SYS (system PLL) ---- */
#define PLL_SYS_CS    (*(volatile uint32_t *)(PLL_SYS_BASE + 0x00))
#define PLL_SYS_PWR   (*(volatile uint32_t *)(PLL_SYS_BASE + 0x04))
#define PLL_SYS_FBDIV (*(volatile uint32_t *)(PLL_SYS_BASE + 0x08))
#define PLL_SYS_PRIM  (*(volatile uint32_t *)(PLL_SYS_BASE + 0x0C))
#define PLL_SYS_PRIM1 (*(volatile uint32_t *)(PLL_SYS_BASE + 0x10))
#define PLL_SYS_BYPASS (*(volatile uint32_t *)(PLL_SYS_BASE + 0x18))

/* ---- SPI registers (SPI0/SPI1 share layout) ---- */
typedef struct {
    volatile uint32_t cr0;    /* 0x00: Control register 0 */
    volatile uint32_t cr1;    /* 0x04: Control register 1 */
    volatile uint32_t dr;     /* 0x08: Data register */
    volatile uint32_t sr;     /* 0x0C: Status register */
    volatile uint32_t cpsr;   /* 0x10: Clock prescale */
    volatile uint32_t imsc;   /* 0x14: Interrupt mask */
    volatile uint32_t ris;    /* 0x18: Raw interrupt status */
    volatile uint32_t mis;    /* 0x1C: Masked interrupt status */
    volatile uint32_t icr;   /* 0x20: Interrupt clear */
    volatile uint32_t dmacr;  /* 0x24: DMA control */
    volatile uint32_t _reserved[5];
    volatile uint32_t sspperiphid0; /* 0x3C: Peripheral ID 0 */
} spi_reg_t;

#define SPI0   ((spi_reg_t *)SPI0_BASE)
#define SPI1   ((spi_reg_t *)SPI1_BASE)

/* SPI CR0 bit fields */
#define SPI_CR0_DSS_8BIT   0x07  /* Data size: 8-bit */
#define SPI_CR0_DSS_16BIT  0x0F  /* Data size: 16-bit */
#define SPI_CR0_FRF_MOTOROLA 0x00 /* Frame format: Motorola SPI */
#define SPI_CR0_SPO        (1 << 6) /* Clock polarity (CPOL) */
#define SPI_CR0_SPH        (1 << 7) /* Clock phase (CPHA) */
#define SPI_CR0_SCR_SHIFT  8      /* Serial clock rate divisor shift */

/* SPI SR (status) bit fields */
#define SPI_SR_TFE        (1 << 0) /* Transmit FIFO empty */
#define SPI_SR_TNF        (1 << 1) /* Transmit FIFO not full */
#define SPI_SR_RNE        (1 << 2) /* Receive FIFO not empty */
#define SPI_SR_RFF        (1 << 3) /* Receive FIFO full */
#define SPI_SR_BSY        (1 << 4) /* Busy */

/* ---- UART registers (UART0/UART1 share layout) ---- */
typedef struct {
    volatile uint32_t dr;      /* 0x00: Data register */
    volatile uint32_t rsr;     /* 0x04: Receive status register / error clear */
    volatile uint32_t _res1[4];
    volatile uint32_t fr;      /* 0x18: Flag register */
    volatile uint32_t _res2;
    volatile uint32_t ilpr;    /* 0x20: IrDA low-power counter */
    volatile uint32_t ibrd;    /* 0x24: Integer baud rate divisor */
    volatile uint32_t fbrd;    /* 0x28: Fractional baud rate divisor */
    volatile uint32_t lcr_h;   /* 0x2C: Line control register */
    volatile uint32_t cr;      /* 0x30: Control register */
    volatile uint32_t ifls;    /* 0x34: Interrupt FIFO level select */
    volatile uint32_t imsc;    /* 0x38: Interrupt mask set/clear */
    volatile uint32_t ris;     /* 0x3C: Raw interrupt status */
    volatile uint32_t mis;     /* 0x40: Masked interrupt status */
    volatile uint32_t icr;     /* 0x44: Interrupt clear */
    volatile uint32_t dmacr;   /* 0x48: DMA control */
} uart_reg_t;

#define UART0   ((uart_reg_t *)UART0_BASE)
#define UART1   ((uart_reg_t *)UART1_BASE)

/* UART FR (flag register) bits */
#define UART_FR_TXFE  (1 << 7) /* TX FIFO empty */
#define UART_FR_RXFF  (1 << 6) /* RX FIFO full */
#define UART_FR_TXFF  (1 << 5) /* TX FIFO full */
#define UART_FR_RXFE  (1 << 4) /* RX FIFO empty */
#define UART_FR_BUSY  (1 << 3) /* UART busy */
#define UART_FR_CTS   (1 << 0) /* Clear to send */

/* UART LCR_H bits */
#define UART_LCR_H_FEN   (1 << 4)  /* Enable FIFOs */
#define UART_LCR_H_WLEN_8 (3 << 5) /* 8-bit word length */

/* UART CR bits */
#define UART_CR_UARTEN   (1 << 0)  /* UART enable */
#define UART_CR_TXE      (1 << 8)  /* TX enable */
#define UART_CR_RXE      (1 << 9)  /* RX enable */
#define UART_CR_RTSEN    (1 << 14) /* RTS flow control enable */
#define UART_CR_CTSEN    (1 << 15) /* CTS flow control enable */

/* ---- I2C registers (I2C0 only, for DAC) ---- */
typedef struct {
    volatile uint32_t con;     /* 0x00: Control register */
    volatile uint32_t tar;    /* 0x04: Target address */
    volatile uint32_t sar;     /* 0x08: Slave address */
    volatile uint32_t _res1;
    volatile uint32_t data_cmd; /* 0x10: Data command */
    volatile uint32_t enable;  /* 0x18: Enable */
    volatile uint32_t status;  /* 0x1C: Status */
    volatile uint32_t _res2[4];
    volatile uint32_t ss_scl_hcnt; /* 0x30: Standard speed SCL high count */
    volatile uint32_t ss_scl_lcnt; /* 0x34: Standard speed SCL low count */
    volatile uint32_t _res3[2];
    volatile uint32_t intr_stat;  /* 0x3C: Interrupt status */
    volatile uint32_t intr_mask;  /* 0x40: Interrupt mask */
    volatile uint32_t raw_intr_stat; /* 0x44: Raw interrupt status */
    volatile uint32_t rx_tl;    /* 0x48: RX threshold level */
    volatile uint32_t tx_tl;    /* 0x4C: TX threshold level */
    volatile uint32_t clr_intr; /* 0x50: Clear interrupt */
    volatile uint32_t clr_rx_under; /* 0x54: Clear RX underrun */
    volatile uint32_t clr_rx_over;  /* 0x58: Clear RX overrun */
    volatile uint32_t clr_tx_req;   /* 0x5C: Clear TX abort */
    volatile uint32_t enable_status; /* 0x60: Enable status */
} i2c_reg_t;

#define I2C0    ((i2c_reg_t *)I2C0_BASE)

/* I2C status bits */
#define I2C_STATUS_ACTIVITY  (1 << 0)
#define I2C_STATUS_TFNF      (1 << 1) /* TX FIFO not full */
#define I2C_STATUS_TFE       (1 << 2) /* TX FIFO empty */
#define I2C_STATUS_RFNE      (1 << 3) /* RX FIFO not empty */
#define I2C_MAST activities  (1 << 5)

/* I2C data_cmd bits */
#define I2C_CMD_READ  (1 << 8)
#define I2C_CMD_STOP  (1 << 9)
#define I2C_CMD_RESTART (1 << 10)

/* ---- ADC registers ---- */
typedef struct {
    volatile uint32_t cs;      /* 0x00: Control and status */
    volatile uint32_t result;  /* 0x04: Result */
    volatile uint32_t fcs;     /* 0x08: FIFO control and status */
    volatile uint32_t fifo;    /* 0x0C: FIFO */
    volatile uint32_t div;     /* 0x10: Clock divider */
} adc_reg_t;

#define ADC ((adc_reg_t *)ADC_BASE)

/* ADC CS bits */
#define ADC_CS_EN     (1 << 0)  /* Enable ADC */
#define ADC_CS_TS_EN  (1 << 1)  /* Temperature sensor enable */
#define ADC_CS_START_ONCE (1 << 2) /* Start one conversion */
#define ADC_CS_ERR_STICKY (1 << 9) /* Error sticky flag */
#define ADC_CS_ERR_STICKY_CLR (1 << 10) /* Clear error flag */
#define ADC_CS_READY  (1 << 8)  /* Conversion ready */

/* ---- Timer (for timestamps) ---- */
typedef struct {
    volatile uint32_t timehw;  /* 0x00: Timer high bits (write) */
    volatile uint32_t timelw;  /* 0x04: Timer low bits (write) */
    volatile uint32_t timeh;   /* 0x08: Timer high bits (read) */
    volatile uint32_t timel;   /* 0x0C: Timer low bits (read) */
    volatile uint32_t alarmhi; /* 0x10: Alarm high */
    volatile uint32_t alarmlo; /* 0x14: Alarm low */
    volatile uint32_t armed;    /* 0x18: Armed flags */
    volatile uint32_t timerawh; /* 0x1C: Raw timer high */
    volatile uint32_t timerawl; /* 0x20: Raw timer low */
} timer_reg_t;

#define TIMER ((timer_reg_t *)TIMER_BASE)

/* ---- Watchdog (for safe resets) ---- */
#define WATCHDOG_LOAD  (*(volatile uint32_t *)(WATCHDOG_BASE + 0x000))
#define WATCHDOG_VALUE (*(volatile uint32_t *)(WATCHDOG_BASE + 0x004))
#define WATCHDOG_CTRL  (*(volatile uint32_t *)(WATCHDOG_BASE + 0x008))
#define WATCHDOG_CTRL_TRIGGER (1 << 3)
#define WATCHDOG_CTRL_ENABLE  (1 << 7)

/* ---- PSM (Power State Machine) for peripheral resets ---- */
#define PSM_FRCE_OFF  (*(volatile uint32_t *)(PSM_BASE + 0x000))
#define PSM_FRCE_ON   (*(volatile uint32_t *)(PSM_BASE + 0x004))
#define PSM_WDSEL     (*(volatile uint32_t *)(PSM_BASE + 0x008))
#define PSM_DONE      (*(volatile uint32_t *)(PSM_BASE + 0x00C))

/* PSM periph bits */
#define PSM_USB     (1 << 22)
#define PSM_ADC     (1 << 17)
#define PSM_SPI1    (1 << 8)
#define PSM_SPI0    (1 << 6)
#define PSM_UART1   (1 << 9)
#define PSM_UART0   (1 << 7)
#define PSM_I2C0    (1 << 5)

/* ---- Helper: convert GPIO pin number to SIO bit ---- */
#define GPIO_BIT(n)  (1U << (n))

/* ---- Helper: FPGA SPI register offsets (FPGA side, accessed via SPI) ---- */
#define FPGA_REG_VERSION     0x00  /* FPGA bitstream version (RO) */
#define FPGA_REG_CTRL        0x01  /* Control register */
#define FPGA_REG_STATUS      0x02  /* Status register */
#define FPGA_REG_LINK_RATE  0x03  /* Detected link rate (RO) */
#define FPGA_REG_FRAME_CNT_LO 0x04 /* Frame count (low 32 bits) */
#define FPGA_REG_FRAME_CNT_HI 0x05 /* Frame count (high 32 bits) */
#define FPGA_REG_DROP_CNT    0x06 /* Dropped frame count */
#define FPGA_REG_MITM_ACTIVE 0x07 /* MITM engine active flag */
#define FPGA_REG_RULE_BASE   0x10  /* MITM rule RAM base (0x10–0x4F) */
#define FPGA_REG_DATA_FIFO   0x80  /* Captured frame data FIFO */
#define FPGA_REG_TIMESTAMP_LO 0x90 /* Frame timestamp (low 32 bits) */
#define FPGA_REG_TIMESTAMP_HI 0x91 /* Frame timestamp (high 32 bits) */
#define FPGA_REG_FRAME_LEN   0x92  /* Current frame length */

/* FPGA CTRL bits */
#define FPGA_CTRL_CAP_EN    (1 << 0)  /* Enable capture */
#define FPGA_CTRL_MITM_EN   (1 << 1)  /* Enable MITM engine */
#define FPGA_CTRL_INJECT_EN (1 << 2)  /* Enable VCSEL injection */
#define FPGA_CTRL_LOOPBACK  (1 << 3)  /* Internal loopback (debug) */
#define FPGA_CTRL_RESET_STATS (1 << 4) /* Reset all counters */

/* FPGA STATUS bits */
#define FPGA_STATUS_CDONE   (1 << 0)  /* FPGA configured and ready */
#define FPGA_STATUS_LINK_UP (1 << 1)  /* Optical link detected */
#define FPGA_STATUS_FRAME_READY (1 << 2) /* Frame available in FIFO */
#define FPGA_STATUS_FIFO_FULL (1 << 3) /* Data FIFO full (overflow risk) */
#define FPGA_STATUS_MITM_MATCH (1 << 4) /* MITM rule matched (latched) */
#define FPGA_STATUS_VCSEL_OK (1 << 5)   /* VCSEL bias current OK */

#endif /* FIBER_PHANTOM_REGISTERS_H */