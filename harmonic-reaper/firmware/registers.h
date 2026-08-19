/*
 * registers.h — nRF52840 peripheral register map (subset, hand-mapped)
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Only the registers touched by the Harmonic Reaper firmware are defined
 * here. The full 300-page register map lives in the nRF52840 PS; we pull
 * in the minimum needed to avoid pulling Nordic's full device header.
 */
#ifndef HARMONIC_REAPER_REGISTERS_H
#define HARMONIC_REAPER_REGISTERS_H

#include <stdint.h>

/* ====================================================================== */
/*  Base addresses (nRF52840 Peripheral Map, PS 1.1 §6)                    */
/* ====================================================================== */

#define NRF_CLOCK_BASE       0x40000000u
#define NRF_POWER_BASE       0x40000000u  /* shared with clock page 0 */
#define NRF_POWER_BASE2      0x40008000u
#define NRF_RADIO_BASE       0x40001000u
#define NRF_UART0_BASE       0x40002000u
#define NRF_SPIM0_BASE       0x40003000u
#define NRF_SPIM1_BASE       0x40004000u
#define NRF_SPIM2_BASE       0x40023000u
#define NRF_SPIM3_BASE       0x4002F000u
#define NRF_GPIO_BASE        0x50000000u
#define NRF_GPIOTE_BASE      0x40006000u
#define NRF_SAADC_BASE       0x40007000u
#define NRF_TIMER0_BASE      0x40008000u
#define NRF_TIMER1_BASE      0x40009000u
#define NRF_TIMER2_BASE      0x4000A000u
#define NRF_RTC0_BASE        0x4000B000u
#define NRF_RTC1_BASE        0x40011000u
#define NRF_WDT_BASE         0x40010000u
#define NRF_NVMC_BASE        0x4001E000u
#define NRF_FICR_BASE        0x10000000u
#define NRF_UICR_BASE        0x10001000u
#define NRF_PPI_BASE         0x4001F000u
#define NRF_EGU0_BASE        0x40014000u

/* ====================================================================== */
/*  Generic peripheral register block layout                              */
/* ====================================================================== */

#define REG32(addr) (*(volatile uint32_t *)(addr))

/* Tasks: writing any non-zero value triggers the task. */
#define NRF_TASK_START(base, off)   REG32((base) + (off)) = 1u
#define NRF_TASK_TRIGGER(base, off) REG32((base) + (off)) = 1u

/* Events: read & clear by writing 0. */
#define NRF_EVENT_CHECK(base, off)  (REG32((base) + (off)) != 0u)
#define NRF_EVENT_CLEAR(base, off)  REG32((base) + (off)) = 0u

/* ====================================================================== */
/*  CLOCK — PS §15.6                                                       */
/* ====================================================================== */

#define CLOCK_HFCLKSTART   REG32(NRF_CLOCK_BASE + 0x000u)
#define CLOCK_HFCLKSTARTED REG32(NRF_CLOCK_BASE + 0x104u)
#define CLOCK_LFCLKSTART   REG32(NRF_CLOCK_BASE + 0x008u)
#define CLOCK_LFCLKSTARTED REG32(NRF_CLOCK_BASE + 0x108u)
#define CLOCK_LFCLKSRC     REG32(NRF_CLOCK_BASE + 0x518u)

#define CLOCK_LFCLKSRC_XTAL  0x01u   /* 32.768 kHz crystal */
#define CLOCK_HFCLKSRC_XO    0x00u   /* HFXO (64 MHz XO) */

/* ====================================================================== */
/*  GPIO — PS §15.4                                                        */
/*  nRF52840 has 2 ports: P0 (0..31), P1 (32..47). Bit 13 of pin # → P1.   */
/* ====================================================================== */

#define GPIO_OUTCLR(base)  REG32((base) + 0x504u)
#define GPIO_OUTSET(base)  REG32((base) + 0x508u)
#define GPIO_OUT(base)     REG32((base) + 0x504u - 4u)  /* OUT @ 0x504-0x004 */
#define GPIO_DIR(base)     REG32((base) + 0x504u + 0x04u)
#define GPIO_PIN_CNF(base, n) REG32((base) + 0x700u + (n)*4u)

#define GPIO_CNF_DIR_INPUT   0u
#define GPIO_CNF_DIR_OUTPUT  1u
#define GPIO_CNF_PULL_NONE   0u
#define GPIO_CNF_PULL_DOWN   2u
#define GPIO_CNF_PULL_UP     3u
#define GPIO_CNF_DRIVE_S0S1  0u
#define GPIO_CNF_DRIVE_H0H1  3u
#define GPIO_CNF_SENSE_NONE  0u

/* ====================================================================== */
/*  SPIM — PS §15.5                                                        */
/* ====================================================================== */

#define SPIM_ENABLE(base)     REG32((base) + 0x500u)
#define SPIM_PSEL_SCK(base)   REG32((base) + 0x508u)
#define SPIM_PSEL_MOSI(base)  REG32((base) + 0x50Cu)
#define SPIM_PSEL_MISO(base)  REG32((base) + 0x510u)
#define SPIM_FREQUENCY(base)  REG32((base) + 0x514u)
#define SPIM_TXD_PTR(base)    REG32((base) + 0x524u)
#define SPIM_TXD_MAXCNT(base) REG32((base) + 0x528u)
#define SPIM_RXD_PTR(base)    REG32((base) + 0x534u)
#define SPIM_RXD_MAXCNT(base) REG32((base) + 0x538u)
#define SPIM_CONFIG(base)     REG32((base) + 0x554u)
#define SPIM_ORC(base)        REG32((base) + 0x5C0u)

#define SPIM_FREQ_1M   0x02000000u
#define SPIM_FREQ_2M   0x04000000u
#define SPIM_FREQ_4M   0x08000000u
#define SPIM_FREQ_8M   0x10000000u
#define SPIM_FREQ_16M  0x20000000u
#define SPIM_FREQ_32M  0x40000000u

#define SPIM_MODE0   0x00000000u  /* CPOL=0 CPHA=0 */
#define SPIM_MODE3   0x00000003u  /* CPOL=1 CPHA=1 */

#define SPIM_START_TX(base)  NRF_TASK_START(base, 0x008u)
#define SPIM_END_EVENT(base) NRF_EVENT_CHECK(base, 0x104u)
#define SPIM_END_CLEAR(base) NRF_EVENT_CLEAR(base, 0x104u)

/* ====================================================================== */
/*  UART (legacy) — for USB-CDC bridge backchannel on SPIM2 alt pins        */
/* ====================================================================== */

#define UART_ENABLE(base) REG32((base) + 0x500u)
#define UART_TXD(base)    REG32((base) + 0x51Cu)
#define UART_TXDRDY(base) REG32((base) + 0x11Cu)

/* ====================================================================== */
/*  SAADC — used for battery gauge via internal VDD/1 divider              */
/* ====================================================================== */

#define SAADC_ENABLE       REG32(NRF_SAADC_BASE + 0x500u)
#define SAADC_CH0_PSELP    REG32(NRF_SAADC_BASE + 0x510u)
#define SAADC_CH0_CONFIG   REG32(NRF_SAADC_BASE + 0x508u)
#define SAADC_RESULT_PTR    REG32(NRF_SAADC_BASE + 0x62Cu)
#define SAADC_RESULT_MAXCNT REG32(NRF_SAADC_BASE + 0x630u)
#define SAADC_RESOLUTION    REG32(NRF_SAADC_BASE + 0x5F0u)

#define SAADC_PSELP_VDD     0x0Du
#define SAADC_RES_12BIT     0x00u
#define SAADC_RES_14BIT     0x01u

/* ====================================================================== */
/*  TIMER1 — 1 ms scheduler tick (16 MHz / 16000)                          */
/* ====================================================================== */

#define TIMER_MODE(base)     REG32((base) + 0x504u)
#define TIMER_BITMODE(base)  REG32((base) + 0x508u)
#define TIMER_PRESCALER(base) REG32((base) + 0x50Cu)
#define TIMER_CC0(base)      REG32((base) + 0x540u)
#define TIMER_INTENSET(base) REG32((base) + 0x304u)

#define TIMER_MODE_TIMER   0x00u
#define TIMER_BITMODE_16   0x00u
#define TIMER_BITMODE_32   0x03u

/* ====================================================================== */
/*  GPIOTE — for IMU data-ready IRQ + haptic completion IRQ                */
/* ====================================================================== */

#define GPIOTE_CONFIG(n)     REG32(NRF_GPIOTE_BASE + 0x510u + (n)*4u)
#define GPIOTE_IN_EVENT(n)   REG32(NRF_GPIOTE_BASE + 0x100u + (n)*4u)
#define GPIOTE_INTENSET      REG32(NRF_GPIOTE_BASE + 0x304u)

#define GPIOTE_MODE_EVENT      0x01u
#define GPIOTE_MODE_TASK_HI    0x03u
#define GPIOTE_POLARITY_HITOLO 0x02u
#define GPIOTE_POLARITY_LOTOHI 0x01u
#define GPIOTE_POLARITY_TOGGLE 0x03u

/* ====================================================================== */
/*  NVMC — flash erase/write for hit-log persistence                       */
/* ====================================================================== */

#define NVMC_READY      REG32(NRF_NVMC_BASE + 0x400u)
#define NVMC_CONFIG     REG32(NRF_NVMC_BASE + 0x504u)
#define NVMC_ERASEPAGE  REG32(NRF_NVMC_BASE + 0x508u)

#define NVMC_CFG_WEN    0x01u
#define NVMC_CFG_EEN    0x02u

/* ====================================================================== */
/*  POWER — USB power-detect, reset reason                                 */
/* ====================================================================== */

#define POWER_USBREGSTATUS REG32(NRF_POWER_BASE2 + 0x578u)
#define POWER_RESETREASON  REG32(NRF_POWER_BASE2 + 0x400u)
#define POWER_USBDETECTED  REG32(NRF_POWER_BASE2 + 0x07Cu)

#define POWER_USBREG_VBUS_DETECTED 0x01u
#define POWER_USBREG_USB_POWER_OK  0x02u

/* ====================================================================== */
/*  FICR — factory device ID, used for BLE MAC + serial                    */
/* ====================================================================== */

#define FICR_DEVICEADDR0  REG32(NRF_FICR_BASE + 0x0A0u)
#define FICR_DEVICEADDR1  REG32(NRF_FICR_BASE + 0x0A4u)
#define FICR_DEVICEID0    REG32(NRF_FICR_BASE + 0x060u)
#define FICR_DEVICEID1    REG32(NRF_FICR_BASE + 0x064u)

/* ====================================================================== */
/*  Helper inline wrappers                                                 */
/* ====================================================================== */

static inline void nrf_delay_us(uint32_t us)
{
    /* ~64 MHz core, loop roughly 1 cycle/iter; pad for overhead */
    volatile uint32_t n = us * 12u;
    while (n--) { __asm__ volatile ("nop"); }
}

static inline void nrf_delay_ms(uint32_t ms)
{
    while (ms--) nrf_delay_us(1000u);
}

#endif /* HARMONIC_REAPER_REGISTERS_H */
/* EOF — registers.h — jayis1 */