/**
 * @file registers.h
 * @brief GLYPH-REAPER register definitions and hardware abstraction
 *
 * Low-level register definitions for nRF52840 peripherals used by GLYPH-REAPER.
 * Includes peripheral base addresses, register bit definitions, and
 * hardware-specific constants for the display capture pipeline.
 *
 * Board: GLYPH-REAPER Rev A
 * MCU:   nRF52840-QIAA-R7
 * Author: jayis1
 *
 * @copyright Copyright (c) 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * nRF52840 PERIPHERAL BASE ADDRESSES
 *===========================================================================*/

/* ARM Cortex-M4 system registers */
#define SCB_BASE                0xE000ED00UL
#define SysTick_BASE            0xE000E010UL
#define NVIC_BASE               0xE000E100UL
#define NVIC_ISER0             (*(volatile uint32_t *)(NVIC_BASE + 0x000))
#define NVIC_ISER1             (*(volatile uint32_t *)(NVIC_BASE + 0x004))
#define NVIC_ICPR0             (*(volatile uint32_t *)(NVIC_BASE + 0x080))
#define NVIC_ICPR1             (*(volatile uint32_t *)(NVIC_BASE + 0x084))

#define SCB_SCR                 (*(volatile uint32_t *)(SCB_BASE + 0x010))
#define SCB_SCR_SEVONPEND       (1 << 4)
#define SCB_SCR_SLEEPDEEP       (1 << 2)
#define SCB_SCR_SLEEPONEXIT     (1 << 1)

/* nRF52840 peripheral base addresses */
#define NRF_CLOCK_BASE          0x40000000UL
#define NRF_POWER_BASE          0x40000000UL  /* Shared with CLOCK */
#define NRF_RADIO_BASE          0x40001000UL
#define NRF_UART0_BASE          0x40002000UL
#define NRF_SPIM0_BASE          0x40003000UL
#define NRF_SPIM1_BASE          0x40004000UL
#define NRF_SPIM2_BASE          0x40023000UL
#define NRF_SPIM3_BASE          0x4002F000UL
#define NRF_TWIM0_BASE          0x40003000UL  /* Shared with SPIM0 */
#define NRF_GPIOTE_BASE         0x40006000UL
#define NRF_SAADC_BASE          0x40007000UL
#define NRF_TIMER0_BASE         0x40008000UL
#define NRF_TIMER1_BASE         0x40009000UL
#define NRF_TIMER2_BASE         0x4000A000UL
#define NRF_RTC0_BASE           0x4000B000UL
#define NRF_RTC1_BASE           0x40011000UL
#define NRF_RTC2_BASE           0x40024000UL
#define NRF_TEMP_BASE           0x4000C000UL
#define NRF_RNG_BASE            0x4000D000UL
#define NRF_ECB_BASE            0x4000E000UL
#define NRF_CCM_BASE            0x4000F000UL
#define NRF_AAR_BASE            0x4000F000UL  /* Shared with CCM */
#define NRF_QSPI_BASE           0x4002B000UL
#define NRF_NFCT_BASE           0x4001A000UL
#define NRF_USBD_BASE           0x40027000UL
#define NRF_UICR_BASE           0x10001200UL
#define NRF_FICR_BASE           0x10000000UL
#define NRF_P0_BASE             0x50000000UL
#define NRF_P1_BASE             0x50000300UL
#define NRF_WDT_BASE            0x40010000UL
#define NRF_PDM_BASE            0x4001D000UL
#define NRF_I2S_BASE            0x40025000UL
#define NRF_FPU_BASE            0xE000EF30UL

/*===========================================================================
 * GPIO REGISTER STRUCTURES
 *===========================================================================*/

typedef struct {
    volatile uint32_t OUT;          /* 0x504 — Output */
    volatile uint32_t OUTSET;       /* 0x508 — Output set */
    volatile uint32_t OUTCLR;       /* 0x50C — Output clear */
    volatile uint32_t IN;           /* 0x510 — Input */
    volatile uint32_t DIR;          /* 0x514 — Direction */
    volatile uint32_t LATCH;        /* 0x518 — Latch */
    volatile uint32_t DETECTMODE;   /* 0x51C — Detect mode */
    volatile uint32_t RESERVED[3];
    volatile uint32_t PIN_CNF[32];  /* 0x530 — Pin configuration */
} nrf_gpio_port_t;

#define NRF_P0   ((nrf_gpio_port_t *)NRF_P0_BASE)
#define NRF_P1   ((nrf_gpio_port_t *)NRF_P1_BASE)

/* PIN_CNF register bit fields */
#define GPIO_CNF_DIR_INPUT       (0UL << 0)
#define GPIO_CNF_DIR_OUTPUT      (1UL << 0)
#define GPIO_CNF_INPUT_CONNECT   (0UL << 1)
#define GPIO_CNF_INPUT_DISCONNECT (1UL << 1)
#define GPIO_CNF_PULL_NONE       (0UL << 2)
#define GPIO_CNF_PULL_PULLDOWN   (1UL << 2)
#define GPIO_CNF_PULL_PULLUP     (3UL << 2)
#define GPIO_CNF_DRIVE_S0S1      (0UL << 8)  /* Standard 0, Standard 1 */
#define GPIO_CNF_DRIVE_H0S1      (1UL << 8)  /* High drive 0, Standard 1 */
#define GPIO_CNF_DRIVE_S0H1      (2UL << 8)  /* Standard 0, High drive 1 */
#define GPIO_CNF_DRIVE_H0H1      (3UL << 8)  /* High drive 0, High drive 1 */
#define GPIO_CNF_DRIVE_D0S1      (4UL << 8)  /* Disconnect 0, Standard 1 */
#define GPIO_CNF_DRIVE_D0H1      (5UL << 8)  /* Disconnect 0, High drive 1 */
#define GPIO_CNF_DRIVE_S0D1      (6UL << 8)  /* Standard 0, Disconnect 1 */
#define GPIO_CNF_DRIVE_H0D1      (7UL << 8)  /* High drive 0, Disconnect 1 */
#define GPIO_CNF_SENSE_DISABLED  (0UL << 16)
#define GPIO_CNF_SENSE_HIGH      (2UL << 16)
#define GPIO_CNF_SENSE_LOW       (3UL << 16)

/*===========================================================================
 * GPIOTE REGISTERS
 *===========================================================================*/

#define GPIOTE_CONFIG_MODE_DISABLED  (0UL << 0)
#define GPIOTE_CONFIG_MODE_EVENT     (1UL << 0)
#define GPIOTE_CONFIG_MODE_TASK      (3UL << 0)
#define GPIOTE_CONFIG_POLARITY_NONE  (0UL << 4)  /* N/A for Disabled */
#define GPIOTE_CONFIG_POLARITY_LOTOHI (1UL << 4)
#define GPIOTE_CONFIG_POLARITY_HITOLO (2UL << 4)
#define GPIOTE_CONFIG_POLARITY_TOGGLE (3UL << 4)
#define GPIOTE_CONFIG_PORT           (0UL << 8)  /* Port 0 */
#define GPIOTE_CONFIG_PORT1          (1UL << 8)  /* Port 1 */
#define GPIOTE_CONFIG_PSEL_POS       8
#define GPIOTE_CONFIG_PSEL_MASK      (0x1FUL << 8)

#define GPIOTE_INTENSET_IN0          (1UL << 0)
#define GPIOTE_INTENSET_IN1          (1UL << 1)
#define GPIOTE_INTENSET_IN2          (1UL << 2)
#define GPIOTE_INTENSET_IN3          (1UL << 3)
#define GPIOTE_INTENCLR_IN0          (1UL << 0)

#define GPIOTE_EVENTS_IN0            (*(volatile uint32_t *)(NRF_GPIOTE_BASE + 0x100))
#define GPIOTE_EVENTS_IN1            (*(volatile uint32_t *)(NRF_GPIOTE_BASE + 0x104))
#define GPIOTE_EVENTS_IN2            (*(volatile uint32_t *)(NRF_GPIOTE_BASE + 0x108))
#define GPIOTE_EVENTS_IN3            (*(volatile uint32_t *)(NRF_GPIOTE_BASE + 0x10C))
#define GPIOTE_EVENTS_PORT           (*(volatile uint32_t *)(NRF_GPIOTE_BASE + 0x178))

#define GPIOTE_CONFIG(ch)           (*(volatile uint32_t *)(NRF_GPIOTE_BASE + 0x510 + (ch) * 4))
#define GPIOTE_INTENSET             (*(volatile uint32_t *)(NRF_GPIOTE_BASE + 0x304))
#define GPIOTE_INTENCLR             (*(volatile uint32_t *)(NRF_GPIOTE_BASE + 0x308))

/*===========================================================================
 * SPIM REGISTERS
 *===========================================================================*/

typedef struct {
    volatile uint32_t TASKS_START;     /* 0x000 — Start SPI transaction */
    volatile uint32_t TASKS_STOP;      /* 0x004 — Stop SPI transaction */
    volatile uint32_t TASKS_SUSPEND;   /* 0x008 — Suspend SPI transaction */
    volatile uint32_t TASKS_RESUME;    /* 0x00C — Resume SPI transaction */
    volatile uint32_t RESERVED[5];
    volatile uint32_t EVENTS_STOPPED;  /* 0x028 — SPI stopped */
    volatile uint32_t EVENTS_ENDRX;    /* 0x02C — End of RX buffer */
    volatile uint32_t EVENTS_END;      /* 0x030 — End of transaction */
    volatile uint32_t EVENTS_ENDTX;    /* 0x034 — End of TX buffer */
    volatile uint32_t RESERVED2[2];
    volatile uint32_t EVENTS_STARTED;  /* 0x03C — Transaction started */
    volatile uint32_t RESERVED3[44];
    volatile uint32_t SHORTS;          /* 0x100 — Shortcut register */
    volatile uint32_t RESERVED4[1];
    volatile uint32_t INTENSET;        /* 0x104 — Interrupt enable set */
    volatile uint32_t INTENCLR;        /* 0x108 — Interrupt enable clear */
    volatile uint32_t RESERVED5[2];
    volatile uint32_t EVENTS_ENDRX_2;  /* 0x114 */
    volatile uint32_t RESERVED6[6];
    volatile uint32_t ENABLE;          /* 0x120 — Enable SPIM */
    volatile uint32_t RESERVED7[1];
    volatile uint32_t PSEL_SCK;        /* 0x128 — Pin select SCK */
    volatile uint32_t PSEL_MOSI;       /* 0x12C — Pin select MOSI */
    volatile uint32_t PSEL_MISO;       /* 0x130 — Pin select MISO */
    volatile uint32_t RESERVED8[3];
    volatile uint32_t FREQUENCY;       /* 0x138 — SPI frequency */
    volatile uint32_t CONFIG;          /* 0x13C — SPI config */
    volatile uint32_t RESERVED9[1];
    volatile uint32_t RXD_PTR;         /* 0x144 — RXD pointer */
    volatile uint32_t RXD_MAXCNT;      /* 0x148 — RXD max count */
    volatile uint32_t RXD_AMOUNT;      /* 0x14C — RXD amount */
    volatile uint32_t RXD_LIST;        /* 0x150 — RXD list */
    volatile uint32_t TXD_PTR;         /* 0x154 — TXD pointer */
    volatile uint32_t TXD_MAXCNT;      /* 0x158 — TXD max count */
    volatile uint32_t TXD_AMOUNT;      /* 0x15C — TXD amount */
    volatile uint32_t TXD_LIST;        /* 0x160 — TXD list */
    volatile uint32_t CONFIG_ORDER;    /* 0x164 — Config order (not real, placeholder) */
    volatile uint32_t ORC;             /* 0x16C — Over-read character */
} nrf_spim_t;

#define NRF_SPIM0  ((nrf_spim_t *)NRF_SPIM0_BASE)
#define NRF_SPIM1  ((nrf_spim_t *)NRF_SPIM1_BASE)

#define SPIM_ENABLE_ENABLE        (7UL << 0)   /* Enable SPIM */
#define SPIM_ENABLE_DISABLE       (0UL << 0)
#define SPIM_CONFIG_ORDER_MSB     (0UL << 0)
#define SPIM_CONFIG_ORDER_LSB     (1UL << 0)
#define SPIM_CONFIG_CPOL_ACTIVEHIGH (0UL << 2)
#define SPIM_CONFIG_CPOL_ACTIVELOW  (1UL << 2)
#define SPIM_CONFIG_CPHA_LEADING    (0UL << 3)
#define SPIM_CONFIG_CPHA_TRAILING   (1UL << 3)

/* SPIM frequency values */
#define SPIM_FREQ_125K    0x02000000UL
#define SPIM_FREQ_250K    0x04000000UL
#define SPIM_FREQ_500K    0x08000000UL
#define SPIM_FREQ_1M      0x10000000UL
#define SPIM_FREQ_2M      0x20000000UL
#define SPIM_FREQ_4M      0x40000000UL
#define SPIM_FREQ_8M      0x80000000UL
#define SPIM_FREQ_16M     0x0A000000UL
#define SPIM_FREQ_32M     0x0B000000UL

#define SPIM_INT_END      (1UL << 4)   /* EVENTS_END interrupt */
#define SPIM_INT_ENDRX    (1UL << 2)   /* EVENTS_ENDRX interrupt */

/*===========================================================================
 * QSPI REGISTERS
 *===========================================================================*/

typedef struct {
    volatile uint32_t TASKS_ACTIVATE;   /* 0x000 — Activate QSPI */
    volatile uint32_t RESERVED[4];
    volatile uint32_t TASKS_ERASE;      /* 0x014 — Erase block */
    volatile uint32_t TASKS_ERASE_ALL;  /* 0x018 — Erase all */
    volatile uint32_t RESERVED2[2];
    volatile uint32_t EVENTS_READY;     /* 0x020 — QSPI ready */
    volatile uint32_t RESERVED3[7];
    volatile uint32_t INTENSET;         /* 0x038 — Interrupt enable set */
    volatile uint32_t INTENCLR;         /* 0x03C — Interrupt enable clear */
    volatile uint32_t RESERVED4[4];
    volatile uint32_t ENABLE;           /* 0x050 — Enable QSPI */
    volatile uint32_t READ_SRC;         /* 0x054 — Read source */
    volatile uint32_t STATUS;           /* 0x058 — Status */
    volatile uint32_t RESERVED5[2];
    volatile uint32_t CONFIG_STATUS;    /* 0x064 — Config status */
    volatile uint32_t RESERVED6[4];
    volatile uint32_t IFCONFIG0;        /* 0x078 — IFCONFIG0 (CS0) */
    volatile uint32_t IFCONFIG1;        /* 0x07C — IFCONFIG1 (CS1) */
    volatile uint32_t RESERVED7[2];
    volatile uint32_t ADDRCONF;         /* 0x088 — Address config */
    volatile uint32_t RESERVED8[1];
    volatile uint32_t ERASEALL_LEN;     /* 0x090 — Erase all length */
    volatile uint32_t RESERVED9[4];
    volatile uint32_t ERASE_PTR;        /* 0x0A4 — Erase pointer */
    volatile uint32_t ERASE_LEN;        /* 0x0A8 — Erase length */
    volatile uint32_t RESERVED10[2];
    volatile uint32_t WRITE_SRC;        /* 0x0B4 — Write source */
    volatile uint32_t WRITE_DST;        /* 0x0B8 — Write destination */
    volatile uint32_t WRITE_CNT;        /* 0x0BC — Write count */
    volatile uint32_t RESERVED11[3];
    volatile uint32_t READ_DST;         /* 0x0CC — Read destination */
    volatile uint32_t READ_CNT;         /* 0x0D0 — Read count */
    volatile uint32_t RESERVED12[4];
    volatile uint32_t CINSTRDAT;        /* 0x0E0 — Custom instruction data */
    volatile uint32_t CINSTRCONF;       /* 0x0E4 — Custom instruction config */
    volatile uint32_t RESERVED13[6];
    volatile uint32_t PSEL_SCK;         /* 0x100 — Pin select SCK */
    volatile uint32_t PSEL_CSN0;        /* 0x104 — Pin select CSN0 */
    volatile uint32_t PSEL_CSN1;        /* 0x108 — Pin select CSN1 */
    volatile uint32_t PSEL_IO0;         /* 0x10C — Pin select IO0 */
    volatile uint32_t PSEL_IO1;         /* 0x110 — Pin select IO1 */
    volatile uint32_t PSEL_IO2;         /* 0x114 — Pin select IO2 */
    volatile uint32_t PSEL_IO3;         /* 0x118 — Pin select IO3 */
} nrf_qspi_t;

#define NRF_QSPI  ((nrf_qspi_t *)NRF_QSPI_BASE)

#define QSPI_ENABLE_ENABLE       (1UL << 0)
#define QSPI_ENABLE_DISABLE      (0UL << 0)
#define QSPI_INT_READY           (1UL << 0)

/* IFCONFIG register fields */
#define QSPI_IFCONFIG_READOC(op)       ((op) << 0)
#define QSPI_IFCONFIG_WRITEOC(op)      ((op) << 4)
#define QSPI_IFCONFIG_ADDRMODE(mode)   ((mode) << 8)
#define QSPI_IFCONFIG_DUMMYCNT(cnt)    ((cnt) << 10)
#define QSPI_IFCONFIG_CSNDURATION(dur) ((dur) << 16)
#define QSPI_IFCONFIG_SCKDELAY(dly)    ((dly) << 22)

/* Read opcodes */
#define QSPI_READOC_FAST_READ    0x01
#define QSPI_READOC_READ2O       0x02
#define QSPI_READOC_READ4O       0x06
#define QSPI_READOC_READ2IO      0x0A
#define QSPI_READOC_READ4IO      0x0E

/* Write opcodes */
#define QSPI_WRITEOC_PP          0x00
#define QSPI_WRITEOC_PP2O       0x01
#define QSPI_WRITEOC_PP4O       0x04
#define QSPI_WRITEOC_PP2IO      0x07
#define QSPI_WRITEOC_PP4IO      0x0B

#define QSPI_CINSTRCONF_OPCODE(op)    ((op) << 0)
#define QSPI_CINSTRCONF_LENGTH(len)   ((len) << 8)
#define QSPI_CINSTRCONF_LIO2(li02)    ((li02) << 12)
#define QSPI_CINSTRCONF_LIO3(li03)    ((li03) << 14)
#define QSPI_CINSTRCONF_WIPWAIT       (1UL << 16)
#define QSPI_CINSTRCONF_WREN          (1UL << 17)

/*===========================================================================
 * TIMER REGISTERS
 *===========================================================================*/

typedef struct {
    volatile uint32_t TASKS_START;      /* 0x000 */
    volatile uint32_t TASKS_STOP;       /* 0x004 */
    volatile uint32_t TASKS_COUNT;      /* 0x008 */
    volatile uint32_t TASKS_CLEAR;      /* 0x00C */
    volatile uint32_t TASKS_SHUTDOWN;   /* 0x010 */
    volatile uint32_t RESERVED[11];
    volatile uint32_t EVENTS_COMPARE[6]; /* 0x040-0x054 */
    volatile uint32_t RESERVED2[10];
    volatile uint32_t SHORTS;           /* 0x080 */
    volatile uint32_t INTENSET;         /* 0x084 */
    volatile uint32_t INTENCLR;         /* 0x088 */
    volatile uint32_t RESERVED3[4];
    volatile uint32_t MODE;             /* 0x094 */
    volatile uint32_t BITMODE;          /* 0x098 */
    volatile uint32_t PRESCALER;        /* 0x09C */
    volatile uint32_t CC[6];            /* 0x0A0-0x0B4 */
} nrf_timer_t;

#define NRF_TIMER0 ((nrf_timer_t *)NRF_TIMER0_BASE)
#define NRF_TIMER1 ((nrf_timer_t *)NRF_TIMER1_BASE)
#define NRF_TIMER2 ((nrf_timer_t *)NRF_TIMER2_BASE)

#define TIMER_MODE_TIMER          (0UL << 0)
#define TIMER_MODE_COUNTER        (1UL << 0)
#define TIMER_MODE_LOWPOWER       (2UL << 0)

#define TIMER_BITMODE_08BIT       (0UL << 0)
#define TIMER_BITMODE_16BIT       (1UL << 0)
#define TIMER_BITMODE_24BIT       (2UL << 0)
#define TIMER_BITMODE_32BIT       (3UL << 0)

#define TIMER_PRESCALER_DIV1      0UL   /* 16 MHz / 1 = 16 MHz */
#define TIMER_PRESCALER_DIV2      1UL   /* 8 MHz */
#define TIMER_PRESCALER_DIV4      2UL   /* 4 MHz */
#define TIMER_PRESCALER_DIV8      3UL   /* 2 MHz */
#define TIMER_PRESCALER_DIV16     4UL   /* 1 MHz (used for µs timestamps) */

#define TIMER_INT_COMPARE0        (1UL << 0)
#define TIMER_INT_COMPARE1        (1UL << 1)
#define TIMER_INT_COMPARE2        (1UL << 2)
#define TIMER_INT_COMPARE3        (1UL << 3)

#define TIMER_SHORTS_COMPARE0_CLEAR  (1UL << 0)
#define TIMER_SHORTS_COMPARE1_CLEAR  (1UL << 1)
#define TIMER_SHORTS_COMPARE2_CLEAR  (1UL << 2)

/*===========================================================================
 * RTC REGISTERS
 *===========================================================================*/

typedef struct {
    volatile uint32_t TASKS_START;      /* 0x000 */
    volatile uint32_t TASKS_STOP;       /* 0x004 */
    volatile uint32_t TASKS_CLEAR;      /* 0x008 */
    volatile uint32_t TASKS_TRIGOVRFLW; /* 0x00C */
    volatile uint32_t RESERVED[3];
    volatile uint32_t EVENTS_TICK;      /* 0x020 */
    volatile uint32_t EVENTS_OVRFLW;    /* 0x024 */
    volatile uint32_t RESERVED2[2];
    volatile uint32_t EVENTS_COMPARE[4]; /* 0x028-0x034 */
    volatile uint32_t RESERVED3[11];
    volatile uint32_t INTENSET;         /* 0x060 */
    volatile uint32_t INTENCLR;         /* 0x064 */
    volatile uint32_t RESERVED4[3];
    volatile uint32_t EVTEN;            /* 0x074 */
    volatile uint32_t EVTENSET;         /* 0x078 */
    volatile uint32_t EVTENCLR;         /* 0x07C */
    volatile uint32_t RESERVED5[3];
    volatile uint32_t PRESCALER;        /* 0x088 */
    volatile uint32_t CC[4];            /* 0x08C-0x098 */
} nrf_rtc_t;

#define NRF_RTC0 ((nrf_rtc_t *)NRF_RTC0_BASE)
#define NRF_RTC1 ((nrf_rtc_t *)NRF_RTC1_BASE)
#define NRF_RTC2 ((nrf_rtc_t *)NRF_RTC2_BASE)

#define RTC_PRESCALER_DIV1        0UL   /* 32768 Hz */
#define RTC_PRESCALER_DIV32       4UL   /* 1024 Hz (~1ms tick) */
#define RTC_PRESCALER_DIV32768    12UL  /* 1 Hz */

#define RTC_INT_TICK              (1UL << 0)
#define RTC_INT_OVRFLW            (1UL << 1)
#define RTC_INT_COMPARE0          (1UL << 16)
#define RTC_INT_COMPARE1          (1UL << 17)

/*===========================================================================
 * SAADC REGISTERS (Battery monitoring)
 *===========================================================================*/

typedef struct {
    volatile uint32_t TASKS_START;      /* 0x000 */
    volatile uint32_t TASKS_SAMPLE;     /* 0x004 */
    volatile uint32_t TASKS_STOP;       /* 0x008 */
    volatile uint32_t TASKS_CALIBRATE;  /* 0x00C */
    volatile uint32_t RESERVED[4];
    volatile uint32_t EVENTS_STARTED;   /* 0x020 */
    volatile uint32_t EVENTS_END;       /* 0x024 */
    volatile uint32_t EVENTS_DONE;      /* 0x028 */
    volatile uint32_t EVENTS_RESULTDONE;/* 0x030 */
    volatile uint32_t EVENTS_CALIBRATEDONE; /* 0x038 */
    volatile uint32_t EVENTS_STOPPED;   /* 0x03C */
    volatile uint32_t RESERVED2[6];
    volatile uint32_t INTENSET;         /* 0x074 */
    volatile uint32_t INTENCLR;         /* 0x078 */
    volatile uint32_t RESERVED3[7];
    volatile uint32_t STATUS;           /* 0x094 */
    volatile uint32_t ENABLE;           /* 0x098 */
    volatile uint32_t RESERVED4[1];
    volatile uint32_t PSEL_P;           /* 0x0A0 — Positive input */
    volatile uint32_t PSEL_N;           /* 0x0A4 — Negative input */
    volatile uint32_t RESERVED5[3];
    volatile uint32_t CONFIG_RESN;      /* 0x0B0 */
    volatile uint32_t CONFIG_RESP;      /* 0x0B4 */
    volatile uint32_t CONFIG_GAIN;      /* 0x0B8 */
    volatile uint32_t CONFIG_REFSEL;    /* 0x0BC */
    volatile uint32_t CONFIG_TACQ;      /* 0x0C0 */
    volatile uint32_t CONFIG_MODE;      /* 0x0C4 */
    volatile uint32_t CONFIG_BURST;     /* 0x0C8 */
    volatile uint32_t RESERVED6[8];
    volatile uint32_t LIMIT_INT;        /* 0x0EC */
    volatile uint32_t RESOULT;          /* 0x0F0 — Result pointer */
    volatile uint32_t RESULT_MAXCNT;    /* 0x0F4 */
    volatile uint32_t RESULT_AMOUNT;    /* 0x0F8 */
} nrf_saadc_t;

#define NRF_SAADC ((nrf_saadc_t *)NRF_SAADC_BASE)

#define SAADC_ENABLE_ENABLE      (1UL << 0)
#define SAADC_ENABLE_DISABLE     (0UL << 0)
#define SAADC_INT_END            (1UL << 3)
#define SAADC_INT_DONE           (1UL << 2)

/* SAADC channel config (per-channel registers) */
#define SAADC_CH_PSELP_OFF       (0UL << 0)
#define SAADC_CH_PSELP_ANALOG0   (1UL << 0)
#define SAADC_CH_PSELP_ANALOG2   (3UL << 0)  /* AIN2 = P0.04 (VBAT sense) */

#define SAADC_CH_CONFIG_GAIN_GAIN1_6  (0UL << 8)  /* Gain 1/6 for 3.6V range */
#define SAADC_CH_CONFIG_REFSEL_VDD_1_4 (0UL << 12) /* VDD/4 reference */
#define SAADC_CH_CONFIG_TACQ_3US   (0UL << 16)
#define SAADC_CH_CONFIG_TACQ_10US  (2UL << 16)
#define SAADC_CH_CONFIG_MODE_SE    (0UL << 20)  /* Single-ended */
#define SAADC_CH_CONFIG_MODE_DIFF  (1UL << 20)
#define SAADC_CH_CONFIG_BURST_DIS  (0UL << 24)
#define SAADC_CH_CONFIG_BURST_EN   (1UL << 24)

/*===========================================================================
 * CLOCK REGISTERS
 *===========================================================================*/

typedef struct {
    volatile uint32_t TASKS_HFCLKSTART;     /* 0x000 */
    volatile uint32_t TASKS_HFCLKSTOP;      /* 0x004 */
    volatile uint32_t TASKS_LFCLKSTART;     /* 0x008 */
    volatile uint32_t TASKS_LFCLKSTOP;      /* 0x00C */
    volatile uint32_t TASKS_CALIBRATE;      /* 0x010 */
    volatile uint32_t TASKS_CALIBRATESTOP;  /* 0x014 */
    volatile uint32_t RESERVED[2];
    volatile uint32_t EVENTS_HFCLKSTARTED;  /* 0x020 */
    volatile uint32_t EVENTS_LFCLKSTARTED;  /* 0x024 */
    volatile uint32_t RESERVED2[2];
    volatile uint32_t EVENTS_DONE;          /* 0x030 */
    volatile uint32_t EVENTS_CTTO;          /* 0x034 */
    volatile uint32_t RESERVED3[6];
    volatile uint32_t INTENSET;             /* 0x04C */
    volatile uint32_t INTENCLR;             /* 0x050 */
    volatile uint32_t RESERVED4[2];
    volatile uint32_t HFCLKRUN;             /* 0x05C */
    volatile uint32_t HFCLKSTAT;            /* 0x060 */
    volatile uint32_t RESERVED5[1];
    volatile uint32_t LFCLKRUN;             /* 0x064 */
    volatile uint32_t LFCLKSRCCOPY;         /* 0x068 */
    volatile uint32_t LFCLKSTAT;            /* 0x06C */
    volatile uint32_t RESERVED6[2];
    volatile uint32_t LFCLKSRC;             /* 0x074 */
    volatile uint32_t CTIV;                 /* 0x078 */
    volatile uint32_t TRACECONFIG;          /* 0x07C */
} nrf_clock_t;

#define NRF_CLOCK ((nrf_clock_t *)NRF_CLOCK_BASE)

#define CLOCK_LFCLKSRC_RC        (0UL << 0)
#define CLOCK_LFCLKSRC_XTAL      (1UL << 0)  /* 32.768 kHz crystal */
#define CLOCK_LFCLKSRC_SYNTH     (2UL << 0)

#define CLOCK_HFCLKSTAT_STATE_RUNNING (1UL << 0)
#define CLOCK_HFCLKSTAT_SRC_XO   (1UL << 1)  /* External oscillator */

/*===========================================================================
 * POWER / POWER MANAGEMENT REGISTERS
 *===========================================================================*/

typedef struct {
    volatile uint32_t RESERVED[2];
    volatile uint32_t EVENTS_POFWARN;        /* 0x008 — Power failure warning */
    volatile uint32_t RESERVED2[2];
    volatile uint32_t EVENTS_SLEEPENTER;     /* 0x010 */
    volatile uint32_t EVENTS_SLEEPEXIT;      /* 0x014 */
    volatile uint32_t RESERVED3[17];
    volatile uint32_t INTENSET;              /* 0x030 */
    volatile uint32_t INTENCLR;              /* 0x034 */
    volatile uint32_t RESERVED4[1];
    volatile uint32_t RESETREAS;             /* 0x040 — Reset reason */
    volatile uint32_t RESERVED5[1];
    volatile uint32_t RAMSTATUS;             /* 0x044 */
    volatile uint32_t RESERVED6[3];
    volatile uint32_t SYSTEMOFF;             /* 0x050 — Enter system OFF */
    volatile uint32_t RESERVED7[2];
    volatile uint32_t POFCON;                /* 0x058 — Power failure config */
    volatile uint32_t RESERVED8[1];
    volatile uint32_t GPREGRET;              /* 0x05C — General purpose retention */
    volatile uint32_t GPREGRET2;             /* 0x060 */
    volatile uint32_t RAMON;                 /* 0x064 — (deprecated) */
    volatile uint32_t RESERVED9[3];
    volatile uint32_t DCDCEN;                /* 0x074 — DC/DC converter enable */
} nrf_power_t;

#define NRF_POWER ((nrf_power_t *)NRF_POWER_BASE)

#define POWER_DCDCEN_ENABLE      (1UL << 0)
#define POWER_POFCON_POF_ENABLE  (1UL << 1)
#define POWER_POFCON_THR_2V1     (1UL << 1)  /* Power-off threshold */
#define POWER_POFCON_THR_2V3     (3UL << 1)
#define POWER_SYSTEMOFF_ENTER    (1UL << 0)

/*===========================================================================
 * FICR REGISTERS (Device ID)
 *===========================================================================*/

#define FICR_DEVICEID_0    (*(volatile uint32_t *)(NRF_FICR_BASE + 0x060))
#define FICR_DEVICEID_1    (*(volatile uint32_t *)(NRF_FICR_BASE + 0x064))
#define FICR_DEVICEADDR_0  (*(volatile uint32_t *)(NRF_FICR_BASE + 0x0A4))
#define FICR_DEVICEADDR_1  (*(volatile uint32_t *)(NRF_FICR_BASE + 0x0A8))

/*===========================================================================
 * WDT REGISTERS
 *===========================================================================*/

typedef struct {
    volatile uint32_t TASKS_START;        /* 0x000 */
    volatile uint32_t RESERVED[63];
    volatile uint32_t EVENTS_TIMEOUT;     /* 0x100 */
    volatile uint32_t RESERVED2[52];
    volatile uint32_t INTENSET;           /* 0x150 */
    volatile uint32_t INTENCLR;           /* 0x154 */
    volatile uint32_t RESERVED3[1];
    volatile uint32_t REQSTATUS;          /* 0x158 */
    volatile uint32_t RESERVED4[3];
    volatile uint32_t CRV;                /* 0x164 — Counter reload value */
    volatile uint32_t RREN;               /* 0x168 — Reload enable */
    volatile uint32_t CONFIG;             /* 0x16C — Configuration */
    volatile uint32_t RESERVED5[3];
    volatile uint32_t RR[8];              /* 0x170-0x18C — Reload registers */
} nrf_wdt_t;

#define NRF_WDT ((nrf_wdt_t *)NRF_WDT_BASE)

#define WDT_CONFIG_HALT_RUN     (0UL << 0)  /* Run while CPU halted */
#define WDT_CONFIG_SLEEP_RUN    (0UL << 1)  /* Run while CPU sleeping */
#define WDT_RREN_RR0            (1UL << 0)
#define WDT_RR_VALUE            0x6E524635UL /* Magic reload value */

/*===========================================================================
 * CCM (AES-GCM) REGISTERS
 *===========================================================================*/

typedef struct {
    volatile uint32_t TASKS_KSGEN;        /* 0x000 — Key stream generation */
    volatile uint32_t TASKS_CRYPT;        /* 0x004 — Crypt operation */
    volatile uint32_t TASKS_STOP;         /* 0x008 — Stop */
    volatile uint32_t TASKS_RATEOVERRIDE; /* 0x00C */
    volatile uint32_t RESERVED[4];
    volatile uint32_t EVENTS_ENDKSGEN;    /* 0x020 */
    volatile uint32_t EVENTS_ENDCRYPT;    /* 0x024 */
    volatile uint32_t EVENTS_ERROR;       /* 0x028 */
    volatile uint32_t RESERVED2[2];
    volatile uint32_t INTENSET;           /* 0x034 */
    volatile uint32_t INTENCLR;           /* 0x038 */
    volatile uint32_t RESERVED3[3];
    volatile uint32_t MICSTATUS;          /* 0x044 */
    volatile uint32_t RESERVED4[3];
    volatile uint32_t ENABLE;             /* 0x050 */
    volatile uint32_t RESERVED5[1];
    volatile uint32_t MODE;               /* 0x054 */
    volatile uint32_t CNFPTR;             /* 0x058 — CCM configuration pointer */
    volatile uint32_t INPTR;              /* 0x05C — Input pointer */
    volatile uint32_t OUTPTR;             /* 0x060 — Output pointer */
    volatile uint32_t RESERVED6[2];
    volatile uint32_t MAXPACKETSIZE;      /* 0x06C */
    volatile uint32_t SCRATCHPTR;         /* 0x070 */
} nrf_ccm_t;

#define NRF_CCM ((nrf_ccm_t *)NRF_CCM_BASE)

#define CCM_ENABLE_ENABLE        (9UL << 0)   /* Magic enable value */
#define CCM_ENABLE_DISABLE       (0UL << 0)
#define CCM_MODE_ENCRYPTION      (0UL << 0)   /* Encryption mode */
#define CCM_MODE_DECRYPTION      (1UL << 0)
#define CCM_MODE_LENGTH_FULL     (0UL << 24)  /* Full-length MIC */
#define CCM_INT_ENDKSGEN         (1UL << 0)
#define CCM_INT_ENDCRYPT         (1UL << 1)
#define CCM_INT_ERROR            (1UL << 2)

/*===========================================================================
 * ECB (AES) REGISTERS (used for key derivation)
 *===========================================================================*/

typedef struct {
    volatile uint32_t TASKS_STARTECB;     /* 0x000 */
    volatile uint32_t TASKS_STOPECB;      /* 0x004 */
    volatile uint32_t RESERVED[6];
    volatile uint32_t EVENTS_ENDECB;      /* 0x020 */
    volatile uint32_t EVENTS_ERRORECB;    /* 0x024 */
    volatile uint32_t RESERVED2[4];
    volatile uint32_t INTENSET;           /* 0x034 */
    volatile uint32_t INTENCLR;           /* 0x038 */
    volatile uint32_t RESERVED3[3];
    volatile uint32_t ECBDATAPTR;         /* 0x04C — ECB data pointer */
} nrf_ecb_t;

#define NRF_ECB ((nrf_ecb_t *)NRF_ECB_BASE)

/*===========================================================================
 * USBD (USB Device) REGISTERS
 *===========================================================================*/

#define USBD_TASKS_STARTEPIN(ep)   (*(volatile uint32_t *)(NRF_USBD_BASE + 0x000 + (ep) * 0x4 + 0x100))
#define USBD_TASKS_STARTEPOUT(ep)  (*(volatile uint32_t *)(NRF_USBD_BASE + 0x000 + (ep) * 0x4 + 0x200))
#define USBD_EVENTS_READY          (*(volatile uint32_t *)(NRF_USBD_BASE + 0x040))
#define USBD_EVENTS_ENDISOIN       (*(volatile uint32_t *)(NRF_USBD_BASE + 0x080))
#define USBD_ENABLE                (*(volatile uint32_t *)(NRF_USBD_BASE + 0x100))
#define USBD_USBPULLUP             (*(volatile uint32_t *)(NRF_USBD_BASE + 0x104))

#define USBD_ENABLE_ENABLE         (1UL << 0)

/*===========================================================================
 * iCE40 FPGA CONFIGURATION COMMANDS (SPI)
 *===========================================================================*/

/* iCE40 configuration SPI commands (standard, not vendor-specific) */
#define ICE40_CMD_NONE              0x00    /* No command, just clocking */
#define ICE40_CONFIG_CLK_DIV        8       /* SPI clock divider for config */

/* FPGA status registers (via SPI) */
#define FPGA_STATUS_CAPTURE_ACTIVE  0x01    /* Capture in progress */
#define FPGA_STATUS_FRAME_READY     0x02    /* Full frame available */
#define FPGA_STATUS_LINE_READY      0x04    /* Line data available */
#define FPGA_STATUS_SYNC_LOCKED     0x08    /* Display sync detected */
#define FPGA_STATUS_OVERFLOW        0x10    /* Buffer overflow occurred */
#define FPGA_STATUS_PROTOCOL_ERROR  0x20    /* Protocol decode error */
#define FPGA_STATUS_POWER_DOWN      0x40    /* FPGA in power-down */

/* FPGA control register (via SPI command 0x40) */
#define FPGA_CTRL_RESET_CAPTURE     0x01    /* Reset capture state machine */
#define FPGA_CTRL_ENABLE_CAPTURE    0x02    /* Enable pixel capture */
#define FPGA_CTRL_DISABLE_CAPTURE   0x04    /* Disable pixel capture */
#define FPGA_CTRL_CLEAR_OVERFLOW    0x08    /* Clear overflow flag */
#define FPGA_CTRL_PASS_THROUGH      0x10    /* Pass-through mode (no capture) */

/* FPGA protocol select (via SPI command 0x41) */
#define FPGA_PROTO_SEL_MIPI_DSI     0x01
#define FPGA_PROTO_SEL_RGB_PARA     0x02
#define FPGA_PROTO_SEL_SPI_LCD      0x03
#define FPGA_PROTO_SEL_LVDS         0x04

/*===========================================================================
 * ESP32-S3 DSP COMMAND INTERFACE (via SPI)
 *===========================================================================*/

#define DSP_CMD_NOP                 0x00    /* No operation */
#define DSP_CMD_RESET               0x01    /* Reset DSP subsystem */
#define DSP_CMD_SET_MODE            0x02    /* Set processing mode */
#define DSP_CMD_SET_RESOLUTION      0x03    /* Set frame resolution */
#define DSP_CMD_SET_COLOR_DEPTH     0x04    /* Set color depth */
#define DSP_CMD_START_COMPRESSION   0x05    /* Begin delta compression */
#define DSP_CMD_START_OCR           0x06    /* Begin OCR extraction */
#define DSP_CMD_START_PATTERN       0x07    /* Begin pattern matching */
#define DSP_CMD_GET_STATUS          0x08    /* Get DSP status */
#define DSP_CMD_GET_COMPRESSED      0x09    /* Read compressed frame data */
#define DSP_CMD_GET_OCR_TEXT        0x0A    /* Read OCR text results */
#define DSP_CMD_GET_PATTERNS        0x0B    /* Read pattern match results */
#define DSP_CMD_SET_OCR_MODEL       0x0C    /* Load OCR model from flash */
#define DSP_CMD_SET_PATTERNS        0x0D    /* Load pattern definitions */
#define DSP_CMD_SET_THRESHOLD       0x0E    /* Set delta detection threshold */
#define DSP_CMD_ENABLE_QR_DECODE    0x0F    /* Enable QR/barcode decoder */
#define DSP_CMD_SET_POWER_MODE      0x10    /* Set DSP power mode */

/* DSP status flags */
#define DSP_STATUS_IDLE             0x00
#define DSP_STATUS_COMPRESSING      0x01
#define DSP_STATUS_OCR_RUNNING      0x02
#define DSP_STATUS_PATTERN_RUNNING  0x04
#define DSP_STATUS_DATA_READY       0x08    /* Result data available */
#define DSP_STATUS_ERROR            0x80

/* DSP processing modes */
#define DSP_MODE_COMPRESS_ONLY      0x01    /* Delta compression only */
#define DSP_MODE_OCR_ONLY           0x02    /* OCR text extraction only */
#define DSP_MODE_COMPRESS_OCR       0x03    /* Both compression and OCR */
#define DSP_MODE_FULL_PIPELINE      0x04    /* Compress + OCR + pattern + QR */
#define DSP_MODE_LOW_POWER          0x05    /* Minimal processing for power saving */

/* DSP power modes */
#define DSP_POWER_NORMAL            0x00    /* Full speed 240 MHz */
#define DSP_POWER_REDUCED           0x01    /* 160 MHz */
#define DSP_POWER_LOW               0x02    /* 80 MHz */
#define DSP_POWER_SLEEP             0x03    /* Sleep, wake on frame */

/*===========================================================================
 * PROTOCOL HANDLER CONSTANTS
 *===========================================================================*/

/* Command opcodes (app ↔ device) */
#define CMD_START_CAPTURE           0x01
#define CMD_STOP_CAPTURE            0x02
#define CMD_SET_PROTOCOL            0x03
#define CMD_SET_RESOLUTION          0x04
#define CMD_SET_COLOR_DEPTH         0x05
#define CMD_SET_CAPTURE_FPS         0x06
#define CMD_SET_TRIGGER_MODE        0x07
#define CMD_GET_STATUS              0x08
#define CMD_GET_FRAME               0x09
#define CMD_SET_OCR_MODE            0x0A
#define CMD_SET_PATTERN_LIST        0x0B
#define CMD_EXPORT_FRAMES           0x0C
#define CMD_FIRMWARE_UPDATE         0x0D
#define CMD_SET_ENCRYPTION_KEY      0x0E
#define CMD_GET_FRAME_HISTORY       0x0F
#define CMD_SET_POWER_MODE          0x10
#define CMD_ERASE_HISTORY           0x11
#define CMD_GET_DEVICE_INFO         0x12
#define CMD_FACTORY_RESET           0x13

/* Message opcodes (device → app) */
#define MSG_FRAME_DATA              0x81
#define MSG_OCR_TEXT                0x82
#define MSG_CREDENTIAL_ALERT        0x83
#define MSG_STATUS_UPDATE           0x84
#define MSG_ERROR                   0x85
#define MSG_FRAME_HISTORY_ENTRY     0x86
#define MSG_DEVICE_INFO             0x87
#define MSG_CAPTURE_STATS           0x88

/* Error codes */
#define ERR_NONE                    0x00
#define ERR_PROTOCOL_NOT_SUPPORTED  0x01
#define ERR_RESOLUTION_TOO_LARGE    0x02
#define ERR_FPGA_CONFIG_FAILED      0x03
#define ERR_DSP_INIT_FAILED         0x04
#define ERR_PSRAM_ACCESS_FAILED     0x05
#define ERR_FLASH_ACCESS_FAILED     0x06
#define ERR_BLE_NOT_CONNECTED       0x07
#define ERR_USB_NOT_CONNECTED       0x08
#define ERR_ENCRYPTION_FAILED       0x09
#define ERR_BUFFER_OVERFLOW         0x0A
#define ERR_SD_CARD_ERROR           0x0B
#define ERR_LOW_BATTERY             0x0C
#define ERR_OVERHEATING             0x0D
#define ERR_INVALID_PARAMETER       0x0E
#define ERR_FIRMWARE_CRC            0x0F

/*===========================================================================
 * CREDENTIAL PATTERN TYPES
 *===========================================================================*/

typedef enum {
    PATTERN_TYPE_NONE = 0,
    PATTERN_TYPE_API_KEY,        /* Generic API key patterns */
    PATTERN_TYPE_JWT_TOKEN,      /* JWT (eyJ... format) */
    PATTERN_TYPE_PASSWORD_FIELD, /* Text near "password" label */
    PATTERN_TYPE_OTP_CODE,       /* 4-8 digit OTP */
    PATTERN_TYPE_URL,            /* URLs (http/https) */
    PATTERN_TYPE_EMAIL,          /* Email addresses */
    PATTERN_TYPE_QR_CODE,        /* QR code content */
    PATTERN_TYPE_BARCODE,        /* Barcode content */
    PATTERN_TYPE_CREDIT_CARD,    /* Credit card number patterns */
    PATTERN_TYPE_IP_ADDRESS,     /* IP addresses */
    PATTERN_TYPE_MAC_ADDRESS,    /* MAC addresses */
    PATTERN_TYPE_PHONE_NUMBER,   /* Phone numbers */
    PATTERN_TYPE_CUSTOM          /* User-defined regex pattern */
} pattern_type_t;

/* Maximum number of patterns */
#define MAX_PATTERNS               32
#define MAX_PATTERN_TEXT_LENGTH    256
#define MAX_OCR_TEXT_PER_FRAME     1024
#define MAX_CREDENTIAL_ALERTS      16

/*===========================================================================
 * FRAME METADATA STRUCTURE
 *===========================================================================*/

/* Frame metadata stored per captured frame */
typedef struct {
    uint32_t frame_index;           /* Sequential frame number */
    uint32_t timestamp_us;          /* µs-precision timestamp */
    uint16_t width;                 /* Frame width in pixels */
    uint16_t height;                /* Frame height in pixels */
    uint8_t  color_depth;           /* Bits per pixel */
    uint8_t  compression_type;      /* 0=raw, 1=RLE, 2=delta+RLE */
    uint32_t compressed_size;       /* Size of compressed data in bytes */
    uint32_t ocr_text_offset;       /* Offset to OCR text in PSRAM */
    uint16_t ocr_text_length;       /* Length of OCR text */
    uint16_t alert_count;           /* Number of credential alerts */
    uint8_t  flags;                 /* Bit 0: partial, Bit 1: keyframe */
} __attribute__((packed)) frame_metadata_t;

/* Frame metadata flags */
#define FRAME_FLAG_PARTIAL          0x01    /* Partial frame (delta only) */
#define FRAME_FLAG_KEYFRAME         0x02    /* Full frame (I-frame equivalent) */
#define FRAME_FLAG_OCR_AVAILABLE    0x04    /* OCR text was extracted */
#define FRAME_FLAG_ALERTS_PRESENT   0x08    /* Credential alerts detected */

/*===========================================================================
 * RING BUFFER FOR FRAME QUEUE
 *===========================================================================*/

typedef struct {
    uint32_t head;                  /* Write index */
    uint32_t tail;                  /* Read index */
    uint32_t count;                 /* Current item count */
    uint32_t capacity;              /* Maximum items */
    uint32_t item_size;             /* Size of each item in bytes */
    uint8_t *buffer;                /* Pointer to buffer memory */
} ring_buffer_t;

#define RING_BUFFER_INIT(buf, cap, sz) { \
    .head = 0, .tail = 0, .count = 0, \
    .capacity = (cap), .item_size = (sz), \
    .buffer = (uint8_t *)(buf) \
}

#define RING_BUFFER_FULL(rb)      ((rb)->count >= (rb)->capacity)
#define RING_BUFFER_EMPTY(rb)     ((rb)->count == 0)
#define RING_BUFFER_COUNT(rb)     ((rb)->count)

/*===========================================================================
 * ERROR LOG
 *===========================================================================*/

typedef struct {
    uint32_t error_code;
    uint32_t timestamp;
    uint32_t context;
} error_entry_t;

#define ERROR_LOG_MAX_ENTRIES     32

typedef struct {
    uint32_t magic;                /* Magic number to validate log */
    uint32_t entry_count;
    error_entry_t entries[ERROR_LOG_MAX_ENTRIES];
} error_log_t;

#define ERROR_LOG_MAGIC           0x47524C47UL  /* "GLRG" */

/*===========================================================================
 * DEVICE CONFIGURATION (stored in flash)
 *===========================================================================*/

typedef struct {
    uint32_t magic;                /* Configuration magic number */
    uint32_t version;              /* Config version */
    display_protocol_t protocol;   /* Active display protocol */
    uint16_t default_width;        /* Default frame width */
    uint16_t default_height;       /* Default frame height */
    uint8_t  default_color_depth;  /* Default color depth */
    uint8_t  default_fps;          /* Default capture rate */
    trigger_mode_t trigger_mode;   /* Default trigger mode */
    uint32_t trigger_interval_ms;  /* For scheduled trigger */
    uint8_t  ocr_enabled;          /* OCR enabled flag */
    uint8_t  pattern_enabled;      /* Pattern matching enabled */
    uint8_t  qr_decode_enabled;    /* QR decoding enabled */
    uint8_t  encryption_enabled;   /* AES-256-GCM encryption */
    uint8_t  aes_key[BLE_AES_KEY_SIZE]; /* Encryption key */
    uint8_t  device_name[32];      /* BLE device name */
    uint8_t  power_mode;           /* Default power mode */
    uint16_t delta_threshold;      /* Pixel change threshold for delta */
    uint32_t crc;                  /* CRC32 of config */
} __attribute__((packed)) device_config_t;

#define CONFIG_MAGIC              0x47524346UL  /* "GRCF" */
#define CONFIG_VERSION            1

/*===========================================================================
 * UTILITY MACROS
 *===========================================================================*/

#define MIN(a, b)              ((a) < (b) ? (a) : (b))
#define MAX(a, b)              ((a) > (b) ? (a) : (b))
#define CLAMP(val, lo, hi)     (MIN(MAX((val), (lo)), (hi)))
#define ARRAY_SIZE(arr)        (sizeof(arr) / sizeof((arr)[0]))

#define BIT(n)                 (1UL << (n))
#define IS_SET(reg, bit)       (((reg) & BIT(bit)) != 0)
#define SET_BIT(reg, bit)      ((reg) |= BIT(bit))
#define CLR_BIT(reg, bit)      ((reg) &= ~BIT(bit))

/* RGB565 conversion */
#define RGB888_TO_RGB565(r, g, b)  \
    ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3))

#define RGB565_TO_R(c)  (((c) >> 8) & 0xF8)
#define RGB565_TO_G(c)  (((c) >> 3) & 0xFC)
#define RGB565_TO_B(c)  (((c) << 3) & 0xF8)

/* YUV422 to RGB565 conversion (for MIPI DSI YUV mode) */
#define YUV_TO_R(y, u, v)  CLAMP((y) + 1.402f * ((v) - 128), 0, 255)
#define YUV_TO_G(y, u, v)  CLAMP((y) - 0.344f * ((u) - 128) - 0.714f * ((v) - 128), 0, 255)
#define YUV_TO_B(y, u, v)  CLAMP((y) + 1.772f * ((u) - 128), 0, 255)

#ifdef __cplusplus
}
#endif

#endif /* REGISTERS_H */