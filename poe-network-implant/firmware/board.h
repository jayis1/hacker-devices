/*
 * board.h — PhantomBridge PoE Network Implant
 * Hardware definitions, pin mappings, and board-specific constants
 */

#ifndef BOARD_H
#define BOARD_H

#include "registers.h"

/* ===== Clock Configuration ===== */
#define HSI_FREQ_HZ         64000000UL   /* HSI internal RC */
#define SYSCLK_FREQ_HZ      480000000UL /* PLL1 output = SYSCLK */
#define HCLK_FREQ_HZ        240000000UL /* AHB prescaler /2 */
#define APB1_FREQ_HZ        120000000UL /* APB1 prescaler /2 */
#define APB2_FREQ_HZ        120000000UL /* APB2 prescaler /2 */
#define FMC_CLK_FREQ_HZ     200000000UL /* PLL2 output for FMC/SDRAM */
#define ETH_REFCLK_HZ       50000000UL  /* PLL3 output for ETH REF */

/* PLL1: HSI(64MHz) / M(2) * N(120) / P(2) = 480MHz */
#define PLL1_M              2
#define PLL1_N              120
#define PLL1_P              2
#define PLL1_Q              10  /* 48MHz for USB (unused) */
#define PLL1_R              2

/* PLL2: HSI(64MHz) / M(2) * N(100) / P(2) = 200MHz */
#define PLL2_M              2
#define PLL2_N              100
#define PLL2_P              2

/* PLL3: HSI(64MHz) / M(2) * N(50) / P(2) = 50MHz */
#define PLL3_M              2
#define PLL3_N              50
#define PLL3_P              2

/* ===== Power Rails (for telemetry) ===== */
#define VDD_3V3_MV          3300
#define VDD_1V2_MV          1200
#define VDD_1V8_MV          1800
#define VDD_2V5_MV          2500
#define VPOE_MV             48000

/* ===== GPIO Pin Definitions ===== */

/* --- RGMII Ethernet (Port A) --- */
#define ETH_MDC_PIN          0   /* PA0  AF11 */
#define ETH_MDIO_PIN         1   /* PA1  AF11 */
#define ETH_CRSDV_PIN        7   /* PA7  AF11 */
#define ETH_TXCLK_PIN        8   /* PA8  AF11 */
#define ETH_TXD0_PIN         9   /* PA9  AF11 */
#define ETH_TXD1_PIN         10  /* PA10 AF11 */
#define ETH_TXD2_PIN         11  /* PA11 AF11 */
#define ETH_TXD3_PIN         12  /* PA12 AF11 */
#define ETH_TXCTL_PIN        13  /* PA13 AF11 */

/* --- RGMII Ethernet (Port B) --- */
#define ETH_RXD0_PIN         0   /* PB0  AF11 */
#define ETH_RXD1_PIN         1   /* PB1  AF11 */
#define ETH_RXD2_PIN         2   /* PB2  AF11 */
#define ETH_RXD3_PIN         3   /* PB3  AF11 */
#define ETH_RXCTL_PIN        4   /* PB4  AF11 */
#define ETH_RXCLK_PIN        5   /* PB5  AF11 */

/* --- SPI2 (Flash) --- */
#define FLASH_SCK_PIN        14  /* PB14 AF5 */
#define FLASH_MOSI_PIN       15  /* PB15 AF5 */
#define FLASH_MISO_PIN       4   /* PC4  AF5  */
#define FLASH_CS_PIN         6   /* PC6  GPIO */

/* --- UART4 (BLE) --- */
#define BLE_UART_TX_PIN      1   /* PC1  AF8 */
#define BLE_UART_RX_PIN      2   /* PC2  AF8 */
#define BLE_RESET_PIN        6   /* PC6  GPIO (shared with CS, muxed) */

/* --- I2C1 (PoE) --- */
#define POE_SCL_PIN          3   /* PC3  AF4 */
#define POE_SDA_PIN          4   /* PC4  AF4 */

/* --- System --- */
#define NRST_PIN             0   /* NRST */
#define BOOT0_PIN            0   /* BOOT0 */

/* --- PoE Power Good --- */
#define POE_PG_PIN           0   /* PA0  (alternate use, muxed) */

/* ===== SDRAM Configuration ===== */
#define SDRAM_BASE_ADDR      0xC0000000UL
#define SDRAM_SIZE_BYTES     0x01000000UL  /* 16 MB */
#define SDRAM_CLOCK_HZ       200000000UL
#define SDRAM_CAS_LATENCY    3
#define SDRAM_REFRESH_RATE_US 64000  /* 64ms refresh */
#define SDRAM_ROWS           8192   /* 13-bit row address */
#define SDRAM_COLS           1024   /* 10-bit column address */
#define SDRAM_BANKS           4

/* SDRAM timing (clocks at 200MHz = 5ns/cycle) */
#define SDRAM_TMRD           2   /* Load Mode Register to Active: 2 cycles */
#define SDRAM_TXSR           7   /* Exit self-refresh: 7 cycles (35ns) */
#define SDRAM_TRAS           5   /* Row active time: 5 cycles (25ns) */
#define SDRAM_TRC            7   /* Row cycle time: 7 cycles (35ns) */
#define SDRAM_TRCD           3   /* RAS-to-CAS delay: 3 cycles (15ns) */
#define SDRAM_TRFC            6   /* Row refresh cycle: 6 cycles (30ns) */
#define SDRAM_TRP            3   /* Row precharge: 3 cycles (15ns) */
#define SDRAM_TWR             2   /* Write recovery: 2 cycles (10ns) */
#define SDRAM_TREFI           15625 /* Refresh interval in cycles: 7.8µs / 5ns */

/* ===== Ethernet DMA Descriptor Counts ===== */
#define ETH_RX_DESC_COUNT    256
#define ETH_TX_DESC_COUNT    128
#define ETH_RX_BUF_SIZE      1536  /* Full MTU + headroom */
#define ETH_TX_BUF_SIZE      1536

/* Descriptor base addresses in SDRAM */
#define ETH_RX_DESC_BASE     (SDRAM_BASE_ADDR + 0x00000000UL)
#define ETH_TX_DESC_BASE     (SDRAM_BASE_ADDR + 0x00100000UL)
#define ETH_RX_BUF_BASE      (SDRAM_BASE_ADDR + 0x00200000UL)
#define ETH_TX_BUF_BASE      (SDRAM_BASE_ADDR + 0x00600000UL)
#define CAPTURE_BUF_BASE     (SDRAM_BASE_ADDR + 0x00A00000UL)
#define CAPTURE_BUF_SIZE      0x00600000UL  /* 6 MB capture buffer */

/* ===== SPI Flash ===== */
#define SPI_FLASH_SIZE       0x01000000UL  /* 16 MB (128 Mb) */
#define SPI_FLASH_SECTOR     0x00001000UL  /* 4 KB sector */
#define SPI_FLASH_BLOCK      0x00010000UL  /* 64 KB block */
#define SPI_FLASH_PAGE       0x00000100UL  /* 256 byte page */

/* ===== BLE UART ===== */
#define BLE_UART_BAUD        115200
#define BLE_UART_WORDLEN     8
#define BLE_UART_PARITY      0   /* None */
#define BLE_UART_STOPBITS    1

/* ===== KSZ9897 Switch Register Map ===== */
#define KSZ_CHIP_ID0         0x0000
#define KSZ_CHIP_ID1         0x0001
#define KSZ_P1_CTRL          0x0100
#define KSZ_P2_CTRL          0x0200
#define KSZ_P6_CTRL          0x0600
#define KSZ_GLOBAL_CTRL      0x0300
#define KSZ_MIRROR_CTRL      0x0380
#define KSZ_VLAN_CTRL        0x0400
#define KSZ_P1_MIB           0x1100

/* Mirror control bits */
#define KSZ_MIRROR_EN        (1 << 0)
#define KSZ_MIRROR_PORT1_RX  (1 << 1)
#define KSZ_MIRROR_PORT1_TX  (1 << 2)
#define KSZ_MIRROR_PORT2_RX  (1 << 3)
#define KSZ_MIRROR_PORT2_TX  (1 << 4)
#define KSZ_MIRROR_TO_PORT6  (6 << 5)  /* CPU port */

/* ===== TPS2378 I2C Address ===== */
#define TPS2378_I2C_ADDR     0x20  /* ADDR pin to GND */
#define TPS2378_REG_STATUS   0x00
#define TPS2378_REG_POWER    0x01
#define TPS2378_REG_FAULT    0x02
#define TPS2378_REG_CLASS    0x03

/* ===== Packet Engine ===== */
#define MAX_RULES            256
#define RULE_STORAGE_BASE    0x00040000UL  /* In SPI flash */
#define RULE_STORAGE_SIZE    0x00008000UL  /* 32 KB rule storage */

/* ===== C2 Channel ===== */
#define C2_HEARTBEAT_MS      30000  /* 30 second heartbeat */
#define C2_RECONNECT_MS      5000   /* 5 second reconnect */

#endif /* BOARD_H */