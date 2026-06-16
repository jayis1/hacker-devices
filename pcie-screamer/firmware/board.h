/**
 * board.h — PCIe Screamer Board Pin Definitions
 *
 * Complete pin definitions for the Lattice ECP5-45F FPGA on the
 * PCIe Screamer NVMe interposer board. All pins are defined with
 * their ball number, bank, alternate function, and connected peripheral.
 *
 * Author: jayis1
 * Date: 2026-06-16
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

/*============================================================================
 * CLOCK FREQUENCIES
 *============================================================================*/

#define F_CPU               125000000UL   /* System clock (PLL0 CLKOP) */
#define F_PCIE_REFCLK       100000000UL   /* PCIe reference clock input */
#define F_FT601_CLK         100000000UL   /* FT601 FIFO clock (PLL1 CLKOP) */
#define F_DDR3_USER_CLK     200000000UL   /* DDR3 user interface clock (PLL1 CLKOS) */
#define F_DDR3_PHY_CLK      400000000UL   /* DDR3 PHY clock (DDR3 PLL) */
#define F_DDR3_MEM_CLK      400000000UL   /* DDR3 memory clock (CK/CK#, 800 MT/s) */
#define F_UART_CLK          9615385UL     /* UART clock (125 MHz / 13) */

/*============================================================================
 * BANK 0 — CONFIGURATION, LEDS, DEBUG, SIDEBAND MONITORS (VCCIO = 3.3V)
 *============================================================================*/

/* SPI Flash Configuration Pins (Master SPI Mode) */
#define CSPI_CSN_PIN        0     /* Ball A3  — SPI Flash Chip Select */
#define CSPI_CLK_PIN        1     /* Ball B3  — SPI Flash Clock */
#define CSPI_MOSI_PIN       2     /* Ball C3  — SPI Flash MOSI (DI) */
#define CSPI_MISO_PIN       3     /* Ball D3  — SPI Flash MISO (DO) */

/* Status LEDs */
#define LED0_PCIE_LINK_PIN  4     /* Ball A4  — Green: PCIe Link Up */
#define LED1_USB_ENUM_PIN   5     /* Ball B4  — Blue: USB Enumerated */
#define LED2_CAPTURE_PIN    6     /* Ball C4  — Amber: Capture Active */
#define LED3_ERROR_PIN      7     /* Ball D4  — Red: Error/Fault */

/* Debug UART */
#define UART_TX_PIN         8     /* Ball A5  — Debug UART Transmit */
#define UART_RX_PIN         9     /* Ball B5  — Debug UART Receive */

/* Sideband Signal Monitors (from Host M.2) */
#define PERST_MON_PIN       10    /* Ball C5  — PERST# monitor (active low) */
#define CLKREQ_MON_PIN      11    /* Ball D5  — CLKREQ# monitor (active low) */
#define PEWAKE_MON_PIN      12    /* Ball A6  — PEWAKE# monitor (active low) */

/* Si5332 Clock Generator I2C */
#define SI5332_SCL_PIN      13    /* Ball B6  — I2C Clock */
#define SI5332_SDA_PIN      14    /* Ball C6  — I2C Data */

/* FT601 Interrupt */
#define FT601_INT_PIN       15    /* Ball D6  — FT601 interrupt input */

/* Redriver I2C (shared bus with Si5332) */
#define REDRIVER_SCL_PIN    13    /* Ball A7  — Shared I2C SCL */
#define REDRIVER_SDA_PIN    14    /* Ball B7  — Shared I2C SDA */

/* Power Good Monitor */
#define PGOOD_MON_PIN       16    /* Ball C7  — Wire-OR of all buck PG outputs */

/* Reserved Test Point */
#define TP3_GPIO_PIN        17    /* Ball D7  — Reserved GPIO / Test Point 3 */

/*============================================================================
 * BANK 1 — FT601 CONTROL SIGNALS (VCCIO = 3.3V)
 *============================================================================*/

#define FT_TXE_PIN          18    /* Ball E1  — Transmit FIFO Empty (active low) */
#define FT_RXF_PIN          19    /* Ball F1  — Receive FIFO Full (active low) */
#define FT_WR_PIN           20    /* Ball G1  — Write Strobe (active low) */
#define FT_RD_PIN           21    /* Ball H1  — Read Strobe (active low) */
#define FT_OE_PIN           22    /* Ball J1  — Output Enable (active low) */
#define FT_SIWU_PIN         23    /* Ball K1  — Send Immediate / Wake Up (active low) */
#define FT_RESET_PIN        24    /* Ball L1  — FT601 Reset (active low) */
#define FT_CLK_PIN          25    /* Ball M1  — 100 MHz Clock Output to FT601 */
#define FT_WAKEUP_PIN       26    /* Ball E2  — Wakeup signal */
#define FT_GPIO0_PIN        27    /* Ball F2  — FT601 GPIO0 */
#define FT_GPIO1_PIN        28    /* Ball G2  — FT601 GPIO1 */
#define FT_SUSPEND_PIN      29    /* Ball H2  — Suspend indicator (active low) */

/*============================================================================
 * BANK 2 — FT601 DATA[15:0] (VCCIO = 3.3V)
 *============================================================================*/

#define FT_D0_PIN           30    /* Ball E3 */
#define FT_D1_PIN           31    /* Ball F3 */
#define FT_D2_PIN           32    /* Ball G3 */
#define FT_D3_PIN           33    /* Ball H3 */
#define FT_D4_PIN           34    /* Ball J3 */
#define FT_D5_PIN           35    /* Ball K3 */
#define FT_D6_PIN           36    /* Ball L3 */
#define FT_D7_PIN           37    /* Ball M3 */
#define FT_D8_PIN           38    /* Ball E4 */
#define FT_D9_PIN           39    /* Ball F4 */
#define FT_D10_PIN          40    /* Ball G4 */
#define FT_D11_PIN          41    /* Ball H4 */
#define FT_D12_PIN          42    /* Ball J4 */
#define FT_D13_PIN          43    /* Ball K4 */
#define FT_D14_PIN          44    /* Ball L4 */
#define FT_D15_PIN          45    /* Ball M4 */

/*============================================================================
 * BANK 3 — FT601 DATA[31:16] + BYTE ENABLES (VCCIO = 3.3V)
 *============================================================================*/

#define FT_D16_PIN          46    /* Ball E5 */
#define FT_D17_PIN          47    /* Ball F5 */
#define FT_D18_PIN          48    /* Ball G5 */
#define FT_D19_PIN          49    /* Ball H5 */
#define FT_D20_PIN          50    /* Ball J5 */
#define FT_D21_PIN          51    /* Ball K5 */
#define FT_D22_PIN          52    /* Ball L5 */
#define FT_D23_PIN          53    /* Ball M5 */
#define FT_D24_PIN          54    /* Ball E6 */
#define FT_D25_PIN          55    /* Ball F6 */
#define FT_D26_PIN          56    /* Ball G6 */
#define FT_D27_PIN          57    /* Ball H6 */
#define FT_D28_PIN          58    /* Ball J6 */
#define FT_D29_PIN          59    /* Ball K6 */
#define FT_D30_PIN          60    /* Ball L6 */
#define FT_D31_PIN          61    /* Ball M6 */
#define FT_BE0_PIN          62    /* Ball E7 */
#define FT_BE1_PIN          63    /* Ball F7 */
#define FT_BE2_PIN          64    /* Ball G7 */
#define FT_BE3_PIN          65    /* Ball H7 */

/*============================================================================
 * BANK 4 — PCIe SERDES LANES 0-1 (VCCIO = 1.2V)
 *============================================================================*/

/* These are dedicated SERDES pins, not GPIO. Defined for documentation. */
#define PCIE_RX0_P_CH       0     /* SERDES Channel 0 RX+ (Ball N1) */
#define PCIE_RX0_N_CH       0     /* SERDES Channel 0 RX- (Ball N2) */
#define PCIE_TX0_P_CH       0     /* SERDES Channel 0 TX+ (Ball P1) */
#define PCIE_TX0_N_CH       0     /* SERDES Channel 0 TX- (Ball P2) */
#define PCIE_RX1_P_CH       1     /* SERDES Channel 1 RX+ (Ball N3) */
#define PCIE_RX1_N_CH       1     /* SERDES Channel 1 RX- (Ball N4) */
#define PCIE_TX1_P_CH       1     /* SERDES Channel 1 TX+ (Ball P3) */
#define PCIE_TX1_N_CH       1     /* SERDES Channel 1 TX- (Ball P4) */

/*============================================================================
 * BANK 5 — PCIe SERDES LANES 2-3 (VCCIO = 1.2V)
 *============================================================================*/

#define PCIE_RX2_P_CH       2     /* SERDES Channel 2 RX+ (Ball R1) */
#define PCIE_RX2_N_CH       2     /* SERDES Channel 2 RX- (Ball R2) */
#define PCIE_TX2_P_CH       2     /* SERDES Channel 2 TX+ (Ball T1) */
#define PCIE_TX2_N_CH       2     /* SERDES Channel 2 TX- (Ball T2) */
#define PCIE_RX3_P_CH       3     /* SERDES Channel 3 RX+ (Ball R3) */
#define PCIE_RX3_N_CH       3     /* SERDES Channel 3 RX- (Ball R4) */
#define PCIE_TX3_P_CH       3     /* SERDES Channel 3 TX+ (Ball T3) */
#define PCIE_TX3_N_CH       3     /* SERDES Channel 3 TX- (Ball T4) */

/*============================================================================
 * BANK 6 — DDR3 INTERFACE (VCCIO = 1.35V, SSTL135)
 *============================================================================*/

/* DDR3 Address Bus */
#define DDR3_A0_PIN         66    /* Ball U1 */
#define DDR3_A1_PIN         67    /* Ball U2 */
#define DDR3_A2_PIN         68    /* Ball U3 */
#define DDR3_A3_PIN         69    /* Ball V1 */
#define DDR3_A4_PIN         70    /* Ball V2 */
#define DDR3_A5_PIN         71    /* Ball V3 */
#define DDR3_A6_PIN         72    /* Ball W1 */
#define DDR3_A7_PIN         73    /* Ball W2 */
#define DDR3_A8_PIN         74    /* Ball W3 */
#define DDR3_A9_PIN         75    /* Ball Y1 */
#define DDR3_A10_PIN        76    /* Ball Y2 */
#define DDR3_A11_PIN        77    /* Ball Y3 */
#define DDR3_A12_PIN        78    /* Ball AA1 */
#define DDR3_A13_PIN        79    /* Ball AA2 */
#define DDR3_A14_PIN        80    /* Ball AA3 */

/* DDR3 Bank Address */
#define DDR3_BA0_PIN        81    /* Ball AB1 */
#define DDR3_BA1_PIN        82    /* Ball AB2 */
#define DDR3_BA2_PIN        83    /* Ball AB3 */

/* DDR3 Data Bus (x16) */
#define DDR3_DQ0_PIN        84    /* Ball AC1 */
#define DDR3_DQ1_PIN        85    /* Ball AC2 */
#define DDR3_DQ2_PIN        86    /* Ball AC3 */
#define DDR3_DQ3_PIN        87    /* Ball AD1 */
#define DDR3_DQ4_PIN        88    /* Ball AD2 */
#define DDR3_DQ5_PIN        89    /* Ball AD3 */
#define DDR3_DQ6_PIN        90    /* Ball AE1 */
#define DDR3_DQ7_PIN        91    /* Ball AE2 */
#define DDR3_DQ8_PIN        92    /* Ball AE3 */
#define DDR3_DQ9_PIN        93    /* Ball AF1 */
#define DDR3_DQ10_PIN       94    /* Ball AF2 */
#define DDR3_DQ11_PIN       95    /* Ball AF3 */
#define DDR3_DQ12_PIN       96    /* Ball AG1 */
#define DDR3_DQ13_PIN       97    /* Ball AG2 */
#define DDR3_DQ14_PIN       98    /* Ball AG3 */
#define DDR3_DQ15_PIN       99    /* Ball AH1 */

/* DDR3 Data Strobes (Differential) */
#define DDR3_DQS0_P_PIN     100   /* Ball AH2 — Upper byte DQS+ */
#define DDR3_DQS0_N_PIN     101   /* Ball AH3 — Upper byte DQS- */
#define DDR3_DQS1_P_PIN     102   /* Ball AJ1 — Lower byte DQS+ */
#define DDR3_DQS1_N_PIN     103   /* Ball AJ2 — Lower byte DQS- */

/* DDR3 Data Masks */
#define DDR3_DM0_PIN        104   /* Ball AJ3 — Upper byte DM */
#define DDR3_DM1_PIN        105   /* Ball AK1 — Lower byte DM */

/* DDR3 Differential Clock */
#define DDR3_CK_P_PIN       106   /* Ball AK2 */
#define DDR3_CK_N_PIN       107   /* Ball AK3 */

/* DDR3 Command/Control */
#define DDR3_CKE_PIN        108   /* Ball AL1 */
#define DDR3_CS_PIN         109   /* Ball AL2 */
#define DDR3_RAS_PIN        110   /* Ball AL3 */
#define DDR3_CAS_PIN        111   /* Ball AM1 */
#define DDR3_WE_PIN         112   /* Ball AM2 */
#define DDR3_ODT_PIN        113   /* Ball AM3 */
#define DDR3_RESET_PIN      114   /* Ball AN1 */

/*============================================================================
 * BANK 7 — PCIe REFERENCE CLOCK & CONTROL (VCCIO = 1.2V)
 *============================================================================*/

#define PCIE_REFCLK_P_PIN   115   /* Ball AP1 — PCIe REFCLK+ from Si5332 */
#define PCIE_REFCLK_N_PIN   116   /* Ball AP2 — PCIe REFCLK- from Si5332 */
#define PCIE_PERST_PIN      117   /* Ball AR1 — PERST# output to device */
#define PCIE_CLKREQ_PIN     118   /* Ball AR2 — CLKREQ# to device */
#define PCIE_WAKE_PIN       119   /* Ball AR3 — PEWAKE# to device */

/*============================================================================
 * GPIO HELPER MACROS
 *============================================================================*/

/* GPIO direction */
#define GPIO_IN             0
#define GPIO_OUT            1

/* GPIO level */
#define GPIO_LOW            0
#define GPIO_HIGH           1

/* LED states */
#define LED_ON              1
#define LED_OFF             0

/* Active-low signal helpers */
#define ASSERT_LOW(pin)     gpio_write((pin), GPIO_LOW)
#define DEASSERT_LOW(pin)   gpio_write((pin), GPIO_HIGH)
#define IS_ASSERTED(pin)    (gpio_read(pin) == GPIO_LOW)
#define IS_DEASSERTED(pin)  (gpio_read(pin) == GPIO_HIGH)

/*============================================================================
 * FUNCTION PROTOTYPES
 *============================================================================*/

void gpio_init(void);
void gpio_set_direction(uint8_t pin, uint8_t direction);
void gpio_write(uint8_t pin, uint8_t value);
uint8_t gpio_read(uint8_t pin);
void gpio_toggle(uint8_t pin);

void led_set_pcie_link(uint8_t state);
void led_set_usb_enum(uint8_t state);
void led_set_capture(uint8_t state);
void led_set_error(uint8_t state);
void led_set_all(uint8_t pcie, uint8_t usb, uint8_t capture, uint8_t error);

void uart_init(uint32_t baudrate);
void uart_putc(char c);
void uart_puts(const char *s);
char uart_getc(void);
int uart_getc_nonblock(void);

void delay_us(uint32_t us);
void delay_ms(uint32_t ms);
uint64_t get_timestamp_48bit(void);

#endif /* BOARD_H */
