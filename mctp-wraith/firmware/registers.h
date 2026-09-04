/*
 * MCTP Wraith registers.h
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef MCTP_WRAITH_REGISTERS_H
#define MCTP_WRAITH_REGISTERS_H

#include <stdint.h>

#define MW_RCC_BASE 0x58024400U
#define MW_GPIOA_BASE 0x58020000U
#define MW_GPIOB_BASE 0x58020400U
#define MW_GPIOC_BASE 0x58020800U
#define MW_I2C1_BASE 0x58005400U
#define MW_I3C1_BASE 0x58006000U
#define MW_SPI2_BASE 0x58003800U
#define MW_USART1_BASE 0x58006400U
#define MW_FMC_BASE 0x52004000U

#define MW_GPIO_MODER_OFFSET 0x00U
#define MW_GPIO_IDR_OFFSET   0x10U
#define MW_GPIO_BSRR_OFFSET  0x18U
#define MW_GPIO_REG(base, off) (*(volatile uint32_t *)((base) + (off)))

#define MW_I2C_CR1(base) (*(volatile uint32_t *)((base) + 0x00U))
#define MW_I2C_CR2(base) (*(volatile uint32_t *)((base) + 0x04U))
#define MW_I2C_ISR(base) (*(volatile uint32_t *)((base) + 0x18U))
#define MW_I2C_TXDR(base) (*(volatile uint32_t *)((base) + 0x28U))
#define MW_I2C_RXDR(base) (*(volatile uint32_t *)((base) + 0x24U))

#define MW_I3C_CFG(base) (*(volatile uint32_t *)((base) + 0x00U))
#define MW_I3C_STATUS(base) (*(volatile uint32_t *)((base) + 0x10U))
#define MW_I3C_CMD(base) (*(volatile uint32_t *)((base) + 0x20U))
#define MW_I3C_RX(base) (*(volatile uint32_t *)((base) + 0x24U))
#define MW_I3C_TX(base) (*(volatile uint32_t *)((base) + 0x28U))

#define MW_SPI_CR1(base) (*(volatile uint32_t *)((base) + 0x00U))
#define MW_SPI_CR2(base) (*(volatile uint32_t *)((base) + 0x04U))
#define MW_SPI_SR(base) (*(volatile uint32_t *)((base) + 0x08U))
#define MW_SPI_DR(base) (*(volatile uint32_t *)((base) + 0x0CU))

#define MW_I2C_CR1_PE     (1U << 0)
#define MW_I2C_CR2_START  (1U << 13)
#define MW_I2C_CR2_STOP   (1U << 14)
#define MW_I2C_ISR_TXE    (1U << 0)
#define MW_I2C_ISR_RXNE   (1U << 2)
#define MW_I2C_ISR_BUSY   (1U << 15)

#define MW_I3C_CFG_ENABLE      (1U << 0)
#define MW_I3C_CFG_HJ_ACCEPT   (1U << 1)
#define MW_I3C_STATUS_RXNE     (1U << 0)
#define MW_I3C_STATUS_TXNF     (1U << 1)
#define MW_I3C_STATUS_DA_MATCH (1U << 2)

#define MW_FPGA_REG_VERSION      0x0000U
#define MW_FPGA_REG_STATUS       0x0002U
#define MW_FPGA_REG_CONTROL      0x0004U
#define MW_FPGA_REG_CAPTURE_PTR  0x0006U
#define MW_FPGA_REG_MUTATION_CTL 0x0008U
#define MW_FPGA_REG_DELAY_MS     0x000AU
#define MW_FPGA_REG_ROUTE_SLOT   0x0010U
#define MW_FPGA_REG_POLICY_SLOT  0x0020U
#define MW_FPGA_REG_EID_FILTER   0x0030U
#define MW_FPGA_REG_ALERT_COUNT  0x0032U

#define MW_FPGA_STATUS_BRIDGE_UP   (1U << 0)
#define MW_FPGA_STATUS_CAPTURE_ARM (1U << 1)
#define MW_FPGA_STATUS_MUTATE_ARM  (1U << 2)
#define MW_FPGA_STATUS_ALERT       (1U << 3)

#define MW_FPGA_CTRL_BYPASS        (1U << 0)
#define MW_FPGA_CTRL_CAPTURE       (1U << 1)
#define MW_FPGA_CTRL_MUTATE        (1U << 2)
#define MW_FPGA_CTRL_SAFE_MODE     (1U << 3)

#endif
