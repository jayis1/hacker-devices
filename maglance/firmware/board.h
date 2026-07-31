/*
 * board.h — Hardware pin assignments and board configuration for MagLance
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Target MCU: STM32G474VET6 (LQFP-100)
 * Clock: 170 MHz HSE (16 MHz crystal × PLL)
 *
 * Pin assignments are for the MagLance PCB revision 1.0.
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ---- Clock Configuration ---- */
#define BOARD_HSE_FREQ_HZ       16000000UL
#define BOARD_SYSCLK_HZ         170000000UL
#define BOARD_HCLK_HZ           170000000UL
#define BOARD_PCLK1_HZ          (BOARD_HCLK_HZ / 1)   /* 170 MHz */
#define BOARD_PCLK2_HZ          (BOARD_HCLK_HZ / 1)   /* 170 MHz */
#define BOARD_HRTIM_FREQ_HZ     (BOARD_HCLK_HZ * 32UL) /* 5.44 GHz virtual */
#define BOARD_HRTIM_TICK_PS     184                   /* 184 ps resolution */

/* ---- Port base addresses (from registers.h) ---- */
#define GPIOA_BASE_ADDR   0x48020000UL
#define GPIOB_BASE_ADDR   0x48020400UL
#define GPIOC_BASE_ADDR   0x48020800UL
#define GPIOD_BASE_ADDR   0x48020C00UL
#define GPIOE_BASE_ADDR   0x48021000UL
#define GPIOF_BASE_ADDR   0x48021400UL
#define GPIOG_BASE_ADDR   0x48021800UL

/* ---- Pin Definitions ---- */
/* HRTIM Channel A — H-bridge high-side left (Q1 gate via UCC21520 #1 HO) */
#define HRTIM_HSL_PORT      GPIOA
#define HRTIM_HSL_PIN       8

/* HRTIM Channel B — H-bridge low-side left (Q2 gate via UCC21520 #1 LO) */
#define HRTIM_LSL_PORT      GPIOA
#define HRTIM_LSL_PIN       9

/* HRTIM Channel C — H-bridge high-side right (Q3 gate via UCC21520 #2 HO) */
#define HRTIM_HSR_PORT      GPIOA
#define HRTIM_HSR_PIN       10

/* HRTIM Channel D — H-bridge low-side right (Q4 gate via UCC21520 #2 LO) */
#define HRTIM_LSR_PORT      GPIOA
#define HRTIM_LSR_PIN       11

/* SPI2 — Magnetometer array (3× RM3100, separate CS) */
#define MAG_SPI_PORT        GPIOB
#define MAG_SCK_PIN         13
#define MAG_MISO_PIN        14
#define MAG_MOSI_PIN        15
#define MAG_CS0_PORT        GPIOB    /* Sensor 0 (0mm) */
#define MAG_CS0_PIN         5
#define MAG_CS1_PORT        GPIOB    /* Sensor 1 (15mm) */
#define MAG_CS1_PIN         6
#define MAG_CS2_PORT        GPIOB    /* Sensor 2 (30mm) */
#define MAG_CS2_PIN         7

/* I2C1 — OLED display (SSD1306) + EEPROM (24LC256) + ADS1115 */
#define I2C1_PORT           GPIOB
#define I2C1_SCL_PIN        8
#define I2C1_SDA_PIN        9

/* ADC1 — Coil current sense (ACS724 on channel 5) */
#define CURRENT_SENSE_PORT  GPIOA
#define CURRENT_SENSE_PIN   0
#define CURRENT_SENSE_ADC_CH 5

/* ADC2 — Battery voltage divider (channel 3) */
#define VBAT_SENSE_PORT     GPIOA
#define VBAT_SENSE_PIN      1
#define VBAT_SENSE_ADC_CH   3

/* USART1 — BLE module (nRF52840 UART bridge) */
#define BLE_UART_PORT       GPIOC
#define BLE_TX_PIN          4
#define BLE_RX_PIN          5

/* USART2 — USB CDC virtual UART (internally routed) */
/* (USB peripheral: PA11=DM, PA12=DP) */
#define USB_DM_PORT         GPIOA
#define USB_DM_PIN          11
#define USB_DP_PORT         GPIOA
#define USB_DP_PIN          12

/* TIM3 — Rotary encoder */
#define ENC_A_PORT          GPIOC
#define ENC_A_PIN           6
#define ENC_B_PORT          GPIOC
#define ENC_B_PIN           7

/* GPIO — User buttons */
#define BTN_FIRE_PORT       GPIOC
#define BTN_FIRE_PIN        10   /* Dead-man's switch: must hold to inject */
#define BTN_MODE_PORT       GPIOC
#define BTN_MODE_PIN        11   /* Cycle through operating modes */
#define BTN_SELECT_PORT    GPIOC
#define BTN_SELECT_PIN      12   /* Confirm / fire single pulse */

/* GPIO — Safety / enable */
#define SAFETY_ENABLE_PORT  GPIOD
#define SAFETY_ENABLE_PIN   2    /* Hardware safety enable; gates HRTIM output */

/* GPIO — 1-Wire (DS2431 coil tip ID) */
#define TIP_OW_PORT         GPIOA
#define TIP_OW_PIN          15

/* ---- I2C Device Addresses ---- */
#define SSD1306_I2C_ADDR    0x3C
#define EEPROM24LC256_ADDR  0x50
#define ADS1115_I2C_ADDR    0x48

/* ---- EEPROM Profile Storage ---- */
#define EEPROM_NUM_PROFILES   16
#define EEPROM_PROFILE_SIZE   64   /* bytes per profile slot */
#define EEPROM_PROFILE_BASE   0x0000
#define EEPROM_CAL_BASE       (EEPROM_PROFILE_BASE + \
                               (EEPROM_NUM_PROFILES * EEPROM_PROFILE_SIZE))

/* ---- Coil Driver Limits ---- */
#define COIL_MAX_CURRENT_MA   80000UL  /* 80 A peak */
#define COIL_MAX_DC_CURRENT_MA 8000UL  /* 8 A continuous */
#define COIL_MIN_PULSE_NS     100      /* 100 ns minimum pulse */
#define COIL_MAX_PULSE_NS     100000000UL /* 100 ms maximum pulse */
#define COIL_MAX_TEMP_C       80        /* Shutdown temperature */
#define COIL_COOLDOWN_MS      500       /* Mandatory cooldown between pulses */

/* ---- Battery Limits ---- */
#define BAT_LOW_MV            3200      /* Low battery warning */
#define BAT_CRIT_MV           3000      /* Critical — shutdown */
#define BAT_FULL_MV           4200      /* Fully charged */
#define BAT_NOMINAL_MV        3700      /* Nominal 18650 voltage */

/* ---- OLED Display ---- */
#define OLED_WIDTH            128
#define OLED_HEIGHT           64
#define OLED_PAGES            (OLED_HEIGHT / 8)

/* ---- Operating Modes ---- */
typedef enum {
    MODE_IDLE = 0,
    MODE_PULSE,
    MODE_DC,
    MODE_SWEEP,
    MODE_SENSE,
    MODE_PROFILE,
    MODE_CALIBRATE,
    MODE_ERROR,
    MODE_COUNT
} operating_mode_t;

/* ---- Field Polarity ---- */
typedef enum {
    POLARITY_NORTH = 0,   /* Current flows positive → North pole at tip */
    POLARITY_SOUTH = 1    /* Current flows negative → South pole at tip */
} polarity_t;

/* ---- Profile Structure (64 bytes, fits in one EEPROM page) ---- */
typedef struct {
    uint8_t  mode;           /* operating_mode_t */
    uint8_t  polarity;       /* polarity_t */
    uint8_t  tip_id;         /* Coil tip identifier */
    uint8_t  reserved1;      /* Alignment padding */
    uint32_t pulse_width_ns; /* Pulse width in nanoseconds */
    uint32_t pulse_count;   /* Number of pulses (0 = single) */
    uint32_t delay_us;      /* Inter-pulse delay in microseconds */
    uint32_t current_ma;    /* Target coil current in milliamps */
    uint32_t sweep_start_hz; /* Sweep start frequency (SWEEP mode) */
    uint32_t sweep_stop_hz;  /* Sweep stop frequency (SWEEP mode) */
    uint16_t sweep_steps;    /* Number of frequency steps */
    uint16_t sweep_dwell_ms; /* Dwell time per step */
    char     name[24];       /* Human-readable profile name */
    uint8_t  reserved2[8];   /* Reserved for future use */
    uint8_t  checksum;       /* Simple XOR checksum of bytes 0..55 */
} __attribute__((packed)) profile_t;

_Static_assert(sizeof(profile_t) <= EEPROM_PROFILE_SIZE,
               "Profile struct must fit in EEPROM slot");

/* ---- System Status ---- */
typedef struct {
    operating_mode_t mode;
    polarity_t       polarity;
    uint32_t         current_ma;      /* Measured coil current */
    uint32_t         target_current_ma;
    uint32_t         pulse_width_ns;
    uint32_t         pulse_count;
    uint32_t         pulse_fired;
    uint32_t         delay_us;
    uint16_t         vbat_mv;
    int16_t          temp_bridge_c;   /* H-bridge temperature */
    int16_t          temp_coil_c;     /* Coil temperature */
    int16_t          field_x[3];     /* RM3100 X readings (µT×100) */
    int16_t          field_y[3];     /* RM3100 Y readings */
    int16_t          field_z[3];     /* RM3100 Z readings */
    uint8_t          active_profile; /* Currently loaded profile slot */
    uint8_t          tip_id;         /* Connected coil tip */
    uint8_t          safety_armed;   /* Safety switch state */
    uint8_t          error_code;     /* Last error code */
} system_status_t;

/* ---- Error Codes ---- */
typedef enum {
    ERR_NONE = 0,
    ERR_OVERCURRENT,
    ERR_OVERTEMP_BRIDGE,
    ERR_OVERTEMP_COIL,
    ERR_BAT_LOW,
    ERR_BAT_CRIT,
    ERR_SAFETY_NOT_ARMED,
    ERR_NO_TIP,
    ERR_UNKNOWN_TIP,
    ERR_PROFILE_CRC,
    ERR_PROFILE_EMPTY,
    ERR_INVALID_PARAM,
    ERR_HRTIM_FAULT,
} error_code_t;

/* ---- Global Externs ---- */
extern system_status_t g_status;

#endif /* BOARD_H */