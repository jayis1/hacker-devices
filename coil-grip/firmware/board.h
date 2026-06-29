/*
 * board.h — Hardware board definitions for CoilGrip
 * Wireless Power Transfer (Qi) Manipulation, MITM & Covert Channel Platform
 *
 * Author:  jayis1
 * Copyright (C) 2026 jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef COILGRIP_BOARD_H
#define COILGRIP_BOARD_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define AUTHOR                  "jayis1"
#define DEVICE_FW_VERSION       "1.0.0"
#define DEVICE_NAME             "CoilGrip"

/* ---- STM32H743 base addresses (subset we use) --------------------------- */
#define HSEM_BASE               0x48002400U
#define RCC_BASE                0x58024400U
#define PWR_BASE                0x58024800U
#define GPIOA_BASE              0x58020000U
#define GPIOB_BASE              0x58020400U
#define GPIOC_BASE              0x58020800U
#define GPIOD_BASE              0x58020C00U
#define GPIOE_BASE              0x58021000U
#define GPIOH_BASE              0x58021C00U
#define HRTIM1_BASE             0x40016800U
#define SPI1_BASE               0x40013000U
#define SPI2_BASE               0x40015000U
#define I2C1_BASE               0x40005400U
#define I2C4_BASE               0x58001C00U
#define USART3_BASE             0x40004800U
#define DMA1_BASE               0x40020000U
#define DMA1_STR0_BASE          (DMA1_BASE + 0x0010U)
#define ADC1_BASE               0x40022000U
#define DMAMUX1_BASE            0x40020800U
#define EXTI_BASE               0x56000800U
#define TIM2_BASE               0x40000000U
#define TIM3_BASE               0x40000400U

/* ---- Pin assignments ---------------------------------------------------- */
/* PA8  = HRTIM Master/ChA PWM output (Qi TX PWM)                          */
/* PA9  = USART3_TX (debug)                                                */
/* PA10 = USART3_RX (debug)                                                */
/* PB2  = QSPI CS (SPI NOR)                                                */
/* PB6  = I2C1_SCL (BQ51013B telemetry)                                    */
/* PB7  = I2C1_SDA                                                         */
/* PB10 = SPI2_SCK (MWCT1011)                                              */
/* PB12 = SPI2_NSS (MWCT1011 CS)                                           */
/* PB14 = SPI2_MISO                                                        */
/* PB15 = SPI2_MOSI                                                        */
/* PC4  = ADC1_INP4 (current-sense amplifier INA240 output)                */
/* PC5  = ADC1_INP5 (coil-voltage envelope ADL5511)                         */
/* PC8  = SDMMC1_D0                                                        */
/* PC9  = SDMMC1_D1                                                        */
/* PC10 = SDMMC1_D2                                                        */
/* PC11 = SDMMC1_D3                                                        */
/* PC12 = SDMMC1_CK                                                        */
/* PD2  = SDMMC1_CMD                                                       */
/* PE0  = FPGA_INT (iCE40 done/IRQ)                                        */
/* PE1  = FPGA_CS (SPI1 to iCE40)                                          */
/* PE2  = FPGA_RESET                                                       */
/* PE5  = BLE_UART_TX (USART2 alt)                                         */
/* PE6  = BLE_UART_RX                                                       */
/* PH0  = OSC_IN                                                           */
/* PH1  = OSC_OUT                                                          */
/* PA0  = LOAD_MOD_DRV (ADG849 select)                                     */
/* PA1  = GLITCH_FET_EN (H-bridge enable override)                         */
/* PA2  = TRIG_IN0                                                         */
/* PA3  = TRIG_OUT0                                                        */
/* PA4  = TRIG_IN1                                                         */
/* PA5  = TRIG_OUT1                                                        */
/* PA6  = TMP117_ALERT                                                     */
/* PA7  = USB-C VBUS sense                                                 */
/* PA15 = OLED_DC                                                         */
/* PB5  = OLED_CS                                                          */
/* PB8  = OLED_SCL (SPI3 alt)                                              */
/* PB9  = OLED_SDA (SPI3 alt, bit-banged)                                  */

#define PIN_PA0                0
#define PIN_PA1                1
#define PIN_PA2                2
#define PIN_PA3                3
#define PIN_PA4                4
#define PIN_PA5                5
#define PIN_PA6                6
#define PIN_PA7                7
#define PIN_PA8                8
#define PIN_PA9                9
#define PIN_PA10               10
#define PIN_PA15               15
#define PIN_PB2                (16+2)
#define PIN_PB5                (16+5)
#define PIN_PB6                (16+6)
#define PIN_PB7                (16+7)
#define PIN_PB8                (16+8)
#define PIN_PB9                (16+9)
#define PIN_PB10               (16+10)
#define PIN_PB12               (16+12)
#define PIN_PB14               (16+14)
#define PIN_PB15               (16+15)
#define PIN_PC4                (32+4)
#define PIN_PC5                (32+5)
#define PIN_PC8                (32+8)
#define PIN_PD2                (48+2)
#define PIN_PE0                (64+0)
#define PIN_PE1                (64+1)
#define PIN_PE2                (64+2)
#define PIN_PE5                (64+5)
#define PIN_PE6                (64+6)

/* ---- Electrical limits (safety) ----------------------------------------- */
#define COIL_TEMP_SHUTDOWN_C   60      /* hard thermal shutdown             */
#define COIL_TEMP_WARN_C        50      /* warn operator                    */
#define TX_POWER_LIMIT_W        15.0f   /* never exceed Qi BPP-class         */
#define GLITCH_MIN_WIDTH_US     1       /* minimum glitch width              */
#define GLITCH_MAX_WIDTH_US     65000   /* maximum glitch width              */
#define FOD_SAFETY_UNLOCK_NONE  0       /* FOD manipulation locked by default */

/* ---- Clock constants (defined in board_init.c, exposed here) ----------- */
#define HSE_HZ                  25000000U
#define PLL1_VCO_HZ             960000000U
#define SYSCLK_HZ               480000000U
#define HCLK_HZ                 240000000U
#define APB_HZ                  120000000U
#define HRTIM_CLK_HZ            240000000U

/* ---- Profiler constants ------------------------------------------------ */
#define PROFILER_FS_HZ         256000U

/* ---- Mode enumeration --------------------------------------------------- */
typedef enum {
    MODE_IDLE = 0,
    MODE_SNIFFER,        /* passive capture of an adjacent Qi link           */
    MODE_MITM,           /* inline MITM with packet rewriting                */
    MODE_PROFILE,        /* power profiling / device fingerprinting         */
    MODE_GLITCH,         /* contactless power-glitch injection               */
    MODE_EXFIL,          /* covert-channel exfil/inject                      */
    MODE_FOD_RESEARCH    /* FOD calibration manipulation (safety-locked)     */
} coil_mode_t;

/* ---- Glitch trigger sources -------------------------------------------- */
typedef enum {
    TRIG_NONE = 0,
    TRIG_TIMER,
    TRIG_QI_PKT_MATCH,
    TRIG_EXT_GPIO,
    TRIG_ADC_THRESHOLD
} glitch_trig_t;

/* ---- Forward declarations for drivers ----------------------------------- */
void board_init(void);

/* qi_tx.c */
int  qi_tx_start(uint32_t freq_hz, uint8_t duty_pct);
void qi_tx_stop(void);
int  qi_tx_send_packet(const uint8_t *pkt, uint8_t len);
void qi_tx_set_power_pct(uint8_t pct);
int  qi_tx_fod_override(int32_t offset_mw);
int  qi_tx_get_power_mw(void);
int  qi_tx_get_state(void);
bool qi_tx_is_running(void);
uint32_t qi_tx_get_freq(void);
uint8_t qi_tx_get_duty(void);

/* qi_rx.c */
int  qi_rx_init(void);
int  qi_rx_read_telemetry(uint16_t *vrect_mv, uint16_t *iout_ma,
                          int8_t *temp_c, uint8_t *ctrl_state);
int  qi_rx_send_packet(const uint8_t *pkt, uint8_t len);
void qi_rx_set_load_modulation(bool on);

/* power_profiler.c */
int  profiler_start(uint32_t sample_rate_hz);
void profiler_stop(void);
int  profiler_get_sample(int32_t *current_ua, int32_t *voltage_mv);
int  profiler_classify_state(uint8_t *state_out, float *confidence);
int  profiler_add_fingerprint(const char *name, const uint8_t *sig, uint16_t len);
int  profiler_match_fingerprint(const uint8_t *sig, uint16_t len,
                                char *name_out, uint16_t name_sz);

/* glitch_engine.c */
typedef struct {
    glitch_trig_t trigger;
    uint32_t      delay_us;
    uint32_t      width_us;
    uint32_t      repeat;
    int32_t       ramp_us;     /* per-repetition width increment            */
    uint32_t      period_ms;    /* for TRIG_TIMER                            */
    uint8_t       trig_pin;     /* for TRIG_EXT_GPIO                         */
    int32_t       adc_threshold_ua; /* for TRIG_ADC_THRESHOLD                */
    uint16_t      qi_pkt_match;     /* for TRIG_QI_PKT_MATCH                 */
} glitch_params_t;
int  glitch_engine_arm(const glitch_params_t *p);
int  glitch_engine_fire(void);
void glitch_engine_disarm(void);
uint32_t glitch_engine_get_count(void);
bool glitch_engine_is_armed(void);
bool glitch_engine_uses_fpga(void);

/* load_modulator.c */
int  load_mod_init(void);
int  load_mod_send(const uint8_t *data, uint16_t len);
int  load_mod_recv(uint8_t *buf, uint16_t buf_sz, uint16_t *out_len);
uint16_t load_mod_crc8(const uint8_t *data, uint16_t len);
uint8_t load_mod_tx_pending(void);
uint8_t load_mod_rx_pending(void);

/* fod.c */
int  fod_calibrate(void);
int  fod_get_status(int32_t *offset_mw, int8_t *coil_temp_c);
int  fod_set_safety_unlock(uint32_t code);
bool fod_safety_unlocked(void);
void fod_thermal_interlock_check(void);
bool fod_thermal_shutdown_active(void);
int8_t fod_get_coil_temp(void);

/* ble_uart.c */
int  ble_init(void);
int  ble_send(const uint8_t *data, uint16_t len);
int  ble_recv(uint8_t *buf, uint16_t buf_sz, uint16_t *out_len, uint32_t timeout_ms);
int  ble_set_key(const uint8_t key[32]);
bool ble_is_connected(void);

/* usb_cdc.c */
int  usb_cdc_init(void);
int  usb_cdc_write(const uint8_t *data, uint16_t len);
int  usb_cdc_read(uint8_t *buf, uint16_t buf_sz);
void usb_cdc_poll(void);

/* oled.c */
int  oled_init(void);
void oled_clear(void);
void oled_draw_str(uint8_t col, uint8_t row, const char *s);
void oled_draw_status(const char *mode, const char *state, int8_t coil_temp_c,
                       float current_a, float power_w);

/* sdcard.c */
int  sdcard_init(void);
int  sdcard_open_log(const char *fname);
int  sdcard_write_log(const uint8_t *data, uint16_t len);
int  sdcard_close_log(void);
bool sdcard_present(void);
uint32_t sdcard_log_size(void);

/* config.c */
int  config_load(void);
int  config_save(void);
int  config_get(const char *key, char *val_out, uint16_t val_sz);
int  config_set(const char *key, const char *val);

/* Global state */
extern volatile coil_mode_t g_mode;
extern volatile int8_t       g_coil_temp_c;
extern volatile float        g_current_a;
extern volatile float        g_power_w;

#endif /* COILGRIP_BOARD_H */