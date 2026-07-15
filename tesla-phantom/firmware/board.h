/*
 * tesla-phantom — board.h
 * Board-level definitions, pin assignments, and types for the
 * Electromagnetic Fault Injection & Magnetic Side-Channel Analysis Platform.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef TESLA_PHANTOM_BOARD_H
#define TESLA_PHANTOM_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ── Crystal / clock ──────────────────────────────────────────────── */
#define HSE_VALUE          25000000UL   /* 25 MHz external crystal   */
#define HSI_VALUE          64000000UL   /* 64 MHz internal           */
#define SYSCLK_FREQ        480000000UL  /* 480 MHz system clock      */
#define APB1_FREQ          120000000UL  /* 120 MHz APB1              */
#define APB2_FREQ          240000000UL  /* 240 MHz APB2              */

/* ── Pin assignments (STM32H743 LQFP-100) ─────────────────────────── */

/* SPI1 → iCE40 FPGA (timing/pulse control) */
#define FPGA_SPI           SPI1
#define FPGA_SCK_PIN       5    /* PA5  = SPI1_SCK   */
#define FPGA_MISO_PIN      6    /* PA6  = SPI1_MISO  */
#define FPGA_MOSI_PIN      7    /* PA7  = SPI1_MOSI  */
#define FPGA_CS_PORT       GPIOB
#define FPGA_CS_PIN        0    /* PB0  = FPGA_CS_N  */
#define FPGA_IRQ_PIN       1    /* PB1  = FPGA_IRQ   */
#define FPGA_CDONE_PIN     2    /* PB2  = FPGA_CDONE */
#define FPGA_RESET_PIN     3    /* PB3  = FPGA_RESET_N (SWO remapped) */

/* SPI4 → microSD card */
#define SD_SPI             SPI4
#define SD_SCK_PORT        GPIOE
#define SD_SCK_PIN         2    /* PE2 = SPI4_SCK  */
#define SD_MISO_PIN        5    /* PE5 = SPI4_MISO */
#define SD_MOSI_PIN        6    /* PE6 = SPI4_MOSI */
#define SD_CS_PORT         GPIOE
#define SD_CS_PIN          4    /* PE4 = SD_CS_N   */
#define SD_DETECT_PIN      3    /* PE3 = SD_DETECT */

/* SPI2 → AD7606 ADC + AD8331 VGA + LTC1564 filter */
#define ADC_SPI            SPI2
#define ADC_SCK_PORT       GPIOI
#define ADC_SCK_PIN        1    /* PI1  = SPI2_SCK  (remapped)        */
#define ADC_MISO_PORT      GPIOI
#define ADC_MISO_PIN       2    /* PI2  = SPI2_MISO */
#define ADC_MOSI_PORT      GPIOI
#define ADC_MOSI_PIN       3    /* PI3  = SPI2_MOSI */
#define ADC_CS_PORT        GPIOB
#define ADC_CS_PIN         4    /* PB4  = ADC_CS_N  */
#define ADC_CONVST_PIN     5    /* PB5  = ADC_CONVST */
#define ADC_BUSY_PIN       6    /* PB6  = ADC_BUSY  */
#define ADC_RESET_PIN      7    /* PB7  = ADC_RESET_N */
#define ADC_RANGE_PIN      8    /* PB8  = ADC_RANGE */
#define VGA_CS_PORT        GPIOB
#define VGA_CS1_PIN        9    /* PB9  = VGA_CS1_N  */
#define VGA_CS2_PIN        10   /* PB10 = VGA_CS2_N  */
#define FILTER_CS_PIN      11   /* PB11 = FILTER_CS_N */

/* USART3 → NINA-B306 BLE module */
#define BLE_UART           USART3
#define BLE_TX_PORT        GPIOD
#define BLE_TX_PIN         8    /* PD8  = USART3_TX */
#define BLE_RX_PORT        GPIOD
#define BLE_RX_PIN         9    /* PD9  = USART3_RX */
#define BLE_CTS_PIN        10   /* PD10 = BLE_CTS   */
#define BLE_RTS_PIN        11   /* PD11 = BLE_RTS   */
#define BLE_RESET_PORT     GPIOD
#define BLE_RESET_PIN      12   /* PD12 = BLE_RESET_N */
#define BLE_BOOT_PIN       13   /* PD13 = BLE_BOOT  */

/* USART2 → TMC2209 stepper drivers (daisy-chained UART) */
#define STEPPER_UART       USART2
#define STEPPER_TX_PORT    GPIOD
#define STEPPER_TX_PIN     4    /* PD4 = USART2_TX */
#define STEPPER_RX_PORT    GPIOD
#define STEPPER_RX_PIN     5    /* PD5 = USART2_RX */

/* I2C1 → SSD1306 OLED + battery fuel gauge */
#define OLED_I2C           I2C1
#define OLED_SCL_PORT      GPIOB
#define OLED_SCL_PIN       8    /* PB8  = I2C1_SCL  (remapped) */
#define OLED_SDA_PORT      GPIOB
#define OLED_SDA_PIN       9    /* PB9  = I2C1_SDA  (remapped) */
#define BATT_FUEL_ADDR     0x0B /* MAX17048 fuel gauge I2C addr  */

/* DAC1 → HV charge pump voltage setpoint */
#define HV_DAC             DAC1
#define HV_DAC_CHANNEL     1
#define HV_DAC_PORT        GPIOA
#define HV_DAC_PIN         4    /* PA4 = DAC1_OUT1 */

/* Trigger input (external SMA) */
#define TRIG_EXT_PORT      GPIOC
#define TRIG_EXT_PIN       4    /* PC4  = TRIG_EXT (EXTI4) */
#define TRIG_OUT_PORT      GPIOC
#define TRIG_OUT_PIN       5    /* PC5  = TRIG_OUT (to FPGA) */

/* Stepper limit switches (optical end-stops) */
#define LIM_X_PORT        GPIOC
#define LIM_X_PIN         6    /* PC6 = LIMIT_X */
#define LIM_Y_PORT        GPIOC
#define LIM_Y_PIN         7    /* PC7 = LIMIT_Y */
#define LIM_Z_PORT        GPIOC
#define LIM_Z_PIN         8    /* PC8 = LIMIT_Z */

/* LEDs and buttons */
#define LED_STATUS_PORT    GPIOB
#define LED_STATUS_PIN     14   /* PB14 = LED_STATUS  */
#define LED_ARMED_PORT     GPIOB
#define LED_ARMED_PIN      15   /* PB15 = LED_ARMED   */
#define LED_CHARGE_PORT    GPIOD
#define LED_CHARGE_PIN     14   /* PD14 = LED_CHARGE  */
#define BTN_ARM_PORT       GPIOD
#define BTN_ARM_PIN        15   /* PD15 = BTN_ARM     */

/* USB */
#define USB_DM_PORT        GPIOA
#define USB_DM_PIN         11   /* PA11 = USB_DM */
#define USB_DP_PORT        GPIOA
#define USB_DP_PIN         12   /* PA12 = USB_DP */

/* FMC → external SRAM (IS66WV51216BLL, 1 MB) */
/* Uses FMC bank 1, region 0, PSRAM/SRAM mode */

/* QSPI → external flash (W25Q128, 16 MB) — profiles & firmware OTA */
#define QSPI_CLK_PORT      GPIOB
#define QSPI_CLK_PIN       6    /* PB6  = QSPI_CLK  */
#define QSPI_NCS_PORT      GPIOB
#define QSPI_NCS_PIN       7    /* PB7  = QSPI_NCS  */
#define QSPI_IO0_PORT      GPIOC
#define QSPI_IO0_PIN       9    /* PC9  = QSPI_IO0  */
#define QSPI_IO1_PORT      GPIOC
#define QSPI_IO1_PIN       10   /* PC10 = QSPI_IO1  */
#define QSPI_IO2_PORT      GPIOC
#define QSPI_IO2_PIN       11   /* PC11 = QSPI_IO2  */
#define QSPI_IO3_PORT      GPIOC
#define QSPI_IO3_PIN       12   /* PC12 = QSPI_IO3  */

/* ── FPGA register map (via SPI1) ─────────────────────────────────── */
#define FPGA_REG_TRIGGER_MODE    0x00  /* 0=ext, 1=timer, 2=magnetic */
#define FPGA_REG_PULSE_WIDTH     0x01  /* pulse width in 200MHz ticks */
#define FPGA_REG_PULSE_DELAY     0x02  /* delay after trigger (ticks) */
#define FPGA_REG_PULSE_COUNT     0x03  /* number of pulses per event  */
#define FPGA_REG_PULSE_INTERVAL  0x04  /* interval between pulses     */
#define FPGA_REG_HV_FIRE         0x05  /* write 1 to fire Marx bank   */
#define FPGA_REG_ADC_SCHED       0x06  /* ADC sample scheduling config */
#define FPGA_REG_MAG_THRESHOLD   0x07  /* magnetic trigger threshold  */
#define FPGA_REG_MAG_PATTERN_LO  0x08  /* pattern match mask low 32b  */
#define FPGA_REG_MAG_PATTERN_HI  0x09  /* pattern match mask high 32b */
#define FPGA_REG_STATUS          0x0A  /* read: status flags          */
#define FPGA_REG_VERSION         0x0B  /* read: FPGA bitstream version */
#define FPGA_REG_SCRATCH         0x0F  /* loopback test               */

/* FPGA status flags */
#define FPGA_ST_ARMED          (1U << 0)
#define FPGA_ST_CHARGED        (1U << 1)
#define FPGA_ST_FIRED          (1U << 2)
#define FPGA_ST_ADC_READY      (1U << 3)
#define FPGA_ST_TRIG_PEND      (1U << 4)
#define FPGA_ST_MAG_MATCH      (1U << 5)
#define FPGA_ST_FAULT          (1U << 6)

/* ── TMC2209 stepper driver ───────────────────────────────────────── */
#define TMC_REG_GCONF          0x00
#define TMC_REG_GSTAT          0x01
#define TMC_REG_IHOLD_IRUN     0x10
#define TMC_REG_CHOPCONF       0x6C
#define TMC_REG_COOLCONF       0x70
#define TMC_REG_DRVSTATUS      0x6F
#define TMC_REG_SGTHRS         0x74  /* StallGuard4 threshold */
#define TMC_REG_XDIRECT        0x2D  /* Direct mode */

#define TMC_AXIS_X             0
#define TMC_AXIS_Y             1
#define TMC_AXIS_Z             2

/* ── AD7606 ADC ───────────────────────────────────────────────────── */
#define ADC_CHANNELS           6
#define ADC_MAX_RATE_KSPS      800
#define ADC_DEFAULT_RANGE_5V   1   /* ±5V range by default */

/* ── SCA trace buffer ─────────────────────────────────────────────── */
#define SDRAM_BASE_ADDR        0x60000000UL  /* FMC bank 1 region 0 */
#define SDRAM_SIZE             (1024UL * 1024UL) /* 1 MB */
#define SCA_MAX_SAMPLES        (SDRAM_SIZE / (ADC_CHANNELS * sizeof(int16_t)))

/* ── Types ────────────────────────────────────────────────────────── */

/* Device operational state */
typedef enum {
    ST_BOOT = 0,
    ST_IDLE,
    ST_ARMED,
    ST_CHARGING,
    ST_PULSE,
   ST_CAPTURE,
    ST_ANALYZE,
    ST_SCAN,
    ST_CALIBRATE,
    ST_FAULT,
    ST_SHUTDOWN
} device_state_t;

/* Operating mode */
typedef enum {
    MODE_EMFI = 0,     /* Electromagnetic fault injection */
    MODE_SCA,          /* Magnetic side-channel capture   */
    MODE_SCAN,         /* Automated cartography           */
    MODE_MONITOR       /* Passive monitoring only         */
} op_mode_t;

/* Trigger source */
typedef enum {
    TRIG_EXTERNAL = 0,  /* SMA input from target           */
    TRIG_INTERNAL,      /* MCU/FPGA timer                  */
    TRIG_MAGNETIC,      /* Magnetic signature pattern      */
    TRIG_MANUAL         /* Software / button               */
} trigger_source_t;

/* EMFI pulse parameters */
typedef struct {
    uint16_t voltage;       /* 50–400 V (DAC setpoint)     */
    uint16_t width_ns;      /* 5–200 ns                    */
    uint32_t delay_ns;      /* delay after trigger         */
    uint16_t count;         /* pulses per trigger event    */
    uint32_t interval_us;   /* interval between pulses     */
} emfi_params_t;

/* SCA capture parameters */
typedef struct {
    uint32_t samples;       /* samples per channel         */
    uint16_t rate_khz;      /* sample rate (kHz)           */
    uint8_t  gain_db;       /* VGA gain (0–48 dB)          */
    uint16_t filter_cutoff; /* LPF cutoff (Hz)             */
    uint8_t  range_5v;      /* 1=±5V, 0=±10V               */
} sca_params_t;

/* Scan grid parameters */
typedef struct {
    float    x0, y0;        /* start position (mm)         */
    float    x1, y1;        /* end position (mm)           */
    float    step_mm;       /* grid step size              */
    float    z_height;      /* probe height above target   */
    emfi_params_t pulse;    /* pulse params for each point */
    uint8_t  pulses_per_pt; /* pulses per grid point       */
} scan_params_t;

/* Probe position */
typedef struct {
    float x_mm;
    float y_mm;
    float z_mm;
} position_t;

/* Fault classification result */
typedef enum {
    FAULT_NONE = 0,     /* no observable effect           */
    FAULT_DATA_CORRUPT, /* output differs but no crash    */
    FAULT_SKIP,         /* instruction skipped (branch)   */
    FAULT_CRASH,        /* target crashed/reset           */
    FAULT_UNKNOWN       /* response uninterpretable       */
} fault_class_t;

/* Scan point result */
typedef struct {
    position_t  pos;
    fault_class_t fault;
    uint8_t     pulse_idx;
    uint16_t    hv_actual;
    uint32_t    timestamp;
} scan_point_t;

/* Burst/capture record */
typedef struct {
    uint32_t    timestamp;
    uint32_t    samples;
    uint16_t    rate_khz;
    uint8_t     gain_db;
    uint16_t    trigger_offset;
    int16_t     peak_magnitude;
    fault_class_t fault;
} burst_record_t;

/* Device configuration (persisted to QSPI flash) */
typedef struct {
    uint32_t    magic;
    uint16_t    version;
    emfi_params_t  emfi;
    sca_params_t   sca;
    trigger_source_t trig_src;
    uint8_t     safety_interlock;
    uint8_t     auto_discharge;
    uint8_t     ble_psk[16];
    uint32_t    crc32;
} config_t;

#define CONFIG_MAGIC   0x7E510001UL
#define CONFIG_VERSION 1

/* ── Global state (externed for drivers) ──────────────────────────── */
extern volatile device_state_t   g_state;
extern volatile op_mode_t        g_mode;
extern volatile trigger_source_t g_trig_src;
extern emfi_params_t             g_emfi;
extern sca_params_t              g_sca;
extern scan_params_t             g_scan;
extern position_t                g_position;
extern config_t                  g_config;
extern volatile uint8_t          g_fault_flag;
extern volatile uint16_t         g_hv_voltage;
extern volatile uint8_t          g_battery_pct;
extern volatile uint32_t         g_uptime_ms;

/* ── Function prototypes (drivers) ────────────────────────────────── */

/* board_init.c */
void board_init_clock(void);
void board_init_gpio(void);
void board_init_fmc_sram(void);
void board_init_qspi(void);
void board_init_exti(void);
void board_init_dma(void);
void board_init_timers(void);

/* fpga_spi.c */
int  fpga_init(void);
int  fpga_write_reg(uint8_t reg, uint32_t value);
int  fpga_read_reg(uint8_t reg, uint32_t *value);
int  fpga_configure_pulse(uint16_t width_ns, uint32_t delay_ns,
                           uint16_t count, uint32_t interval_us);
int  fpga_set_trigger_mode(trigger_source_t src);
int  fpga_arm(void);
int  fpga_disarm(void);
int  fpga_fire(void);
uint8_t fpga_get_status(void);
int  fpga_set_mag_trigger(uint16_t threshold, uint64_t pattern);

/* hv_pulse.c */
int  hv_init(void);
int  hv_set_voltage(uint16_t voltage);
int  hv_charge(void);
int  hv_discharge(void);
int  hv_is_charged(void);
uint16_t hv_read_actual(void);

/* stepper.c */
int  stepper_init(void);
int  stepper_move_to(uint8_t axis, float pos_mm);
int  stepper_move_xyz(float x, float y, float z);
int  stepper_home(uint8_t axis);
int  stepper_home_all(void);
int  stepper_stop(uint8_t axis);
int  stepper_set_current(uint8_t axis, uint16_t mA_run, uint16_t mA_hold);
int  stepper_enable(uint8_t axis, uint8_t en);
float stepper_get_position(uint8_t axis);
int  stepper_set_speed(uint8_t axis, uint16_t speed_mm_s);

/* fluxgate.c */
int  fluxgate_init(void);
int  fluxgate_read(float *field_mt);
int  fluxgate_read_raw(int16_t *raw);
int  fluxgate_set_offset(int16_t offset);
int  fluxgate_calibrate(void);

/* adc_capture.c */
int  adc_init(void);
int  adc_start_capture(int16_t *buf, uint32_t samples);
int  adc_read_burst(int16_t *buf, uint32_t samples);
void adc_set_range(int range_5v);
void adc_reset(void);
int  adc_set_oversampling(uint8_t ratio);
int  adc_set_filter_cutoff(uint16_t cutoff_hz);
int  adc_set_vga_gain(uint8_t gain_db);

/* sca_engine.c */
int  sca_init(void);
int  sca_process_trace(int16_t *raw, uint32_t samples, float *features);
void sca_bandpass_filter(float *signal, int len, int low_hz, int high_hz);
void sca_compute_envelope(float *signal, int len, float *env);
float sca_correlate(float *trace, float *model, int len);
int  sca_detect_leakage(float *trace, int len, float *score);
void sca_compute_fft(float *signal, int len, float *mag_out);

/* trigger.c */
int  trigger_init(void);
int  trigger_set_source(trigger_source_t src);
int  trigger_arm(void);
int  trigger_disarm(void);
int  trigger_wait(uint32_t timeout_ms);
void trigger_ext_callback(void);

/* scan_controller.c */
int  scan_start(const scan_params_t *params);
int  scan_stop(void);
int  scan_step(void);
int  scan_is_complete(void);
int  scan_get_result(uint32_t idx, scan_point_t *result);
uint32_t scan_get_count(void);
int  scan_export_map(void *buf, uint32_t maxlen);

/* ble_uart.c */
int  ble_init(void);
int  ble_send_status(uint8_t state, uint8_t battery, const char *info);
int  ble_send_trace_chunk(const void *data, uint16_t len);
int  ble_send_scan_map(const void *data, uint16_t len);
int  ble_recv_cmd(uint8_t *cmd, void *payload, uint16_t *len);
int  ble_authenticate(const uint8_t *psk, uint32_t len);
void ble_process(void);

/* usb_cdc.c */
int  usb_init(void);
int  usb_send(const void *data, uint16_t len);
int  usb_recv(void *buf, uint16_t maxlen);
void usb_process(void);

/* oled.c */
void oled_init(void);
void oled_clear(void);
void oled_draw_string(int x, int y, const char *str, int size);
void oled_draw_status(int armed, int battery, const char *mode_str);
void oled_draw_position(float x, float y, float z, uint16_t hv);
void oled_refresh(void);

/* sdcard.c */
int  sdcard_init(void);
int  sdcard_log_burst(burst_record_t *rec);
int  sdcard_log_scan_point(scan_point_t *pt);
int  sdcard_log_session(const char *text, uint32_t len);
int  sdcard_save_trace(const char *name, int16_t *data, uint32_t samples);

/* config.c */
int  config_load(void);
int  config_save(void);
int  config_set_defaults(void);
int  config_set_emfi(const emfi_params_t *params);
int  config_set_sca(const sca_params_t *params);

/* ── Utility ──────────────────────────────────────────────────────── */
void delay_ms(uint32_t ms);
void delay_us(uint32_t us);
uint32_t millis(void);
uint32_t crc32(const void *data, size_t len);

#endif /* TESLA_PHANTOM_BOARD_H */