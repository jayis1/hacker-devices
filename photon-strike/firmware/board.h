/**
 * board.h — Pin map & peripheral assignments for PhotonStrike
 * Author: jayis1
 * License: GPL-2.0
 *
 * PhotonStrike hardware:
 *   MCU   : STM32H757VIT6 (Cortex-M7 @ 480 MHz + Cortex-M4 @ 240 MHz)
 *   FPGA  : Lattice iCE40UP5K-SG48 (SPI-configured from M7)
 *   BLE   : ESP32-C3-MINI on UART3
 *   Laser : 1064 nm pulsed diode, driver on PA8/TIM1 CH1 gate
 *   MEMS  : Mirrorcle 2-axis MEMS mirror on I2C1 + DAC1 (piezo Z)
 *   SD    : MicroSD on SDMMC1
 *   USB   : USB-C, USB FS OTG on PA11/PA12
 *
 * The M4 core (0x080E0000) runs the trigger arbiter and safety interlock.
 * It cannot be blocked by M7 work; it gates the laser driver hard line.
 */

#ifndef PHOTONSTRIKE_BOARD_H
#define PHOTONSTRIKE_BOARD_H

#include <stdint.h>
#include <stdbool.h>
#include "registers.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ─── Board identity ──────────────────────────────────────────────────── */
#define BOARD_NAME          "PhotonStrike"
#define BOARD_VERSION       0x00010000u   /* 1.0.0 */
#define BOARD_AUTHOR        "jayis1"

/* ─── Core addresses ──────────────────────────────────────────────────── */
#define M7_FLASH_BASE       0x08000000u
#define M4_FLASH_BASE       0x080E0000u   /* M4 image lives at end of flash */
#define M4_RELEASE_REG      0x38000000u   /* D3 SRAM handshake word        */

/* ─── Pin map (port, pin number) ──────────────────────────────────────── */
typedef struct {
    volatile uint32_t *port;   /* GPIOA..GPIOH base */
    uint8_t            pin;
} ps_pin_t;

#define PS_PIN(port, n)  { (volatile uint32_t *)(port + (n)), (n) }

/* Laser driver */
#define PIN_LASER_GATE       PS_PIN(GPIOA_BASE, 8)    /* TIM1_CH1 PWM gate  */
#define PIN_LASER_ENABLE     PS_PIN(GPIOB_BASE, 0)    /* driver enable      */
#define PIN_LASER_SHUTTER    PS_PIN(GPIOB_BASE, 1)    /* M4-controlled      */
#define PIN_LASER_FAULT      PS_PIN(GPIOB_BASE, 2)    /* driver fault input */
#define PIN_LASER_TEC_PWM    PS_PIN(GPIOA_BASE, 9)    /* TEC current        */
#define PIN_LASER_TEMP       PS_PIN(GPIOC_BASE, 0)    /* ADC1_IN0 thermistor*/

/* FPGA (SPI1: PA5/6/7 + PA4 CS) */
#define PIN_FPGA_SCK         PS_PIN(GPIOA_BASE, 5)
#define PIN_FPGA_MISO        PS_PIN(GPIOA_BASE, 6)
#define PIN_FPGA_MOSI        PS_PIN(GPIOA_BASE, 7)
#define PIN_FPGA_CS          PS_PIN(GPIOA_BASE, 4)
#define PIN_FPGA_CDONE       PS_PIN(GPIOB_BASE, 3)    /* config done        */
#define PIN_FPGA_CRESET      PS_PIN(GPIOB_BASE, 4)    /* config reset       */
#define PIN_FPGA_IRQ         PS_PIN(GPIOB_BASE, 5)    /* FPGA→M7 interrupt  */

/* Trigger inputs */
#define PIN_TRIG_GPIO1       PS_PIN(GPIOC_BASE, 6)    /* SMA channel 1      */
#define PIN_TRIG_GPIO2       PS_PIN(GPIOC_BASE, 7)    /* SMA channel 2      */
#define PIN_TRIG_PATTERN     PS_PIN(GPIOC_BASE, 8)    /* pattern-match header */
#define PIN_TRIG_ARM_BTN     PS_PIN(GPIOC_BASE, 9)    /* physical arm button */
#define PIN_TRIG_TARGET_CLK  PS_PIN(GPIOC_BASE, 10)   /* target clock probe */

/* Power trigger ADC */
#define PIN_TRIG_POWER_ADC   PS_PIN(GPIOC_BASE, 1)    /* ADC1_IN1 shunt     */

/* MEMS mirror (I2C1: PB8/PB9) */
#define PIN_MEMS_SCL         PS_PIN(GPIOB_BASE, 8)
#define PIN_MEMS_SDA         PS_PIN(GPIOB_BASE, 9)
#define PIN_PIEZO_Z_DAC      PS_PIN(GPIOA_BASE, 4)    /* DAC1_OUT1 (shared) */

/* SD card (SDMMC1: PC8..PC12 + PD2) */
#define PIN_SD_CMD           PS_PIN(GPIOD_BASE, 2)
#define PIN_SD_CK            PS_PIN(GPIOC_BASE, 12)
#define PIN_SD_D0            PS_PIN(GPIOC_BASE, 8)
#define PIN_SD_D1            PS_PIN(GPIOC_BASE, 9)
#define PIN_SD_D2            PS_PIN(GPIOC_BASE, 10)
#define PIN_SD_D3            PS_PIN(GPIOC_BASE, 11)
#define PIN_SD_CD            PS_PIN(GPIOC_BASE, 13)   /* card detect         */

/* BLE (UART3: PB10/PB11 + PB12 reset) */
#define PIN_BLE_TX           PS_PIN(GPIOB_BASE, 10)
#define PIN_BLE_RX           PS_PIN(GPIOB_BASE, 11)
#define PIN_BLE_RESET        PS_PIN(GPIOB_BASE, 12)
#define PIN_BLE_BOOT         PS_PIN(GPIOB_BASE, 13)

/* USB-C (USB FS: PA11/PA12) */
#define PIN_USB_DM           PS_PIN(GPIOA_BASE, 11)
#define PIN_USB_DP           PS_PIN(GPIOA_BASE, 12)

/* Safety */
#define PIN_KEY_SWITCH       PS_PIN(GPIOB_BASE, 14)   /* keyed main power   */
#define PIN_INTERLOCK        PS_PIN(GPIOB_BASE, 15)   /* enclosure interlock*/
#define PIN_ESTOP            PS_PIN(GPIOH_BASE, 0)    /* emergency stop     */

/* Status LEDs */
#define PIN_LED_STATUS       PS_PIN(GPIOE_BASE, 0)    /* green               */
#define PIN_LED_ARMED        PS_PIN(GPIOE_BASE, 1)    /* red, pulsing when armed */
#define PIN_LED_FAULT        PS_PIN(GPIOE_BASE, 2)    /* yellow              */

/* Fuel gauge (I2C1 shared) */
#define FUEL_GAUGE_ADDR      0x6C                     /* MAX17048            */

/* ─── Safety envelope (compiled-in hard limits) ───────────────────────── */
#define PS_MAX_PULSE_NS          200u     /* max pulse width, ns            */
#define PS_MIN_PULSE_NS          5u
#define PS_MAX_ENERGY_NJ         8u       /* absolute energy ceiling (nJ)   */
#define PS_MAX_DUTY_PPM          1000u    /* 0.1% max average duty          */
#define PS_MAX_SHOTS_PER_SEC     200u     /* shot rate cap                  */
#define PS_LASER_TEMP_CUTOFF_C   55u      /* TEC thermistor cutoff          */

/* ─── FPGA command opcodes (over SPI) ─────────────────────────────────── */
#define FPGA_CMD_SET_DELAY          0x01   /* 32-bit delay in 250ps units   */
#define FPGA_CMD_SET_PULSE_WIDTH    0x02   /* 32-bit width in 250ps units   */
#define FPGA_CMD_SET_PATTERN        0x03   /* 64-bit pattern + mask         */
#define FPGA_CMD_SET_TRIG_SRC       0x04   /* 0=gpio1 1=gpio2 2=pattern 3=pwr 4=man */
#define FPGA_CMD_ARM                0x05
#define FPGA_CMD_DISARM             0x06
#define FPGA_CMD_FIRE_NOW           0x07   /* software trigger              */
#define FPGA_CMD_READ_STATUS        0x08   /* returns shot count + last fault*/
#define FPGA_CMD_MEMS_GOTO          0x09   /* 16-bit x, 16-bit y            */
#define FPGA_CMD_MEMS_RASTER        0x0A   /* program a raster sequence     */
#define FPGA_CMD_MEASURE_CLK        0x0B   /* returns target clock Hz       */
#define FPGA_CMD_SET_ENERGY         0x0C   /* 16-bit driver current DAC     */

/* ─── BLE protocol opcodes (over UART3 to ESP32-C3) ───────────────────── */
#define BLE_MSG_SCAN_DESC       0x01
#define BLE_MSG_STATUS          0x02
#define BLE_MSG_DFA             0x03
#define BLE_MSG_CONTROL         0x04
#define BLE_MSG_LOG             0x05
#define BLE_MSG_ACK             0x06
#define BLE_MSG_NACK            0x07
#define BLE_MSG_HELLO           0x08

/* ─── Fault classification ────────────────────────────────────────────── */
typedef enum {
    FAULT_NO_CHANGE = 0,
    FAULT_SINGLE_BIT,
    FAULT_SINGLE_BYTE,
    FAULT_MULTI_BYTE,
    FAULT_CRASH,
    FAULT_RESET,
    FAULT_TIMEOUT
} ps_fault_class_t;

/* ─── Scan descriptor (built by app, sent over BLE) ───────────────────── */
typedef struct __attribute__((packed)) {
    uint32_t  magic;             /* 0x50534331 'PSC1'                     */
    uint8_t   trig_src;          /* 0=gpio1 1=gpio2 2=pattern 3=pwr 4=man */
    uint8_t   trig_edge;         /* 0=rising 1=falling                    */
    uint16_t  trig_pattern_lo;   /* pattern-match lower 16 bits           */
    uint16_t  trig_pattern_hi;
    uint16_t  trig_mask_lo;
    uint16_t  trig_mask_hi;
    uint16_t  power_threshold;   /* ADC counts for power trigger          */
    uint16_t  x_start;           /* µm                                    */
    uint16_t  x_stop;
    uint16_t  x_step;
    uint16_t  y_start;
    uint16_t  y_stop;
    uint16_t  y_step;
    uint32_t  delay_start_ps;    /* picoseconds after trigger             */
    uint32_t  delay_stop_ps;
    uint32_t  delay_step_ps;
    uint16_t  width_start_ns;
    uint16_t  width_stop_ns;
    uint16_t  width_step_ns;
    uint16_t  energy_start;      /* driver DAC counts                     */
    uint16_t  energy_stop;
    uint16_t  energy_step;
    uint16_t  shots_per_point;   /* repeat each point N times             */
    uint32_t  expected_hash;     /* CRC32 of expected (correct) output    */
    uint16_t  expected_len;      /* length of expected output in bytes    */
    uint8_t   dfa_mode;          /* 0=none 1=AES128_Piret                */
    uint8_t   reserved[3];
} ps_scan_desc_t;

#define PS_SCAN_MAGIC 0x50534331u

/* ─── Shot result (logged to SD + streamed over BLE) ──────────────────── */
typedef struct __attribute__((packed)) {
    uint32_t  seq;
    uint16_t  x_um;
    uint16_t  y_um;
    uint32_t  delay_ps;
    uint16_t  width_ns;
    uint16_t  energy;
    uint8_t   trig_src;
    uint8_t   fault_class;
    uint32_t  output_hash;       /* CRC32 of target output                */
    uint8_t   output[32];        /* first 32 bytes of output (for DFA)    */
    uint8_t   output_len;
    int16_t   target_clk_khz;
    uint16_t  laser_temp_c;
    uint16_t  flags;
} ps_shot_t;

/* ─── Function prototypes (implemented in main.c / drivers) ───────────── */
void  ps_board_init(void);
void  ps_led_set(uint8_t which, bool on);
void  ps_led_pulse(uint8_t which);

/* Safety — called from M4 only */
bool  ps_safety_ok(void);        /* true if interlock+key+estop+temp ok   */
void  ps_laser_kill(void);       /* hard-cut the laser (M4)               */

#ifdef __cplusplus
}
#endif
#endif /* PHOTONSTRIKE_BOARD_H */