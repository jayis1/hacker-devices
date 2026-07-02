/*
 * board.h — Aurora-Phantom board pin map and peripheral instances
 *
 * Device:  Aurora-Phantom — Optical Compromising-Emanations Reconstruction
 * Author:  jayis1
 * License: GPL-2.0
 *
 * Target MCU: Espressif ESP32-S3 (application core)
 * Target FPGA: Lattice iCE40UP5K (signal core, memory-mapped via SPI)
 *
 * This header defines the static board configuration: GPIO pin assignments,
 * peripheral instances, and hardware constants. It is consumed by all driver
 * modules and by main.c.
 */
#ifndef AURORA_PHANTOM_BOARD_H
#define AURORA_PHANTOM_BOARD_H

/* ---- Author / version metadata ----------------------------------------- */
#define AURORA_AUTHOR      "jayis1"
#define AURORA_VERSION     "1.0.0"
#define AURORA_BOARD_NAME  "aurora-phantom"

/* ---- System timing ----------------------------------------------------- */
#define BOARD_XTAL_HZ      40000000ULL   /* 40 MHz crystal on ESP32-S3 */
#define BOARD_CPU_HZ       240000000ULL  /* 240 MHz dual-core */

/* ---- FPGA signal-core SPI bus ------------------------------------------ */
/* The iCE40UP5K is configured from flash and then presents a memory-mapped
 * register file to the ESP32-S3 over a dedicated SPI link (SPI3 / HSPI).
 * We bit-bang the CS to allow single-byte register peeks without DMA setup. */
#define FPGA_SPI_HOST      3          /* SPI3_HOST (HSPI) */
#define FPGA_PIN_SCK       12
#define FPGA_PIN_MOSI      11
#define FPGA_PIN_MISO      13
#define FPGA_PIN_CS        10
#define FPGA_PIN_CDONE     9          /* config-done input from FPGA */
#define FPGA_PIN_CRSTB     14         /* config-reset (open-drain) */
#define FPGA_SPI_HZ        40000000   /* 40 MHz; FPGA SPRAM bridge tolerates */

/* ---- Si5351C clock generator (I2C) ------------------------------------- */
#define SI5351_I2C_PORT    0
#define SI5351_SDA         8
#define SI5351_SCL         18
#define SI5351_ADDR        0x60       /* Si5351 default I2C addr */
#define SI5351_REF_HZ      26000000ULL /* TCXO 0.5 ppm feeding Si5351 */

/* ---- SPAD array control (via FPGA register file) ---------------------- */
/* The SPAD bias and gating are controlled through FPGA registers; see
 * registers.h. No direct GPIO is used for the array itself. */
#define SPAD_PIXELS_X      16
#define SPAD_PIXELS_Y      16
#define SPAD_PIXELS        (SPAD_PIXELS_X * SPAD_PIXELS_Y) /* 256 */
#define SPAD_MAX_RATE_HZ   80000000ULL  /* 80 MHz per pixel */
#define SPAD_DEADTIME_NS   12           /* typical SPAD dead time */

/* ---- Liquid-crystal tunable bandpass filter --------------------------- */
/* The LC bandpass is driven by an analog voltage from a DAC; the DAC is on
 * the ESP32's I2C peripheral #1. Wavelength 450–950 nm maps to 0–3V3. */
#define LC_DAC_I2C_PORT    1
#define LC_DAC_SDA         17
#define LC_DAC_SCL         16
#define LC_DAC_ADDR        0x4C        /* MCP4725-class DAC */
#define LC_WAVELENGTH_MIN  450         /* nm */
#define LC_WAVELENGTH_MAX  950         /* nm */

/* ---- microSD card (SDMMC) --------------------------------------------- */
#define SDMMC_SLOT         1           /* SDMMC slot 1: 4-bit, dedicated pins */
#define SDMMC_PIN_CMD      0
#define SDMMC_PIN_CLK      5
#define SDMMC_PIN_D0       2
#define SDMMC_PIN_D1       3
#define SDMMC_PIN_D2       4
#define SDMMC_PIN_D3       1
#define SD_FREQ_HZ         40000000    /* 40 MHz after init */

/* ---- nPM1300 PMIC (I2C) ----------------------------------------------- */
#define PMIC_I2C_PORT      0
#define PMIC_ADDR          0x6B        /* Nordic nPM1300 default */
/* PMIC shares I2C0 with Si5351; SDA/SCL are SI5351_SDA/SCL. */

/* ---- Operator interface ----------------------------------------------- */
#define BTN_PIN            41          /* tactile button (mode/rendezvous) */
#define RGB_LED_PIN_R      42          /* WS2812-style via RMT — single LED */
#define RGB_LED_PIN_G      2           /* (alternate; board rev dependent)   */
#define RGB_LED_PIN_B      41          /* (shared with BTN via analog mux)   */
#define OLED_I2C_PORT      1
#define OLED_ADDR          0x3C        /* SSD1306 1.3" debug OLED */

/* ---- USB-C ------------------------------------------------------------- */
#define USB_CDC_RX_BUF      4096
#define USB_CDC_TX_BUF      4096

/* ---- BLE rendezvous ---------------------------------------------------- */
#define BLE_DEVICE_NAME    "aurora-phantom"
#define BLE_SERVICE_UUID   "a80d0001-5b3e-4c92-9f1d-0c1b2a3c4d5e"
#define BLE_CHAR_CMD_UUID  "a80d0002-5b3e-4c92-9f1d-0c1b2a3c4d5e"
#define BLE_CHAR_DATA_UUID "a80d0003-5b3e-4c92-9f1d-0c1b2a3c4d5e"
#define BLE_MTU            512

/* ---- Storage / mission ------------------------------------------------- */
#define SD_MISSION_DIR     "/aurora/missions"
#define SD_EVENT_DIR       "/aurora/events"
#define SD_FRAME_DIR       "/aurora/frames"
#define SD_EVENT_BUF_SZ    (256 * 1024)  /* 256 KiB raw-event ring */

/* ---- Power rails (PMIC-controlled) ------------------------------------ */
#define RAIL_SPAD_ANALOG   0x01   /* 3V3 analog for SPAD bias + LC filter */
#define RAIL_FPGA_CORE     0x02   /* 1V2 FPGA core */
#define RAIL_FPGA_IO       0x04   /* 3V3 FPGA I/O + SPI */
#define RAIL_ESP_PERIPH    0x08   /* 3V3 ESP32-S3 peripheral domain */
#define RAIL_RF            0x10   /* BLE/Wi-Fi RF rail — OFF in capture mode */
#define RAIL_USB           0x20   /* USB-C 5V boost / OTG */

#endif /* AURORA_PHANTOM_BOARD_H */