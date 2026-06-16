/**
 * @file board.h
 * @brief CAN Creeper board pin definitions and hardware configuration
 *
 * Complete pin mapping for nRF52840-QIAA on CAN Creeper Rev A PCB.
 * All pins defined with their primary function and alternate functions.
 *
 * Board: CAN Creeper Rev A
 * MCU:   nRF52840-QIAA-R7 (Cortex-M4F @ 64 MHz, 1MB Flash, 256KB RAM)
 * Date:  2026-06-16
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * CLOCK CONFIGURATION
 *===========================================================================*/

#define BOARD_HFCLK_FREQ_HZ         64000000UL   /* 64 MHz (32 MHz crystal × PLL) */
#define BOARD_LFCLK_FREQ_HZ         32768UL      /* 32.768 kHz crystal */
#define BOARD_TIMESTAMP_FREQ_HZ     1000000UL    /* 1 MHz for µs timestamps */

/*===========================================================================
 * PIN DEFINITIONS — PORT 0
 *===========================================================================*/

/* HF Crystal (32 MHz) */
#define BOARD_PIN_XL1               0    /* P0.00 — 32 MHz crystal input */
#define BOARD_PIN_XL2               1    /* P0.01 — 32 MHz crystal output */

/* Analog inputs (unused by default) */
#define BOARD_PIN_AIN0              2    /* P0.02 — ADC input 0 */
#define BOARD_PIN_AIN1              3    /* P0.03 — ADC input 1 */
#define BOARD_PIN_AIN2              4    /* P0.04 — Battery voltage sense (VBAT/2 divider) */
#define BOARD_PIN_AIN3              5    /* P0.05 — ADC input 3 */

/* Unused GPIO */
#define BOARD_PIN_P006              6    /* P0.06 — NC */
#define BOARD_PIN_P007              7    /* P0.07 — NC */
#define BOARD_PIN_P008              8    /* P0.08 — NC */

/* NFC pins (unused) */
#define BOARD_PIN_NFC1              9    /* P0.09 — NFC antenna 1 (NC) */
#define BOARD_PIN_NFC2              10   /* P0.10 — NFC antenna 2 (NC) */

/* SPI1 (CAN Channel 1) */
#define BOARD_PIN_SPI1_MOSI         11   /* P0.11 — SPIM1 MOSI → MCP2518FD #1 SDI */
#define BOARD_PIN_SPI1_MISO         12   /* P0.12 — SPIM1 MISO → MCP2518FD #1 SDO */
#define BOARD_PIN_QSPI_CSN_PSRAM   13   /* P0.13 — QSPI CS# for APS6404L PSRAM */
#define BOARD_PIN_SPI1_SCK          14   /* P0.14 — SPIM1 SCK → MCP2518FD #1 SCK */
#define BOARD_PIN_CAN1_INT          15   /* P0.15 — MCP2518FD #1 nINT (GPIO input, sense high) */
#define BOARD_PIN_CAN0_INT          16   /* P0.16 — MCP2518FD #0 nINT (GPIO input, sense high) */
#define BOARD_PIN_SPI0_CSN          17   /* P0.17 — SPIM0 CSN → MCP2518FD #0 nCS */
#define BOARD_PIN_nRESET            18   /* P0.18 — Configurable reset (100k pull-up) */
#define BOARD_PIN_QSPI_CSN_FLASH   19   /* P0.19 — QSPI CS# for W25Q128JV Flash */
#define BOARD_PIN_QSPI_IO0          20   /* P0.20 — QSPI IO0 / D0 */
#define BOARD_PIN_QSPI_IO1          21   /* P0.21 — QSPI IO1 / D1 */
#define BOARD_PIN_SPI1_CSN          22   /* P0.22 — SPIM1 CSN → MCP2518FD #1 nCS */
#define BOARD_PIN_QSPI_IO2          23   /* P0.23 — QSPI IO2 / D2 */
#define BOARD_PIN_QSPI_IO3          24   /* P0.24 — QSPI IO3 / D3 */
#define BOARD_PIN_QSPI_CLK          25   /* P0.25 — QSPI SCK */
#define BOARD_PIN_I2C_SDA           26   /* P0.26 — TWIM0 SDA → SSD1306 OLED */
#define BOARD_PIN_I2C_SCL           27   /* P0.27 — TWIM0 SCL → SSD1306 OLED */
#define BOARD_PIN_SPI0_MISO         28   /* P0.28 — SPIM0 MISO → MCP2518FD #0 SDO */
#define BOARD_PIN_SPI0_MOSI         29   /* P0.29 — SPIM0 MOSI → MCP2518FD #0 SDI */
#define BOARD_PIN_SPI0_SCK          30   /* P0.30 — SPIM0 SCK → MCP2518FD #0 SCK */
#define BOARD_PIN_LED_RED           31   /* P0.31 — Red status LED (active high, via 1kΩ) */

/*===========================================================================
 * PIN DEFINITIONS — PORT 1
 *===========================================================================*/

#define BOARD_PIN_CAN0_STB          0    /* P1.00 — TJA1445 #0 STB (low=normal, high=standby) */
#define BOARD_PIN_CAN1_STB          1    /* P1.01 — TJA1445 #1 STB */
#define BOARD_PIN_CAN0_TERM         2    /* P1.02 — CAN0 120Ω termination FET gate */
#define BOARD_PIN_CAN1_TERM         3    /* P1.03 — CAN1 120Ω termination FET gate */
#define BOARD_PIN_LED_GRN           4    /* P1.04 — Green activity LED */
#define BOARD_PIN_LED_BLU           5    /* P1.05 — Blue BLE connected LED */
#define BOARD_PIN_BTN_USER          6    /* P1.06 — User button (active low, pull-up) */
#define BOARD_PIN_P107              7    /* P1.07 — NC (available) */
#define BOARD_PIN_P108              8    /* P1.08 — NC */
#define BOARD_PIN_P109              9    /* P1.09 — NC */
#define BOARD_PIN_CAN0_EN           10   /* P1.10 — TJA1445 #0 EN (active high) */
#define BOARD_PIN_CAN1_EN           11   /* P1.11 — TJA1445 #1 EN (active high) */
#define BOARD_PIN_P112              12   /* P1.12 — NC */
#define BOARD_PIN_P113              13   /* P1.13 — NC */
#define BOARD_PIN_P114              14   /* P1.14 — NC */
#define BOARD_PIN_P115              15   /* P1.15 — NC */

/*===========================================================================
 * FIXED-FUNCTION PINS (nRF52840)
 *===========================================================================*/

/* These pins are hardwired to specific functions and cannot be reassigned */
#define BOARD_PIN_SWDIO             65   /* SWD data I/O */
#define BOARD_PIN_SWDCLK            66   /* SWD clock input */
#define BOARD_PIN_USB_DPLUS         59   /* USB D+ */
#define BOARD_PIN_USB_DMINUS        60   /* USB D- */
#define BOARD_PIN_USB_VBUS         58   /* USB VBUS detect */
#define BOARD_PIN_XC1               63   /* 32.768 kHz crystal input */
#define BOARD_PIN_XC2               64   /* 32.768 kHz crystal output */

/* Power pins (not GPIO, listed for reference) */
#define BOARD_PIN_VDD_NRF_A         49   /* VDD_nRF supply */
#define BOARD_PIN_VDD_NRF_B         50   /* VDD_nRF supply */
#define BOARD_PIN_DEC4              51   /* Internal regulator decoupling */
#define BOARD_PIN_DEC5              52   /* Internal regulator decoupling */
#define BOARD_PIN_DCDCEN            53   /* DC/DC converter enable */
#define BOARD_PIN_DEC6              54   /* Decoupling */
#define BOARD_PIN_DEC7              55   /* Decoupling */
#define BOARD_PIN_VSS_A             56   /* Ground */
#define BOARD_PIN_VSS_B             57   /* Ground */
#define BOARD_PIN_VDD_USB           61   /* USB PHY supply */
#define BOARD_PIN_DECUSB            62   /* USB regulator decoupling */

/*===========================================================================
 * SPI CONFIGURATION
 *===========================================================================*/

/* SPI0 — CAN Channel 0 */
#define CAN0_SPI_INSTANCE           0
#define CAN0_SPI_FREQ_HZ            8000000UL   /* 8 MHz */
#define CAN0_SPI_MODE               0           /* CPOL=0, CPHA=0 */
#define CAN0_SPI_ORC                0xFF        /* Over-read character */
#define CAN0_SPI_CSN_PIN            BOARD_PIN_SPI0_CSN
#define CAN0_SPI_SCK_PIN            BOARD_PIN_SPI0_SCK
#define CAN0_SPI_MOSI_PIN           BOARD_PIN_SPI0_MOSI
#define CAN0_SPI_MISO_PIN           BOARD_PIN_SPI0_MISO
#define CAN0_INT_PIN                BOARD_PIN_CAN0_INT
#define CAN0_INT_GPIOTE_CHANNEL     0

/* SPI1 — CAN Channel 1 */
#define CAN1_SPI_INSTANCE           1
#define CAN1_SPI_FREQ_HZ            8000000UL   /* 8 MHz */
#define CAN1_SPI_MODE               0           /* CPOL=0, CPHA=0 */
#define CAN1_SPI_ORC                0xFF        /* Over-read character */
#define CAN1_SPI_CSN_PIN            BOARD_PIN_SPI1_CSN
#define CAN1_SPI_SCK_PIN            BOARD_PIN_SPI1_SCK
#define CAN1_SPI_MOSI_PIN           BOARD_PIN_SPI1_MOSI
#define CAN1_SPI_MISO_PIN           BOARD_PIN_SPI1_MISO
#define CAN1_INT_PIN                BOARD_PIN_CAN1_INT
#define CAN1_INT_GPIOTE_CHANNEL     1

/*===========================================================================
 * QSPI CONFIGURATION
 *===========================================================================*/

#define QSPI_FREQ_HZ                64000000UL  /* 64 MHz */
#define QSPI_CSN_PSRAM_PIN          BOARD_PIN_QSPI_CSN_PSRAM
#define QSPI_CSN_FLASH_PIN          BOARD_PIN_QSPI_CSN_FLASH
#define QSPI_SCK_PIN                BOARD_PIN_QSPI_CLK
#define QSPI_IO0_PIN                BOARD_PIN_QSPI_IO0
#define QSPI_IO1_PIN                BOARD_PIN_QSPI_IO1
#define QSPI_IO2_PIN                BOARD_PIN_QSPI_IO2
#define QSPI_IO3_PIN                BOARD_PIN_QSPI_IO3

/* PSRAM (APS6404L-3SQR-SN) */
#define PSRAM_SIZE                  (8 * 1024 * 1024)   /* 8 MB */
#define PSRAM_READ_OPCODE           0xEB    /* Fast Read Quad I/O */
#define PSRAM_WRITE_OPCODE          0x38    /* Write Quad I/O */
#define PSRAM_ENTER_QUAD_OPCODE     0x35    /* Enter Quad Mode */
#define PSRAM_DUMMY_CYCLES          6       /* 6 dummy cycles for 80 MHz */

/* NOR Flash (W25Q128JVSIQ) */
#define FLASH_SIZE                  (16 * 1024 * 1024)  /* 16 MB */
#define FLASH_READ_OPCODE           0xEB    /* Fast Read Quad I/O */
#define FLASH_WRITE_OPCODE          0x32    /* Quad Page Program */
#define FLASH_ERASE_4K_OPCODE       0x20    /* Sector Erase (4KB) */
#define FLASH_ERASE_32K_OPCODE      0x52    /* Block Erase (32KB) */
#define FLASH_ERASE_64K_OPCODE      0xD8    /* Block Erase (64KB) */
#define FLASH_ERASE_CHIP_OPCODE     0xC7    /* Chip Erase */
#define FLASH_READ_STATUS_OPCODE    0x05    /* Read Status Register */
#define FLASH_WRITE_ENABLE_OPCODE   0x06    /* Write Enable */
#define FLASH_DUMMY_CYCLES          8       /* 8 dummy cycles for 133 MHz */

/*===========================================================================
 * I²C CONFIGURATION
 *===========================================================================*/

#define I2C_INSTANCE                0       /* TWIM0 */
#define I2C_FREQ_HZ                 400000UL /* 400 kHz */
#define I2C_SDA_PIN                 BOARD_PIN_I2C_SDA
#define I2C_SCL_PIN                 BOARD_PIN_I2C_SCL
#define SSD1306_I2C_ADDR            0x3C    /* 7-bit address */

/*===========================================================================
 * CAN TRANSCEIVER CONTROL
 *===========================================================================*/

#define CAN0_STB_PIN                BOARD_PIN_CAN0_STB
#define CAN1_STB_PIN                BOARD_PIN_CAN1_STB
#define CAN0_EN_PIN                 BOARD_PIN_CAN0_EN
#define CAN1_EN_PIN                 BOARD_PIN_CAN1_EN
#define CAN0_TERM_PIN               BOARD_PIN_CAN0_TERM
#define CAN1_TERM_PIN               BOARD_PIN_CAN1_TERM

/* Transceiver states */
#define CAN_TRX_NORMAL              0   /* STB=0, EN=1 */
#define CAN_TRX_STANDBY             1   /* STB=1, EN=1 */
#define CAN_TRX_SLEEP               2   /* STB=1, EN=0 */
#define CAN_TRX_OFF                 3   /* STB=0, EN=0 (not recommended) */

/*===========================================================================
 * USER INTERFACE
 *===========================================================================*/

#define LED_RED_PIN                 BOARD_PIN_LED_RED
#define LED_GRN_PIN                 BOARD_PIN_LED_GRN
#define LED_BLU_PIN                 BOARD_PIN_LED_BLU
#define BTN_USER_PIN                BOARD_PIN_BTN_USER

/* LED active states */
#define LED_ACTIVE_HIGH             1   /* LEDs are active high (VDD → LED → resistor → GPIO) */

/* Button debounce */
#define BTN_DEBOUNCE_MS             50

/*===========================================================================
 * BATTERY MONITORING
 *===========================================================================*/

#define VBAT_SENSE_PIN              BOARD_PIN_AIN2
#define VBAT_SENSE_ADC_CHANNEL      2   /* AIN2 = ADC channel 2 */
#define VBAT_DIVIDER_RATIO          2   /* 2:1 divider (2× 100kΩ) */
#define VBAT_ADC_REF_MV             600 /* Internal 0.6V reference */
#define VBAT_ADC_GAIN               (1.0f / 6.0f) /* Gain = 1/6 → input range 0–3.6V */
#define VBAT_ADC_MAX_MV             3600
#define VBAT_FULL_MV                4200
#define VBAT_LOW_MV                 3400
#define VBAT_CRITICAL_MV            3100

/*===========================================================================
 * TIMER CONFIGURATION
 *===========================================================================*/

#define TIMESTAMP_TIMER_INSTANCE    0   /* TIMER0 — 1 MHz µs counter */
#define INJECTION_TIMER_INSTANCE    1   /* TIMER1 — injection scheduler */
#define WDT_KICK_TIMER_INSTANCE     2   /* TIMER2 — watchdog kick timer */

/*===========================================================================
 * RTC CONFIGURATION
 *===========================================================================*/

#define SYSTEM_RTC_INSTANCE         0   /* RTC0 — system tick */
#define BLE_RTC_INSTANCE            1   /* RTC1 — BLE SoftDevice timeslot */
#define APP_RTC_INSTANCE            2   /* RTC2 — application timer */

/*===========================================================================
 * GPIOTE CHANNEL ASSIGNMENTS
 *===========================================================================*/

#define GPIOTE_CH_CAN0_INT          0
#define GPIOTE_CH_CAN1_INT          1
#define GPIOTE_CH_BTN_USER          2
#define GPIOTE_CH_LED_RED           3   /* Task mode for LED toggling */
#define GPIOTE_CH_LED_GRN           4
#define GPIOTE_CH_LED_BLU           5

/*===========================================================================
 * WATCHDOG CONFIGURATION
 *===========================================================================*/

#define WDT_TIMEOUT_SEC             8
#define WDT_RELOAD_CHANNEL          0

/*===========================================================================
 * BLE CONFIGURATION
 *===========================================================================*/

#define BLE_DEVICE_NAME             "CAN Creeper"
#define BLE_MANUFACTURER_NAME       "Nous Research"
#define BLE_ADV_INTERVAL_MS        100     /* 100 ms advertising interval */
#define BLE_CONN_INTERVAL_MIN_MS   7.5     /* Fast connection for low latency */
#define BLE_CONN_INTERVAL_MAX_MS   15
#define BLE_SLAVE_LATENCY          0
#define BLE_SUP_TIMEOUT_MS         4000
#define BLE_TX_POWER_DBM           0       /* 0 dBm default */
#define BLE_NUS_SERVICE_UUID       0x6E400001  /* Nordic UART Service base UUID */
#define BLE_NUS_TX_CHAR_UUID       0x6E400002
#define BLE_NUS_RX_CHAR_UUID       0x6E400003
#define BLE_NUS_MAX_DATA_LEN       244     /* With DLE */

/*===========================================================================
 * USB CONFIGURATION
 *===========================================================================*/

#define USB_VID                     0x1915  /* Nordic Semiconductor */
#define USB_PID                     0xCAFE  /* CAN Creeper */
#define USB_BCD_DEVICE              0x0100  /* Rev 1.0 */
#define USB_MANUFACTURER_STRING     "Nous Research"
#define USB_PRODUCT_STRING          "CAN Creeper"
#define USB_CDC_BAUD_RATE           921600  /* Reported baud (ignored for raw USB) */
#define USB_CDC_DATA_BITS           8
#define USB_CDC_STOP_BITS           1
#define USB_CDC_PARITY              0       /* None */
#define USB_EP_BULK_IN              1
#define USB_EP_BULK_OUT             1
#define USB_EP_INTERRUPT_IN         2
#define USB_BULK_MAX_PACKET_SIZE    64
#define USB_INTERRUPT_MAX_PACKET_SIZE 16

/*===========================================================================
 * MEMORY MAP
 *===========================================================================*/

/* Internal Flash */
#define FLASH_APP_START_ADDR        0x00026000UL
#define FLASH_APP_SIZE              (872 * 1024)  /* 872 KB */
#define FLASH_DFU_SETTINGS_ADDR     0x0006E000UL
#define FLASH_CONFIG_PAGE_ADDR      0x0006E000UL  /* Shared with DFU settings */

/* Internal RAM */
#define RAM_APP_START_ADDR          0x2000B000UL
#define RAM_APP_SIZE                (212 * 1024)  /* 212 KB */
#define RAM_HEAP_SIZE               (32 * 1024)   /* 32 KB */

/* External PSRAM (via QSPI) */
#define PSRAM_BASE_ADDR             0x00800000UL
#define PSRAM_CH0_BUFFER_OFFSET     0x00000000UL
#define PSRAM_CH0_BUFFER_SIZE       (512 * 1024)  /* 512 KB = 16K frames */
#define PSRAM_CH1_BUFFER_OFFSET     0x00080000UL
#define PSRAM_CH1_BUFFER_SIZE       (512 * 1024)
#define PSRAM_SCRIPT_OFFSET         0x00100000UL
#define PSRAM_SCRIPT_SIZE           (1 * 1024 * 1024)
#define PSRAM_DBC_OFFSET            0x00200000UL
#define PSRAM_DBC_SIZE              (2 * 1024 * 1024)

/* External NOR Flash (via QSPI) */
#define NOR_FLASH_BASE_ADDR         0x01000000UL
#define NOR_FLASH_DFU_IMAGE_OFFSET  0x00000000UL
#define NOR_FLASH_DFU_IMAGE_SIZE    (1 * 1024 * 1024)  /* 1 MB for OTA images */
#define NOR_FLASH_DBC_STORE_OFFSET  0x00100000UL
#define NOR_FLASH_DBC_STORE_SIZE    (4 * 1024 * 1024)  /* 4 MB for DBC files */
#define NOR_FLASH_SCRIPT_STORE_OFFSET 0x00500000UL
#define NOR_FLASH_SCRIPT_STORE_SIZE (2 * 1024 * 1024)  /* 2 MB for scripts */
#define NOR_FLASH_RESERVED_OFFSET   0x00700000UL
#define NOR_FLASH_RESERVED_SIZE     (9 * 1024 * 1024)  /* 9 MB reserved */

/*===========================================================================
 * CAN FRAME BUFFER CONFIGURATION
 *===========================================================================*/

#define CAN_FRAME_ENTRY_SIZE        32      /* Bytes per frame entry in ring buffer */
#define CAN_RING_BUFFER_FRAMES      16384   /* 16K frames per channel */
#define CAN_RING_BUFFER_SIZE        (CAN_RING_BUFFER_FRAMES * CAN_FRAME_ENTRY_SIZE)

/*===========================================================================
 * MACROS
 *===========================================================================*/

/* GPIO manipulation */
#define GPIO_OUT_SET(port, pin)     NRF_P##port->OUTSET = (1UL << (pin))
#define GPIO_OUT_CLR(port, pin)     NRF_P##port->OUTCLR = (1UL << (pin))
#define GPIO_OUT_TGL(port, pin)     NRF_P##port->OUT ^= (1UL << (pin))
#define GPIO_IN_READ(port, pin)     ((NRF_P##port->IN >> (pin)) & 1UL)

/* LED control */
#define LED_RED_ON()                GPIO_OUT_SET(0, BOARD_PIN_LED_RED)
#define LED_RED_OFF()               GPIO_OUT_CLR(0, BOARD_PIN_LED_RED)
#define LED_RED_TGL()               GPIO_OUT_TGL(0, BOARD_PIN_LED_RED)
#define LED_GRN_ON()                GPIO_OUT_SET(1, BOARD_PIN_LED_GRN)
#define LED_GRN_OFF()               GPIO_OUT_CLR(1, BOARD_PIN_LED_GRN)
#define LED_BLU_ON()                GPIO_OUT_SET(1, BOARD_PIN_LED_BLU)
#define LED_BLU_OFF()               GPIO_OUT_CLR(1, BOARD_PIN_LED_BLU)

/* CAN transceiver control */
#define CAN0_TRX_NORMAL()           do { GPIO_OUT_CLR(1, CAN0_STB_PIN); GPIO_OUT_SET(1, CAN0_EN_PIN); } while(0)
#define CAN0_TRX_STANDBY()          do { GPIO_OUT_SET(1, CAN0_STB_PIN); GPIO_OUT_SET(1, CAN0_EN_PIN); } while(0)
#define CAN0_TRX_SLEEP()            do { GPIO_OUT_SET(1, CAN0_STB_PIN); GPIO_OUT_CLR(1, CAN0_EN_PIN); } while(0)
#define CAN1_TRX_NORMAL()           do { GPIO_OUT_CLR(1, CAN1_STB_PIN); GPIO_OUT_SET(1, CAN1_EN_PIN); } while(0)
#define CAN1_TRX_STANDBY()          do { GPIO_OUT_SET(1, CAN1_STB_PIN); GPIO_OUT_SET(1, CAN1_EN_PIN); } while(0)
#define CAN1_TRX_SLEEP()            do { GPIO_OUT_SET(1, CAN1_STB_PIN); GPIO_OUT_CLR(1, CAN1_EN_PIN); } while(0)

/* Termination control */
#define CAN0_TERM_ON()              GPIO_OUT_SET(1, CAN0_TERM_PIN)
#define CAN0_TERM_OFF()             GPIO_OUT_CLR(1, CAN0_TERM_PIN)
#define CAN1_TERM_ON()              GPIO_OUT_SET(1, CAN1_TERM_PIN)
#define CAN1_TERM_OFF()             GPIO_OUT_CLR(1, CAN1_TERM_PIN)

/* Button reading */
#define BTN_IS_PRESSED()            (GPIO_IN_READ(1, BOARD_PIN_BTN_USER) == 0)

/*===========================================================================
 * FUNCTION PROTOTYPES
 *===========================================================================*/

/**
 * @brief Initialize all board GPIO pins to their default states
 */
void board_gpio_init(void);

/**
 * @brief Initialize board-specific clocks (HFCLK, LFCLK)
 */
void board_clock_init(void);

/**
 * @brief Initialize power management (DC/DC enable, regulator config)
 */
void board_power_init(void);

/**
 * @brief Read battery voltage in millivolts
 * @return Battery voltage in mV
 */
uint16_t board_battery_read_mv(void);

/**
 * @brief Get board temperature from nRF52840 internal TEMP sensor
 * @return Temperature in °C (signed)
 */
int8_t board_temperature_read(void);

/**
 * @brief Enter system OFF mode (deepest sleep, wake on pin reset or NFC field)
 */
void board_system_off(void);

/**
 * @brief Get unique device ID from FICR
 * @param id_out Buffer to store 8-byte device ID
 */
void board_get_device_id(uint8_t id_out[8]);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */
