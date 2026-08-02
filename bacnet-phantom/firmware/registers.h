/*
 * registers.h — Hardware register / peripheral pin map for the BACnet Phantom.
 *
 * Author: jayis1
 * License: GPLv3
 *
 * This file centralises every fixed hardware assignment on the ESP32-S3 so
 * the rest of the firmware never has to sprinkle IO indices through driver
 * code. Anything referenced by a #define here is a hard-wired board route that
 * the KiCad schematic in ../kicad/ enforces. If you change a route on the
 * PCB, change it here first.
 */
#ifndef BACNET_PHANTOM_REGISTERS_H
#define BACNET_PHANTOM_REGISTERS_H

#ifdef __cplusplus
extern "C" {
#endif

/* ---- MCU identity ------------------------------------------------------ */
#define BP_MCU_FAMILY               "ESP32-S3"
#define BP_MCU_CORE_COUNT           2
#define BP_MCU_FREQ_HZ              240000000UL
#define BP_BUILD_AUTHOR             "jayis1"

/* ---- W5500 SPI (Ethernet, BACnet/IP) ----------------------------------- */
#define BP_W5500_SPI_HOST           SPI2_HOST
#define BP_W5500_PIN_MOSI          11
#define BP_W5500_PIN_MISO          13
#define BP_W5500_PIN_SCLK          12
#define BP_W5500_PIN_CS            10
#define BP_W5500_PIN_RST           9
#define BP_W5500_PIN_INT           14
#define BP_W5500_SPI_HZ            40000000UL
#define BP_W5500_SOCKET_RAW        0       /* promiscuous raw UDP socket */
#define BP_W5500_SOCKET_TX         1       /* active TX socket (whois / write) */
#define BP_W5500_SOCKET_BBMD       2       /* BBMD foreign-device socket */

/* ---- RS-485 (BACnet MS/TP) --------------------------------------------- */
#define BP_MSTP_UART_HOST           UART_NUM_1
#define BP_MSTP_PIN_TX             17
#define BP_MSTP_PIN_RX             18
#define BP_MSTP_PIN_DE             8        /* driver-enable, active high */
#define BP_MSTP_PIN_RE_N           3        /* receiver-enable, active low */
#define BP_MSTP_BAUDRATE           115200
#define BP_MSTP_UART_BUF_SIZE      512
#define BP_MSTP_TICK_US            5000     /* 5 ms MS/TP token tick */

/* ---- SSD1306 status OLED (I2C) ----------------------------------------- */
#define BP_OLED_I2C_HOST            I2C_NUM_0
#define BP_OLED_PIN_SDA             6
#define BP_OLED_PIN_SCL             7
#define BP_OLED_ADDR                0x3C
#define BP_OLED_WIDTH               128
#define BP_OLED_HEIGHT              64

/* ---- BLE / companion app ------------------------------------------------ */
/* ESP32-S3 integrated radio; no external pin assignments required, the
 * antenna trace is routed into the case window by the PCB layout. */

/* ---- Power / battery --------------------------------------------------- */
#define BP_BATT_ADC_CHANNEL        ADC1_CHANNEL_4   /* GPIO4 */
#define BP_BATT_DIVIDER_SCALE      2                /* 2x divider on Vbat */
#define TP4056_PIN_CHRG             5                /* active-low charging */
#define TP4056_PIN_STDBY            21               /* active-low standby */

/* ---- Safety interlocks ------------------------------------------------- */
#define BP_PIN_WRITE_BYPASS_JUMPER  15               /* hardware write interlock
                                                      * 0 = write blocked,
                                                      * 1 = write permitted
                                                      * (must be set at boot) */
#define BP_PIN_TAMPER_LOOP          16               /* reed switch; 0 = closed,
                                                      * transition to 1 = breach */
#define BP_PIN_USB_EFUSE_FAULT      4                /* MAX1352 eFuse fault out */

/* ---- RGB status LED ---------------------------------------------------- */
#define BP_LED_PIN_R                38
#define BP_LED_PIN_G                39
#define BP_LED_PIN_B                40
#define BP_LEDC_TIMER               LEDC_TIMER_0
#define BP_LEDC_FREQ_HZ             5000
#define BP_LEDC_RES_BITS            8

/* ---- Run-time register limits ------------------------------------------ */
#define BP_MAX_DEVICES             256
#define BP_MAX_OBJECTS_PER_DEVICE   512
#define BP_MAX_OBJECT_TOTAL         (BP_MAX_DEVICES * BP_MAX_OBJECTS_PER_DEVICE)
#define BP_NPDU_MAX                 1497               /* BACnet/IP MTU minus headers */
#define BP_MSTP_FRAME_MAX           501                /* MS/TP max frame */
#define BP_INVOKEID_MAX             254                /* BACnet invoke IDs 0..254 */
#define BP_ACK_TIMEOUT_MS           1500               /* WriteProperty ACK watch */
#define BBMD_MAX_FOREIGN_ENTRIES    16

/* ---- Default object-type blocklist (life-safety) ----------------------- */
/* Object types whose WriteProperty is always blocked regardless of the
 * jumper; see watchdog_task() in main.c. */
#define BP_LIFESAFETY_BLOCKLIST { \
    18,  /* life-safety-point          */ \
    19,  /* life-safety-zone           */ \
    24,  /* pulse-converter            */ \
    21,  /* program                    */ \
    39,  /* life-safety-array          */ \
}

#ifdef __cplusplus
}
#endif
#endif /* BACNET_PHANTOM_REGISTERS_H */