/*
 * board.h — DRAM-Phantom hardware pin map and peripheral assignments
 *
 * Target MCU: STM32G474CEU6 (Cortex-M4F, 170 MHz, 512 KB flash, 128 KB SRAM)
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The MCU is the companion controller to the Artix-7 FPGA. It owns:
 *   - SPI to the FPGA (command + FIFO drain)
 *   - USB-C CDC for app comms
 *   - UART to the nRF52840 BLE bridge
 *   - I2C to the OLED + fuel gauge
 *   - SDIO to the microSD card
 *   - GPIO: mode button, host-VDD sense, host-RESET# open-drain gate, LED
 */

#ifndef DRAMPHANTOM_BOARD_H
#define DRAMPHANTOM_BOARD_H

#include <stdint.h>
#include <stddef.h>

#define BOARD_NAME      "DRAM-Phantom v1.0"
#define BOARD_AUTHOR    "jayis1"
#define BOARD_FW_VER    "1.0.0"

/* ---- Clock tree ---- */
#define HSE_VALUE        24000000UL   /* 24 MHz crystal on board */
#define SYSCLK_VALUE     170000000UL  /* PLL: VCO=170 MHz, sys=170 MHz */
#define APB1_VALUE       170000000UL  /* G4: PPRE1=/1 (max 170 MHz) */
#define APB2_VALUE       170000000UL  /* G4: PPRE2=/1 */

/* ---- Port A ---- */
#define PA0  0   /* MODE_BTN        (input, pull-up, EXTI rising+falling) */
#define PA1  1   /* LED_STAT        (output, push-pull, active high) */
#define PA2  2   /* HOST_VDD_SENSE  (analog, ADC1_IN2) — host 1.2 V rail via divider */
#define PA3  3   /* BATT_SENSE     (analog, ADC1_IN3) — battery via divider */
#define PA4  4   /* FPGA_CS_N       (output, SPI1 NSS — manual control) */
#define PA5  5   /* SPI1_SCK        (AF5) */
#define PA6  6   /* SPI1_MISO       (AF5) */
#define PA7  7   /* SPI1_MOSI       (AF5) */
#define PA8  8   /* (free) */
#define PA9  9   /* USB_DM          (AF14) */
#define PA10 10  /* USB_DP          (AF14) */
#define PA11 11  /* (free) */
#define PA12 12  /* (free) */
#define PA13 13  /* SWDIO           (AF5) */
#define PA14 14  /* SWCLK           (AF5) */
#define PA15 15  /* HOST_RESET_GATE (output, push-pull) — drives FET gating host RESET# */

/* ---- Port B ---- */
#define PB2  2   /* FPGA_INTR_N     (input, pull-up, EXTI falling) — FPGA has data/flip event */
#define PB3  3   /* SWO            (AF6) */
#define PB4  4   /* SDIO_CMD        (AF12) */
#define PB5  5   /* (free) */
#define PB6  6   /* I2C1_SCL        (AF4) — OLED + MAX17048 */
#define PB7  7   /* I2C1_SDA        (AF4) */
#define PB8  8   /* BLE_UART_TX      (AF7) — USART1 TX to nRF52840 */
#define PB9  9   /* BLE_UART_RX      (AF7) — USART1 RX from nRF52840 */
#define PB10 10  /* SDIO_D2         (AF12) */
#define PB11 11  /* SDIO_D3         (AF12) */
#define PB12 12  /* SDIO_CLK        (AF12) */
#define PB13 13  /* (free) */
#define PB14 14  /* SDIO_D0         (AF12) */
#define PB15 15  /* SDIO_D1         (AF12) */

/* ---- Port C ---- */
#define PC0  0   /* (free) */
#define PC1  1   /* (free) */
#define PC2  2   /* (free) */
#define PC3  3   /* (free) */
#define PC4  4   /* FPGA_CFG_DONE_N (input) — low when bitstream loaded */
#define PC5  5   /* FPGA_PROG_N     (output) — pulse low to reprogram FPGA */
#define PC6  6   /* (free) */
#define PC7  7   /* (free) */
#define PC8  8   /* (free) */
#define PC9  9   /* (free) */
#define PC10 10  /* (free) */
#define PC11 11  /* (free) */
#define PC12 12  /* (free) */
#define PC13 13  /* (free) */
#define PC14 14  /* OSC32_IN        — 32.768 kHz for RTC */
#define PC15 15  /* OSC32_OUT */

/* ---- Mode enumeration (matches FPGA command set) ---- */
typedef enum {
    MODE_IDLE = 0,
    MODE_SNOOP,
    MODE_ROWHAMMER,
    MODE_WARMBOOT,
    MODE_COVERT,
    MODE_COUNT
} device_mode_t;

/* ---- Two-key arm token (refreshed each mode entry) ---- */
#define ARM_TOKEN_LEN 6

/* ---- Timing constants (nanoseconds, from JEDEC DDR4) ---- */
#define DDR4_TREFI_NS      7800UL   /* 7.8 us refresh interval */
#define DDR4_TRC_NS        48       /* ACT-to-ACT same bank, DDR4-3200 */
#define DDR4_TRFC_NS       350      /* REF to ACT (per density) */
#define DDR4_TRCD_NS       16       /* ACT to READ */
#define DDR4_TCL_NS        22        /* CAS latency in ns approx */
#define DDR4_TWR_NS        15        /* Write recovery */

/* ---- SPI to FPGA protocol ---- */
#define FPGA_CMD_NOP        0x00
#define FPGA_CMD_STATUS     0x01
#define FPGA_CMD_SETMODE    0x02   /* arg = mode enum */
#define FPGA_CMD_ARM        0x03   /* arg = token (4 bytes) */
#define FPGA_CMD_DISARM     0x04
#define FPGA_CMD_DRAIN_SNOOP 0x10  /* read N snoop records */
#define FPGA_CMD_DRAIN_FLIP 0x11   /* read bit-flip result */
#define FPGA_CMD_DRAIN_IMG  0x12   /* read warm-boot image chunk */
#define FPGA_CMD_HAMMER_PAT 0x20   /* set aggressor row, count, pattern */
#define FPGA_CMD_COVERT_CFG 0x30   /* set bank for covert timing */

/* Status bits from FPGA */
#define FSTAT_BITSTREAM_OK  (1u<<0)
#define FSTAT_DDR_PRESENT   (1u<<1)
#define FSTAT_TARGET_ARMED  (1u<<2)
#define FSTAT_FLIP_DETECTED (1u<<3)
#define FSTAT_REFRESH_ACTIVE (1u<<4)
#define FSTAT_FIFO_OVF      (1u<<5)
#define FSTAT_HOST_RESET    (1u<<6)

/* ---- Function prototypes ---- */
void board_init(void);

/* drivers */
int  fpga_init(void);
uint32_t fpga_status(void);
int  fpga_set_mode(device_mode_t m);
int  fpga_arm(const char token[ARM_TOKEN_LEN]);
void fpga_disarm(void);
int  fpga_hammer_pattern(uint32_t aggressor_row, uint32_t count, uint8_t pattern_id);
int  fpga_drain_snoop(void *buf, uint16_t max_records, uint16_t *out_records);
int  fpga_drain_flip(uint32_t *victim_row, uint32_t *flip_mask);
int  fpga_drain_image(void *buf, uint32_t len, uint32_t *out_len);
int  fpga_covert_config(uint8_t bank_group, uint8_t bank);

int  ddr4_snoop_decode(const void *rec, void *out, uint16_t n);

int  rowhammer_arm(uint32_t aggressor, uint32_t count, uint8_t pat);
int  rowhammer_run(uint32_t *victim_row, uint32_t *flip_mask);

int  mem_capture_start(uint32_t base_row, uint32_t row_count);
int  mem_capture_drain_to_sd(void);

int  covert_channel_set(uint8_t bg, uint8_t bank, uint32_t *baseline_ns);

int  usb_cdc_init(void);
int  usb_cdc_write(const void *buf, uint16_t len);
int  usb_cdc_read(void *buf, uint16_t maxlen);

int  ble_uart_init(void);
int  ble_uart_write(const void *buf, uint16_t len);
int  ble_uart_read(void *buf, uint16_t maxlen);

int  sdcard_init(void);
int  sdcard_write_file(const char *name, const void *buf, uint32_t len);

int  oled_init(void);
void oled_status(device_mode_t mode, uint32_t aux);
void oled_clear(void);
void oled_printf(int row, const char *fmt, ...);

uint32_t adc_read(uint8_t channel);

void delay_ms(uint32_t ms);
uint32_t millis(void);

#endif /* DRAMPHANTOM_BOARD_H */