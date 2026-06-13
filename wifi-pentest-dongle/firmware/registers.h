/*
 * WFP-100 — MMIO Register Definitions
 * StarFive JH7110 (RISC-V, 4×U74 + S7 @ 1.5 GHz)
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef WFP100_REGISTERS_H
#define WFP100_REGISTERS_H

#include <stdint.h>

/* ============================================================================
 * Core Peripherals — StarFive JH7110
 * ============================================================================ */

/* Clock and Reset Unit (CRG) */
#define CRG_BASE                0x13000000UL
#define CRG_CLK_CPU_CORE        0x0000
#define CRG_CLK_CPU_AXI         0x0004
#define CRG_CLK_AHB0            0x0008
#define CRG_CLK_APB0            0x000C
#define CRG_CLK_APB1            0x0010
#define CRG_CLK_APB12           0x0014
#define CRG_CLK_PCIE_AUX        0x0018
#define CRG_CLK_PCIE_TL         0x001C
#define CRG_CLK_USB_APB         0x0020
#define CRG_CLK_USB_UTMI        0x0024
#define CRG_CLK_EMMC            0x0028
#define CRG_CLK_QSPI            0x002C
#define CRG_RST_CPU             0x0100
#define CRG_RST_PCIE            0x0104
#define CRG_RST_USB             0x0108
#define CRG_RST_EMMC            0x010C
#define CRG_RST_QSPI            0x0110
#define CRG_RST_I2C0            0x0114
#define CRG_RST_I2C1            0x0118
#define CRG_RST_SPI0            0x011C
#define CRG_RST_UART0           0x0120

/* System Registers */
#define SYS_BASE                0x13020000UL
#define SYS_GPIO_MUX0           0x0020
#define SYS_GPIO_MUX1           0x0024
#define SYS_GPIO_MUX2           0x0028
#define SYS_GPIO_MUX3           0x002C

/* GPIO Registers */
#define GPIO0_BASE              0x10040000UL
#define GPIO1_BASE              0x10050000UL
#define GPIO2_BASE              0x10060000UL
#define GPIO3_BASE              0x10070000UL
#define GPIO_SIZE               0x00001000UL
#define GPIO_DOEN               0x0000
#define GPIO_DOUT               0x0004
#define GPIO_DIN                0x0008

/* I2C Registers */
#define I2C0_BASE               0x10080000UL
#define I2C1_BASE               0x10090000UL
#define I2C_SIZE                0x00001000UL
#define I2C_CR                  0x0000
#define I2C_SR                  0x0004
#define I2C_DR                  0x0008
#define I2C_TAR                 0x0010
#define I2C_CCR                 0x0018

/* SPI Registers */
#define SPI0_BASE               0x100A0000UL
#define QSPI_BASE               0x13010000UL

/* UART Registers */
#define UART0_BASE              0x100C0000UL

/* eMMC Controller */
#define EMMC_BASE               0x10100000UL

/* USB 3.0 Controller (DWC3) */
#define USB_BASE                0x10200000UL

/* PCIe Controller */
#define PCIE_BASE               0x11000000UL

/* DMA Controller */
#define DMA_BASE                0x12000000UL

/* DDR Controller */
#define DDR_CTRL_BASE           0x14000000UL

/* DRAM */
#define DRAM_BASE               0x40000000UL
#define DRAM_SIZE               0x80000000UL  /* 2GB */

/* PMIC I2C Address */
#define AXP2101_I2C_ADDR        0x35

/* PMIC Registers */
#define AXP2101_POWER_STATUS    0x00
#define AXP2101_CHARGE_STATUS   0x01
#define AXP2101_DCDC1_CTRL      0x10
#define AXP2101_DCDC2_CTRL      0x11
#define AXP2101_DCDC3_CTRL      0x12
#define AXP2101_DCDC4_CTRL      0x13
#define AXP2101_LDO1_CTRL       0x20
#define AXP2101_LDO2_CTRL       0x21

/* Clock Frequencies */
#define CLK_CPU_FREQ            1500000000UL
#define CLK_AHB_FREQ            250000000UL
#define CLK_APB0_FREQ           125000000UL
#define CLK_UART_FREQ           100000000UL

/* MMIO access */
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

#endif /* WFP100_REGISTERS_H */