/*
 * eddy-phantom — board.h
 * Board pin definitions and hardware constants for the
 * Electromagnetic Emanation Keyboard Reconnaissance Device.
 *
 * MCU: STM32H743VIT6 (Cortex-M7 @ 480 MHz)
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef EDDY_PHANTOM_BOARD_H
#define EDDY_PHANTOM_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ── MCU identity ─────────────────────────────────────────────── */
#define BOARD_MCU_NAME      "STM32H743VIT6"
#define BOARD_MCU_CORE_CLK  480000000UL   /* 480 MHz core clock     */
#define BOARD_MCU_HSE_CLK   25000000UL    /* 25 MHz external crystal */
#define BOARD_MCU_HSI_CLK   64000000UL    /* 64 MHz internal HSI    */

/* ── Base addresses (STM32H743) ───────────────────────────────── */
#define PERIPH_BASE         0x40000000UL
#define AHB1_BASE           (PERIPH_BASE + 0x00020000UL)
#define AHB4_BASE           (PERIPH_BASE + 0x00380000UL)
#define APB1_BASE           (PERIPH_BASE + 0x00010000UL)
#define APB2_BASE           (PERIPH_BASE + 0x00040000UL)

#define GPIOA_BASE          (AHB4_BASE + 0x0000)
#define GPIOB_BASE          (AHB4_BASE + 0x0400)
#define GPIOC_BASE          (AHB4_BASE + 0x0800)
#define GPIOD_BASE          (AHB4_BASE + 0x0C00)
#define GPIOE_BASE          (AHB4_BASE + 0x1000)
#define GPIOF_BASE          (AHB4_BASE + 0x1400)
#define GPIOG_BASE          (AHB4_BASE + 0x1800)
#define GPIOH_BASE          (AHB4_BASE + 0x1C00)

#define SPI1_BASE           (APB2_BASE + 0x3000)
#define SPI2_BASE           (APB1_BASE + 0x3800)
#define SPI4_BASE           (APB2_BASE + 0x4000)
#define USART3_BASE         (APB1_BASE + 0x4800)
#define USART2_BASE         (APB1_BASE + 0x4000)

#define FMC_BASE            0xA0000000UL
#define FMC_BANK1_SDRAM     (FMC_BASE + 0xC0000000UL)  /* SDRAM mapped at 0xC0000000 */
#define QSPI_BASE           0xA0001000UL
#define QSPI_MM_BASE        0x90000000UL  /* QSPI memory-mapped region */

#define DMA1_BASE           (AHB1_BASE + 0x0000)
#define DMA1_STREAM0        (DMA1_BASE + 0x010)
#define DMA1_STREAM1        (DMA1_BASE + 0x028)
#define DMA1_STREAM2        (DMA1_BASE + 0x040)
#define DMA1_STREAM3        (DMA1_BASE + 0x058)
#define DMA1_STREAM4        (DMA1_BASE + 0x070)

#define RCC_BASE            (AHB1_BASE + 0x4400)
#define PWR_BASE            (AHB1_BASE + 0x4800)
#define SYSCFG_BASE         (APB2_BASE + 0x3800)
#define NVIC_BASE           0xE000E100UL
#define SCB_BASE            0xE000ED00UL
#define STK_BASE            0xE000E010UL  /* SysTick */

/* ── Pin assignments ──────────────────────────────────────────── */

/*
 * SPI1 — AD7606C ADC interface (4 probe channels + 2 aux)
 *   PA5  = SPI1_SCK
 *   PA6  = SPI1_MISO
 *   PA7  = SPI1_MOSI (for register config of AD7606C)
 *   PB0  = AD7606_CSA  (chip select, active low)
 *   PB1  = AD7606_RST  (reset)
 *   PB2  = AD7606_CONVSTA (conversion start)
 *   PB10 = AD7606_BUSY  (busy signal, falling-edge IRQ)
 *   PB11 = AD7606_RANGE (range select: 0 = ±10V, 1 = ±5V)
 *   PC0  = AD7606_OS0   (oversampling bit 0)
 *   PC1  = AD7606_OS1   (oversampling bit 1)
 *   PC2  = AD7606_OS2   (oversampling bit 2)
 */
#define ADC_SPI             SPI1_BASE
#define ADC_SCK_PIN         5    /* GPIOA */
#define ADC_MISO_PIN        6    /* GPIOA */
#define ADC_MOSI_PIN        7    /* GPIOA */
#define ADC_CS_PORT         GPIOB_BASE
#define ADC_CS_PIN          0
#define ADC_RST_PORT        GPIOB_BASE
#define ADC_RST_PIN         1
#define ADC_CONVST_PORT     GPIOB_BASE
#define ADC_CONVST_PIN      2
#define ADC_BUSY_PORT       GPIOB_BASE
#define ADC_BUSY_PIN        10
#define ADC_RANGE_PORT      GPIOB_BASE
#define ADC_RANGE_PIN       11
#define ADC_OS0_PORT        GPIOC_BASE
#define ADC_OS0_PIN         0
#define ADC_OS1_PORT        GPIOC_BASE
#define ADC_OS1_PIN         1
#define ADC_OS2_PORT        GPIOC_BASE
#define ADC_OS2_PIN         2

/*
 * SPI2 — QSPI flash (W25Q128) — actually using QSPI peripheral
 *   PB6  = QSPI_CLK
 *   PB7  = QSPI_NCS
 *   PC9  = QSPI_IO0 (MOSI)
 *   PC10 = QSPI_IO1 (MISO)
 *   PC11 = QSPI_IO2 (WP)
 *   PC12 = QSPI_IO3 (HOLD)
 */

/*
 * USART3 — BLE module (NINA-B306)
 *   PD8  = USART3_TX
 *   PD9  = USART3_RX
 *   PD10 = BLE_CTS
 *   PD11 = BLE_RTS
 *   PD12 = BLE_RESET
 *   PD13 = BLE_BOOT (boot mode select)
 */
#define BLE_UART            USART3_BASE
#define BLE_TX_PIN          8    /* GPIOD */
#define BLE_RX_PIN          9    /* GPIOD */
#define BLE_CTS_PIN         10   /* GPIOD */
#define BLE_RTS_PIN         11   /* GPIOD */
#define BLE_RESET_PORT      GPIOD_BASE
#define BLE_RESET_PIN       12
#define BLE_BOOT_PORT       GPIOD_BASE
#define BLE_BOOT_PIN        13

/*
 * SPI4 — microSD card (FatFS)
 *   PE2  = SPI4_SCK
 *   PE5  = SPI4_MISO
 *   PE6  = SPI4_MOSI
 *   PE4  = SD_CS
 *   PE3  = SD_DETECT (card present, active low)
 */
#define SD_SPI              SPI4_BASE
#define SD_SCK_PIN          2    /* GPIOE */
#define SD_MISO_PIN         5    /* GPIOE */
#define SD_MOSI_PIN         6    /* GPIOE */
#define SD_CS_PORT          GPIOE_BASE
#define SD_CS_PIN           4
#define SD_DETECT_PORT      GPIOE_BASE
#define SD_DETECT_PIN       3

/*
 * VGA gain control — 4-channel AD8331 via SPI DAC (MCP4922 x2)
 *   Uses SPI2 (separate from QSPI) — actually on GPIOF
 *   PF7  = VGAIN_SPI_SCK
 *   PF8  = VGAIN_SPI_MISO (unused but wired)
 *   PF9  = VGAIN_SPI_MOSI
 *   PF6  = VGA_DAC_CS1 (channels 1&2)
 *   PF10 = VGA_DAC_CS2 (channels 3&4)
 *   PC3  = VGA_LNA_BYPASS (hardware bypass for high-signal scenarios)
 */
#define VGA_SPI             SPI2_BASE
#define VGA_SCK_PIN         7    /* GPIOF */
#define VGA_MOSI_PIN        9    /* GPIOF */
#define VGA_CS1_PORT        GPIOF_BASE
#define VGA_CS1_PIN         6
#define VGA_CS2_PORT        GPIOF_BASE
#define VGA_CS2_PIN         10
#define VGA_LNA_BYPASS_PORT GPIOC_BASE
#define VGA_LNA_BYPASS_PIN  3

/*
 * LTC1564 anti-alias filter cutoff control via SPI pot (MCP41010)
 *   PF11 = FILTER_SPI_SCK
 *   PF12 = FILTER_SPI_MOSI
 *   PF13 = FILTER_SPI_CS
 */
#define FILTER_SPI_SCK_PORT GPIOF_BASE
#define FILTER_SPI_SCK_PIN  11
#define FILTER_SPI_MOSI_PORT GPIOF_BASE
#define FILTER_SPI_MOSI_PIN 12
#define FILTER_SPI_CS_PORT  GPIOF_BASE
#define FILTER_SPI_CS_PIN   13

/*
 * OLED display (SSD1306 via I2C — bit-banged on GPIO)
 *   PB8  = I2C_SCL
 *   PB9  = I2C_SDA
 */
#define OLED_SCL_PORT       GPIOB_BASE
#define OLED_SCL_PIN        8
#define OLED_SDA_PORT       GPIOB_BASE
#define OLED_SDA_PIN        9

/*
 * Trigger comparator (TLV3201) output
 *   PC4  = TRIG_OUT (falling-edge EXTI interrupt)
 */
#define TRIG_PORT           GPIOC_BASE
#define TRIG_PIN            4

/*
 * LEDs and button
 *   PB12 = LED_STATUS (green)
 *   PB13 = LED_CAPTURE (red)
 *   PB14 = BTN_ARM (tactile button, active low with pull-up)
 *   PB15 = LED_CHARGE (amber, charge status)
 */
#define LED_STATUS_PORT     GPIOB_BASE
#define LED_STATUS_PIN      12
#define LED_CAPTURE_PORT    GPIOB_BASE
#define LED_CAPTURE_PIN     13
#define LED_CHARGE_PORT     GPIOB_BASE
#define LED_CHARGE_PIN      15
#define BTN_ARM_PORT        GPIOB_BASE
#define BTN_ARM_PIN         14

/*
 * USB-C (charging only — no data to target)
 *   PA11 = USB_DM (used for CDC during firmware update mode only)
 *   PA12 = USB_DP
 */
#define USB_DM_PIN          11   /* GPIOA */
#define USB_DP_PIN          12   /* GPIOA */

/*
 * MAX17048 fuel gauge (I2C, shares OLED bus)
 *   Address: 0x36
 */

/* ── Analog front-end constants ───────────────────────────────── */
#define ADC_SAMPLE_RATE_HZ  1000000UL   /* 1 MSPS per channel    */
#define ADC_CHANNELS        4            /* 4 probe channels      */
#define ADC_BITS            16
#define ADC_RANGE_MV        5000         /* ±5V range             */
#define BURST_SAMPLES       2048         /* samples per burst      */
#define BURST_BYTES         (BURST_SAMPLES * ADC_CHANNELS * 2) /* 16 KB */
#define SDRAM_BURST_SLOTS   2000         /* 32 MB / 16 KB          */

/* ── DSP constants ────────────────────────────────────────────── */
#define DSP_FIR_TAPS        64
#define DSP_MFCC_COEFFS     13
#define DSP_FEATURE_DIM     19  /* 13 MFCC + 4 probe ratio + 2 timing */
#define DSP_FFT_SIZE        256
#define DSP_BANDPASS_LOW_HZ 50000UL    /* 50 kHz  */
#define DSP_BANDPASS_HIGH_HZ 350000UL  /* 350 kHz */

/* ── Classifier constants ─────────────────────────────────────── */
#define CLS_LAYER1_IN       19
#define CLS_LAYER1_OUT      64
#define CLS_LAYER2_OUT      32
#define CLS_MAX_SCANCODES   128
#define CLS_CONF_THRESHOLD  70   /* 0.70 * 100 */
#define CLS_WEIGHTS_SIZE    (CLS_LAYER1_IN*CLS_LAYER1_OUT + \
                             CLS_LAYER1_OUT*CLS_LAYER2_OUT + \
                             CLS_LAYER2_OUT*CLS_MAX_SCANCODES)

/* ── BLE C2 protocol ──────────────────────────────────────────── */
#define BLE_PSKEY_LEN       32     /* pre-shared key length   */
#define BLE_MAX_PAYLOAD     180    /* BLE MTU - 3 byte header  */
#define BLE_CMD_ARM         0x01
#define BLE_CMD_DISARM      0x02
#define BLE_CMD_SET_PROFILE 0x03
#define BLE_CMD_SET_THRESH  0x04
#define BLE_CMD_REQ_BURST   0x05
#define BLE_CMD_CAL_START   0x06
#define BLE_CMD_CAL_KEY     0x07
#define BLE_CMD_CAL_FINISH  0x08
#define BLE_CMD_STATUS      0x09
#define BLE_CMD_AUTH        0x0A
#define BLE_CMD_FW_UPDATE   0x0B

/* ── Operating states ─────────────────────────────────────────── */
typedef enum {
    STATE_BOOT = 0,
    STATE_IDLE,
    STATE_ARMED,
    STATE_CAPTURE,       /* burst captured, processing */
    STATE_CALIBRATE,
    STATE_ERROR
} device_state_t;

/* ── Profile structures ───────────────────────────────────────── */
#define PROFILE_NAME_LEN   32
#define PROFILE_MAX_KEYS   128

typedef struct {
    char     name[PROFILE_NAME_LEN];
    uint16_t controller_id;     /* e.g., 0x8229 for Holtek HT82K629A */
    uint16_t num_keys;
    uint16_t scancodes[PROFILE_MAX_KEYS];
    /* Per-key reference feature vectors stored in QSPI */
    uint32_t ref_offset;        /* offset into QSPI for ref features */
    uint32_t ref_size;
} keyboard_profile_t;

/* ── Burst capture structure ──────────────────────────────────── */
typedef struct {
    uint32_t timestamp_ms;      /* capture timestamp */
    uint16_t trigger_channel;   /* which probe triggered */
    uint16_t vga_gain[4];       /* gain settings at capture time */
    int16_t  samples[BURST_SAMPLES * ADC_CHANNELS]; /* interleaved */
    uint8_t  classified;
    uint8_t  scancode;          /* predicted scancode */
    uint8_t  confidence;        /* 0-100 */
    uint8_t  reserved;
} burst_record_t;

/* ── Function prototypes (extern) ─────────────────────────────── */
void board_init(void);
void board_delay_ms(uint32_t ms);
uint32_t board_millis(void);

#endif /* EDDY_PHANTOM_BOARD_H */