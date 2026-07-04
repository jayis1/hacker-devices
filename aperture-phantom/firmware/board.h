/*
 * aperture-phantom / firmware / board.h
 *
 * Board-level configuration for the AperturePhantom control MCU board:
 * pin assignments, clock tree constants, peripheral choices, and the
 * exported driver API prototypes.
 *
 * MCU: STM32H743VIT6 (Cortex-M7 @ 480 MHz, LQFP100)
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef APERTURE_PHANTOM_BOARD_H
#define APERTURE_PHANTOM_BOARD_H

#include <stdint.h>
#include <stddef.h>
#include "registers.h"

/* ---- Clock tree (HSE 25 MHz xtal → PLL1 480 MHz sysclk) ---- */
#define HSE_VALUE_HZ        25000000u
#define SYSCLK_HZ           480000000u
#define AHB_HZ              240000000u
#define APB1_HZ             120000000u
#define APB2_HZ             120000000u
#define HCLK_HZ             240000000u

/* ---- Pin assignments (LQFP100) ---- *
 *  PA9  USB_OTG_HS_VBUS
 *  PA11 USB_OTG_HS_DM
 *  PA12 USB_OTG_HS_DP
 *  PA13 PA14 SWD (debug)
 *  PB6  USART3_TX  → BLE module (EFR32BG22 UART console)
 *  PB7  USART3_RX  ← BLE module
 *  PB8  I2C1_SCL   → sensor side-A CCI
 *  PB9  I2C1_SDA   → sensor side-A CCI
 *  PA8  I2C2_SCL   → host side-B CCI (mirror/spoof)
 *  PC9  I2C2_SDA   → host side-B CCI
 *  PA5  SPI1_SCK   → FPGA bridge SPI
 *  PA6  SPI1_MISO  ← FPGA
 *  PA7  SPI1_MOSI  → FPGA
 *  PB0  GPIO out   FPGA_RST_N
 *  PB1  GPIO out   FPGA_CS_N
 *  PC4  GPIO in    FPGA_IRQ_N (FIFO threshold / cap-done)
 *  PC5  GPIO out   LED_R
 *  PC6  GPIO out   LED_G
 *  PC7  GPIO out   LED_B
 *  PC8  GPIO out   MOTOR_EN
 *  PC10-Touch_C0  capacitive pad zone 0
 *  PC11-Touch_C1  capacitive pad zone 1
 *  PC12-Touch_C2  capacitive pad zone 2
 *  PC13 GPIO in   SD_CARD_DETECT
 *  PC14 GPIO out  SENSOR_MUX_SEL (D-PHY pass-through vs gate)
 *  PD0  SDMMC1_CDIR
 *  PD2  SDMMC1_CMD
 *  PB14 SDMMC1_CK
 *  PB15 SDMMC1_D0
 *  PC0  INA219_ALERT
 *  PC1  DS28E07_DQ (1-wire secure element)
 *  PD4  FMC_D0  ... PD15 FMC_D15 (data)
 *  PE0..PE12 FMC_A0..FMC_A12 (addr, BA0, BA1, SDNCAS, SDNRAS, etc.)
 */

/* GPIO helpers */
#define GPIO_PIN_OUT_PP(port, pin) do { \
    GPIO_MODER(port) &= ~(3u << (2*(pin))); \
    GPIO_MODER(port) |=  (1u << (2*(pin))); \
    GPIO_OTYPER(port) &= ~(1u<<(pin)); \
    GPIO_OSPEEDR(port) |= (3u << (2*(pin))); /* high speed */ \
    GPIO_PUPDR(port) &= ~(3u << (2*(pin))); \
} while (0)

#define GPIO_PIN_IN_PU(port, pin) do { \
    GPIO_MODER(port) &= ~(3u << (2*(pin))); \
    GPIO_PUPDR(port) &= ~(3u << (2*(pin))); \
    GPIO_PUPDR(port) |=  (1u << (2*(pin))); \
} while (0)

#define GPIO_PIN_AF(port, pin, af) do { \
    GPIO_MODER(port) &= ~(3u << (2*(pin))); \
    GPIO_MODER(port) |=  (2u << (2*(pin))); \
    if ((pin) < 8) { GPIO_AFRL(port) &= ~(0xFu << (4*(pin))); \
                     GPIO_AFRL(port) |=  ((af) << (4*(pin))); } \
    else { GPIO_AFRH(port) &= ~(0xFu << (4*((pin)-8))); \
           GPIO_AFRH(port) |=  ((af) << (4*((pin)-8))); } \
} while (0)

#define GPIO_SET(port, pin)  GPIO_BSRR(port) = (1u << (pin))
#define GPIO_CLR(port, pin)  GPIO_BSRR(port) = (1u << ((pin)+16))
#define GPIO_GET(port, pin)  ((GPIO_IDR(port) >> (pin)) & 1u)

/* ---- Pin constants ---- */
#define PIN_FPGA_RST     0   /* PB0 */
#define PIN_FPGA_CS      1   /* PB1 */
#define PIN_FPGA_IRQ     4   /* PC4 */
#define PIN_LED_R        5   /* PC5 */
#define PIN_LED_G        6   /* PC6 */
#define PIN_LED_B        7   /* PC7 */
#define PIN_MOTOR_EN     8   /* PC8 */
#define PIN_TOUCH0       10  /* PC10 */
#define PIN_TOUCH1       11  /* PC11 */
#define PIN_TOUCH2       12  /* PC12 */
#define PIN_SD_CD         13 /* PC13 */
#define PIN_SENSOR_MUX    14 /* PC14 */
#define PIN_DS28E07_DQ    1  /* PC1 (1-wire open-drain) */

#define AF_USART3_TX_PB6  7u
#define AF_USART3_RX_PB7  7u
#define AF_I2C1_SCL_PB8   4u
#define AF_I2C1_SDA_PB9   4u
#define AF_I2C2_SCL_PA8   4u
#define AF_I2C2_SDA_PC9   4u
#define AF_SPI1_PA5_7     5u

/* ---- Driver API ---- */

/* board_init.c */
void board_init(void);            /* clocks, GPIO, NVIC priority, peripherals */
void board_led_set(uint8_t r, uint8_t g, uint8_t b);
void board_motor_pulse(uint16_t ms);
uint8_t board_touch_read(void);    /* bitmask of zones currently touched */
uint8_t board_sd_present(void);
void board_sensor_mux(uint8_t passthrough); /* 1 = sensor→host, 0 = FPGA TX→host */

/* fpga.c */
void     fpga_init(void);
void     fpga_reset(void);
void     fpga_write_reg(uint8_t addr, uint32_t val);
uint32_t fpga_read_reg(uint8_t addr);
int      fpga_wait_rx_lock(uint32_t timeout_ms);
void     fpga_set_mode(op_mode_t m);
void     fpga_start_capture(void);
void     fpga_stop_capture(void);
uint32_t fpga_capture_drain(uint8_t *dst, uint32_t max);   /* DMA from cap FIFO */
uint32_t fpga_capture_level(void);
int      fpga_inject_load(const uint8_t *src, uint32_t n);  /* DMA into inj FIFO */
void     fpga_inject_trigger(void);
void     fpga_mute_sensor(int on);
uint8_t  fpga_set_fuzz_bits(uint8_t mask);
uint32_t fpga_status(void);
uint32_t fpga_frame_count(void);
uint32_t fpga_frame_len(void);
uint32_t fpga_crc_err_count(void);

/* sdram.c */
void  sdram_init(void);
void *sdram_alloc(uint32_t bytes);
void  sdram_write32(uint32_t off, uint32_t v);
uint32_t sdram_read32(uint32_t off);

/* sdcard.c */
int  sdcard_init(void);
int  sdcard_mount(void);
int  sdcard_read_block(uint32_t blk, uint8_t *buf);
int  sdcard_write_block(uint32_t blk, const uint8_t *buf);
int  sdcard_open_write(const char *name);
int  sdcard_write(const uint8_t *buf, uint32_t len);
int  sdcard_close(void);
int  sdcard_open_read(const char *name);
int  sdcard_read(uint8_t *buf, uint32_t len);
int  sdcard_list_replay(char names[][24], int max);
int  sdcard_present(void);

/* i2c.c — CCI master (two instances) */
typedef enum { CCI_SIDE_A = 0, CCI_SIDE_B = 1 } cci_side_t;
int cci_init(cci_side_t side);
int cci_scan(cci_side_t side, uint8_t *found, int max);
int cci_write(cci_side_t side, uint8_t addr, uint16_t reg, uint16_t val);
int cci_read(cci_side_t side, uint8_t addr, uint16_t reg, uint16_t *val);
int cci_read_block(cci_side_t side, uint8_t addr, uint16_t reg,
                   uint8_t *buf, uint16_t len);

/* uart.c — USART3 (BLE console + status) */
void uart_init(uint32_t baud);
void uart_putc(char c);
void uart_puts(const char *s);
int  uart_getc(char *c, uint32_t timeout_ms);
void uart_write(const uint8_t *buf, uint16_t n);
int  uart_read(uint8_t *buf, uint16_t n, uint32_t timeout_ms);

/* ble_uart.c — framed command protocol over BLE UART bridge */
void ble_init(void);
void ble_send_cmd(uint8_t op, const uint8_t *payload, uint16_t len);
int  ble_poll_cmd(uint8_t *op, uint8_t *payload, uint16_t *len);
void ble_send_frame(const uint8_t *data, uint16_t len, uint8_t dt);
void ble_notify_log(const char *s);
void ble_notify_status(uint32_t status, uint32_t frame_count,
                       uint32_t crc_errs, uint8_t mode, uint8_t armed);

/* usb_cdc.c — USB CDC backhaul (laptop) */
void usb_cdc_init(void);
void usb_cdc_poll(void);
int  usb_cdc_tx(const uint8_t *buf, uint16_t len);
int  usb_cdc_rx(uint8_t *buf, uint16_t len);

/* onewire.c — DS28E07 secure element (auth) */
int onewire_init(void);
int onewire_challenge(uint8_t *challenge, uint8_t *response);
int onewire_read_serial(uint8_t *serial8);

/* ---- Application-level (in main.c) ---- */
void script_engine_run(const uint8_t *bc, uint16_t len);
void script_engine_stop(void);
int  script_engine_running(void);
void trigger_set_reference(const uint8_t *downscaled, uint16_t len);
int  trigger_match(const uint8_t *frame_down, uint16_t len);
void fuzz_engine_run(uint8_t strategy, uint16_t count, uint16_t delay_ms);
void fuzz_engine_stop(void);
void sensor_autoconfig(void);

/* ---- Logging ---- */
#define LOG_BUF 256
extern char g_log[LOG_BUF];
void logf(const char *fmt, ...);

/* ---- Global state (in main.c) ---- */
typedef struct {
    op_mode_t mode;
    uint8_t   armed;
    uint8_t   lanes;
    uint16_t  rate_mbps_x10;
    uint8_t   data_type;
    uint8_t   sensor_addr;
    char      sensor_name[24];
    uint32_t  frame_count;
    uint32_t  crc_errs;
    uint32_t  sd_free_kb;
    uint8_t   battery_pct;
    int       sensor_power_ok;
} device_state_t;

extern device_state_t g_state;

#endif /* APERTURE_PHANTOM_BOARD_H */