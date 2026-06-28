/*
 * board.h - Hardware pin mappings and board configuration
 * for Thermal Phantom thermal fault injection device
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPLv3
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

/* ============================================================
 * MCU: STM32H743VIT6 (Cortex-M7, 480 MHz, BGA-100)
 * ============================================================ */

/* System clock configuration */
#define BOARD_HSE_FREQ_HZ       8000000UL   /* 8 MHz external crystal */
#define BOARD_SYSCLK_FREQ_HZ    480000000UL /* 480 MHz max */
#define BOARD_APB1_FREQ_HZ      120000000UL
#define BOARD_APB2_FREQ_HZ      240000000UL
#define BOARD_APB4_FREQ_HZ      120000000UL

/* ============================================================
 * Pin Assignments (STM32H743 Port mapping)
 * ============================================================ */

/* --- I2C Bus 1 (MLX90614 + SSD1306 OLED) --- */
#define MLX_I2C                 I2C1
#define MLX_I2C_SCL_PORT        GPIOB
#define MLX_I2C_SCL_PIN         8
#define MLX_I2C_SDA_PORT        GPIOB
#define MLX_I2C_SDA_PIN         9
#define MLX_I2C_ADDR            0x5A   /* MLX90614 default I2C address */

#define OLED_I2C                I2C1
#define OLED_I2C_ADDR           0x3C   /* SSD1306 128x64 OLED */
#define OLED_WIDTH              128
#define OLED_HEIGHT             64

/* --- 1-Wire (DS18B20 contact temperature sensor) --- */
#define DS18B20_PORT            GPIOC
#define DS18B20_PIN             10
#define DS18B20_GPIO            GPIOC->ODR
#define DS18B20_PIN_MASK        (1U << 10)

/* --- TEC H-Bridge Driver (DRV8873) --- */
#define TEC_TIM                 TIM1   /* Advanced timer for high-res PWM */
#define TEC_PWM_CHANNEL         1      /* TIM1 CH1 */
#define TEC_PWM_FREQ_HZ         20000  /* 20 kHz PWM */
#define TEC_IN1_PORT            GPIOE
#define TEC_IN1_PIN             9
#define TEC_IN2_PORT            GPIOE
#define TEC_IN2_PIN             11
#define TEC_NSLEEP_PORT          GPIOE
#define TEC_NSLEEP_PIN          13
#define TEC_NFAULT_PORT         GPIOE
#define TEC_NFAULT_PIN          15
#define TEC_IPROPI_PORT         GPIOA
#define TEC_IPROPI_PIN          0      /* ADC1_IN0 */
#define TEC_IPROPI_ADC          ADC1
#define TEC_IPROPI_CHANNEL      0
#define TEC_MAX_CURRENT_MA      10000  /* 10A software limit */

/* --- Laser Driver (LM359 constant-current) --- */
#define LASER_TIM               TIM2
#define LASER_PWM_CHANNEL       1      /* TIM2 CH1 */
#define LASER_PWM_FREQ_HZ       1000   /* 1 kHz base, gated by pulse timer */
#define LASER_EN_PORT           GPIOD
#define LASER_EN_PIN            12
#define LASER_SHUTTER_PORT      GPIOD
#define LASER_SHUTTER_PIN       13     /* HIGH = open, LOW = closed (fail-safe) */
#define LASER_INTERLOCK_PORT    GPIOD
#define LASER_INTERLOCK_PIN     14     /* LOW = interlock engaged (safe) */
#define LASER_MAX_POWER_PCT     100
#define LASER_MAX_PULSE_MS      500    /* Hardware limit: 500ms max pulse */
#define LASER_POWER_MW          1000   /* 1W at 100% */

/* --- USB CDC (FT4232H) --- */
#define USB_UART                USART3
#define USB_UART_BAUD           921600
#define USB_UART_TX_PORT        GPIOB
#define USB_UART_TX_PIN         10
#define USB_UART_RX_PORT        GPIOB
#define USB_UART_RX_PIN         11
#define USB_UART_IRQn           USART3_IRQn

/* --- BLE UART (nRF52840) --- */
#define BLE_UART                UART4
#define BLE_UART_BAUD           115200
#define BLE_UART_TX_PORT        GPIOA
#define BLE_UART_TX_PIN         0
#define BLE_UART_RX_PORT        GPIOA
#define BLE_UART_RX_PIN         1
#define BLE_UART_IRQn           UART4_IRQn
#define BLE_RST_PORT            GPIOA
#define BLE_RST_PIN             2

/* --- Trigger I/O (SMA connectors) --- */
#define TRIG_IN_PORT            GPIOC
#define TRIG_IN_PIN             6      /* EXTI6 */
#define TRIG_IN_EXTI_LINE       6
#define TRIG_OUT_PORT           GPIOC
#define TRIG_OUT_PIN            7
#define TRIG_OUT_TIM            TIM3   /* For precise output pulse timing */
#define TRIG_OUT_CHANNEL        1

/* --- Safety / Emergency Stop --- */
#define ESTOP_PORT              GPIOC
#define ESTOP_PIN               13     /* User button / E-Stop */
#define ESTOP_EXTI_LINE         13

/* --- Overtemperature Hardware Cutoff --- */
#define OTP comparator          COMP1
#define OTP_REF_VOLTAGE_MV      3300   /* ~130°C threshold via NTC divider */
#define OTP_CUTOFF_PORT         GPIOB
#define OTP_CUTOFF_PIN          5      /* Drives TEC enable MOSFET gate */
#define OTP_THRESHOLD_C        130

/* --- Battery Monitoring --- */
#define BAT_ADC                 ADC1
#define BAT_ADC_CHANNEL         5      /* ADC1_IN5 = PA5 */
#define BAT_DIVIDER_RATIO       4.0f   /* 14.8V / 4 = 3.7V at ADC */
#define BAT_FULL_MV             16800  /* 4S fully charged */
#define BAT_EMPTY_MV            12000  /* 4S discharged */
#define BAT_WARN_PCT            20

/* --- Status LEDs --- */
#define LED_STATUS_PORT         GPIOB
#define LED_STATUS_PIN          0      /* Green: system OK */
#define LED_ARM_PORT            GPIOB
#define LED_ARM_PIN             1      /* Red: armed/hazard */
#define LED_BLE_PORT            GPIOB
#define LED_BLE_PIN             2      /* Blue: BLE connected */

/* ============================================================
 * Timing Constants
 * ============================================================ */
#define THERMAL_LOOP_PERIOD_MS      100    /* Outer PID loop: 100ms */
#define THERMAL_FAST_LOOP_PERIOD_MS 1      /* Inner PID loop: 1ms */
#define TEMP_SENSOR_READ_MS         50     /* MLX90614 read interval */
#define BATTERY_READ_MS             5000  /* Battery check interval */
#define SAFETY_CHECK_MS             10    /* Safety loop interval */
#define COMM_HEARTBEAT_MS           1000  /* Status broadcast interval */

/* ============================================================
 * Thermal Profiles
 * ============================================================ */
#define MAX_PROFILE_STEPS       32
#define MAX_PROFILE_NAME_LEN    32

typedef enum {
    PROFILE_STEP_HEAT,
    PROFILE_STEP_COOL,
    PROFILE_STEP_HOLD,
    PROFILE_STEP_LASER,
    PROFILE_STEP_TRIGGER,
    PROFILE_STEP_WAIT_TRIGGER,
    PROFILE_STEP_LOOP,
    PROFILE_STEP_END
} profile_step_type_t;

typedef struct {
    profile_step_type_t type;
    float target_temp;      /* °C */
    uint32_t duration_ms;   /* Duration for this step */
    float ramp_rate;        /* °C/sec, 0 = max */
    float laser_power;      /* % 0-100 */
    uint32_t laser_duration; /* ms */
    uint16_t loop_count;    /* For LOOP steps */
} profile_step_t;

typedef struct {
    char name[MAX_PROFILE_NAME_LEN];
    profile_step_t steps[MAX_PROFILE_STEPS];
    uint8_t step_count;
} thermal_profile_t;

/* ============================================================
 * System States
 * ============================================================ */
typedef enum {
    STATE_INIT = 0,
    STATE_IDLE,
    STATE_HEATING,
    STATE_COOLING,
    STATE_HOLDING,
    STATE_LASER_ARMED,
    STATE_LASER_FIRING,
    STATE_TRIGGER_ARMED,
    STATE_PROFILE_RUNNING,
    STATE_EMERGENCY_STOP,
    STATE_FAULT
} system_state_t;

/* ============================================================
 * Safety Limits
 * ============================================================ */
#define SAFETY_MAX_TEMP_C          120.0f
#define SAFETY_MAX_TEMP_HARDWARE   130.0f
#define SAFETY_MIN_TEMP_C          -15.0f
#define SAFETY_MAX_LASER_TEMP_C   60.0f  /* Don't fire laser if ambient >60°C */
#define SAFETY_BATTERY_MIN_MV      12000
#define SAFETY_WATCHDOG_TIMEOUT_MS 500

/* ============================================================
 * Default PID Parameters
 * ============================================================ */
#define PID_DEFAULT_KP            8.0f
#define PID_DEFAULT_KI            0.5f
#define PID_DEFAULT_KD            2.0f
#define PID_OUTPUT_MAX            100.0f   /* % PWM */
#define PID_OUTPUT_MIN            -100.0f  /* Negative = cool */
#define PID_INTEGRAL_LIMIT        50.0f

/* ============================================================
 * Communication Protocol
 * ============================================================ */
#define COMM_SYNC_BYTE             0xAA
#define COMM_MAX_PAYLOAD           256
#define COMM_HEADER_SIZE           4      /* SYNC + CMD + LEN(2) */
#define COMM_CRC_SIZE              2
#define COMM_MAX_PACKET_SIZE       (COMM_HEADER_SIZE + COMM_MAX_PAYLOAD + COMM_CRC_SIZE)

#define CMD_SET_TEMP               0x01
#define CMD_SET_MODE               0x02
#define CMD_ARM_TRIGGER            0x03
#define CMD_FIRE_LASER             0x04
#define CMD_READ_TEMP              0x05
#define CMD_LOAD_PROFILE           0x06
#define CMD_START_PROFILE          0x07
#define CMD_STOP                   0x08
#define CMD_GET_STATUS             0x09
#define CMD_SET_PID                0x0A
#define CMD_DISARM                 0x0B
#define CMD_LASER_SHUTTER_OPEN     0x0C
#define CMD_LASER_SHUTTER_CLOSE    0x0D
#define CMD_READ_BATTERY           0x0E
#define CMD_SET_TRIGGER_CONFIG     0x0F
#define CMD_GET_VERSION             0x10

#define RESP_OK                    0x80
#define RESP_ERROR                 0x81
#define RESP_DATA                  0x82
#define RESP_STATUS                0x83
#define RESP_LOG                   0x84

#define ERR_NONE                   0x00
#define ERR_CRC                    0x01
#define ERR_LENGTH                 0x02
#define ERR_UNKNOWN_CMD            0x03
#define ERR_NOT_READY              0x04
#define ERR_SAFETY                 0x05
#define ERR_PROFILE_FULL           0x06
#define ERR_PROFILE_INVALID        0x07
#define ERR_LASER_INTERLOCK        0x08
#define ERR_TEMP_OUT_OF_RANGE      0x09

#endif /* BOARD_H */