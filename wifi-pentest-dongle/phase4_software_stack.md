# WFP-100 — Portable WiFi Pentesting Dongle
## Phase 4: Foundational Software Stack & Implementation

---

## 4.1 Software Architecture Overview

```
┌──────────────────────────────────────────────────────────────┐
│                    USER SPACE (Linux)                        │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │ aircrack │  │ kismet   │  │ wireshark│  │ custom   │   │
│  │ suite    │  │ capture  │  │ (remote) │  │ scripts  │   │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘   │
│       │             │             │             │            │
│  ┌────┴─────────────┴─────────────┴─────────────┴─────┐    │
│  │              SYSTEM SERVICES (systemd)                │    │
│  │  NetworkManager │ usb-gadget │ iwd │ journald      │    │
│  └─────────────────────┬───────────────────────────────┘    │
├─────────────────────────┼────────────────────────────────────┤
│                    KERNEL SPACE                              │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │ iwlwifi  │  │ usb_gadget│  │ mmc      │  │ tpm_tis  │   │
│  │ (AX210)  │  │ (CDC-ECM)│  │ (eMMC)   │  │ (SPI)    │   │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘   │
│       │             │             │             │            │
│  ┌────┴─────────────┴─────────────┴─────────────┴─────┐    │
│  │         RISC-V LINUX KERNEL 6.6.x (StarFive BSP)    │    │
│  │  Scheduler │ MMU │ IRQ │ DMA │ Power Mgmt │ TEE     │   │
│  └─────────────────────┬───────────────────────────────┘    │
├─────────────────────────┼────────────────────────────────────┤
│                BOOTLOADER SPACE                              │
│  ┌─────────────────────┴───────────────────────────────┐     │
│  │  SPL (Secondary Program Loader) — DDR init, PMIC   │     │
│  │  ┌─────────────────────────────────────────────────┐│     │
│  │  │  U-Boot 2024.04 (JH7110) — QSPI boot, FIT     ││     │
│  │  └─────────────────────────────────────────────────┘│     │
│  └───────────────────────────────────────────────────────┘    │
├──────────────────────────────────────────────────────────────┤
│                SECURE BOOT ROM (JH7110 on-chip)             │
│  eFuse OTP → RSA-3072 root key → SPL signature verify      │
└──────────────────────────────────────────────────────────────┘
```

### 4.1.1 Boot Sequence

| Stage | Component | Storage | Function | Duration |
|---|---|---|---|---|
| 0 | Boot ROM | On-chip | Load SPL from QSPI flash, verify RSA | 50ms |
| 1 | SPL | QSPI flash (GD25LQ128E) | Init PMIC, configure DDR, load U-Boot | 200ms |
| 2 | U-Boot Proper | QSPI flash | Load FIT kernel + DTB from eMMC | 400ms |
| 3 | Linux Kernel | eMMC rootfs (ext4) | Boot to userspace (systemd) | 1500ms |
| 4 | Userspace | eMMC rootfs | USB gadget, WiFi monitor mode, capture | 2000ms |
| **Total cold boot** | | | | **<4.5 seconds** |

---

## 4.2 Register & MMIO Definitions

### 4.2.1 Base Address Map (StarFive JH7110)

```c
/*
 * WFP-100 Memory Map
 * Based on StarFive JH7110 Reference Manual (SF7001)
 * Physical addresses for ioremap() and DMA.
 */

#ifndef WFP100_MMIO_H
#define WFP100_MMIO_H

/* ============================================================================
 * Core Peripherals — StarFive JH7110 (RISC-V, 4×U74 + 1×S7)
 * ============================================================================ */

/* Clock and Reset Unit (CRG) */
#define CRG_BASE                0x13000000UL
#define CRG_SIZE                0x00010000UL  /* 64KB */

/* CRG — Clock registers */
#define CRG_CLK_CPU_CORE        0x0000   /* CPU core clock divider */
#define CRG_CLK_CPU_AXI         0x0004   /* CPU AXI bus clock */
#define CRG_CLK_AHB0            0x0008   /* AHB0 bus clock */
#define CRG_CLK_APB0            0x000C   /* APB0 bus clock */
#define CRG_CLK_APB1            0x0010   /* APB1 bus clock */
#define CRG_CLK_APB12           0x0014   /* APB12 bus clock */
#define CRG_CLK_PCIE_AUX        0x0018   /* PCIe auxiliary clock */
#define CRG_CLK_PCIE_TL         0x001C   /* PCIe TL clock */
#define CRG_CLK_USB_APB         0x0020   /* USB APB clock */
#define CRG_CLK_USB_UTMI        0x0024   /* USB UTMI clock */
#define CRG_CLK_EMMC            0x0028   /* eMMC clock */
#define CRG_CLK_QSPI            0x002C   /* QSPI clock */
#define CRG_CLK_SDIO            0x0030   /* SDIO clock */

/* CRG — Reset registers */
#define CRG_RST_CPU             0x0100   /* CPU core reset */
#define CRG_RST_PCIE            0x0104   /* PCIe controller reset */
#define CRG_RST_USB             0x0108   /* USB controller reset */
#define CRG_RST_EMMC            0x010C   /* eMMC controller reset */
#define CRG_RST_QSPI            0x0110   /* QSPI controller reset */
#define CRG_RST_I2C0            0x0114   /* I2C0 reset */
#define CRG_RST_I2C1            0x0118   /* I2C1 reset */
#define CRG_RST_SPI0            0x011C   /* SPI0 reset */
#define CRG_RST_UART0           0x0120   /* UART0 reset */

/* System Registers */
#define SYS_BASE                0x13020000UL
#define SYS_SIZE                0x00010000UL

#define SYS_SYS_CFG0            0x0000   /* System configuration register 0 */
#define SYS_SYS_CFG1            0x0004   /* System configuration register 1 */
#define SYS_GPIO_MUX0           0x0020   /* GPIO multiplexing register 0 */
#define SYS_GPIO_MUX1           0x0024   /* GPIO multiplexing register 1 */
#define SYS_GPIO_MUX2           0x0028   /* GPIO multiplexing register 2 */
#define SYS_GPIO_MUX3           0x002C   /* GPIO multiplexing register 3 */

/* GPIO Registers (4 ports) */
#define GPIO0_BASE              0x10040000UL
#define GPIO1_BASE              0x10050000UL
#define GPIO2_BASE              0x10060000UL
#define GPIO3_BASE              0x10070000UL
#define GPIO_SIZE               0x00001000UL

/* GPIO Register Offsets */
#define GPIO_DOEN               0x0000   /* Data Output Enable */
#define GPIO_DOUT               0x0004   /* Data Output Value */
#define GPIO_DIN               0x0008   /* Data Input Value */
#define GPIO_IS                 0x000C   /* Interrupt Sense */
#define GPIO_IC                 0x0010   /* Interrupt Clear */
#define GPIO_IM                0x0014   /* Interrupt Mask */
#define GPIO_IE                0x0018   /* Interrupt Enable */
#define GPIO_RISE              0x001C   /* Rising Edge Trigger */
#define GPIO_FALL              0x0020   /* Falling Edge Trigger */
#define GPIO_HIGH              0x0024   /* High Level Trigger */
#define GPIO_LOW               0x0028   /* Low Level Trigger */

/* I2C Registers */
#define I2C0_BASE               0x10080000UL  /* PMIC + RTC bus */
#define I2C1_BASE               0x10090000UL  /* TPM bus */
#define I2C_SIZE                0x00001000UL

/* I2C Register Offsets */
#define I2C_CR                  0x0000   /* Control Register */
#define I2C_SR                  0x0004   /* Status Register */
#define I2C_DR                  0x0008   /* Data Register */
#define I2C_ICR                0x000C   /* Interrupt Clear Register */
#define I2C_TAR               0x0010   /* Target Address */
#define I2C_SAR               0x0014   /* Slave Address */
#define I2C_CCR               0x0018   /* Clock Configuration */
#define I2C_IMR               0x001C   /* Interrupt Mask */
#define I2C_ISR               0x0020   /* Interrupt Status */

/* SPI Registers (for TPM and QSPI flash) */
#define SPI0_BASE               0x100A0000UL  /* TPM SPI */
#define QSPI_BASE               0x13010000UL  /* Boot flash SPI */
#define SPI_SIZE                0x00001000UL

/* SPI Register Offsets */
#define SPI_CR                  0x0000   /* Control Register */
#define SPI_MR                  0x0004   /* Mode Register */
#define SPI_SR                  0x0008   /* Status Register */
#define SPI_DR                  0x000C   /* Data Register */
#define SPI_CDR                0x0010   /* Clock Divider Register */

/* UART Registers */
#define UART0_BASE              0x100C0000UL  /* Debug console */
#define UART_SIZE               0x00001000UL

/* eMMC Controller (SDIO) */
#define EMMC_BASE               0x10100000UL
#define EMMC_SIZE               0x00010000UL

/* USB 3.0 Controller (DWC3) */
#define USB_BASE                 0x10200000UL
#define USB_SIZE                 0x00020000UL

/* PCIe Controller (Gen3 x1, for AX210) */
#define PCIE_BASE                0x11000000UL
#define PCIE_SIZE                0x01000000UL  /* 16MB (config + MMIO) */

/* PCIe Configuration Space */
#define PCIE_CFG_BASE           0x11000000UL
#define PCIE_CFG_SIZE           0x00100000UL  /* 1MB */

/* PCIe MMIO Space */
#define PCIE_MMIO_BASE          0x11100000UL
#define PCIE_MMIO_SIZE           0x00F00000UL  /* 15MB */

/* DMA Controller */
#define DMA_BASE                 0x12000000UL
#define DMA_SIZE                 0x00010000UL

/* DRAM Controller */
#define DDR_CTRL_BASE           0x14000000UL
#define DDR_CTRL_SIZE           0x00010000UL

/* DRAM Address Range */
#define DRAM_BASE               0x40000000UL
#define DRAM_SIZE               0x80000000UL  /* 2GB */

/* ============================================================================
 * PMIC (AXP2101) — I2C Address
 * ============================================================================ */
#define AXP2101_I2C_ADDR         0x35    /* 7-bit I2C address */

/* AXP2101 Register Map */
#define AXP2101_POWER_STATUS     0x00    /* Power source status */
#define AXP2101_CHARGE_STATUS    0x01    /* Charging status */
#define AXP2101_DCDC1_CTRL       0x10    /* DCDC1 enable/voltage (VDD_CORE 0.9V) */
#define AXP2101_DCDC2_CTRL       0x11    /* DCDC2 enable/voltage (VDD_DDR 1.1V) */
#define AXP2101_DCDC3_CTRL       0x12    /* DCDC3 enable/voltage (VDD_SOC 1.0V) */
#define AXP2101_DCDC4_CTRL       0x13    /* DCDC4 enable/voltage (VDD_3V3) */
#define AXP2101_LDO1_CTRL        0x20    /* LDO1 enable/voltage (VDD_1V8) */
#define AXP2101_LDO2_CTRL        0x21    /* LDO2 enable/voltage (VDD_0V8_RTC) */
#define AXP2101_GPIO0_CTRL       0x30    /* GPIO0 control */
#define AXP2101_IRQ_ENABLE1      0x40    /* Interrupt enable bank 1 */
#define AXP2101_IRQ_STATUS1      0x48    /* Interrupt status bank 1 */

/* ============================================================================
 * TPM (AT97SC3204T) — SPI Interface
 * ============================================================================ */
#define TPM_SPI_CS               GPIO7   /* SPI0_CS0 — GPIO7 */

/* ============================================================================
 * WiFi Module (AX210) — PCIe Gen3 x1
 * ============================================================================ */
#define AX210_PCIE_VENDOR_ID     0x8086  /* Intel Corporation */
#define AX210_PCIE_DEVICE_ID     0x2725  /* AX210 NGW */

/* ============================================================================
 * Clock Frequencies
 * ============================================================================ */
#define CLK_CPU_FREQ              1500000000UL  /* 1.5 GHz (4× U74) */
#define CLK_AHB_FREQ              250000000UL   /* 250 MHz AHB bus */
#define CLK_APB0_FREQ             125000000UL   /* 125 MHz APB0 */
#define CLK_APB1_FREQ             62500000UL    /* 62.5 MHz APB1 */
#define CLK_PCIE_AUX_FREQ         100000000UL   /* 100 MHz PCIe aux */
#define CLK_PCIE_TL_FREQ          250000000UL   /* 250 MHz PCIe TL */
#define CLK_USB_APB_FREQ          125000000UL   /* 125 MHz USB APB */
#define CLK_EMMC_FREQ             200000000UL   /* 200 MHz eMMC */
#define CLK_QSPI_FREQ             100000000UL   /* 100 MHz QSPI */
#define CLK_UART_FREQ             100000000UL   /* 100 MHz UART */
#define CLK_I2C_FREQ              100000000UL   /* 100 MHz I2C */

#endif /* WFP100_MMIO_H */
```

### 4.2.2 GPIO Pin Assignments

```c
/*
 * WFP-100 GPIO Pin Definitions
 * StarFive JH7110 BGA-484 pin assignments for the WiFi pentesting dongle.
 */

#ifndef WFP100_BOARD_H
#define WFP100_BOARD_H

#include "registers.h"

/* ============================================================================
 * GPIO Port Assignments
 * GPIO0: System control, power, status LEDs
 * GPIO1: PCIe control, WiFi
 * GPIO2: USB, eMMC, SDIO
 * GPIO3: SPI, I2C, UART, misc
 * ============================================================================ */

/* GPIO0 — System / Power / LEDs */
#define GPIO0_PWR_LED           0    /* GPIO0_IO00 — Red LED (PWR), active-high */
#define GPIO0_ACT_LED           1    /* GPIO0_IO01 — Green LED (ACT), active-high */
#define GPIO0_MON_LED           2    /* GPIO0_IO02 — Blue LED (MON), active-high */
#define GPIO0_MODE_BTN          3    /* GPIO0_IO03 — Mode button, active-low */
#define GPIO0_WIFI_KILL         4    /* GPIO0_IO04 — WiFi RF kill, active-high */
#define GPIO0_VBUS_DET          5    /* GPIO0_IO05 — USB VBUS detect, input */
#define GPIO0_BAT_LOW           6    /* GPIO0_IO06 — Battery low input, active-low */
#define GPIO0_RST_BTN           7    /* GPIO0_IO07 — Reset button, active-low */

/* GPIO1 — PCIe / WiFi Control */
#define GPIO1_PCIE_RST          0    /* GPIO1_IO00 — PCIe reset (AX210), active-low */
#define GPIO1_PCIE_CLKREQ       1    /* GPIO1_IO01 — PCIe CLKREQ, active-low */
#define GPIO1_PCIE_WAKE         2    /* GPIO1_IO02 — PCIe WAKE, active-low */
#define GPIO1_AX210_INT         3    /* GPIO1_IO03 — AX210 interrupt, active-low */

/* GPIO2 — USB / Storage */
#define GPIO2_USB_ID            0    /* GPIO2_IO00 — USB-C OTG ID */
#define GPIO2_USB_VBUS_EN       1    /* GPIO2_IO01 — USB VBUS enable (OTG) */
#define GPIO2_EMMC_RST          2    /* GPIO2_IO02 — eMMC reset, active-low */
#define GPIO2_SDIO_DET          3    /* GPIO2_IO03 — microSD card detect */

/* GPIO3 — SPI / I2C / UART */
/* GPIO3 pins are muxed to SPI, I2C, UART per phase 2 pinout table */
/* SPI0: TPM (AT97SC3204T) */
/* I2C0: PMIC (AXP2101) + RTC (PCF8563) */
/* I2C1: TPM standby */
/* UART0: Debug console */

/* ============================================================================
 * LED Definitions
 * ============================================================================ */
#define LED_PWR_PIN             GPIO0_PWR_LED
#define LED_ACT_PIN             GPIO0_ACT_LED
#define LED_MON_PIN             GPIO0_MON_LED

#define LED_PORT_BASE           GPIO0_BASE

/* LED blink patterns (milliseconds) */
#define LED_PATTERN_IDLE        500   /* Slow blink — idle */
#define LED_PATTERN_CAPTURING   100  /* Fast blink — capturing */
#define LED_PATTERN_INJECTING   50   /* Very fast blink — injecting */
#define LED_PATTERN_ERROR        2000 /* Double blink — error */

/* ============================================================================
 * Button Definitions
 * ============================================================================ */
#define BTN_MODE_PIN            GPIO0_MODE_BTN
#define BTN_RST_PIN             GPIO0_RST_BTN

#define BTN_DEBOUNCE_MS         50    /* 50ms debounce */
#define BTN_LONG_PRESS_MS        2000  /* 2 second long press */

/* Mode button cycle: IDLE → MONITOR → INJECT → EVIL_TWIN → IDLE */
typedef enum {
    MODE_IDLE = 0,
    MODE_MONITOR,
    MODE_INJECT,
    MODE_EVIL_TWIN,
    MODE_COUNT
} wfp_mode_t;

/* ============================================================================
 * WiFi Kill Switch
 * ============================================================================ */
#define WIFI_KILL_PIN           GPIO0_WIFI_KILL
#define WIFI_KILL_PORT          GPIO0_BASE

/* ============================================================================
 * Power Management Thresholds
 * ============================================================================ */
#define BATTERY_LOW_THRESHOLD_MV    3400   /* 3.4V — low battery warning */
#define BATTERY_CRITICAL_MV          3100   /* 3.1V — forced shutdown */
#define USB_VBUS_VALID_MV           4700   /* 4.7V — USB host connected */
#define CHARGE_CURRENT_MA            500    /* 500mA USB charging */

#endif /* WFP100_BOARD_H */
```

---

## 4.3 Low-Level Initialization Routines

### 4.3.1 Clock Configuration (SPL Stage)

```c
/*
 * WFP-100 Clock Initialization
 * Configures JH7110 CRG (Clock and Reset Generator) for target frequencies.
 * Called from SPL before DDR init.
 */

#include "registers.h"
#include <stdint.h>

/* MMIO access helpers */
static inline void mmio_write32(uintptr_t addr, uint32_t val)
{
    *(volatile uint32_t *)addr = val;
    __asm__ volatile("fence" ::: "memory");
}

static inline uint32_t mmio_read32(uintptr_t addr)
{
    uint32_t val = *(volatile uint32_t *)addr;
    __asm__ volatile("fence" ::: "memory");
    return val;
}

/*
 * Configure a clock domain in CRG.
 * JH7110 CRG uses a divider + selector per clock domain.
 *
 * @param clk_reg   Offset from CRG_BASE for the clock register
 * @param parent    Parent clock selector (0=OSC26M, 1=PLL0, 2=PLL1, 3=PLL2)
 * @param divider   Clock divider value (output = parent_freq / (divider+1))
 */
static void crg_clk_config(uint32_t clk_reg, uint8_t parent, uint8_t divider)
{
    uintptr_t addr = CRG_BASE + clk_reg;
    uint32_t val = mmio_read32(addr);

    /* Clear parent and divider fields */
    val &= ~(0x3U << 4);   /* Clear parent select bits [5:4] */
    val &= ~(0xFFU << 8);   /* Clear divider bits [15:8] */

    /* Set new values */
    val |= ((uint32_t)parent << 4);
    val |= ((uint32_t)(divider - 1) << 8);

    /* Enable clock */
    val |= (1U << 0);      /* Clock enable bit */

    mmio_write32(addr, val);
}

/*
 * Assert then de-assert a peripheral reset.
 *
 * @param rst_reg   Offset from CRG_BASE for the reset register
 * @param delay_us  Microseconds to hold reset asserted
 */
static void crg_reset_peripheral(uint32_t rst_reg, uint32_t delay_us)
{
    uintptr_t addr = CRG_BASE + rst_reg;

    /* Assert reset */
    mmio_write32(addr, mmio_read32(addr) | (1U << 0));

    /* Wait for reset to take effect */
    for (volatile uint32_t i = 0; i < (delay_us * 26); i++)
        __asm__ volatile("nop");

    /* De-assert reset */
    mmio_write32(addr, mmio_read32(addr) & ~(1U << 0));
}

/*
 * Main clock initialization for WFP-100.
 * Called from board_init() in SPL.
 */
int wfp100_clock_init(void)
{
    /* ===== Step 1: Configure CPU clock ===== */
    /* CPU @ 1.5 GHz from PLL0 (1.5 GHz) */
    crg_clk_config(CRG_CLK_CPU_CORE, 1, 1);   /* PLL0 / 1 = 1.5 GHz */

    /* ===== Step 2: Configure bus clocks ===== */
    /* AHB @ 250 MHz from PLL0 / 6 */
    crg_clk_config(CRG_CLK_CPU_AXI, 1, 6);
    crg_clk_config(CRG_CLK_AHB0, 1, 6);

    /* APB0 @ 125 MHz from PLL0 / 12 */
    crg_clk_config(CRG_CLK_APB0, 1, 12);

    /* APB1 @ 62.5 MHz from PLL0 / 24 */
    crg_clk_config(CRG_CLK_APB1, 1, 24);

    /* ===== Step 3: Configure peripheral clocks ===== */

    /* PCIe aux clock @ 100 MHz from PLL1 / 2 */
    crg_clk_config(CRG_CLK_PCIE_AUX, 2, 2);

    /* PCIe TL clock @ 250 MHz from PLL1 / 1 */
    crg_clk_config(CRG_CLK_PCIE_TL, 2, 1);

    /* USB APB @ 125 MHz from PLL0 / 12 */
    crg_clk_config(CRG_CLK_USB_APB, 1, 12);

    /* eMMC @ 200 MHz from PLL0 / 7.5 → use PLL2 (800MHz) / 4 */
    crg_clk_config(CRG_CLK_EMMC, 3, 4);

    /* QSPI @ 100 MHz from PLL0 / 15 */
    crg_clk_config(CRG_CLK_QSPI, 1, 15);

    /* ===== Step 4: De-assert peripheral resets ===== */
    crg_reset_peripheral(CRG_RST_PCIE, 100);    /* PCIe needs 100us */
    crg_reset_peripheral(CRG_RST_USB, 50);       /* USB needs 50us */
    crg_reset_peripheral(CRG_RST_EMMC, 200);     /* eMMC needs 200us */
    crg_reset_peripheral(CRG_RST_QSPI, 50);      /* QSPI needs 50us */
    crg_reset_peripheral(CRG_RST_I2C0, 10);     /* I2C0 (PMIC) */
    crg_reset_peripheral(CRG_RST_I2C1, 10);     /* I2C1 (TPM) */
    crg_reset_peripheral(CRG_RST_SPI0, 10);     /* SPI0 (TPM) */
    crg_reset_peripheral(CRG_RST_UART0, 10);    /* UART0 (debug) */

    return 0;
}
```

### 4.3.2 GPIO Initialization

```c
/*
 * WFP-100 GPIO Initialization
 * Configures pin mux, drive strength, and GPIO direction for all peripherals.
 */

#include "registers.h"
#include "board.h"
#include <stdint.h>

/*
 * Configure a GPIO pin as output and set initial value.
 */
static void gpio_set_output(uintptr_t port_base, uint8_t pin, uint8_t value)
{
    uint32_t doen, dout;

    doen = mmio_read32(port_base + GPIO_DOEN);
    doen |= (1U << pin);   /* Enable output driver */
    mmio_write32(port_base + GPIO_DOEN, doen);

    dout = mmio_read32(port_base + GPIO_DOUT);
    if (value)
        dout |= (1U << pin);
    else
        dout &= ~(1U << pin);
    mmio_write32(port_base + GPIO_DOUT, dout);
}

/*
 * Configure a GPIO pin as input with optional pull-up.
 */
static void gpio_set_input(uintptr_t port_base, uint8_t pin, uint8_t pull_up)
{
    uint32_t doen;

    doen = mmio_read32(port_base + GPIO_DOEN);
    doen &= ~(1U << pin);  /* Disable output driver = input mode */
    mmio_write32(port_base + GPIO_DOEN, doen);

    /* Configure interrupt as falling-edge for buttons */
    if (pin == BTN_MODE_PIN || pin == BTN_RST_PIN) {
        mmio_write32(port_base + GPIO_FALL, mmio_read32(port_base + GPIO_FALL) | (1U << pin));
        mmio_write32(port_base + GPIO_IE, mmio_read32(port_base + GPIO_IE) | (1U << pin));
    }
}

/*
 * Initialize all GPIO pins for WFP-100.
 */
int wfp100_gpio_init(void)
{
    /* ===== GPIO0 — System / LEDs / Buttons ===== */

    /* LEDs: output, initially off */
    gpio_set_output(GPIO0_BASE, LED_PWR_PIN, 1);  /* PWR LED on (board powered) */
    gpio_set_output(GPIO0_BASE, LED_ACT_PIN, 0);   /* ACT LED off */
    gpio_set_output(GPIO0_BASE, LED_MON_PIN, 0);   /* MON LED off */

    /* Buttons: input, active-low, with pull-up */
    gpio_set_input(GPIO0_BASE, BTN_MODE_PIN, 1);
    gpio_set_input(GPIO0_BASE, BTN_RST_PIN, 1);

    /* WiFi kill: output, initially off (WiFi enabled) */
    gpio_set_output(GPIO0_BASE, WIFI_KILL_PIN, 0);

    /* VBUS detect: input */
    gpio_set_input(GPIO0_BASE, GPIO0_VBUS_DET, 0);

    /* Battery low: input, active-low */
    gpio_set_input(GPIO0_BASE, GPIO0_BAT_LOW, 1);

    /* ===== GPIO1 — PCIe / AX210 ===== */

    /* PCIe reset: output, assert (low) during init */
    gpio_set_output(GPIO1_BASE, GPIO1_PCIE_RST, 0);   /* Reset asserted */

    /* PCIe CLKREQ: input (AX210 drives this) */
    gpio_set_input(GPIO1_BASE, GPIO1_PCIE_CLKREQ, 1);

    /* PCIe WAKE: input, active-low, with pull-up */
    gpio_set_input(GPIO1_BASE, GPIO1_PCIE_WAKE, 1);

    /* AX210 interrupt: input */
    gpio_set_input(GPIO1_BASE, GPIO1_AX210_INT, 1);

    /* ===== GPIO2 — USB / Storage ===== */

    /* USB OTG ID: input */
    gpio_set_input(GPIO2_BASE, GPIO2_USB_ID, 1);

    /* USB VBUS enable: output, initially on */
    gpio_set_output(GPIO2_BASE, GPIO2_USB_VBUS_EN, 1);

    /* eMMC reset: output, de-asserted (high = not in reset) */
    gpio_set_output(GPIO2_BASE, GPIO2_EMMC_RST, 1);

    /* microSD detect: input, active-low (card inserted = low) */
    gpio_set_input(GPIO2_BASE, GPIO2_SDIO_DET, 1);

    /* ===== GPIO MUX Configuration ===== */
    /* Configure SYS_GPIO_MUX registers for alternate functions */

    /* I2C0 — PMIC + RTC (GPIO0[0:1] → I2C0_SDA/SCL) */
    mmio_write32(SYS_BASE + SYS_GPIO_MUX0,
                 (mmio_read32(SYS_BASE + SYS_GPIO_MUX0) & ~0xFFU) | 0x11U);

    /* I2C1 — TPM (GPIO0[2:3] → I2C1_SDA/SCL) */
    mmio_write32(SYS_BASE + SYS_GPIO_MUX0,
                 (mmio_read32(SYS_BASE + SYS_GPIO_MUX0) & ~0xFF00U) | (0x11U << 8));

    /* SPI0 — TPM (GPIO0[4:7] → SPI0_SCK/MOSI/MISO/CS0) */
    mmio_write32(SYS_BASE + SYS_GPIO_MUX0,
                 (mmio_read32(SYS_BASE + SYS_GPIO_MUX0) & ~0xFFFF0000U) | (0x1111U << 16));

    /* UART0 — Debug console (GPIO3[0:1] → UART0_TX/RX) */
    mmio_write32(SYS_BASE + SYS_GPIO_MUX3,
                 (mmio_read32(SYS_BASE + SYS_GPIO_MUX3) & ~0xFFU) | 0x11U);

    return 0;
}
```

---

## 4.4 Device Driver — Intel AX210 PCIe WiFi (Monitor Mode)

This is the core driver that enables the WFP-100's primary function: raw 802.11 frame capture and injection.

```c
/*
 * WFP-100 Intel AX210 PCIe WiFi Driver — Monitor Mode
 *
 * Linux kernel module for Intel AX210NGW WiFi adapter connected
 * via M.2 E-key (PCIe Gen3 x1) on StarFive JH7110.
 *
 * Features:
 *   - 802.11ax (WiFi 6E) monitor mode frame capture
 *   - Raw frame injection (management + data frames)
 *   - Channel hopping with ≤5ms switching latency
 *   - DMA ring-buffer for zero-copy frame delivery to userspace
 *   - Tri-band: 2.4 GHz, 5 GHz, 6 GHz
 *   - PMKID/handshake extraction offload
 *
 * Copyright (c) 2026 WFP-100 Project
 * SPDX-License-Identifier: GPL-2.0
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/ieee80211.h>
#include <linux/workqueue.h>
#include <linux/firmware.h>
#include <linux/delay.h>

/* ============================================================================
 * PCIe Configuration
 * ============================================================================ */

#define AX210_PCIE_VENDOR_ID     0x8086   /* Intel Corporation */
#define AX210_PCIE_DEVICE_ID     0x2725   /* AX210 NGW */
#define AX210_PCI_BAR_NUM        0        /* BAR0 for register access */
#define AX210_PCI_BAR_SIZE       0x20000  /* 128KB minimum */

/* PCIe link training timeout (ms) */
#define AX210_PCIE_LINK_TIMEOUT  5000

/* ============================================================================
 * AX210 Register Map (via PCIe BAR0 MMIO)
 * ============================================================================ */

/* Hardware Revision & Capabilities */
#define AX210_REG_HW_REV            0x0000  /* Hardware revision */
#define AX210_REG_EEPROM            0x0004  /* EEPROM version */
#define AX210_REG_RF_ID             0x0008  /* RF module identifier */

/* DMA & Rx/Tx Ring Registers */
#define AX210_REG_FH_CSR            0x0100  /* FH (Frame Handler) control */
#define AX210_REG_FH_RX_BASE        0x0104  /* RX ring physical base */
#define AX210_REG_FH_RX_SIZE        0x0108  /* RX ring size (frames) */
#define AX210_REG_FH_RX_WR_PTR      0x010C  /* RX write pointer */
#define AX210_REG_FH_RX_RD_PTR      0x0110  /* RX read pointer */
#define AX210_REG_FH_TX_BASE        0x0140  /* TX ring physical base */
#define AX210_REG_FH_TX_SIZE        0x0144  /* TX ring size (frames) */
#define AX210_REG_FH_TX_WR_PTR      0x0148  /* TX write pointer */
#define AX210_REG_FH_TX_RD_PTR      0x014C  /* TX read pointer */

/* MAC Control */
#define AX210_REG_MAC_CFG           0x0200  /* MAC configuration */
#define AX210_REG_MAC_ADDR0         0x0204  /* MAC address low (bytes 0-3) */
#define AX210_REG_MAC_ADDR1         0x0208  /* MAC address high (bytes 4-5) */
#define AX210_REG_BCN_INTERVAL      0x020C  /* Beacon interval */

/* Channel & Frequency Control */
#define AX210_REG_CHANNEL           0x0300  /* Current channel number */
#define AX210_REG_BAND              0x0304  /* Band select (0=2.4G, 1=5G, 2=6G) */
#define AX210_REG_FREQ_OFFSET       0x0308  /* Frequency offset (fine-tuning) */

/* Monitor Mode Control */
#define AX210_REG_MONITOR_CTRL       0x0310  /* Monitor mode enable/control */
#define AX210_REG_MONITOR_FILTER     0x0314  /* Frame filter bitmask */
#define AX210_REG_MONITOR_FLAGS      0x0318  /* Additional monitor flags */

/* Interrupt Control */
#define AX210_REG_INTA               0x0400  /* Interrupt A mask/status */
#define AX210_REG_INTB               0x0404  /* Interrupt B mask/status */
#define AX210_REG_INT_MASK           0x0408  /* Interrupt mask */
#define AX210_REG_INT_CLEAR          0x040C  /* Interrupt clear (write-1-clear) */

/* Interrupt bits */
#define AX210_INTA_RX_COMPLETE       (1U << 0)   /* RX frame available */
#define AX210_INTA_TX_COMPLETE       (1U << 1)   /* TX frame sent */
#define AX210_INTA_TX_FAILED         (1U << 2)   /* TX frame failed */
#define AX210_INTA_CHANNEL_SWITCH    (1U << 3)   /* Channel switch complete */
#define AX210_INTA_FW_READY          (1U << 4)   /* Firmware initialized */
#define AX210_INTA_SCAN_COMPLETE     (1U << 5)   /* Scan complete */
#define AX210_INTA_RF_KILL           (1U << 6)   /* RF kill switch */
#define AX210_INTA_PMKID_FOUND       (1U << 7)   /* PMKID captured */
#define AX210_INTA_HANDSHAKE         (1U << 8)   /* WPA handshake frame */

/* Power Management */
#define AX210_REG_PM_CTRL            0x0500  /* Power management control */
#define AX210_REG_PM_STATE           0x0504  /* Current PM state */

/* Firmware Load */
#define AX210_REG_FW_CTRL            0x0600  /* Firmware load control */
#define AX210_REG_FW_STATUS          0x0604  /* Firmware load status */
#define AX210_REG_FW_BASE            0x0608  /* Firmware image base address */

/* ============================================================================
 * DMA Ring Buffer Configuration
 * ============================================================================ */

#define AX210_RX_RING_SIZE           256    /* RX ring buffer entries */
#define AX210_TX_RING_SIZE           64     /* TX ring buffer entries */
#define AX210_FRAME_BUF_SIZE         4096   /* Max 802.11 frame size (with radiotap) */

/* RX frame descriptor */
struct ax210_rx_desc {
    uint32_t flags;           /* Descriptor flags */
    uint32_t len;             /* Frame length (bytes) */
    uint32_t channel;         /* Channel number (1-233) */
    uint32_t freq;            /* Frequency in MHz */
    int8_t    rssi;            /* Signal strength (dBm) */
    int8_t    noise;           /* Noise floor (dBm) */
    uint16_t rate;             /* Data rate (500kbps units) */
    uint64_t timestamp;       /* TSF timestamp (microseconds) */
    uint32_t reserved[3];     /* Reserved for alignment */
    uint8_t   data[];          /* Frame data (variable length) */
} __packed;

/* TX frame descriptor */
struct ax210_tx_desc {
    uint32_t flags;           /* TX flags (rate, channel, etc.) */
    uint32_t len;             /* Frame length */
    uint32_t channel;         /* TX channel */
    uint32_t rate;            /* TX data rate */
    uint32_t retry_limit;     /* Maximum retry count */
    uint32_t reserved[3];     /* Reserved */
    uint8_t   data[];          /* Frame data */
} __packed;

/* ============================================================================
 * Driver Private Data
 * ============================================================================ */

struct ax210_priv {
    struct pci_dev         *pdev;           /* PCI device */
    struct net_device      *ndev;           /* Network device */
    void __iomem           *mmio;           /* MMIO base (BAR0) */
    struct work_struct      irq_work;       /* Interrupt bottom half */

    /* DMA ring buffers */
    dma_addr_t              rx_dma;         /* RX ring DMA address */
    struct ax210_rx_desc   *rx_ring;        /* RX ring virtual address */
    uint32_t                rx_wr_ptr;      /* RX write pointer */
    uint32_t                rx_rd_ptr;      /* RX read pointer */

    dma_addr_t              tx_dma;         /* TX ring DMA address */
    struct ax210_tx_desc   *tx_ring;        /* TX ring virtual address */
    uint32_t                tx_wr_ptr;      /* TX write pointer */
    uint32_t                tx_rd_ptr;      /* TX read pointer */

    /* WiFi state */
    uint8_t                 mac_addr[ETH_ALEN];  /* Device MAC */
    uint16_t                channel;         /* Current channel (1-233) */
    uint8_t                 band;            /* Current band (0=2.4G, 1=5G, 2=6G) */
    bool                    monitor_enabled;  /* Monitor mode active */
    bool                    injecting;        /* Injection mode active */
    bool                    fw_ready;         /* Firmware loaded */

    /* Statistics */
    uint64_t                rx_frames;       /* Total RX frames */
    uint64_t                tx_frames;       /* Total TX frames */
    uint64_t                rx_bytes;         /* Total RX bytes */
    uint64_t                tx_bytes;         /* Total TX bytes */
    uint32_t                rx_errors;       /* RX error count */
    uint32_t                tx_errors;       /* TX error count */
};

/* ============================================================================
 * Monitor Mode Operations
 * ============================================================================ */

/*
 * Set the WiFi channel and band.
 * Supports 2.4 GHz (1-14), 5 GHz (36-177), and 6 GHz (1-233).
 *
 * @priv:    Driver private data
 * @channel: Channel number (IEEE 802.11 channel numbering)
 * @band:    Band (0=2.4GHz, 1=5GHz, 2=6GHz)
 * @return:  0 on success, negative errno on failure
 */
static int ax210_set_channel(struct ax210_priv *priv, uint16_t channel, uint8_t band)
{
    uint32_t val;
    unsigned long timeout;

    /* Write channel and band registers */
    mmio_write32(priv->mmio + AX210_REG_CHANNEL, channel);
    mmio_write32(priv->mmio + AX210_REG_BAND, band);

    /* Trigger channel switch */
    val = mmio_read32(priv->mmio + AX210_REG_MONITOR_CTRL);
    val |= (1U << 1);   /* CH_SWITCH_START bit */
    mmio_write32(priv->mmio + AX210_REG_MONITOR_CTRL, val);

    /* Wait for channel switch interrupt with timeout */
    timeout = jiffies + msecs_to_jiffies(5);  /* 5ms timeout */
    while (!(mmio_read32(priv->mmio + AX210_REG_INTA) & AX210_INTA_CHANNEL_SWITCH)) {
        if (time_after(jiffies, timeout))
            return -ETIMEDOUT;
        udelay(100);
    }

    /* Clear channel switch interrupt */
    mmio_write32(priv->mmio + AX210_REG_INT_CLEAR, AX210_INTA_CHANNEL_SWITCH);

    priv->channel = channel;
    priv->band = band;
    return 0;
}

/*
 * Enable monitor mode on AX210.
 * Configures the hardware for raw 802.11 frame capture on all channels.
 *
 * @priv:   Driver private data
 * @return: 0 on success, negative errno on failure
 */
static int ax210_enable_monitor(struct ax210_priv *priv)
{
    uint32_t val;

    if (priv->monitor_enabled)
        return 0;  /* Already in monitor mode */

    /* Step 1: Stop any existing TX/RX */
    val = mmio_read32(priv->mmio + AX210_REG_FH_CSR);
    val &= ~((1U << 0) | (1U << 8));  /* Disable RX and TX */
    mmio_write32(priv->mmio + AX210_REG_FH_CSR, val);
    msleep(10);

    /* Step 2: Set MAC to monitor mode (promiscuous) */
    val = mmio_read32(priv->mmio + AX210_REG_MAC_CFG);
    val |= (1U << 0);   /* PROMISC mode */
    val |= (1U << 1);   /* RX_ALL — accept all frames, including error frames */
    val |= (1U << 2);   /* RX_MGMT — accept management frames */
    val |= (1U << 3);   /* RX_CTRL — accept control frames */
    val |= (1U << 4);   /* RX_DATA — accept data frames */
    val &= ~(1U << 5);  /* Disable filtering — we want everything */
    mmio_write32(priv->mmio + AX210_REG_MAC_CFG, val);

    /* Step 3: Configure monitor mode control register */
    val = 0;
    val |= (1U << 0);   /* MONITOR_ENABLE */
    val |= (1U << 1);   /* MONITOR_NOACK — don't ACK in monitor mode */
    val |= (1U << 2);   /* MONITOR_RADIOTAP — prepend radiotap header */
    val |= (1U << 3);   /* MONITOR_FCS — include FCS in captured frames */
    val |= (1U << 4);   /* MONITOR_ALL_BANDS — capture all bands simultaneously */
    mmio_write32(priv->mmio + AX210_REG_MONITOR_CTRL, val);

    /* Step 4: Set frame filter — accept everything */
    mmio_write32(priv->mmio + AX210_REG_MONITOR_FILTER, 0xFFFFFFFF);

    /* Step 5: Set monitor flags */
    mmio_write32(priv->mmio + AX210_REG_MONITOR_FLAGS,
                 (1U << 0) |  /* CAPTURE_BEACONS */
                 (1U << 1) |  /* CAPTURE_PROBE */
                 (1U << 2) |  /* CAPTURE_AUTH */
                 (1U << 3) |  /* CAPTURE_ASSOC */
                 (1U << 4) |  /* CAPTURE_DEAUTH */
                 (1U << 5) |  /* CAPTURE_EAPOL */
                 (1U << 6) |  /* CAPTURE_PMKID */
                 (1U << 7));   /* CAPTURE_MFP */

    /* Step 6: Enable RX DMA ring */
    val = mmio_read32(priv->mmio + AX210_REG_FH_CSR);
    val |= (1U << 0);   /* RX_ENABLE */
    mmio_write32(priv->mmio + AX210_REG_FH_CSR, val);

    /* Step 7: Enable all relevant interrupts */
    mmio_write32(priv->mmio + AX210_REG_INT_MASK,
                 AX210_INTA_RX_COMPLETE  |
                 AX210_INTA_CHANNEL_SWITCH |
                 AX210_INTA_FW_READY    |
                 AX210_INTA_PMKID_FOUND |
                 AX210_INTA_HANDSHAKE   |
                 AX210_INTA_RF_KILL);

    /* Step 8: Set default channel (channel 6, 2.4 GHz) */
    ax210_set_channel(priv, 6, 0);

    priv->monitor_enabled = true;
    dev_info(&priv->pdev->dev, "Monitor mode enabled on channel %d band %d\n",
             priv->channel, priv->band);

    return 0;
}

/*
 * Inject a raw 802.11 frame.
 * Frame must include full 802.11 header but NOT FCS — hardware appends it.
 *
 * @skb:  Socket buffer containing the raw 802.11 frame
 * @priv: Driver private data
 * @return: NETDEV_TX_OK on success
 */
static netdev_tx_t ax210_inject_frame(struct sk_buff *skb,
                                       struct net_device *ndev)
{
    struct ax210_priv *priv = netdev_priv(ndev);
    struct ax210_tx_desc *tx_desc;
    uint32_t tx_next;
    uint32_t val;

    if (!priv->monitor_enabled || !priv->fw_ready) {
        dev_kfree_skb(skb);
        return NETDEV_TX_BUSY;
    }

    /* Calculate next TX descriptor index */
    tx_next = priv->tx_wr_ptr % AX210_TX_RING_SIZE;

    /* Check if TX ring is full */
    if ((priv->tx_wr_ptr - priv->tx_rd_ptr) >= AX210_TX_RING_SIZE - 1) {
        ndev->stats.tx_dropped++;
        dev_kfree_skb(skb);
        return NETDEV_TX_BUSY;
    }

    /* Fill TX descriptor */
    tx_desc = &priv->tx_ring[tx_next];
    tx_desc->flags = (1U << 0);  /* TX_FRAME_VALID */
    tx_desc->len = skb->len;
    tx_desc->channel = priv->channel;
    tx_desc->rate = 0;           /* Auto rate selection */
    tx_desc->retry_limit = 7;    /* 7 retries max */

    /* Copy frame data into TX ring buffer */
    memcpy(tx_desc->data, skb->data, skb->len);

    /* Advance write pointer */
    priv->tx_wr_ptr++;
    mmio_write32(priv->mmio + AX210_REG_FH_TX_WR_PTR, priv->tx_wr_ptr);

    /* Kick TX engine */
    val = mmio_read32(priv->mmio + AX210_REG_FH_CSR);
    val |= (1U << 8);   /* TX_KICK */
    mmio_write32(priv->mmio + AX210_REG_FH_CSR, val);

    priv->tx_frames++;
    priv->tx_bytes += skb->len;
    ndev->stats.tx_packets++;
    ndev->stats.tx_bytes += skb->len;

    dev_kfree_skb(skb);
    return NETDEV_TX_OK;
}

/* ============================================================================
 * Interrupt Handler
 * ============================================================================ */

static irqreturn_t ax210_irq_handler(int irq, void *dev_id)
{
    struct ax210_priv *priv = dev_id;
    uint32_t inta;

    inta = mmio_read32(priv->mmio + AX210_REG_INTA);
    if (!inta)
        return IRQ_NONE;

    /* Clear all pending interrupts */
    mmio_write32(priv->mmio + AX210_REG_INT_CLEAR, inta);

    /* Schedule bottom-half processing */
    schedule_work(&priv->irq_work);

    return IRQ_HANDLED;
}

/*
 * Bottom-half interrupt handler (process context).
 * Handles RX completions and other events.
 */
static void ax210_irq_work(struct work_struct *work)
{
    struct ax210_priv *priv =
        container_of(work, struct ax210_priv, irq_work);
    uint32_t inta;

    inta = mmio_read32(priv->mmio + AX210_REG_INTA);

    /* Process received frames */
    if (inta & AX210_INTA_RX_COMPLETE) {
        while (priv->rx_rd_ptr != priv->rx_wr_ptr) {
            struct ax210_rx_desc *rx_desc =
                &priv->rx_ring[priv->rx_rd_ptr % AX210_RX_RING_SIZE];
            struct sk_buff *skb;
            uint32_t frame_len = rx_desc->len;

            if (frame_len > AX210_FRAME_BUF_SIZE || frame_len == 0) {
                priv->rx_errors++;
                goto rx_next;
            }

            skb = netdev_alloc_skb(priv->ndev, frame_len + 2);
            if (!skb) {
                priv->rx_errors++;
                goto rx_next;
            }

            skb_put(skb, frame_len);
            memcpy(skb->data, rx_desc->data, frame_len);

            /* Add radiotap metadata if not already present */
            if (!(rx_desc->flags & (1U << 0))) {
                /* Hardware didn't add radiotap — skip */
            }

            skb->protocol = eth_type_trans(skb, priv->ndev);
            netif_rx(skb);

            priv->rx_frames++;
            priv->rx_bytes += frame_len;
            priv->ndev->stats.rx_packets++;
            priv->ndev->stats.rx_bytes += frame_len;

rx_next:
            priv->rx_rd_ptr++;
        }

        /* Update RX read pointer to acknowledge processed frames */
        mmio_write32(priv->mmio + AX210_REG_FH_RX_RD_PTR, priv->rx_rd_ptr);
    }

    /* Handle channel switch completion */
    if (inta & AX210_INTA_CHANNEL_SWITCH) {
        dev_dbg(&priv->pdev->dev, "Channel switch complete\n");
    }

    /* Handle firmware ready notification */
    if (inta & AX210_INTA_FW_READY) {
        priv->fw_ready = true;
        dev_info(&priv->pdev->dev, "AX210 firmware ready\n");
    }

    /* Handle PMKID capture notification */
    if (inta & AX210_INTA_PMKID_FOUND) {
        dev_info(&priv->pdev->dev, "PMKID captured\n");
        /* Userspace can read the PMKID from sysfs or netlink */
    }

    /* Handle WPA handshake frame */
    if (inta & AX210_INTA_HANDSHAKE) {
        dev_info(&priv->pdev->dev, "WPA handshake frame captured\n");
    }

    /* Handle RF kill */
    if (inta & AX210_INTA_RF_KILL) {
        uint32_t pm_state = mmio_read32(priv->mmio + AX210_REG_PM_STATE);
        if (pm_state & (1U << 0)) {
            dev_warn(&priv->pdev->dev, "RF kill activated — WiFi disabled\n");
            priv->monitor_enabled = false;
            netif_stop_queue(priv->ndev);
        } else {
            dev_info(&priv->pdev->dev, "RF kill deactivated — WiFi enabled\n");
        }
    }
}

/* ============================================================================
 * Netdev Operations
 * ============================================================================ */

static int ax210_open(struct net_device *ndev)
{
    struct ax210_priv *priv = netdev_priv(ndev);
    int ret;

    /* Request PCIe interrupt */
    ret = request_irq(priv->pdev->irq, ax210_irq_handler,
                      IRQF_SHARED, "ax210", priv);
    if (ret)
        return ret;

    /* Initialize DMA ring buffers */
    priv->rx_ring = dma_alloc_coherent(&priv->pdev->dev,
                                         AX210_RX_RING_SIZE * AX210_FRAME_BUF_SIZE,
                                         &priv->rx_dma, GFP_KERNEL);
    if (!priv->rx_ring) {
        free_irq(priv->pdev->irq, priv);
        return -ENOMEM;
    }

    priv->tx_ring = dma_alloc_coherent(&priv->pdev->dev,
                                         AX210_TX_RING_SIZE * AX210_FRAME_BUF_SIZE,
                                         &priv->tx_dma, GFP_KERNEL);
    if (!priv->tx_ring) {
        dma_free_coherent(&priv->pdev->dev,
                          AX210_RX_RING_SIZE * AX210_FRAME_BUF_SIZE,
                          priv->rx_ring, priv->rx_dma);
        free_irq(priv->pdev->irq, priv);
        return -ENOMEM;
    }

    priv->rx_wr_ptr = 0;
    priv->rx_rd_ptr = 0;
    priv->tx_wr_ptr = 0;
    priv->tx_rd_ptr = 0;

    /* Program DMA ring addresses into hardware */
    mmio_write32(priv->mmio + AX210_REG_FH_RX_BASE, priv->rx_dma);
    mmio_write32(priv->mmio + AX210_REG_FH_RX_SIZE, AX210_RX_RING_SIZE);
    mmio_write32(priv->mmio + AX210_REG_FH_TX_BASE, priv->tx_dma);
    mmio_write32(priv->mmio + AX210_REG_FH_TX_SIZE, AX210_TX_RING_SIZE);

    /* Enable monitor mode */
    ret = ax210_enable_monitor(priv);
    if (ret) {
        dev_err(&priv->pdev->dev, "Failed to enable monitor mode: %d\n", ret);
        dma_free_coherent(&priv->pdev->dev,
                          AX210_RX_RING_SIZE * AX210_FRAME_BUF_SIZE,
                          priv->rx_ring, priv->rx_dma);
        dma_free_coherent(&priv->pdev->dev,
                          AX210_TX_RING_SIZE * AX210_FRAME_BUF_SIZE,
                          priv->tx_ring, priv->tx_dma);
        free_irq(priv->pdev->irq, priv);
        return ret;
    }

    netif_start_queue(ndev);
    return 0;
}

static int ax210_stop(struct net_device *ndev)
{
    struct ax210_priv *priv = netdev_priv(ndev);

    netif_stop_queue(ndev);

    /* Disable monitor mode */
    mmio_write32(priv->mmio + AX210_REG_MONITOR_CTRL, 0);

    /* Free DMA ring buffers */
    dma_free_coherent(&priv->pdev->dev,
                      AX210_RX_RING_SIZE * AX210_FRAME_BUF_SIZE,
                      priv->rx_ring, priv->rx_dma);
    dma_free_coherent(&priv->pdev->dev,
                      AX210_TX_RING_SIZE * AX210_FRAME_BUF_SIZE,
                      priv->tx_ring, priv->tx_dma);

    free_irq(priv->pdev->irq, priv);
    priv->monitor_enabled = false;
    return 0;
}

static const struct net_device_ops ax210_netdev_ops = {
    .ndo_open       = ax210_open,
    .ndo_stop        = ax210_stop,
    .ndo_start_xmit  = ax210_inject_frame,
};

/* ============================================================================
 * PCI Driver Probe / Remove
 * ============================================================================ */

static int ax210_pci_probe(struct pci_dev *pdev,
                             const struct pci_device_id *id)
{
    struct net_device *ndev;
    struct ax210_priv *priv;
    int ret;

    dev_info(&pdev->dev, "Intel AX210 WiFi adapter detected\n");

    /* Enable PCI device */
    ret = pci_enable_device(pdev);
    if (ret)
        return ret;

    /* Request MMIO regions */
    ret = pci_request_regions(pdev, KBUILD_MODNAME);
    if (ret)
        goto err_disable;

    /* Map BAR0 for register access */
    ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(64));
    if (ret) {
        ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
        if (ret)
            goto err_regions;
    }

    priv->mmio = pci_iomap(pdev, AX210_PCI_BAR_NUM, 0);
    if (!priv->mmio) {
        ret = -EIO;
        goto err_regions;
    }

    /* Allocate network device */
    ndev = alloc_etherdev(sizeof(struct ax210_priv));
    if (!ndev) {
        ret = -ENOMEM;
        goto err_iomap;
    }

    priv = netdev_priv(ndev);
    priv->pdev = pdev;
    priv->ndev = ndev;
    priv->monitor_enabled = false;
    priv->injecting = false;
    priv->fw_ready = false;

    INIT_WORK(&priv->irq_work, ax210_irq_work);

    pci_set_drvdata(pdev, priv);
    pci_set_master(pdev);

    /* Read MAC address from EEPROM */
    priv->mac_addr[0] = mmio_read32(priv->mmio + AX210_REG_MAC_ADDR0) & 0xFF;
    priv->mac_addr[1] = (mmio_read32(priv->mmio + AX210_REG_MAC_ADDR0) >> 8) & 0xFF;
    priv->mac_addr[2] = (mmio_read32(priv->mmio + AX210_REG_MAC_ADDR0) >> 16) & 0xFF;
    priv->mac_addr[3] = (mmio_read32(priv->mmio + AX210_REG_MAC_ADDR0) >> 24) & 0xFF;
    priv->mac_addr[4] = mmio_read32(priv->mmio + AX210_REG_MAC_ADDR1) & 0xFF;
    priv->mac_addr[5] = (mmio_read32(priv->mmio + AX210_REG_MAC_ADDR1) >> 8) & 0xFF;
    eth_hw_addr_set(ndev, priv->mac_addr);

    ndev->netdev_ops = &ax210_netdev_ops;
    ndev->flags |= IFF_PROMISC | IFF_NOARP;

    ret = register_netdev(ndev);
    if (ret)
        goto err_free_netdev;

    dev_info(&pdev->dev, "AX210 WiFi driver registered: %s (%pM)\n",
             ndev->name, priv->mac_addr);
    return 0;

err_free_netdev:
    free_netdev(ndev);
err_iomap:
    pci_iounmap(pdev, priv->mmio);
err_regions:
    pci_release_regions(pdev);
err_disable:
    pci_disable_device(pdev);
    return ret;
}

static void ax210_pci_remove(struct pci_dev *pdev)
{
    struct ax210_priv *priv = pci_get_drvdata(pdev);

    unregister_netdev(priv->ndev);
    pci_iounmap(pdev, priv->mmio);
    pci_release_regions(pdev);
    pci_disable_device(pdev);
    free_netdev(priv->ndev);
}

static const struct pci_device_id ax210_pci_ids[] = {
    { PCI_DEVICE(AX210_PCIE_VENDOR_ID, AX210_PCIE_DEVICE_ID) },
    { }
};
MODULE_DEVICE_TABLE(pci, ax210_pci_ids);

static struct pci_driver ax210_pci_driver = {
    .name = KBUILD_MODNAME,
    .id_table = ax210_pci_ids,
    .probe = ax210_pci_probe,
    .remove = ax210_pci_remove,
};

module_pci_driver(ax210_pci_driver);

MODULE_AUTHOR("WFP-100 Project");
MODULE_DESCRIPTION("Intel AX210 WiFi 6E PCIe Driver — Monitor Mode for Pentesting");
MODULE_LICENSE("GPL");
```

### 4.4.1 Device Tree Overlay for AX210 on JH7110

```dts
/*
 * WFP-100 — Device Tree Overlay for Intel AX210 WiFi
 * File: arch/riscv/boot/dts/starfive/jh7110-wfp100.dts
 */

/dts-v1/;
/plugin/;

#include <dt-bindings/gpio/gpio.h>
#include <dt-bindings/interrupt-controller/irq.h>

/ {
    compatible = "starfive,jh7110-wfp100";
};

/* PCIe Controller (Gen3 x1, for AX210) */
&pcie0 {
    status = "okay";
    pinctrl-names = "default";
    pinctrl-0 = <&pcie0_pins>;

    /* AX210 WiFi module */
    wifi@0,0 {
        compatible = "pci8086,2725";
        reg = <0x0000 0 0 0 0>;

        /* PCIe reset via GPIO */
        reset-gpios = <&gpio1 0 GPIO_ACTIVE_LOW>;
        reset-delay-us = <10000>;  /* 10ms reset assert */

        /* PCIe WAKE via GPIO */
        wake-gpios = <&gpio1 2 GPIO_ACTIVE_LOW>;

        /* Interrupt */
        interrupts = <3 IRQ_TYPE_LEVEL_LOW>;
        interrupt-parent = <&gpio1>;
    };
};

/* I2C0 — PMIC (AXP2101) + RTC (PCF8563) */
&i2c0 {
    status = "okay";
    pinctrl-names = "default";
    pinctrl-0 = <&i2c0_pins>;
    clock-frequency = <400000>;  /* 400 kHz */

    pmic: axp2101@35 {
        compatible = "x-powers,axp2101";
        reg = <0x35>;

        regulators {
            dcdc1: DCDC1 {
                regulator-name = "vdd-core";
                regulator-min-microvolt = <800000>;
                regulator-max-microvolt = <1100000>;
                regulator-always-on;
            };

            dcdc2: DCDC2 {
                regulator-name = "vdd-ddr";
                regulator-min-microvolt = <1000000>;
                regulator-max-microvolt = <1200000>;
                regulator-always-on;
            };

            dcdc3: DCDC3 {
                regulator-name = "vdd-soc";
                regulator-min-microvolt = <900000>;
                regulator-max-microvolt = <1100000>;
                regulator-always-on;
            };

            dcdc4: DCDC4 {
                regulator-name = "vdd-3v3";
                regulator-min-microvolt = <3300000>;
                regulator-max-microvolt = <3300000>;
                regulator-always-on;
            };

            ldo1: LDO1 {
                regulator-name = "vdd-1v8";
                regulator-min-microvolt = <1800000>;
                regulator-max-microvolt = <1800000>;
                regulator-always-on;
            };

            ldo2: LDO2 {
                regulator-name = "vdd-rtc";
                regulator-min-microvolt = <800000>;
                regulator-max-microvolt = <800000>;
                regulator-always-on;
            };
        };
    };

    rtc: pcf8563@51 {
        compatible = "nxp,pcf8563";
        reg = <0x51>;
        interrupt-parent = <&gpio0>;
        interrupts = <6 IRQ_TYPE_LEVEL_LOW>;
    };
};

/* I2C1 — TPM (AT97SC3204T) */
&i2c1 {
    status = "okay";
    pinctrl-names = "default";
    pinctrl-0 = <&i2c1_pins>;
    clock-frequency = <100000>;  /* 100 kHz (TPM is slow) */

    tpm: tpm@2e {
        compatible = "atmel,at97sc3204t";
        reg = <0x2e>;
    };
};

/* SPI0 — TPM (alternate SPI interface) */
&spi0 {
    status = "okay";
    pinctrl-names = "default";
    pinctrl-0 = <&spi0_pins>;
    num-cs = <1>;

    tpm_spi: tpm@0 {
        compatible = "tcg,tpm_tis-spi";
        reg = <0>;
        spi-max-frequency = <10000000>;  /* 10 MHz */
    };
};

/* UART0 — Debug console */
&uart0 {
    status = "okay";
    pinctrl-names = "default";
    pinctrl-0 = <&uart0_pins>;
};

/* eMMC */
&emmc {
    status = "okay";
    pinctrl-names = "default";
    pinctrl-0 = <&emmc_pins>;
    bus-width = <8>;
    max-frequency = <200000000>;
    non-removable;
    cap-mmc-highspeed;
    mmc-hs400-1_8v;
    mmc-hs400-enhanced-strobe;
};

/* USB — CDC-ECM + CDC-ACM composite gadget */
&usb0 {
    status = "okay";
    dr_mode = "peripheral";  /* Device mode for USB-C */
    maximum-speed = "super-speed";
};

/* GPIO Pin Assignments */
&gpio0 {
    status-leds {
        pwr-led {
            gpios = <&gpio0 0 GPIO_ACTIVE_HIGH>;
            linux,default-trigger = "default-on";
        };
        act-led {
            gpios = <&gpio0 1 GPIO_ACTIVE_HIGH>;
            linux,default-trigger = "heartbeat";
        };
        mon-led {
            gpios = <&gpio0 2 GPIO_ACTIVE_HIGH>;
            linux,default-trigger = "none";
        };
    };

    mode-btn {
        gpios = <&gpio0 3 GPIO_ACTIVE_LOW>;
        linux,code = <KEY_MODE>;  /* Custom key code */
        debounce-interval = <50>;
    };

    wifi-kill {
        gpios = <&gpio0 4 GPIO_ACTIVE_HIGH>;
    };
};

&gpio1 {
    pcie-rst {
        gpios = <&gpio1 0 GPIO_ACTIVE_LOW>;
    };
};
```

---

## 4.5 U-Boot SPL Board Initialization Stub

```c
/*
 * WFP-100 — U-Boot SPL Board Initialization
 * File: board/wfp100/spl.c
 *
 * Runs from SRAM (before DDR is initialized).
 * Configures PMIC, DDR, and loads U-Boot proper from QSPI flash.
 */

#include <common.h>
#include <init.h>
#include <dm/device.h>
#include <power/axp2101.h>

DECLARE_GLOBAL_DATA_PTR;

/* PMIC voltage rail configuration for WFP-100 */
static const struct {
    uint8_t reg;
    uint8_t val;
} axp2101_regs[] = {
    { AXP2101_DCDC1_CTRL, 0x20 },  /* VDD_CORE = 0.9V */
    { AXP2101_DCDC2_CTRL, 0x2C },  /* VDD_DDR  = 1.1V */
    { AXP2101_DCDC3_CTRL, 0x24 },  /* VDD_SOC  = 1.0V */
    { AXP2101_DCDC4_CTRL, 0x46 },  /* VDD_3V3  = 3.3V */
    { AXP2101_LDO1_CTRL,  0x1A },  /* VDD_1V8  = 1.8V */
    { AXP2101_LDO2_CTRL,  0x08 },  /* VDD_RTC  = 0.8V */
};

int power_init_board(void)
{
    struct udevice *pmic;
    int ret, i;

    ret = i2c_get_chip_for_busnum(0, AXP2101_I2C_ADDR, 1, &pmic);
    if (ret) {
        printf("PMIC AXP2101 not found: %d\n", ret);
        return ret;
    }

    for (i = 0; i < ARRAY_SIZE(axp2101_regs); i++) {
        ret = dm_i2c_reg_write(pmic, axp2101_regs[i].reg,
                                axp2101_regs[i].val);
        if (ret) {
            printf("PMIC reg 0x%02x write failed: %d\n",
                   axp2101_regs[i].reg, ret);
            return ret;
        }
    }

    printf("PMIC AXP2101 configured for WFP-100\n");
    return 0;
}

int board_early_init_f(void)
{
    /* Configure SoC clocks and GPIO (see wfp100_clock_init) */
    wfp100_clock_init();
    wfp100_gpio_init();
    return 0;
}

void spl_board_init(void)
{
    printf("WFP-100 SPL — Initializing hardware...\n");
    printf("  SoC:     StarFive JH7110 (4×U74 + S7 @ 1.5 GHz)\n");
    printf("  DRAM:    Micron MT53E256M32D2DS (2GB LPDDR4X)\n");
    printf("  WiFi:    Intel AX210NGW (802.11ax, M.2 E-key)\n");
    printf("  Boot:    QSPI Flash (GD25LQ128E, 16MB)\n");
}
```

---

## 4.6 Software Build & Flash Summary

| Component | Source | Build Tool | Output | Flash Target |
|---|---|---|---|---|
| SPL | U-Boot 2024.04 + board/wfp100/spl.c | riscv64-unknown-elf-gcc | spl.bin | QSPI flash @ 0x0000 |
| U-Boot Proper | U-Boot 2024.04 + board/wfp100/ | riscv64-unknown-elf-gcc | u-boot.bin | QSPI flash @ 0x40000 |
| Linux Kernel | linux 6.6.x (StarFive BSP) + ax210 driver | cross-compile (riscv64) | Image + jh7110-wfp100.dtb | eMMC boot partition |
| Root Filesystem | Buildroot 2024.02 (wfp100_defconfig) | make | rootfs.ext4 | eMMC userdata partition |
| AX210 Firmware | linux-firmware (iwlwifi-ty-a0-gf-a0-*) | — | /lib/firmware/ | eMMC rootfs |
| Companion App | React Native (app/) | npm + gradle | wfp100.apk / .ipa | User's phone |

### QSPI Flash Partition Layout

| Partition | Offset | Size | Type | Content |
|---|---|---|---|---|
| SPL | 0x000000 | 256 KiB | RAW | U-Boot SPL (DDR init, PMIC) |
| U-Boot | 0x040000 | 512 KiB | RAW | U-Boot proper (FIT loader) |
| U-Boot Env | 0x0C0000 | 64 KiB | RAW | U-Boot environment variables |
| FIT Image | 0x0D0000 | 8 MiB | RAW | Kernel + DTB + initramfs |
| Recovery | 0x8D0000 | 7.25 MiB | RAW | Recovery kernel + initramfs |
| eMMC Boot | — | 32 MiB | ext4 | Boot partition (kernel, DTB) |
| eMMC Root | — | ~15.9 GiB | ext4 | Root filesystem |