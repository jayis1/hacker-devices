/**
 * board.h — eMMC Flash Dumper Pin Definitions & Board Configuration
 *
 * Complete pin mapping for STM32H743VIT6 (LQFP-100) on the eMMC Flash Dumper
 * PCB v1.0. All alternate function assignments, peripheral mappings, and
 * board-specific constants are defined here.
 *
 * Copyright (c) 2026 Nous Research
 * SPDX-License-Identifier: MIT
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

/*===========================================================================
 * System Clock Configuration
 *===========================================================================*/

#define HSE_FREQ_HZ         25000000UL   /* 25 MHz external crystal */
#define LSE_FREQ_HZ         32768UL      /* 32.768 kHz RTC crystal */
#define SYSCLK_FREQ_HZ      480000000UL  /* PLL1: 25MHz /5 *192 /2 */
#define HCLK_FREQ_HZ        240000000UL  /* AHB: SYSCLK / 2 */
#define APB1_FREQ_HZ        60000000UL   /* APB1: HCLK / 4 */
#define APB2_FREQ_HZ        120000000UL  /* APB2: HCLK / 2 */
#define APB3_FREQ_HZ        120000000UL  /* APB3: HCLK / 2 */
#define APB4_FREQ_HZ        120000000UL  /* APB4: HCLK / 2 */
#define PLL2_FREQ_HZ        400000000UL  /* PLL2: for SDMMC1, OCTOSPI */
#define PLL3_FREQ_HZ        60000000UL   /* PLL3: for USB ULPI */

/*===========================================================================
 * GPIO Port Base Addresses
 *===========================================================================*/

#define GPIOA_BASE          0x52020800UL
#define GPIOB_BASE          0x52020400UL
#define GPIOC_BASE          0x52020800UL
#define GPIOD_BASE          0x52020C00UL
#define GPIOE_BASE          0x52021000UL
#define GPIOF_BASE          0x52021400UL
#define GPIOG_BASE          0x52021800UL
#define GPIOH_BASE          0x52021C00UL
#define GPIOI_BASE          0x52022000UL

/*===========================================================================
 * GPIO Register Offsets (for direct register access)
 *===========================================================================*/

#define GPIO_MODER_OFFSET   0x00
#define GPIO_OTYPER_OFFSET  0x04
#define GPIO_OSPEEDR_OFFSET 0x08
#define GPIO_PUPDR_OFFSET   0x0C
#define GPIO_IDR_OFFSET     0x10
#define GPIO_ODR_OFFSET     0x14
#define GPIO_BSRR_OFFSET    0x18
#define GPIO_LCKR_OFFSET    0x1C
#define GPIO_AFRL_OFFSET    0x20
#define GPIO_AFRH_OFFSET    0x24

/*===========================================================================
 * Pin Definitions — Organized by Function
 *===========================================================================*/

/*--------------------------------------------------------------------
 * Buttons (PA0-PA3) — Input with pull-up, active low
 *--------------------------------------------------------------------*/
#define BTN_UP_PORT         GPIOA
#define BTN_UP_PIN          0
#define BTN_DOWN_PORT       GPIOA
#define BTN_DOWN_PIN        1
#define BTN_SELECT_PORT     GPIOA
#define BTN_SELECT_PIN      2
#define BTN_BACK_PORT       GPIOA
#define BTN_BACK_PIN        3

/*--------------------------------------------------------------------
 * FPGA Configuration SPI (SPI1: PA4-PA7)
 *--------------------------------------------------------------------*/
#define FPGA_SPI_SCK_PORT   GPIOA
#define FPGA_SPI_SCK_PIN    5
#define FPGA_SPI_SCK_AF     5

#define FPGA_SPI_MISO_PORT  GPIOA
#define FPGA_SPI_MISO_PIN   6
#define FPGA_SPI_MISO_AF    5

#define FPGA_SPI_MOSI_PORT  GPIOA
#define FPGA_SPI_MOSI_PIN   7
#define FPGA_SPI_MOSI_AF    5

#define FPGA_SPI_CS_PORT    GPIOA
#define FPGA_SPI_CS_PIN     4
#define FPGA_SPI_CS_AF      5

/*--------------------------------------------------------------------
 * FPGA Control Signals
 *--------------------------------------------------------------------*/
#define FPGA_CRESET_PORT    GPIOB
#define FPGA_CRESET_PIN     12
#define FPGA_CDONE_PORT     GPIOB
#define FPGA_CDONE_PIN      13
#define FPGA_INTR_PORT      GPIOG
#define FPGA_INTR_PIN       6
#define FPGA_INTR_EXTI_LINE 6

/*--------------------------------------------------------------------
 * OCTOSPI1 — Shared bus for FPGA runtime SPI + SPI NOR target
 * PA8=IO0, PA9=IO1, PA10=IO2, PA11=IO3, PA12=CLK, PB2=NCS
 *--------------------------------------------------------------------*/
#define QSPI_IO0_PORT       GPIOA
#define QSPI_IO0_PIN        8
#define QSPI_IO0_AF         10

#define QSPI_IO1_PORT       GPIOA
#define QSPI_IO1_PIN        9
#define QSPI_IO1_AF         10

#define QSPI_IO2_PORT       GPIOA
#define QSPI_IO2_PIN        10
#define QSPI_IO2_AF         10

#define QSPI_IO3_PORT       GPIOA
#define QSPI_IO3_PIN        11
#define QSPI_IO3_AF         10

#define QSPI_CLK_PORT       GPIOA
#define QSPI_CLK_PIN        12
#define QSPI_CLK_AF         10

#define QSPI_NCS_PORT       GPIOB
#define QSPI_NCS_PIN        2
#define QSPI_NCS_AF         10

/*--------------------------------------------------------------------
 * I2C1 — OLED Display (PB0=SCL, PB1=SDA)
 *--------------------------------------------------------------------*/
#define I2C1_SCL_PORT       GPIOB
#define I2C1_SCL_PIN        0
#define I2C1_SCL_AF         4

#define I2C1_SDA_PORT       GPIOB
#define I2C1_SDA_PIN        1
#define I2C1_SDA_AF         4

/*--------------------------------------------------------------------
 * USB OTG_HS ULPI Interface (PB5-PB15, PC0)
 *--------------------------------------------------------------------*/
#define ULPI_CLK_PORT       GPIOC
#define ULPI_CLK_PIN        0
#define ULPI_CLK_AF         10

#define ULPI_DATA0_PORT     GPIOB
#define ULPI_DATA0_PIN      12
#define ULPI_DATA0_AF       10

#define ULPI_DATA1_PORT     GPIOB
#define ULPI_DATA1_PIN      11
#define ULPI_DATA1_AF       10

#define ULPI_DATA2_PORT     GPIOB
#define ULPI_DATA2_PIN      10
#define ULPI_DATA2_AF       10

#define ULPI_DATA3_PORT     GPIOB
#define ULPI_DATA3_PIN      9
#define ULPI_DATA3_AF       10

#define ULPI_DATA4_PORT     GPIOB
#define ULPI_DATA4_PIN      8
#define ULPI_DATA4_AF       10

#define ULPI_DATA5_PORT     GPIOB
#define ULPI_DATA5_PIN      7
#define ULPI_DATA5_AF       10

#define ULPI_DATA6_PORT     GPIOB
#define ULPI_DATA6_PIN      6
#define ULPI_DATA6_AF       10

#define ULPI_DATA7_PORT     GPIOB
#define ULPI_DATA7_PIN      5
#define ULPI_DATA7_AF       10

#define ULPI_DIR_PORT       GPIOB
#define ULPI_DIR_PIN        14
#define ULPI_DIR_AF         10

#define ULPI_STP_PORT       GPIOB
#define ULPI_STP_PIN        15
#define ULPI_STP_AF         10

#define ULPI_NXT_PORT       GPIOB
#define ULPI_NXT_PIN        13
#define ULPI_NXT_AF         10

/*--------------------------------------------------------------------
 * SDMMC2 — eMMC Interface (PC8-PC12, PD2-PD4, PC6-PC7)
 *--------------------------------------------------------------------*/
#define EMMC_CLK_PORT       GPIOC
#define EMMC_CLK_PIN        12
#define EMMC_CLK_AF         12

#define EMMC_CMD_PORT       GPIOD
#define EMMC_CMD_PIN        2
#define EMMC_CMD_AF         12

#define EMMC_DAT0_PORT      GPIOC
#define EMMC_DAT0_PIN       8
#define EMMC_DAT0_AF        12

#define EMMC_DAT1_PORT      GPIOC
#define EMMC_DAT1_PIN       9
#define EMMC_DAT1_AF        12

#define EMMC_DAT2_PORT      GPIOC
#define EMMC_DAT2_PIN       10
#define EMMC_DAT2_AF        12

#define EMMC_DAT3_PORT      GPIOC
#define EMMC_DAT3_PIN       11
#define EMMC_DAT3_AF        12

#define EMMC_DAT4_PORT      GPIOD
#define EMMC_DAT4_PIN       3
#define EMMC_DAT4_AF        12

#define EMMC_DAT5_PORT      GPIOC
#define EMMC_DAT5_PIN       6
#define EMMC_DAT5_AF        12

#define EMMC_DAT6_PORT      GPIOC
#define EMMC_DAT6_PIN       7
#define EMMC_DAT6_AF        12

#define EMMC_DAT7_PORT      GPIOD
#define EMMC_DAT7_PIN       4
#define EMMC_DAT7_AF        12

/*--------------------------------------------------------------------
 * SDMMC1 — microSD Card (PD8-PD12)
 *--------------------------------------------------------------------*/
#define SD_CLK_PORT         GPIOD
#define SD_CLK_PIN          12
#define SD_CLK_AF           12

#define SD_CMD_PORT         GPIOD
#define SD_CMD_PIN          8
#define SD_CMD_AF           12

#define SD_DAT0_PORT        GPIOD
#define SD_DAT0_PIN         9
#define SD_DAT0_AF          12

#define SD_DAT1_PORT        GPIOD
#define SD_DAT1_PIN         10
#define SD_DAT1_AF          12

#define SD_DAT2_PORT        GPIOD
#define SD_DAT2_PIN         11
#define SD_DAT2_AF          12

#define SD_DAT3_PORT        GPIOD
#define SD_DAT3_PIN         13
#define SD_DAT3_AF          12

/*--------------------------------------------------------------------
 * FMC — DDR3L SDRAM Interface
 * Address: PF0-PF5, PF12-PF15, PG0-PG5
 * Data:    PH8-PH15, PI0-PI3, PD0-PD1, PD14-PD15, PE7-PE8
 * DQS:     PE9-PE10 (UDQS), PE13-PE14 (LDQS)
 * DM:      PE11 (LDM), PE12 (UDM)
 * Control: PC0(WE#), PC2(CS#), PC3(CKE), PF11(RAS#), PG15(CAS#), PG8(ODT)
 * Bank:    PE15(BA0), PB8(BA1), PB9(BA2)
 *--------------------------------------------------------------------*/
#define FMC_AF              12

/* Address pins */
#define FMC_A0_PORT         GPIOF
#define FMC_A0_PIN          0
#define FMC_A1_PORT         GPIOF
#define FMC_A1_PIN          1
#define FMC_A2_PORT         GPIOF
#define FMC_A2_PIN          2
#define FMC_A3_PORT         GPIOF
#define FMC_A3_PIN          3
#define FMC_A4_PORT         GPIOF
#define FMC_A4_PIN          4
#define FMC_A5_PORT         GPIOF
#define FMC_A5_PIN          5
#define FMC_A6_PORT         GPIOF
#define FMC_A6_PIN          12
#define FMC_A7_PORT         GPIOF
#define FMC_A7_PIN          13
#define FMC_A8_PORT         GPIOF
#define FMC_A8_PIN          14
#define FMC_A9_PORT         GPIOF
#define FMC_A9_PIN          15
#define FMC_A10_PORT        GPIOG
#define FMC_A10_PIN         0
#define FMC_A11_PORT        GPIOG
#define FMC_A11_PIN         1
#define FMC_A12_PORT        GPIOG
#define FMC_A12_PIN         2
#define FMC_A13_PORT        GPIOG
#define FMC_A13_PIN         3
#define FMC_A14_PORT        GPIOG
#define FMC_A14_PIN         4
#define FMC_A15_PORT        GPIOG
#define FMC_A15_PIN         5

/* Data pins — lower byte lane */
#define FMC_D0_PORT         GPIOH
#define FMC_D0_PIN          8
#define FMC_D1_PORT         GPIOH
#define FMC_D1_PIN          9
#define FMC_D2_PORT         GPIOH
#define FMC_D2_PIN          10
#define FMC_D3_PORT         GPIOH
#define FMC_D3_PIN          11
#define FMC_D4_PORT         GPIOH
#define FMC_D4_PIN          12
#define FMC_D5_PORT         GPIOH
#define FMC_D5_PIN          13
#define FMC_D6_PORT         GPIOH
#define FMC_D6_PIN          14
#define FMC_D7_PORT         GPIOH
#define FMC_D7_PIN          15

/* Data pins — upper byte lane */
#define FMC_D8_PORT         GPIOI
#define FMC_D8_PIN          0
#define FMC_D9_PORT         GPIOI
#define FMC_D9_PIN          1
#define FMC_D10_PORT        GPIOD
#define FMC_D10_PIN         0
#define FMC_D11_PORT        GPIOD
#define FMC_D11_PIN         1
#define FMC_D12_PORT        GPIOD
#define FMC_D12_PIN         14
#define FMC_D13_PORT        GPIOD
#define FMC_D13_PIN         15
#define FMC_D14_PORT        GPIOE
#define FMC_D14_PIN         7
#define FMC_D15_PORT        GPIOE
#define FMC_D15_PIN         8

/* DQS and DM */
#define FMC_LDQS_PORT       GPIOE
#define FMC_LDQS_PIN        14
#define FMC_LDQS_N_PORT     GPIOE
#define FMC_LDQS_N_PIN      13
#define FMC_UDQS_PORT       GPIOE
#define FMC_UDQS_PIN        10
#define FMC_UDQS_N_PORT     GPIOE
#define FMC_UDQS_N_PIN      9
#define FMC_LDM_PORT        GPIOE
#define FMC_LDM_PIN         11
#define FMC_UDM_PORT        GPIOE
#define FMC_UDM_PIN         12

/* Bank address */
#define FMC_BA0_PORT        GPIOE
#define FMC_BA0_PIN         15
#define FMC_BA1_PORT        GPIOB
#define FMC_BA1_PIN         8
#define FMC_BA2_PORT        GPIOB
#define FMC_BA2_PIN         9

/* Control */
#define FMC_SDCLK_PORT      GPIOG
#define FMC_SDCLK_PIN       8
#define FMC_SDCLK_N_PORT    GPIOG
#define FMC_SDCLK_N_PIN     7
#define FMC_SDCKE0_PORT     GPIOC
#define FMC_SDCKE0_PIN      3
#define FMC_SDNE0_PORT      GPIOC
#define FMC_SDNE0_PIN       2
#define FMC_SDNRAS_PORT     GPIOF
#define FMC_SDNRAS_PIN      11
#define FMC_SDNCAS_PORT     GPIOG
#define FMC_SDNCAS_PIN      15
#define FMC_SDNWE_PORT      GPIOC
#define FMC_SDNWE_PIN       0
#define FMC_ODT_PORT        GPIOG
#define FMC_ODT_PIN         8

/*--------------------------------------------------------------------
 * FMC — NAND Flash Interface (via FPGA)
 *--------------------------------------------------------------------*/
#define FMC_NAND_D0_PORT    GPIOD
#define FMC_NAND_D0_PIN     14
#define FMC_NAND_D1_PORT    GPIOD
#define FMC_NAND_D1_PIN     15
#define FMC_NAND_D2_PORT    GPIOD
#define FMC_NAND_D2_PIN     0
#define FMC_NAND_D3_PORT    GPIOD
#define FMC_NAND_D3_PIN     1
#define FMC_NAND_D4_PORT    GPIOE
#define FMC_NAND_D4_PIN     7
#define FMC_NAND_D5_PORT    GPIOE
#define FMC_NAND_D5_PIN     8
#define FMC_NAND_D6_PORT    GPIOE
#define FMC_NAND_D6_PIN     9
#define FMC_NAND_D7_PORT    GPIOE
#define FMC_NAND_D7_PIN     10

#define FMC_NOE_PORT        GPIOD
#define FMC_NOE_PIN         4
#define FMC_NWE_PORT        GPIOD
#define FMC_NWE_PIN         5
#define FMC_NCE_PORT        GPIOG
#define FMC_NCE_PIN         9
#define FMC_NWAIT_PORT      GPIOD
#define FMC_NWAIT_PIN       6
#define FMC_INT_PORT        GPIOF
#define FMC_INT_PIN         10

/*--------------------------------------------------------------------
 * WS2812B RGB LEDs (PB14, PB15) — Bit-banged via SPI or timer PWM
 *--------------------------------------------------------------------*/
#define LED1_DATA_PORT      GPIOB
#define LED1_DATA_PIN       14
#define LED2_DATA_PORT      GPIOB
#define LED2_DATA_PIN       15

/*--------------------------------------------------------------------
 * Buzzer (PE2) — TIM1_CH3 PWM output
 *--------------------------------------------------------------------*/
#define BUZZER_PORT         GPIOE
#define BUZZER_PIN          2
#define BUZZER_AF           1
#define BUZZER_TIM_CHANNEL  3

/*--------------------------------------------------------------------
 * SWD Debug Header (PA13=SWDIO, PA14=SWCLK, PB3=SWO)
 *--------------------------------------------------------------------*/
#define SWDIO_PORT          GPIOA
#define SWDIO_PIN           13
#define SWCLK_PORT          GPIOA
#define SWCLK_PIN           14
#define SWO_PORT            GPIOB
#define SWO_PIN             3

/*--------------------------------------------------------------------
 * NRST Pin
 *--------------------------------------------------------------------*/
#define NRST_PORT           GPIOH
#define NRST_PIN            0  /* Actually dedicated NRST pin, not GPIO */

/*--------------------------------------------------------------------
 * HSE / LSE Crystals
 *--------------------------------------------------------------------*/
#define HSE_IN_PORT         GPIOH
#define HSE_IN_PIN          0
#define HSE_OUT_PORT        GPIOH
#define HSE_OUT_PIN         1
#define LSE_IN_PORT         GPIOC
#define LSE_IN_PIN          14
#define LSE_OUT_PORT        GPIOC
#define LSE_OUT_PIN         15

/*===========================================================================
 * Peripheral Instance Macros
 *===========================================================================*/

#define I2C1_INSTANCE       ((I2C_TypeDef *)0x50005400UL)
#define SPI1_INSTANCE       ((SPI_TypeDef *)0x50013000UL)
#define OCTOSPI1_INSTANCE   ((OCTOSPI_TypeDef *)0x52005000UL)
#define SDMMC1_INSTANCE     ((SDMMC_TypeDef *)0x52007000UL)
#define SDMMC2_INSTANCE     ((SDMMC_TypeDef *)0x52007400UL)
#define USB_OTG_HS_INSTANCE ((USB_OTG_HS_TypeDef *)0x50040000UL)
#define HASH_INSTANCE       ((HASH_TypeDef *)0x50060400UL)
#define TIM1_INSTANCE       ((TIM_TypeDef *)0x50010000UL)
#define TIM2_INSTANCE       ((TIM_TypeDef *)0x50000000UL)
#define RTC_INSTANCE        ((RTC_TypeDef *)0x52002800UL)
#define FMC_INSTANCE        ((FMC_TypeDef *)0x52004000UL)

/*===========================================================================
 * I2C Addresses
 *===========================================================================*/

#define OLED_I2C_ADDR       0x3C
#define PMIC_I2C_ADDR       0x24
#define CHARGER_I2C_ADDR    0x6A

/*===========================================================================
 * FPGA Register Map (accessed via SPI)
 *===========================================================================*/

#define FPGA_REG_ID         0x00
#define FPGA_REG_CTRL       0x04
#define FPGA_REG_STATUS     0x08
#define FPGA_REG_NAND_TIM0  0x0C
#define FPGA_REG_NAND_TIM1  0x10
#define FPGA_REG_NAND_TIM2  0x14
#define FPGA_REG_NAND_DIN   0x18
#define FPGA_REG_NAND_DOUT  0x1C
#define FPGA_REG_NAND_CMD   0x20
#define FPGA_REG_NAND_ADDR  0x24
#define FPGA_REG_FIFO_COUNT 0x28
#define FPGA_REG_FIFO_DATA  0x2C
#define FPGA_REG_INTR_MASK  0x30
#define FPGA_REG_INTR_STAT  0x34

#define FPGA_EXPECTED_ID    0x464E4144UL  /* "FNAD" */

/* FPGA interrupt flags */
#define FPGA_INTR_RB_RISE   (1 << 0)
#define FPGA_INTR_RB_FALL   (1 << 1)
#define FPGA_INTR_FIFO_THR  (1 << 2)
#define FPGA_INTR_FIFO_OVF  (1 << 3)
#define FPGA_INTR_CMD_DONE  (1 << 4)

/*===========================================================================
 * SDRAM Configuration Constants
 *===========================================================================*/

#define SDRAM_BASE_ADDR     0xC0000000UL
#define SDRAM_SIZE          0x10000000UL  /* 256 MB */
#define SDRAM_COL_BITS      9
#define SDRAM_ROW_BITS      13
#define SDRAM_DATA_WIDTH    16
#define SDRAM_CAS_LATENCY   6
#define SDRAM_T_RCD_NS      13.75
#define SDRAM_T_RP_NS       13.75
#define SDRAM_T_RAS_NS      35.0
#define SDRAM_T_RC_NS       48.75
#define SDRAM_T_WR_NS       15.0
#define SDRAM_T_RFC_NS      260.0
#define SDRAM_T_XSR_NS      140.0
#define SDRAM_T_MRD_NS      10.0
#define SDRAM_REFRESH_US    7.8

/*===========================================================================
 * Ring Buffer Configuration
 *===========================================================================*/

#define RING_BUFFER_BASE    SDRAM_BASE_ADDR
#define RING_BUFFER_SIZE    0x0F000000UL  /* 240 MB for acquisition */
#define RING_BLOCK_SIZE     65536UL       /* 64 KB blocks */
#define RING_BLOCK_COUNT    (RING_BUFFER_SIZE / RING_BLOCK_SIZE)

/*===========================================================================
 * USB Configuration
 *===========================================================================*/

#define USB_VID             0x4E4F  /* "NO" — Nous */
#define USB_PID             0x4644  /* "FD" — Flash Dumper */
#define USB_BCD_DEVICE      0x0100  /* v1.0 */
#define USB_MANUFACTURER    "Nous Research"
#define USB_PRODUCT         "eMMC Flash Dumper"
#define USB_SERIAL_TEMPLATE "EMFD-%08X"
#define USB_BULK_EP_IN      0x81   /* EP1 IN */
#define USB_BULK_EP_OUT     0x01   /* EP1 OUT */
#define USB_BULK_EP_SIZE    512    /* HS bulk max packet */
#define USB_BULK_EP_BURST   16     /* SuperSpeed burst size */

/*===========================================================================
 * OLED Display Configuration
 *===========================================================================*/

#define OLED_WIDTH          128
#define OLED_HEIGHT         64
#define OLED_PAGES          (OLED_HEIGHT / 8)
#define OLED_FRAMEBUF_SIZE  (OLED_WIDTH * OLED_PAGES)

/*===========================================================================
 * Button Debounce Configuration
 *===========================================================================*/

#define BTN_DEBOUNCE_MS     20
#define BTN_LONG_PRESS_MS   800
#define BTN_REPEAT_MS       200

/*===========================================================================
 * Battery Thresholds
 *===========================================================================*/

#define BATTERY_FULL_MV     4200
#define BATTERY_LOW_MV      3500
#define BATTERY_CRITICAL_MV 3300
#define BATTERY_SHUTDOWN_MV 3100

/*===========================================================================
 * Helper Macros
 *===========================================================================*/

#define BIT(n)              (1UL << (n))
#define ARRAY_SIZE(a)       (sizeof(a) / sizeof((a)[0]))
#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#define MAX(a, b)           ((a) > (b) ? (a) : (b))
#define ALIGN_UP(x, a)      (((x) + (a) - 1) & ~((a) - 1))
#define ALIGN_DOWN(x, a)    ((x) & ~((a) - 1))

#endif /* BOARD_H */
