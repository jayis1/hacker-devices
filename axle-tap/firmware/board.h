/*
 * board.h — Pin/peripheral map and board macros for AxleTap
 * Automotive Ethernet (100/1000BASE-T1) Tap, MITM & gPTP spoofing platform
 *
 * Author: jayis1
 * License: GPL-2.0
 * Date:   2026-07-22
 *
 * STM32H723ZGT6 (LQFP144) pin assignments.
 * Two Marvell 88Q2112 1000BASE-T1 PHYs are connected via RGMII to the
 * STM32H7's built-in Ethernet MAC (PHY A) and to an external Marvell
 * 88E6141 L2 switch fabric (PHY B), which is in turn controlled over
 * MDIO from the STM32H7.
 */

#ifndef AXLETAP_BOARD_H
#define AXLETAP_BOARD_H

#include "registers.h"

/* ------------------------------------------------------------------ */
/* Clock configuration                                                  */
/* ------------------------------------------------------------------ */
#define HSE_VALUE_HZ        25000000UL   /* 25 MHz crystal */
#define HSI_VALUE_HZ        64000000UL   /* 64 MHz HSI */
#define SYSCLK_HZ           550000000UL  /* 550 MHz PLL1 output */
#define HCLK_HZ             275000000UL  /* 275 MHz AHB after /2 (axi div) */
#define APB1_HZ             137500000UL
#define APB2_HZ             137500000UL
#define PCLK4_HZ            137500000UL

/* TIM2 clock for gPTP fine timer: 2x PCLK1 = 275 MHz, gives 3.64 ns tick */
#define GPTP_TIMER_HZ       275000000UL

/* ------------------------------------------------------------------ */
/* GPIO pin map                                                         */
/* ------------------------------------------------------------------ */

/* Ethernet MAC1 RGMII pins (to PHY A — Marvell 88Q2112 #1) */
#define ETH1_TX_CLK_PIN     1   /* PA1  AF11 RGMII_TX_CLK */
#define ETH1_TX_CTL_PIN     2   /* PA2  AF11 RGMII_TX_EN */
#define ETH1_TXD0_PIN       3   /* PA7  AF11 RGMII_TXD0 */
#define ETH1_TXD1_PIN       4   /* PC1  AF11 RGMII_TXD1 */
#define ETH1_TXD2_PIN       5   /* PC4  AF11 RGMII_TXD2 */
#define ETH1_TXD3_PIN       6   /* PC5  AF11 RGMII_TXD3 */
#define ETH1_RX_CLK_PIN     7   /* PA3  AF11 RGMII_RX_CLK */
#define ETH1_RX_CTL_PIN     8   /* PC0  AF11 RGMII_RX_DV */
#define ETH1_RXD0_PIN       9   /* PC8  AF11 RGMII_RXD0 */
#define ETH1_RXD1_PIN      10   /* PC9  AF11 RGMII_RXD1 */
#define ETH1_RXD2_PIN      11   /* PA8  AF11 RGMII_RXD2 */
#define ETH1_RXD3_PIN      12   /* PA9  AF11 RGMII_RXD3 */

/* MDIO for both PHYs and the L2 switch */
#define ETH_MDC_PIN         13   /* PC2  AF11 MDC */
#define ETH_MDIO_PIN        14   /* PA10 AF11 MDIO */

/* PHY A / PHY B reset (active low) */
#define PHYA_RESET_PORT     GPIOA
#define PHYA_RESET_PIN      11   /* PA11 */
#define PHYB_RESET_PORT     GPIOA
#define PHYB_RESET_PIN      12   /* PA12 */

/* PHY A / PHY B IRQ / link status (input) */
#define PHYA_LINK_PORT      GPIOC
#define PHYA_LINK_PIN       13   /* PC13 — bluepill user button */
#define PHYB_LINK_PORT      GPIOC
#define PHYB_LINK_PIN       14   /* PC14 */

/* USART3 — debug console (PD8/PD12 on STM32H723Z LQFP144) */
#define DBG_UART            USART3
#define DBG_TX_PORT         GPIOD
#define DBG_TX_PIN          8
#define DBG_RX_PORT         GPIOD
#define DBG_RX_PIN          9
#define DBG_UART_AF         7

/* USB OTG-HS — uses ULPI-less FS-mode via PA11/PA12 internally; on
 * STM32H723 the OTG-HS peripheral can run in built-in FS PHY mode.
 */
#define USB_DM_PORT        GPIOA
#define USB_DM_PIN         11
#define USB_DP_PORT        GPIOA
#define USB_DP_PIN         12
#define USB_AF             10

/* SPI6 — nRF52820 BLE bridge */
#define BLE_SPI             SPI6
#define BLE_SCK_PORT        GPIOG
#define BLE_SCK_PIN         13   /* PG13 AF5 */
#define BLE_MISO_PORT       GPIOG
#define BLE_MISO_PIN        12   /* PG12 AF5 */
#define BLE_MOSI_PORT       GPIOG
#define BLE_MOSI_PIN        14   /* PG14 AF5 */
#define BLE_CS_PORT         GPIOG
#define BLE_CS_PIN          15   /* PG15 GPIO */
#define BLE_IRQ_PORT        GPIOG
#define BLE_IRQ_PIN         9    /* PG9 EXTI9 */
#define BLE_AF              5

/* SDMMC1 — SD card capture (PC8..PC12 + PD2 CMD) */
#define SDMMC_D0_PORT       GPIOC
#define SDMMC_D0_PIN        8
#define SDMMC_D1_PORT       GPIOC
#define SDMMC_D1_PIN        9
#define SDMMC_D2_PORT       GPIOC
#define SDMMC_D2_PIN       10
#define SDMMC_D3_PORT       GPIOC
#define SDMMC_D3_PIN       11
#define SDMMC_CK_PORT       GPIOC
#define SDMMC_CK_PIN       12
#define SDMMC_CMD_PORT      GPIOD
#define SDMMC_CMD_PIN       2
#define SDMMC_AF            12

/* Arming switch (input, active low — read PC15) */
#define ARM_SWITCH_PORT     GPIOC
#define ARM_SWITCH_PIN      15
/* Kill key (jumper — read PB0) */
#define KILL_KEY_PORT       GPIOB
#define KILL_KEY_PIN        0

/* Status LEDs (active high) */
#define LED_LINKA_PORT      GPIOE
#define LED_LINKA_PIN       0
#define LED_LINKB_PORT      GPIOE
#define LED_LINKB_PIN       1
#define LED_ARM_PORT         GPIOE
#define LED_ARM_PIN          2
#define LED_CAP_PORT         GPIOE
#define LED_CAP_PIN          3
#define LED_BLE_PORT         GPIOE
#define LED_BLE_PIN          4

/* ------------------------------------------------------------------ */
/* MDIO addresses (assigned by PHY strap)                               */
/* ------------------------------------------------------------------ */
#define PHYA_MDIO_ADDR       0x01
#define PHYB_MDIO_ADDR       0x02
#define SWITCH_MDIO_ADDR     0x10   /* Marvell 88E6141 L2 switch */

/* ------------------------------------------------------------------ */
/* Marvell 88Q2112 PHY register map (subset)                            */
/* ------------------------------------------------------------------ */
#define PHY_BMCR            0x00    /* Basic mode control */
#define PHY_BMSR            0x01    /* Basic mode status */
#define PHY_ID1             0x02
#define PHY_ID2             0x03
#define PHY_ANAR            0x04    /* Auto-neg advertisement */
#define PHY_ANLPAR          0x05
#define PHY_100T1_CTRL      0x08    /* 100BASE-T1 control */
#define PHY_1000T1_CTRL     0x09    /* 1000BASE-T1 control */
#define PHY_1000T1_STAT     0x0A
#define PHY_MMD_CTRL        0x0D
#define PHY_MMD_DATA        0x0E
#define PHY_EXT_STATUS      0x1F

/* BMCR bits */
#define BMCR_RESET          0x8000
#define BMCR_LOOPBACK       0x4000
#define BMCR_SPEED_LSB      0x2000
#define BMCR_ANEG_EN        0x1000
#define BMCR_POWER_DOWN     0x0800
#define BMCR_ISOLATE        0x0400
#define BMCR_RESTART_ANEG   0x0200
#define BMCR_DUPLEX         0x0100
#define BMCR_SPEED_MSB      0x0040

/* BMSR bits */
#define BMSR_1000T1         0x0800
#define BMSR_100T1          0x0400
#define BMSR_ANEG_DONE      0x0020
#define BMSR_LINK           0x0004

/* ------------------------------------------------------------------ */
/* Memory map for AxleTap — link script constants                       */
/* ------------------------------------------------------------------ */
/* ITCM: 64 KB at 0x00000000 — bridge engine + gPTP stack */
#define ITCM_BASE           0x00000000UL
#define ITCM_SIZE           0x00010000UL
/* DTCM: 128 KB at 0x20000000 — DMA descriptors, frame rings */
#define DTCM_BASE           0x20000000UL
#define DTCM_SIZE           0x00020000UL
/* AXI SRAM: 1 MB at 0x24000000 — frame buffers, PCAP, service map */
#define AXI_SRAM_BASE       0x24000000UL
#define AXI_SRAM_SIZE       0x00100000UL
/* Flash: 2 MB at 0x08000000 */
#define FLASH_BASE_ADDR     0x08000000UL
#define FLASH_SIZE          0x00200000UL

/* Bridge engine placement macros — force functions into ITCM via
 * a section attribute consumed by the linker script.
 */
#define ITCM_FUNC           __attribute__((section(".itcm"), long_call))
#define DTCM_DATA           __attribute__((section(".dtcm")))

/* ------------------------------------------------------------------ */
/* Ring buffer sizes                                                     */
/* ------------------------------------------------------------------ */
#define ETH_RX_DESC_COUNT   32
#define ETH_TX_DESC_COUNT   16
#define ETH_MAX_FRAME       1518
#define ETH_JUMBO_FRAME     9022

/* PCAP / SD card */
#define PCAP_BLOCK_SIZE     512
#define PCAP_RING_BLOCKS    (512UL * 1024UL)   /* 256 MB ring */

/* BLE backhaul MTU (encrypted) */
#define BLE_MTU             247

/* ------------------------------------------------------------------ */
/* Interlock helper macros                                              */
/* ------------------------------------------------------------------ */
/* Returns 1 only if arming switch is closed (PC15 == 0, active low)
 * AND kill key is not set (PB0 == 1, no jumper = transmit allowed).
 * The firmware refuses to enable TX unless armed() == 1.
 */
static inline int arm_switch_closed(void)
{
    return (ARM_SWITCH_PORT->IDR & BIT(ARM_SWITCH_PIN)) == 0;
}
static inline int kill_key_set(void)
{
    /* PB0 reads 0 with jumper installed (pulled high otherwise) */
    return (KILL_KEY_PORT->IDR & BIT(KILL_KEY_PIN)) == 0;
}
static inline int armed(void)
{
    return arm_switch_closed() && !kill_key_set();
}

/* LED helpers */
static inline void led_on(volatile uint32_t *bsrr, int pin)  { *bsrr = BIT(pin); }
static inline void led_off(volatile uint32_t *bsrr, int pin) { *bsrr = BIT(pin) << 16; }
static inline void led_linka_set(int on) { on ? led_on(&LED_LINKA_PORT->BSRR, LED_LINKA_PIN) : led_off(&LED_LINKA_PORT->BSRR, LED_LINKA_PIN); }
static inline void led_linkb_set(int on) { on ? led_on(&LED_LINKB_PORT->BSRR, LED_LINKB_PIN) : led_off(&LED_LINKB_PORT->BSRR, LED_LINKB_PIN); }
static inline void led_arm_set(int on)    { on ? led_on(&LED_ARM_PORT->BSRR, LED_ARM_PIN)    : led_off(&LED_ARM_PORT->BSRR, LED_ARM_PIN); }
static inline void led_cap_set(int on)    { on ? led_on(&LED_CAP_PORT->BSRR, LED_CAP_PIN)    : led_off(&LED_CAP_PORT->BSRR, LED_CAP_PIN); }
static inline void led_ble_set(int on)   { on ? led_on(&LED_BLE_PORT->BSRR, LED_BLE_PIN)    : led_off(&LED_BLE_PORT->BSRR, LED_BLE_PIN); }

#endif /* AXLETAP_BOARD_H */