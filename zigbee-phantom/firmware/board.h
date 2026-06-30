/*
 * board.h — Pin assignments and hardware constants for ZIGBEE-PHANTOM
 * Author: jayis1
 * License: GPL-2.0
 *
 * Target MCU: CC2652R1F (ARM Cortex-M4F, 48 MHz, 128 KB SRAM)
 * Companion:  ESP32-S3-MINI-1 (drives backhaul)
 *
 * All pin names use CC2652 DIO numbering (0..31).
 */
#ifndef ZIGBEE_PHANTOM_BOARD_H
#define ZIGBEE_PHANTOM_BOARD_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ---- Clocking ---- */
#define SYSCLK_HZ           48000000UL
#define LFCLK_HZ            32768UL
#define RTC_TICK_US         1           /* AON_RTC subsecond tick = ~30.5 us; we scale */

/* ---- RF core / antenna path ---- */
#define RFC_DBELL_BASE       0x40081000UL
#define RFC_DBELL_O_CMDSTA   0x00000000UL
#define RFC_DBELL_O_CMDR     0x00000004UL
#define RFC_DBELL_O_CMDACK   0x00000008UL

#define RF_IRQ_DIO           7           /* CC2652 -> application IRQ from RF core */
#define RF_CPE_IRQ_DIO       8           /* RF CPE event line */
#define FEM_PA_EN_DIO        12          /* nRF21540 PA enable (TX) */
#define FEM_LNA_EN_DIO       13          /* nRF21540 LNA enable (RX) */
#define FEM_GAIN_DIO         14          /* nRF21540 gain select (0=low,1=high) */
#define ANT_SW_A_DIO         15          /* SKY13330 antenna switch A */
#define ANT_SW_B_DIO         16          /* SKY13330 antenna switch B */

/* ---- ESP32-S3 backhaul (SPI slave + IRQ) ---- */
#define ESP_SPI_BASE         0x40003000UL   /* SSI0 base, used as SPI slave to ESP32 */
#define ESP_SPI_IRQ_DIO      21
#define ESP_SPI_CS_DIO        22
#define ESP_SPI_MISO_DIO      23
#define ESP_SPI_MOSI_DIO     24
#define ESP_SPI_CLK_DIO      25
#define ESP_HOST_READY_DIO   26          /* CC2652 signals frame ready to ESP32 */

/* ---- MicroSD (SPI master) ---- */
#define SD_SPI_BASE          0x40004000UL   /* SSI1 base */
#define SD_CS_DIO            17
#define SD_MISO_DIO          18
#define SD_MOSI_DIO          19
#define SD_CLK_DIO           20
#define SD_CD_DIO            27          /* card detect */

/* ---- External flash (SPI master, capture buffer) ---- */
#define FLASH_SPI_BASE       0x40008000UL   /* SSI2 base */
#define FLASH_CS_DIO         28
#define FLASH_MISO_DIO       9
#define FLASH_MOSI_DIO       10
#define FLASH_CLK_DIO        11

/* ---- OLED SH1106 (SPI master) ---- */
#define OLED_SPI_BASE        0x40009000UL   /* SSI3 base */
#define OLED_CS_DIO          1
#define OLED_DC_DIO          2            /* 0=cmd, 1=data */
#define OLED_RST_DIO         3
#define OLED_MOSI_DIO        4
#define OLED_CLK_DIO         5

/* ---- 5-way joystick (ADC + center button) ---- */
#define JOY_ADC_CH           0            /* ADC channel 0 (DIO30 analog) */
#define JOY_BTN_DIO          6            /* center press */

/* ---- Status LEDs (active high) ---- */
#define LED_RX_DIO           29
#define LED_TX_DIO           30
#define LED_KEY_DIO          31
#define LED_ERR_DIO          0

/* ---- MAX17048 fuel gauge (I2C) ---- */
#define I2C_BASE             0x40002000UL
#define MAX17048_ADDR        0x36         /* 7-bit I2C address */

/* ---- Button / power ---- */
#define PWR_EN_DIO           24           /* load switch enable (self-hold) */
#define CHRG_STAT_DIO        25           /* TP4056 charge status input */

/* ---- Battery thresholds ---- */
#define BATT_MV_FULL        4200
#define BATT_MV_LOW         3300
#define BATT_MV_CRIT        3000

/* ---- 802.15.4 channel table ---- */
#define IEEE802154_CHAN_MIN  11
#define IEEE802154_CHAN_MAX  26
#define IEEE802154_CHAN_OF(ch)  (2405 + 5 * (ch))   /* MHz */

/* ---- Frame constants ---- */
#define IEEE802154_MAX_FRM_LEN   127
#define IEEE802154_FCS_LEN        2
#define ZB_CAP_BUF_FRAMES         512   /* ring buffer depth in external flash */
#define ZB_PCAP_LINKTYPE          195   /* LINKTYPE_IEEE802_15_4_WITHFCS */

/* ---- Mode state machine ---- */
typedef enum {
    MODE_SNIFF        = 0,    /* passive promiscuous capture */
    MODE_KEYHUNT       = 1,    /* focused Transport-Key interception */
    MODE_ROGUE_COORD   = 2,    /* forged beacon / Transport-Key injection */
    MODE_JAM           = 3,    /* frame-filtered reactive jamming */
    MODE_RELAY         = 4,    /* cross-channel relay */
    MODE_NUM
} zb_mode_t;

/* ---- Device context (global singleton) ---- */
typedef struct {
    zb_mode_t  mode;
    uint8_t    channel;            /* current 802.15.4 channel */
    int8_t     rssi;                /* last frame RSSI */
    uint8_t    lqi;                 /* last frame LQI */
    uint32_t   frame_counter;       /* total frames captured */
    uint32_t   key_frames;          /* Transport-Key frames captured */
    uint32_t   injected_frames;     /* frames injected */
    bool       sd_present;
    bool       esp_link_up;
    bool       jam_filter_set;
    uint16_t   filter_panid;        /* 0xFFFF = all PANs */
    uint8_t    filter_eui[8];       /* 00.. = any */
    int8_t     jam_rssi_thresh;     /* dBm threshold to trigger jam */
    uint16_t   batt_mv;
    uint8_t    batt_pct;
} zb_ctx_t;

extern zb_ctx_t g_ctx;

/* ---- GPIO / pin mux helpers ---- */
#define GPIO_OUTPUT(pin)    do { *((volatile uint32_t *)(0x50000000UL + 0x0400 + (pin/8)*4)) |= (1u<<(pin%8)); } while(0)
#define GPIO_INPUT(pin)     do { *((volatile uint32_t *)(0x50000000UL + 0x0400 + (pin/8)*4)) &= ~(1u<<(pin%8)); } while(0)
#define GPIO_SET(pin)       do { *((volatile uint32_t *)(0x50000000UL + 0x0400*3 + (pin/8)*4)) = (1u<<(pin%8)); } while(0)
#define GPIO_CLR(pin)       do { *((volatile uint32_t *)(0x50000000UL + 0x0400*4 + (pin/8)*4)) = (1u<<(pin%8)); } while(0)
#define GPIO_RD(pin)        ((*((volatile uint32_t *)(0x50000000UL + 0x0400*2 + (pin/8)*4)) >> (pin%8)) & 1)

#endif /* ZIGBEE_PHANTOM_BOARD_H */