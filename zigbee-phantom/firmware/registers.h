/*
 * registers.h — CC2652R1F register map excerpt for ZIGBEE-PHANTOM firmware
 * Author: jayis1
 * License: GPL-2.0
 *
 * Minimal register definitions needed by the firmware. Production builds
 * should link against TI's CC26xx ROM driverlib, but these definitions
 * make the firmware self-contained and reviewable without TI's headers.
 */
#ifndef ZIGBEE_PHANTOM_REGISTERS_H
#define ZIGBEE_PHANTOM_REGISTERS_H

#include <stdint.h>

/* ---- Memory map ---- */
#define FLASH_BASE_ADDR      0x00000000UL
#define SRAM_BASE_ADDR       0x20000000UL
#define ROM_BASE_ADDR       0x10000000UL

/* ---- AON RTC (always-on real-time counter, used for timestamps) ---- */
#define AON_RTC_BASE         0x40090000UL
#define AON_RTC_O_CTL        0x00000000UL
#define AON_RTC_O_EVFLAGS    0x00000004UL
#define AON_RTC_O_SEC        0x00000008UL
#define AON_RTC_O_SUBSEC     0x0000000CUL
#define AON_RTC_O_UPD        0x00000030UL

#define AON_RTC_CTL_EN            (1u << 1)
#define AON_RTC_CTL_RTC_UPD_EN    (1u << 7)

/* ---- RF core doorbell ---- */
#define RFC_DBELL_BASE       0x40081000UL
#define RFC_DBELL_O_CMDSTA   0x00000000UL
#define RFC_DBELL_O_CMDR     0x00000004UL
#define RFC_DBELL_O_CMDACK   0x00000008UL
#define RFC_DBELL_O_IRQFLAGS 0x0000000CUL

#define RFC_CMDSTA_DONE      0x00000000UL
#define RFC_CMDSTA_PENDING   0x00010000UL
#define RFC_CMDSTA_ERR_MASK  0x000F0000UL

/* ---- RF core memory (CPE / RAM) ---- */
#define RFC_RAM_BASE         0x21000000UL
#define RFC_RAM_SIZE         0x00004000UL   /* 16 KB PE RAM */
#define RFC_CPE_RAM_OFFSET   0x00000000UL
#define RFC_HW_IRQ_OFFSET    0x00004000UL

/* ---- RF core PE mailbox (command queue) ---- */
#define RFC_MBOX_BASE        0x21000000UL
#define RFC_MBOX_O_CMD       0x00000000UL
#define RFC_MBOX_O_RSP       0x00000004UL

/* ---- CPE interrupt vectors ---- */
#define RFC_CPE_IRQ0         0x00000001UL
#define RFC_CPE_IRQ1         0x00000002UL
#define RFC_CPE_IRQ2         0x00000004UL
#define RFC_CPE_IRQ_LASTCMD  (1u << 0)
#define RFC_CPE_IRQ_RXENTRY  (1u << 1)
#define RFC_CPE_IRQ_TXDONE   (1u << 2)
#define RFC_CPE_IRQ_RXBUF_FULL (1u << 3)

/* ---- AES crypto module (CC2652 AESC) ---- */
#define AESC_BASE            0x40030000UL
#define AESC_O_CTL           0x00000000UL
#define AESC_O_STAT          0x00000004UL
#define AESC_O_KEY0          0x00000010UL
#define AESC_O_KEY1          0x00000014UL
#define AESC_O_KEY2          0x00000018UL
#define AESC_O_KEY3          0x0000001CUL
#define AESC_O_IV0           0x00000020UL
#define AESC_O_IV1           0x00000024UL
#define AESC_O_IV2           0x00000028UL
#define AESC_O_IV3           0x0000002CUL
#define AESC_O_DATA0         0x00000030UL
#define AESC_O_DATA1         0x00000034UL
#define AESC_O_DATA2         0x00000038UL
#define AESC_O_DATA3         0x0000003CUL
#define AESC_O_TAG0          0x00000040UL
#define AESC_O_TAG1          0x00000044UL
#define AESC_O_TAG2          0x00000048UL
#define AESC_O_TAG3          0x0000004CUL

#define AESC_CTL_EN          (1u << 0)
#define AESC_CTL_START       (1u << 4)
#define AESC_CTL_CCM_MODE    (1u << 8)
#define AESC_CTL_CBC_MODE    (1u << 9)
#define AESC_STAT_DONE       (1u << 0)
#define AESC_STAT_RDY        (1u << 1)

/* ---- GPIO (CC2652 GPIO_BASE = 0x50000000) ---- */
#define GPIO_BASE            0x50000000UL
#define GPIO_O_DOE31_0       0x00000400UL
#define GPIO_O_DOUT31_0      0x00000800UL
#define GPIO_O_DIN31_0       0x00000C00UL
#define GPIO_O_DOE31_0_BYTES 4
#define GPIO_O_DOUT31_0_BYTES 4
#define GPIO_O_DIN31_0_BYTES 4

/* ---- I/O control (pin mux) ---- */
#define IOC_BASE             0x40080000UL
#define IOC_O_GPIO0          0x00000000UL
#define IOC_IOCFG_PORT_OFFSET 0x20UL      /* per-pin stride */

/* ---- SSI (SPI) ---- */
#define SSI_O_CR0            0x00000000UL
#define SSI_O_CR1            0x00000004UL
#define SSI_O_DR             0x0000000CUL
#define SSI_O_SR             0x0000000CUL
#define SSI_O_CPSR           0x00000010UL
#define SSI_SR_TFE           (1u << 0)
#define SSI_SR_TNF           (1u << 1)
#define SSI_SR_RNE           (1u << 2)
#define SSI_SR_RFF           (1u << 3)
#define SSI_SR_BSY           (1u << 4)

/* ---- I2C ---- */
#define I2C_O_SA             0x00000000UL
#define I2C_O_MCTRL          0x00000004UL
#define I2C_O_MSTAT          0x00000008UL
#define I2C_O_MDR            0x0000000CUL
#define I2C_MCTRL_RUN        (1u << 0)
#define I2C_MCTRL_START      (1u << 1)
#define I2C_MCTRL_STOP       (1u << 2)
#define I2C_MCTRL_ACK        (1u << 3)
#define I2C_MSTAT_BUSBSY     (1u << 0)
#define I2C_MSTAT_IDLE       (1u << 5)

/* ---- ADC (CC2652 AUX analog) ---- */
#define AUX_BASE             0x400C9000UL
#define AUXADC_BASE          0x400CB000UL
#define AUXADC_O_CTL         0x00000000UL
#define AUXADC_O_PCTL        0x00000004UL
#define AUXADC_O_DATA        0x00000020UL
#define AUXADC_CTL_EN        (1u << 0)
#define AUXADC_CTL_START     (1u << 1)

/* ---- Watchdog (disabled in capture firmware for timing determinism) ---- */
#define WDT_BASE             0x40080000UL
#define WDT_O_CTL            0x00000000UL
#define WDT_CTL_DIS          0x00000000UL

/* ---- System control (clocks, power) ---- */
#define SYSCTL_BASE          0x400F0000UL
#define SYSCTL_O_MCUCLK       0x00000000UL
#define SYSCTL_O_GPRAM        0x00000010UL
#define SYSCTL_O_SCLK         0x00000020UL
#define SYSCTL_O_RFPOWER      0x00000040UL

/* ---- Flash controller ---- */
#define FLASHC_BASE          0x40030000UL
#define FLASHC_O_FADDR       0x00000000UL
#define FLASHC_O_FDATA        0x00000004UL
#define FLASHC_O_FCTL         0x00000008UL
#define FLASHC_FCTL_WRITE     (1u << 1)
#define FLASHC_FCTL_ERASE     (1u << 2)

/* ---- NVIC helpers ---- */
#define NVIC_BASE            0xE000E100UL
#define NVIC_O_ISER0         0x00000000UL
#define NVIC_O_ICER0         0x00000080UL
#define NVIC_O_ISPR0         0x00000100UL
#define NVIC_O_ICPR0         0x00000180UL

#define IRQ_ENABLE(n)        (*((volatile uint32_t *)(NVIC_BASE + NVIC_O_ISER0 + (n/32)*4)) = (1u << (n%32)))
#define IRQ_DISABLE(n)       (*((volatile uint32_t *)(NVIC_BASE + NVIC_O_ICER0 + (n/32)*4)) = (1u << (n%32)))
#define IRQ_CLEAR(n)         (*((volatile uint32_t *)(NVIC_BASE + NVIC_O_ICPR0 + (n/32)*4)) = (1u << (n%32)))

/* ---- RF command opcodes (used in RF_cmdIeeeRx / RF_cmdIeeeTx) ---- */
#define IEEE_CMD_RADIO_SETUP  0x0001
#define IEEE_CMD_FS           0x0002   /* frequency synth off */
#define IEEE_CMD_RX           0x0003
#define IEEE_CMD_TX           0x0004
#define IEEE_CMD_ABORT        0x0005
#define IEEE_CMD_STOP        0x0006

/* ---- RF command status bits ---- */
#define RF_CMD_OK             0x00000000UL
#define RF_CMD_PENDING        0x00010000UL
#define RF_CMD_ERROR          0x000F0000UL

#endif /* ZIGBEE_PHANTOM_REGISTERS_H */