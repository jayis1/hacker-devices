/*
 * board.h — 1553-Phantom board pin map and public config
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Central place where every board-level decision (pin assignments, role
 * enumeration, channel count, FIFO depths) is recorded. The driver files
 * include this and never hard-code pins themselves.
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ---- Identity ---- */
#define DEVICE_NAME   "1553-Phantom"
#define DEVICE_AUTHOR "jayis1"
#define FW_VERSION    "1.0"

/* ---- STM32G474 pin map (LQFP48) ---- */
/* Port A */
#define PA0_OLED_DC       0    /* output, OLED data/command */
#define PA1_OLED_RST      1    /* output, OLED reset (active low) */
#define PA2_USART2_TX     2    /* alt, USB-CDC line-coding echo only */
#define PA3_USART2_RX     3
#define PA4_FPGA_CS       4    /* output, SPI1 NSS to iCE40 */
#define PA5_SPI1_SCK      5    /* alt AF5, SPI1 to FPGA */
#define PA6_SPI1_MISO     6    /* alt AF5 */
#define PA7_SPI1_MOSI     7    /* alt AF5 */
#define PA8_USB_DM        8    /* alt AF11, USB FS */
#define PA9_USB_DP        9    /* alt AF11 */
#define PA10_BTN          10   /* input pullup + EXTI10, user button */
#define PA11_FPGA_IRQ     11   /* input + EXTI11, FPGA "word ready" */
#define PA12_FPGA_RST     12   /* output, FPGA reset (active low) */
#define PA13_SWDIO        13   /* alt SWD */
#define PA14_SWCLK        14   /* alt SWD */
#define PA15_LED_PWR      15   /* output, power LED */

/* Port B */
#define PB0_LED_ACTA      0    /* output, channel-A activity */
#define PB1_LED_ACTB      1    /* output, channel-B activity */
#define PB2_BOOT1         2    /* boot, leave as input */
#define PB3_LED_ARM_RED   3    /* output, armed LED (red) */
#define PB4_LED_INJ_GRN   4    /* output, injecting LED (green) */
#define PB5_FPGA_CDONE    5    /* input, iCE40 config-done */
#define PB6_FPGA_SS_HOLD  6    /* output, holds SPI flash CS (shared) */
#define PB7_BAT_SENSE     7    /* analog, via MAX17048 alerts */
#define PB8_I2C1_SCL      8    /* alt AF4, MAX17048 fuel gauge */
#define PB9_I2C1_SDA      9    /* alt AF4 */
#define PB10_PANIC_SENSE  10   /* input, capacitive-sense panic pad */
#define PB11_TX_GATE      11   /* output, gates HI-1573 tx drivers (arm latch) */

/* Port C (F connectors, not bonded on LQFP48; placeholders for BGA) */
#define PC13_OLED_CS      13   /* output, OLED chip select */

/* ---- Role machine ---- */
typedef enum {
    ROLE_BM = 0,    /* passive bus monitor  (boot default)            */
    ROLE_RT,        /* remote terminal emulator (spoof)               */
    ROLE_BC,        /* bus controller (takeover)                      */
    ROLE_MITM,      /* inline match/replace/drop/inject               */
    ROLE_COUNT
} role_t;

/* Map role -> is TX-capable (needs arm latch) */
#define ROLE_CAN_TX(r)  ((r) == ROLE_RT || (r) == ROLE_BC || (r) == ROLE_MITM)

/* ---- Channels ---- */
#define N_CHAN      2     /* primary (A) + redundant (B)                 */
#define N_FIFOS     2     /* one RX FIFO per channel in the FPGA         */

/* ---- FPGA codec register map (mirrors codec_reg.h) ---- */
#define FPGA_REG_STATUS      0x00   /* R:  ch_rdy, ch_err, ch_active      */
#define FPGA_REG_TX_CTL      0x01   /* W:  channel select + GO bit        */
#define FPGA_REG_RX_RD      0x02   /* R:  next RX word + ts[31:16]        */
#define FPGA_REG_RX_TS_LO   0x03   /* R:  ts[15:0]                        */
#define FPGA_REG_FUZZ       0x04   /* W:  fuzz mask (parity/sync/gap/...) */
#define FPGA_REG_GAP_US     0x05   /* W:  inter-message gap override      */
#define FPGA_REG_ARM        0x06   /* W:  arm latch gate (per FPGA side)  */
#define FPGA_REG_TIMESTAMP  0x07   /* R:  free-running 96 MHz counter[31:0] */

#define STATUS_CH0_RDY      (1u<<0)
#define STATUS_CH1_RDY      (1u<<1)
#define STATUS_CH0_ERR      (1u<<2)
#define STATUS_CH1_ERR      (1u<<3)
#define STATUS_CH0_ACTIVE   (1u<<4)
#define STATUS_CH1_ACTIVE   (1u<<5)
#define STATUS_TX_BUSY      (1u<<6)
#define STATUS_PARITY_FAULT (1u<<7)

/* ---- Capture ring ---- */
#define CAP_RING_WORDS  8192     /* 8K 32-bit words ≈ 32 KiB ring          */
#define CAP_RECORD_SZ   3        /* 3 words per captured 1553 word record */

/* ---- RT emulation ---- */
#define MAX_RT_EMUL  8           /* up to 8 RT addresses at once          */
#define MAX_SA       32          /* subaddresses 0..31 (31 = mode code)   */

/* ---- Scheduler timing ---- */
#define TICK_HZ      1000        /* 1 ms TIM6 tick                        */
#define MAJOR_MS     200         /* default major frame 200 ms            */

/* ---- Arm interlock timeouts ---- */
#define ARM_REQUEST_TIMEOUT_MS  5000
#define ARM_BUTTON_HOLD_MS      3000

#endif /* BOARD_H */