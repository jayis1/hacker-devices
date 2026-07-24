/*
 * board.h — Hardware board definitions for the Joule-Phantom
 *           smart-battery SMBus/PMBus MITM implant.
 *
 * Target MCU : STM32L432KCU6 (Ultra-low-power Cortex-M4, QFN32)
 * Author     : jayis1
 * License    : GPL-2.0
 *
 * The Joule-Phantom sits inline between a host system (laptop / drone /
 * IoT controller) and its smart battery pack.  It bridges the SMBus /
 * PMBus link (I2C-derived) while transparently capturing, modifying and
 * injecting SBS telemetry frames.
 *
 *   HOST  <--[ SMBus/PMBus ]-->  Joule-Phantom  <--[ SMBus/PMBus ]-->  BATTERY
 *
 * Power is drawn parasitically from the 3V3 rail present on most smart
 * battery connectors (pin VCC/SMBus pull-up) or from an optional micro
 * coin cell on the interposer flex PCB.
 */

#ifndef JOULE_PHANTOM_BOARD_H
#define JOULE_PHANTOM_BOARD_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/*  MCU core / clocking                                                */
/* ------------------------------------------------------------------ */
#define BOARD_MCU_NAME          "STM32L432KCU6"
#define BOARD_CPU_CORTEX_M4
#define BOARD_HSI_CLOCK_HZ      16000000UL   /* internal 16 MHz RC     */
#define BOARD_LSE_CLOCK_HZ      32768UL      /* 32.768 kHz crystal     */
#define BOARD_SYSCLK_HZ         80000000UL   /* 80 MHz from PLL MSI    */
#define BOARD_APB1_HZ           80000000UL   /* PCLK1 = SYSCLK         */
#define BOARD_APB2_HZ           80000000UL   /* PCLK2 = SYSCLK         */

/* ------------------------------------------------------------------ */
/*  Pin assignments (QFN32)                                           */
/* ------------------------------------------------------------------ */
/* Host-side SMBus (I2C1) — the side facing the laptop/host controller */
#define PIN_HOST_SMB_CLK        6     /* PA6  -> I2C1_SCL (host)      */
#define PIN_HOST_SMB_DAT        7     /* PA7  -> I2C1_SDA (host)      */
#define I2C_HOST                I2C1

/* Battery-side SMBus (I2C2) — the side facing the smart battery       */
#define PIN_BATT_SMB_CLK        13    /* PB13 -> I2C2_SCL (battery)   */
#define PIN_BATT_SMB_DAT        14    /* PB14 -> I2C2_SDA (battery)   */
#define I2C_BATT                I2C2

/* SMBus Alert / battery present detect                                */
#define PIN_SMBALERT_HOST       1     /* PA1  -> SMBALERT# from host  */
#define PIN_BATTERY_PRESENT     0     /* PA0  -> BATT_PRSNT# signal   */
#define PIN_THERMISTOR          8     /* PA8  -> analog thermistor    */

/* Status LED (covert: only lit during operator window)               */
#define PIN_STATUS_LED          15    /* PB15 -> status LED (gated)   */

/* BLE back-haul UART to ESP32-C3 co-processor                        */
#define PIN_BLE_USART_TX        2     /* PA2  -> USART2_TX -> ESP32   */
#define PIN_BLE_USART_RX        3     /* PA3  -> USART2_RX <- ESP32   */
#define BLE_USART               USART2
#define BLE_BAUDRATE            115200UL

/* Optional trigger / glitch output to battery side                   */
#define PIN_GLITCH_OUT          5     /* PA5  -> glitch/EN gate       */

/* ADC for thermistor and voltage sense                                */
#define ADC_VBAT_SENSE          4     /* ADC channel 4 (PA4)          */
#define ADC_THERM_SENSE         8     /* ADC channel 8 (internal ch)  */

/* ------------------------------------------------------------------ */
/*  SMBus / PMBus electrical                                           */
/* ------------------------------------------------------------------ */
#define SMBUS_CLOCK_HZ          100000UL     /* SMBus 2.0 = 10-100 kHz */
#define SMBUS_TIMEOUT_MS        35           /* SMBus spec: 25-35 ms   */
#define SMBUS_PULLUP_OHM        10000        /* 10 kOhm pull-ups       */
#define PMBUS_CLOCK_HZ          400000UL     /* PMBus up to 1 MHz      */

/* SBS Smart Battery Data Specification standard commands              */
#define SBS_MANUFACTURER_ACCESS  0x00
#define SBS_AT_RATE              0x01
#define SBS_AT_RATE_TIME_TO_FULL 0x02
#define SBS_AT_RATE_TIME_TO_EMPTY 0x03
#define SBS_AT_RATE_OK           0x04
#define SBS_TEMPERATURE          0x05   /* 0.1 K units                */
#define SBS_VOLTAGE              0x06   /* mV                          */
#define SBS_CURRENT              0x07   /* mA                          */
#define SBS_AVG_CURRENT          0x08   /* mA                          */
#define SBS_MAX_ERROR            0x09
#define SBS_REL_STATE_OF_CHARGE  0x0D   /* %                           */
#define SBS_ABS_STATE_OF_CHARGE  0x0E   /* %                           */
#define SBS_REM_CAPACITY         0x0F   /* mAh                         */
#define SBS_FULL_CHARGE_CAPACITY 0x10   /* mAh                         */
#define SBS_RUN_TIME_TO_EMPTY    0x11   /* min                         */
#define SBS_AVG_TIME_TO_EMPTY   0x12   /* min                         */
#define SBS_AVG_TIME_TO_FULL     0x13   /* min                         */
#define SBS_CHARGING_CURRENT     0x14
#define SBS_CHARGING_VOLTAGE     0x15
#define SBS_BATTERY_STATUS       0x16
#define SBS_CYCLE_COUNT          0x17
#define SBS_DESIGN_CAPACITY      0x18
#define SBS_MANUFACTURE_DATE     0x1B
#define SBS_SERIAL_NUMBER        0x1C
#define SBS_MANUFACTURER_NAME    0x20   /* string                     */
#define SBS_DEVICE_NAME          0x21   /* string                     */
#define SBS_DEVICE_CHEMISTRY     0x22   /* string                     */
#define SBS_MANUFACTURER_DATA    0x23   /* opaque                     */
#define SBS_MANUFACTURER_INFO    0x25
#define SBS_FUNCTION_MODE        0x3C
#define SBS_BATTERY_MODE         0x03   /* (some packs)               */

/* SBS BatteryStatus bit flags                                         */
#define BSTAT_OVER_CHARGED_ALARM  0x8000
#define BSTAT_TERMINATE_CHARGE    0x4000
#define BSTAT_OVER_TEMP_ALARM     0x2000
#define BSTAT_TERMINATE_DISCHARGE 0x1000
#define BSTAT_OVER_CURRENT_ALARM  0x0800
#define BSTAT_OVERLOAD_ALARM      0x0400
#define BSTAT_REMAINING_CAP_ALARM 0x0200
#define BSTAT_REMAINING_TIME_ALARM 0x0100
#define BSTAT_INITIALIZED         0x0080
#define BSTAT_DISCHARGING         0x0040
#define BSTAT_FULLY_CHARGED       0x0020
#define BSTAT_FULLY_DISCHARGED    0x0010

/* PMBus extension commands (subset used by charger ICs)               */
#define PMBUS_PAGE                0x00
#define PMBUS_OPERATION           0x01
#define PMBUS_CLEAR_FAULTS        0x03
#define PMBUS_VOUT_MODE           0x20
#define PMBUS_VOUT_COMMAND        0x21
#define PMBUS_IOUT_OC_FAULT_LIMIT 0x46
#define PMBUS_STATUS_BYTE         0x78
#define PMBUS_STATUS_WORD         0x79
#define PMBUS_READ_VIN            0x88
#define PMBUS_READ_IIN            0x89
#define PMBUS_READ_VOUT           0x8B
#define PMBUS_READ_IOUT           0x8C
#define PMBUS_READ_TEMPERATURE_1  0x8D
#define PMBUS_FAN_COMMAND_1       0x3B

/* ------------------------------------------------------------------ */
/*  Operational limits (safety interlocks — author jayis1)             */
/* ------------------------------------------------------------------ */
#define SAFE_TEMP_MAX_K          3330   /* 60.0 C -> 3330 dK; abort   */
#define SAFE_VOLT_MAX_MV         13000  /* 13.000 V hard ceiling       */
#define SAFE_CURR_MAX_MA         8000   /* 8.000 A hard ceiling        */
#define GLITCH_PULSE_MAX_US      500    /* glitch pulse ceiling        */

/* Ring-buffer / capture sizes                                        */
#define CAPTURE_RING_BYTES       (32 * 1024)
#define CAPTURE_FRAME_MAX        256
#define MITM_RULE_MAX            32

/* BLE framing                                                         */
#define BLE_FRAME_SOF            0xAA
#define BLE_FRAME_EOF            0x55
#define BLE_MTU                  128

/* LED behaviour — covert; off by default                              */
#define LED_HEARTBEAT_MS         4096

/* ------------------------------------------------------------------ */
/*  Exported board API                                                 */
/* ------------------------------------------------------------------ */
void     board_init(void);
void     board_led_set(uint8_t on);
void     board_led_toggle(void);
uint32_t board_millis(void);
void     board_delay_ms(uint32_t ms);
uint16_t board_adc_read(uint8_t ch);
int      board_battery_present(void);
int      board_smbalert_host(void);
void     board_glitch_pulse(uint32_t us);

#ifdef __cplusplus
}
#endif

#endif /* JOULE_PHANTOM_BOARD_H */