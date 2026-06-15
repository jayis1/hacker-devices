/*
 * board.h — Infrared Phantom hardware definitions
 * STM32H743VIT6 Cortex-M7 @ 480 MHz
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stddef.h>

/* Clock configuration */
#define HSE_VALUE           25000000U    /* 25 MHz external crystal */
#define LSE_VALUE           32768U       /* 32.768 kHz RTC crystal */
#define SYSCLK_FREQ         480000000U   /* 480 MHz system clock */
#define AHB_FREQ            480000000U   /* 480 MHz AHB */
#define APB1_FREQ           120000000U   /* 120 MHz APB1 */
#define APB2_FREQ           240000000U   /* 240 MHz APB2 */
#define APB4_FREQ           120000000U   /* 120 MHz APB4 */

/* Peripheral clock frequencies (APB prescaler outputs) */
#define TIM_APB1_FREQ       240000000U   /* APB1 timer clock = APB1 × 2 */
#define TIM_APB2_FREQ      480000000U   /* APB2 timer clock = APB2 × 2 */
#define SPI1_CLK_FREQ       120000000U   /* APB2 / 2 = SPI1 max */
#define SPI2_CLK_FREQ       60000000U    /* APB4 / 2 = SPI2 max */
#define USART1_CLK_FREQ    120000000U    /* APB2 USART clock */

/* Memory map */
#define FLASH_BASE          0x08000000U
#define FLASH_SIZE           0x00100000U  /* 1 MB */
#define ITCM_BASE           0x00000000U
#define DTCM_BASE           0x20000000U
#define AXI_SRAM_BASE       0x24000000U
#define SRAM1_BASE           0x30000000U
#define SRAM2_BASE           0x30020000U
#define SRAM4_BASE           0x38000000U

/* Sizes */
#define DTCM_SIZE            0x00020000U  /* 128 KB */
#define AXI_SRAM_SIZE        0x00080000U  /* 512 KB */
#define SRAM1_SIZE           0x00020000U  /* 128 KB */
#define SRAM2_SIZE           0x00008000U  /* 32 KB */
#define SRAM4_SIZE           0x00010000U   /* 64 KB */

/* DMA buffer placements */
#define ADC_BUF_BASE        DTCM_BASE            /* ADC capture in DTCM (zero-wait) */
#define ADC_BUF_SIZE        0x00004000U           /* 16 KB = 8192 samples × 2 bytes */
#define IR_TIM_BUF_BASE     (DTCM_BASE + ADC_BUF_SIZE)  /* TIM capture buffer */
#define IR_TIM_BUF_SIZE     0x00004000U           /* 16 KB */
#define USB_BUF_BASE        SRAM1_BASE            /* USB packet buffers in SRAM1 */
#define USB_BUF_SIZE        0x00008000U            /* 32 KB */
#define SD_BUF_BASE         (SRAM1_BASE + USB_BUF_SIZE)  /* SD card buffers */
#define SD_BUF_SIZE         0x00008000U            /* 32 KB */
#define BLE_BUF_BASE        SRAM2_BASE            /* BLE UART ring buffer */
#define BLE_BUF_SIZE        0x00004000U            /* 16 KB */

/* GPIO pin assignments */
/* Port A */
#define nRF_INT_PIN         0    /* PA0  — nRF52840 interrupt input */
#define USER_BTN_PIN        1    /* PA1  — User button (active-low) */
#define SPI1_MOSI_PIN       2    /* PA2  — SPI1 MOSI (W25Q128) */
#define SPI1_MISO_PIN       3    /* PA3  — SPI1 MISO (W25Q128) */
#define SPI1_NSS_PIN        4    /* PA4  — SPI1 NSS (W25Q128, manual) */
#define SPI1_SCK_PIN        5    /* PA5  — SPI1 SCK (W25Q128) */
#define IR_DIGITAL_PIN      6    /* PA6  — TSMP58000 digital output */
#define IR_ANALOG_PIN       7    /* PA7  — TSMP58000 analog output (ADC1_IN7) */
#define USART1_TX_PIN       9    /* PA9  — USART1 TX → nRF52840 RX */
#define USART1_RX_PIN       10   /* PA10 — USART1 RX ← nRF52840 TX */
#define SWDIO_PIN           13   /* PA13 — SWD data */
#define SWCLK_PIN           14   /* PA14 — SWD clock */

/* Port B */
#define SPI3_SCK_PIN        3    /* PB3  — SPI3 SCK (microSD) */
#define SPI3_MISO_PIN       4    /* PB4  — SPI3 MISO (microSD) */
#define SPI3_MOSI_PIN       5    /* PB5  — SPI3 MOSI (microSD) */
#define OLED_RST_PIN        6    /* PB6  — SSD1306 reset */
#define OLED_DC_PIN         7    /* PB7  — SSD1306 data/command */

/* Port C */
#define ULPI_STP_PIN        0    /* PC0  — ULPI STP */
#define ULPI_DIR_PIN        1    /* PC1  — ULPI DIR */
#define ULPI_NXT_PIN        2    /* PC2  — ULPI NXT */
#define ULPI_CLK_PIN        3    /* PC3  — ULPI CLK (60 MHz) */

/* Port D */
#define nRF_RESET_PIN       0    /* PD0  — nRF52840 reset (active-low) */
#define nRF_RTS_PIN         1    /* PD1  — nRF52840 RTS */
#define IR_PWM_PIN          2    /* PD2  — TIM3_CH1 (IR LED PWM carrier) */
#define SD_D3_PIN           5    /* PD5  — SDMMC1_D3 */
#define SD_D2_PIN           6    /* PD6  — SDMMC1_D2 */
#define SD_D1_PIN           7    /* PD7  — SDMMC1_D1 */
#define ULPI_D0_PIN         8    /* PD8  — ULPI D0 */
#define ULPI_D1_PIN         9    /* PD9  — ULPI D1 */
#define ULPI_D2_PIN         10   /* PD10 — ULPI D2 */
#define ULPI_D3_PIN         11   /* PD11 — ULPI D3 */
#define ULPI_D4_PIN         12   /* PD12 — ULPI D4 */
#define ULPI_D5_PIN         13   /* PD13 — ULPI D5 */
#define ULPI_D6_PIN         14   /* PD14 — ULPI D6 */
#define ULPI_D7_PIN         15   /* PD15 — ULPI D7 */

/* Port E */
#define nRF_CTS_PIN         0    /* PE0  — nRF52840 CTS */
#define IR_LED_EN_PIN       1    /* PE1  — IR LED enable (MOSFET gate) */
#define LED_STATUS_PIN      2    /* PE2  — Green status LED */
#define LED_ERROR_PIN       3    /* PE3  — Red error LED */
#define IR_PWM_GATE_PIN     4    /* PE4  — TIM3_CH2 (IR burst gating) */
#define OLED_CS_PIN         5    /* PE5  — SSD1306 chip select */
#define SD_CD_PIN           6    /* PE6  — microSD card detect */

/* GPIO AF mappings */
#define GPIO_AF_SPI1        5U   /* SPI1 on PA2-PA5 */
#define GPIO_AF_SPI3        6U   /* SPI3 on PB3-PB5 */
#define GPIO_AF_USART1      7U   /* USART1 on PA9-PA10 */
#define GPIO_AF_TIM3        2U   /* TIM3 on PA6, PD2, PE4 */
#define GPIO_AF_ULPI        10U  /* ULPI on PC0-PC3, PD8-PD15 */
#define GPIO_AF_SDMMC1      12U  /* SDMMC1 on PB3-PB5, PD5-PD7 */

/* LED control macros */
#define LED_STATUS_ON()     GPIOE->ODR |= (1U << LED_STATUS_PIN)
#define LED_STATUS_OFF()    GPIOE->ODR &= ~(1U << LED_STATUS_PIN)
#define LED_STATUS_TOGGLE() GPIOE->ODR ^= (1U << LED_STATUS_PIN)
#define LED_ERROR_ON()      GPIOE->ODR |= (1U << LED_ERROR_PIN)
#define LED_ERROR_OFF()     GPIOE->ODR &= ~(1U << LED_ERROR_PIN)
#define LED_ERROR_TOGGLE()  GPIOE->ODR ^= (1U << LED_ERROR_PIN)

/* IR LED enable */
#define IR_LED_ENABLE()     GPIOE->ODR |= (1U << IR_LED_EN_PIN)
#define IR_LED_DISABLE()    GPIOE->ODR &= ~(1U << IR_LED_EN_PIN)

/* IR protocol IDs (match wire protocol) */
#define IR_PROTO_NEC            0x01
#define IR_PROTO_NEC_EXT       0x02
#define IR_PROTO_RC5           0x03
#define IR_PROTO_RC6           0x04
#define IR_PROTO_RC6A          0x05
#define IR_PROTO_SONY_12       0x06
#define IR_PROTO_SONY_15       0x07
#define IR_PROTO_SONY_20       0x08
#define IR_PROTO_SAMSUNG       0x09
#define IR_PROTO_SHARP         0x0A
#define IR_PROTO_DAIKIN        0x0B
#define IR_PROTO_MITSUBISHI    0x0C
#define IR_PROTO_JVC           0x0D
#define IR_PROTO_PANASONIC     0x0E
#define IR_PROTO_IRDA_SIR      0x0F
#define IR_PROTO_RAW_MANCHESTER 0x10
#define IR_PROTO_RAW_WAVEFORM  0xFF

/* Operating modes */
#define MODE_IDLE      0
#define MODE_CAPTURE   1
#define MODE_TRANSMIT  2
#define MODE_FUZZ      3
#define MODE_ANALYZE   4
#define MODE_BRIDGE    5

/* USB CDC command codes */
#define CMD_PING           0x01
#define CMD_GET_VERSION    0x02
#define CMD_CAP_START      0x10
#define CMD_CAP_STOP       0x11
#define CMD_CAP_STATUS     0x12
#define CMD_TX_PROTOCOL    0x20
#define CMD_TX_RAW         0x21
#define CMD_TX_STOP        0x22
#define CMD_FUZZ_START     0x30
#define CMD_FUZZ_STOP      0x31
#define CMD_ANALYZE_START  0x40
#define CMD_ANALYZE_CLUSTER 0x41
#define CMD_FLASH_READ     0x50
#define CMD_FLASH_WRITE     0x51
#define CMD_FLASH_ERASE    0x52
#define CMD_SD_MOUNT       0x60
#define CMD_SD_OPEN        0x61
#define CMD_SD_READ        0x62
#define CMD_SD_WRITE       0x63
#define CMD_BLE_SCAN       0x70
#define CMD_BLE_CONNECT    0x71
#define CMD_BLE_SEND       0x72
#define CMD_OLED_TEXT      0x80
#define CMD_OLED_CLEAR     0x81
#define CMD_RESET          0xFF

/* Protocol sync bytes */
#define PROTO_SYNC_HI      0xAA
#define PROTO_SYNC_LO      0x55
#define PROTO_END          0x0D

/* Version */
#define FW_VERSION_MAJOR   1
#define FW_VERSION_MINOR   0
#define FW_VERSION_PATCH   0
#define FW_VERSION_DEV     1

#endif /* BOARD_H */