/*
 * TRACE-REAPER — Board pin/peripheral assignments & constants
 * STM32H743VI reference target
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef TRACE_REAPER_BOARD_H
#define TRACE_REAPER_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ---- Physical pin assignments (STM32H743 LQFP100 ref) ----
 * These are the port/pin macros used by the drivers. They map to
 * STM32 GPIO port letters and pin numbers; see registers.h for the
 * register base addresses.
 */
#define PIN_LED_STATUS          PB0    /* status RGB red element  */
#define PIN_LED_MODE            PB1    /* mode RGB green element  */
#define PIN_LED_ARM             PB2    /* arm    RGB blue element */
#define PIN_BTN_MODE            PC13   /* tactile: cycle mode      */
#define PIN_BTN_ARM             PC14   /* tactile: arm/disarm     */
#define PIN_BTN_DUMP            PC15   /* tactile: dump-to-SD      */
#define PIN_OLED_SCL            PB6    /* I2C1 OLED                */
#define PIN_OLED_SDA            PB7    /* I2C1 OLED                */
#define PIN_BLE_UART_TX         PA9    /* USART1 TX -> BLE module  */
#define PIN_BLE_UART_RX         PA10   /* USART1 RX <- BLE module  */
#define PIN_BLE_RST             PA8    /* BLE module reset         */
#define PIN_BLE_IRQ             PA11   /* BLE event interrupt      */
#define PIN_FPGA_SPI_NSS        PA4    /* SPI1 NSS -> FPGA         */
#define PIN_FPGA_SPI_SCK        PA5    /* SPI1 SCK                 */
#define PIN_FPGA_SPI_MOSI       PA7    /* SPI1 MOSI                */
#define PIN_FPGA_SPI_MISO       PA6    /* SPI1 MISO                */
#define PIN_FPGA_IRQ            PB10   /* FPGA -> MCU event IRQ    */
#define PIN_FPGA_DONE            PB11   /* FPGA init done            */
#define PIN_FPGA_PROG_B         PB12   /* FPGA program (active low) */
#define PIN_AFE_GAIN_A          PB13   /* AFE gain select bit 0     */
#define PIN_AFE_GAIN_B          PB14   /* AFE gain select bit 1     */
#define PIN_AFE_INPUT_SEL       PB15   /* 0=shunt 1=EM              */
#define PIN_TRIG_EXT            PC0    /* external trigger input    */
#define PIN_TRIG_LVL_EN         PC1    /* enable analog-level trig  */
#define PIN_TAMPER_ACC_INT1     PC2    /* accelerometer INT1         */
#define PIN_TAMPER_ACC_INT2     PC3    /* accelerometer INT2         */
#define PIN_TAMPER_MESH         PC4    /* case-mesh sense (GPIO)     */
#define PIN_USB_DP              PA12   /* USB-C data+               */
#define PIN_USB_DM              PA11   /* USB-C data- (alt fn)      */
#define PIN_SD_CD               PC8    /* microSD card detect       */
#define PIN_QSPI_CS             PB2_ALT /* QSPI NOR chip select     */

/* ---- Clocks ---- */
#define SYSCLK_HZ               480000000UL
#define HCLK_HZ                 240000000UL
#define APB1_CLK_HZ              120000000UL
#define APB2_CLK_HZ              120000000UL
#define ADC_CLK_HZ              250000000UL   /* sample clock; from external XO */
#define SYSTICK_HZ              1000UL

/* ---- Capacities ---- */
#define TRACE_MAX_SAMPLES       8192            /* per trace window           */
#define TRACE_RING_ENTRIES      4096            /* QSPI ring buffer slots      */
#define KEY_BYTES_AES128        16
#define KEY_BYTES_AES256        32
#define HYPOTHESES_PER_BYTE     256             /* 0..255 for a byte           */
#define AES_SBOX_ENTRIES        256
#define SESSION_ID_LEN          16              /* bytes                        */

/* ---- Per-byte correlation accumulator (in FPGA/MCU mirror) ----
 * Five running sums per (keybyte index, hypothesis value):
 *   n, sum_x, sum_xx, sum_y, sum_yy, sum_xy
 * from which Pearson rho = (n*sum_xy - sum_x*sum_y) /
 *   sqrt( (n*sum_xx - sum_x^2) * (n*sum_yy - sum_y^2) )
 */
typedef struct {
    uint32_t n;
    int64_t  sum_x;     /* sum of model value (0..8 typically, integer HW) */
    int64_t  sum_xx;
    int64_t  sum_y;     /* sum of sample value (12-bit signed) */
    int64_t  sum_yy;
    int64_t  sum_xy;
} corr_accum_t;

/* Per-key-byte table of 256 hypothesis accumulators */
typedef struct {
    corr_accum_t cells[HYPOTHESES_PER_BYTE];
} corr_byte_t;

/* ---- Session configuration (pushed from app) ---- */
typedef enum {
    CIPHER_NONE = 0,
    CIPHER_AES128,
    CIPHER_AES256,
    CIPHER_DES,
    CIPHER_PRESENT,
} cipher_id_t;

typedef enum {
    LEAK_NONE = 0,
    LEAK_HW_SBOX_OUT,    /* Hamming weight of S-box output        */
    LEAK_HW_SBOX_IN,     /* Hamming weight of S-box input         */
    LEAK_HD_SBOX_OUT,    /* Hamming distance S-box in -> out      */
    LEAK_HW_MIXCOL,      /* HW of MixColumns output byte          */
    LEAK_HW_LASTROUND,   /* HD of last-round state vs ciphertext   */
} leak_model_t;

typedef enum {
    TRIG_EXTERNAL = 0,   /* external BNC trigger                */
    TRIG_ANALOG,         /* analog level on the power trace      */
    TRIG_TEMPLATE,       /* cross-correlation against reference   */
    TRIG_NONE,           /* triggerless rolling capture           */
} trigger_src_t;

typedef enum {
    INPUT_SHUNT = 0,
    INPUT_EM,
} input_src_t;

typedef enum {
    GAIN_0DB = 0,
    GAIN_14DB,
    GAIN_28DB,
} gain_t;

typedef struct {
    cipher_id_t  cipher;
    leak_model_t model;
    uint32_t     target_traces;   /* how many traces to acquire         */
    uint16_t     window_samples; /* samples around trigger (<=8192)    */
    int16_t      trig_threshold; /* analog trigger level (12-bit signed)*/
    trigger_src_t trig_src;
    input_src_t  input;
    gain_t       gain;
    uint8_t      known_pt[16];   /* for fixed-plaintext attack         */
    uint8_t      pt_random;       /* 1 = use random PT (CPA standard)   */
    uint8_t      decimate;        /* 1,2,4,8 ... 0 = none              */
    uint8_t      session_id[SESSION_ID_LEN];
    char         label[32];      /* human label, null-terminated       */
} session_cfg_t;

/* ---- Session result ---- */
typedef struct {
    uint8_t  best_key[KEY_BYTES_AES256];   /* recovered key bytes   */
    int16_t  best_hyp[KEY_BYTES_AES256];  /* hypothesis per byte    */
    float    rho[KEY_BYTES_AES256];       /* correlation of best    */
    uint8_t  recovered_bytes;             /* how many exceed thresh */
    uint32_t traces_used;
    uint32_t started_ms;
    uint32_t finished_ms;
    uint8_t  confidence_ok;               /* all bytes > threshold? */
} session_result_t;

#define RHO_THRESHOLD        0.5f   /* per-byte acceptance threshold */

/* ---- Device modes (cooperative scheduler) ---- */
typedef enum {
    MODE_IDLE = 0,
    MODE_CONFIGURED,
    MODE_ARMED,
    MODE_ACQUIRING,
    MODE_PROCESSING,
    MODE_DONE,
    MODE_TAMPERED,
    MODE_FAULT,
} device_mode_t;

/* ---- Globals exposed by main.c ---- */
void board_init(void);
void board_set_mode(device_mode_t m);
device_mode_t board_get_mode(void);
uint32_t board_uptime_s(void);
uint32_t board_millis(void);

#endif /* TRACE_REAPER_BOARD_H */