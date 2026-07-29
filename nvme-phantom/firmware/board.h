/*
 * board.h — NVMe-Phantom hardware pin map and peripheral assignments
 *
 * Target:  STM32H563ZIT6 (Cortex-M33, 250 MHz, 2 MB Flash, 640 KB SRAM)
 * Author:  jayis1
 * License: GPL-2.0
 *
 * All pin assignments are for the NVMe-Phantom interposer board rev 1.0.
 * The STM32H5 sits on the control plane; the Lattice ECP5 FPGA handles
 * the Gen3 x4 data plane.  The STM32 talks to the FPGA over SPI1, to the
 * PEX8606 PCIe switch over I2C1, to the nRF52840 BLE module over UART4,
 * to the microSD over SDIO, to the W25Q128 payload NOR over SPI3, to the
 * SSD1306 OLED over I2C2, and to the host over USB.
 */

#ifndef NVME_PHANTOM_BOARD_H
#define NVME_PHANTOM_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ---- Clock configuration ----------------------------------------------- */

#define BOARD_HSE_HZ          16000000UL   /* external 16 MHz crystal */
#define BOARD_SYSCLK_HZ       250000000UL  /* PLL1 Q = HSE * 125 / 8  */
#define BOARD_AHB_HZ          250000000UL
#define BOARD_APB1_HZ         125000000UL
#define BOARD_APB2_HZ         125000000UL
#define BOARD_APB3_HZ         125000000UL

/* ---- LED / button / status pins ---------------------------------------- */

/* PA0  — STATUS_LED (active high, green)
 * PA1  — ERROR_LED  (active high, red)
 * PC13 — USER_BTN   (active low, pull-up, debounced)
 * PA8  — LINK_OK    (input, from PEX8606 LINKUP output)
 */
#define LED_STATUS_PORT       GPIOA
#define LED_STATUS_PIN        0
#define LED_ERROR_PORT        GPIOA
#define LED_ERROR_PIN         1
#define BTN_USER_PORT         GPIOC
#define BTN_USER_PIN          13
#define PEX_LINKOK_PORT       GPIOA
#define PEX_LINKOK_PIN        8

/* ---- SPI1 -> Lattice ECP5 FPGA ----------------------------------------- */
/* Used for:  ECP5 bitstream load (via MSPI) at boot,
 *            runtime rule/command interface (40 MHz) to FPGA data plane.
 * PB3 = SPI1_SCK (AF5), PB4 = SPI1_MISO (AF5), PB5 = SPI1_MOSI (AF5)
 * PA15 = FPGA_CS  (output, active low)
 * PC6  = FPGA_INT (input, FPGA -> MCU, "decoded cmd ready")
 * PC8  = FPGA_CDONE (input, ECP5 config-done)
 * PC9  = FPGA_CRESET (output, ECP5 reset)
 */
#define FPGA_SPI              SPI1
#define FPGA_SCK_PORT         GPIOB
#define FPGA_SCK_PIN          3
#define FPGA_MISO_PORT        GPIOB
#define FPGA_MISO_PIN         4
#define FPGA_MOSI_PORT        GPIOB
#define FPGA_MOSI_PIN         5
#define FPGA_CS_PORT          GPIOA
#define FPGA_CS_PIN           15
#define FPGA_INT_PORT         GPIOC
#define FPGA_INT_PIN          6
#define FPGA_CDONE_PORT       GPIOC
#define FPGA_CDONE_PIN        8
#define FPGA_CRESET_PORT      GPIOC
#define FPGA_CRESET_PIN       9
#define FPGA_SPI_HZ_BOOT      40000000UL
#define FPGA_SPI_HZ_RUNTIME   40000000UL

/* ---- I2C1 -> PEX8606 PCIe switch --------------------------------------- */
/* PB6 = I2C1_SCL (AF4), PB7 = I2C1_SDA (AF4), 400 kHz fast-mode.
 * PEX8606 SMBus address 0x58 (7-bit).
 */
#define PEX_I2C               I2C1
#define PEX_SCL_PORT          GPIOB
#define PEX_SCL_PIN           6
#define PEX_SDA_PORT          GPIOB
#define PEX_SDA_PIN           7
#define PEX_I2C_HZ            400000UL
#define PEX8606_I2C_ADDR      0x58

/* PEX8606 register map (selected) */
#define PEX8606_REG_VID0      0x0000  /* Vendor ID low  */
#define PEX8606_REG_PORTCTRL  0x0C00  /* per-port control */
#define PEX8606_REG_MIRROR    0x0D10  /* port mirror / capture-port config */
#define PEX8606_REG_LANESEL   0x0D20  /* upstream lane width select */
#define PEX8606_REG_ERRCTL    0x0E00  /* error reporting control */
#define PEX8606_REG_MODIFY    0x0D40  /* inline TLP modify override enable */
#define PEX8606_USP           0
#define PEX8606_DSP0_SSD      1
#define PEX8606_DSP1_FPGA     2

/* ---- UART4 -> nRF52840-M2 BLE module ----------------------------------- */
/* PA12 = UART4_TX (AF6), PA11 = UART4_RX (AF6), 1 Mbaud, 8N1.
 * PA9  = BLE_CTS (input), PA10 = BLE_RTS (output)
 * PC10 = BLE_RESET (output, active low)
 * PC11 = BLE_INT  (input, module -> MCU IRQ)
 */
#define BLE_UART              UART4
#define BLE_TX_PORT           GPIOA
#define BLE_TX_PIN            12
#define BLE_RX_PORT           GPIOA
#define BLE_RX_PIN            11
#define BLE_BAUD              1000000UL
#define BLE_RESET_PORT        GPIOC
#define BLE_RESET_PIN         10
#define BLE_INT_PORT          GPIOC
#define BLE_INT_PIN           11

/* ---- SDMMC1 -> microSD (UHS-I) ----------------------------------------- */
/* PC12 = SD_CMD, PC10 = SD_CLK, PC11 = SD_D0, PD2 = SD_D1,
 * PB4  = SD_D2, PB5  = SD_D3  (all AF12, 50 MHz SDIO).
 * PC9  = SD_CD (card detect, active low, pull-up).
 * NOTE: PB4/PB5 are shared with SPI1 via a hardware mux (SD uses SDMMC1 AF12,
 *       SPI1 uses AF5 — only one active at a time; at boot SPI1 is used for
 *       FPGA bitstream load, then SDMMC1 takes over for logging).
 */
#define SD_SDMMC              SDMMC1
#define SD_CMD_PORT           GPIOC
#define SD_CMD_PIN            12
#define SD_CLK_PORT           GPIOC
#define SD_CLK_PIN            10
#define SD_D0_PORT            GPIOC
#define SD_D0_PIN             11
#define SD_D1_PORT            GPIOD
#define SD_D1_PIN             2
#define SD_CD_PORT            GPIOC
#define SD_CD_PIN             9
#define SD_SDMMC_HZ           50000000UL

/* ---- SPI3 -> W25Q128 payload NOR flash --------------------------------- */
/* PC10 = SPI3_SCK (AF6), PC11 = SPI3_MISO (AF6), PC12 = SPI3_MOSI (AF6)
 * PD2  = NOR_CS (output, active low)
 * NOTE: SPI3 pins are on a separate port group from SDMMC1 so both can coexist.
 */
#define NOR_SPI               SPI3
#define NOR_SCK_PORT          GPIOC
#define NOR_SCK_PIN           10
#define NOR_MISO_PORT         GPIOC
#define NOR_MISO_PIN          11
#define NOR_MOSI_PORT         GPIOC
#define NOR_MOSI_PIN          12
#define NOR_CS_PORT           GPIOD
#define NOR_CS_PIN            2
#define NOR_SPI_HZ            50000000UL
#define W25Q128_SIZE_BYTES    0x1000000UL  /* 16 MB */

/* ---- I2C2 -> SSD1306 OLED ---------------------------------------------- */
/* PB10 = I2C2_SCL (AF4), PB11 = I2C2_SDA (AF4), 400 kHz.
 * OLED address 0x3C (7-bit).
 */
#define OLED_I2C              I2C2
#define OLED_SCL_PORT         GPIOB
#define OLED_SCL_PIN          10
#define OLED_SDA_PORT         GPIOB
#define OLED_SDA_PIN          11
#define OLED_I2C_HZ           400000UL
#define SSD1306_I2C_ADDR      0x3C
#define OLED_WIDTH            128
#define OLED_HEIGHT           64

/* ---- USB (STM32H5 built-in USB FS) ------------------------------------- */
/* PA11 = USB_DM, PA12 = USB_DP, USB 2.0 Full-Speed CDC-ACM. */
#define USB_VID               0x1207  /* jayis1 USB VID (open-moco range) */
#define USB_PID               0x9050  /* jayis1 NVMe-Phantom PID */
#define USB_PID_REAL          0x9050
#define USB_CDC_BAUD          115200  /* virtual; USB FS is 12 Mbps */

/* ---- LiPo battery monitor ---------------------------------------------- */
/* PC0 = VBAT_SENSE (ADC1_IN1), 1/3 divider, 12-bit ADC.
 * PC1 = CHRG_STAT (input, from MCP73831, active low while charging).
 */
#define VBAT_ADC              ADC1
#define VBAT_ADC_CHAN         1
#define VBAT_SENSE_PORT       GPIOC
#define VBAT_SENSE_PIN        0
#define CHRG_STAT_PORT        GPIOC
#define CHRG_STAT_PIN         1
#define VBAT_DIVIDER          3       /* external resistor divider */
#define VBAT_FULL_MV          4200
#define VBAT_EMPTY_MV         3300

/* ---- Operating modes --------------------------------------------------- */
typedef enum {
    MODE_PASSIVE_TAP    = 0,   /* capture only, no injection                 */
    MODE_NVME_MITM      = 1,   /* modify/inject NVMe commands in flight      */
    MODE_OPAL_INTERROG  = 2,   /* TCG Opal security command capture/replay   */
    MODE_CTRL_SPOOF     = 3,   /* rewrite Identify Controller response       */
    MODE_DMA_BRIDGE     = 4,   /* storage-shaped DMA read/write primitive    */
    MODE_FW_EXTRACT     = 5,   /* capture vendor firmware-update commands    */
    MODE_HOTPLUG_FAULT  = 6,   /* emulate surprise-removal / replug          */
    MODE_COVERT_EXFIL   = 7,   /* SMART-log / latency-modulation covert chan */
    MODE_SAFE           = 8,   /* pass-through, no capture, no injection     */
    MODE_COUNT
} board_mode_t;

/* ---- Link state reported by PEX8606 ------------------------------------ */
typedef enum {
    LINK_DOWN   = 0,
    LINK_GEN1   = 1,
    LINK_GEN2   = 2,
    LINK_GEN3   = 3,
} link_state_t;

/* ---- Global board state (read by main.c) ------------------------------- */
typedef struct {
    board_mode_t  mode;
    link_state_t  link;
    uint8_t       link_width;       /* 1, 2, 4                                 */
    uint32_t      capture_count;    /* decoded NVMe commands captured          */
    uint32_t      inject_count;     /* commands injected                       */
    uint32_t      sd_free_kb;       /* microSD free space in KB                */
    uint16_t      battery_mv;       /* LiPo voltage in mV                      */
    uint8_t       charging;         /* 1 if charging                           */
    uint8_t       fpga_ready;       /* 1 if ECP5 bitstream loaded & running    */
    uint8_t       switch_ready;     /* 1 if PEX8606 configured                 */
    uint32_t      uptime_s;         /* seconds since boot                      */
} board_state_t;

extern board_state_t g_state;

/* ---- Board init API (implemented in main.c) ---------------------------- */
void board_clock_init(void);
void board_gpio_init(void);
void board_state_init(void);

#endif /* NVME_PHANTOM_BOARD_H */