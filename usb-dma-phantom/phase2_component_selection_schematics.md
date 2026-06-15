# USB DMA Phantom — Phase 2: Component Selection & Schematics

## 1. Bill of Materials

| # | Reference | Part Number | Description | Package | Qty | Unit USD | Ext USD |
|---|-----------|------------|-------------|---------|-----|----------|---------|
| 1 | U1 | STM32F423CHU6 | Cortex-M4F MCU, 150 MHz, 1.5 MB Flash, 256 KB RAM, USB OTG FS | UFQFPN-48 | 1 | $8.50 | $8.50 |
| 2 | U2 | nRF52832-QFAA | BLE 5.0 SoC, 64 MHz Cortex-M4F, 512 KB Flash, 64 KB RAM | QFN-48 | 1 | $4.20 | $4.20 |
| 3 | U3 | XIO2001IZWR | PCIe-to-PCI bridge, 1-lane Gen2, DMA capable, TBT compatible | nFBGA-144 | 1 | $12.80 | $12.80 |
| 4 | U4 | HD3SS460IRSVR | USB-C ALT mode mux/retimer, TBT/USB4/DP switching | MicroSUAR-42 | 1 | $2.90 | $2.90 |
| 5 | U5 | W25Q128JVSIM | 16 MB (128 Mbit) SPI NOR flash, 50 MHz standard, 104 MHz dual | SOIC-8 | 1 | $0.65 | $0.65 |
| 6 | U6 | TPS62840DSQR | 3.3 V, 1 A step-down converter, 2.4-5.5 V input, 360 nA IQ | SOT-5X3-6 | 1 | $1.10 | $1.10 |
| 7 | U7 | DA1220A1-00UK | 4-bit bidirectional level shifter, 1.8-3.3 V, for microSD | UQFN-16 | 1 | $0.30 | $0.30 |
| 8 | U8 | TLV70033DCKR | 3.3 V, 200 mA LDO, for nRF52832 analog | SOT-5 | 1 | $0.25 | $0.25 |
| 9 | Y1 | ECS-2520MV-250-BN-TR | 25 MHz crystal, 20 ppm, for STM32F423 | 2.5×2.0 mm | 1 | $0.40 | $0.40 |
| 10 | Y2 | ECS-3225MV-320-BN-TR | 32 MHz crystal, 20 ppm, for nRF52832 | 3.2×2.5 mm | 1 | $0.45 | $0.45 |
| 11 | Y3 | ABM8-100.000MHZ-20-B2X-T | 100 MHz crystal, 20 ppm, for XIO2001 PCIe refclk | 3.2×2.5 mm | 1 | $0.80 | $0.80 |
| 12 | J1 | USB4105-GF-A | USB-C receptacle, 24-pin, 10 Gbps, TBT3/4 capable | SMT | 1 | $0.85 | $0.85 |
| 13 | J2 | 503182-1852 | microSD card connector, push-push, 8-pin | SMT | 1 | $0.60 | $0.60 |
| 14 | SW1 | EVQPUJ02K | Tactile switch, 6×3.5 mm, mode select | SMT | 1 | $0.10 | $0.10 |
| 15 | D1 | WS2812B-Mini | RGB LED, addressable, status indicator | 3.5×3.5 mm | 1 | $0.25 | $0.25 |
| 16 | D2 | PRTR5V0U2X | Dual TVS diode, USB data line ESD | SOT-143 | 1 | $0.15 | $0.15 |
| 17 | D3 | TPD4E05U06DQAR | 4-ch TVS, USB-C high-speed ESD protection | USON-10 | 1 | $0.45 | $0.45 |
| 18 | L1 | NRV5015T4G | Ferrite bead, 500 mA, 50 Ω @ 100 MHz, power filtering | 0805 | 1 | $0.02 | $0.02 |
| 19 | L2-L3 | CIG22-4.7NK | 4.7 µH, 1.5 A inductor, for TPS62840 buck | 2.0×1.6 mm | 2 | $0.15 | $0.30 |
| 20 | C1-C6 | 0.1 µF 10V X5R 0402 | Decoupling caps for STM32 VDD pins | 0402 | 6 | $0.005 | $0.03 |
| 21 | C7-C9 | 0.1 µF 10V X5R 0402 | Decoupling caps for nRF52832 | 0402 | 3 | $0.005 | $0.015 |
| 22 | C10 | 10 µF 10V X5R 0603 | Bulk decoupling, STM32 VCAP | 0603 | 1 | $0.03 | $0.03 |
| 23 | C11 | 1 µF 10V X5R 0402 | TPS62840 output cap | 0402 | 1 | $0.01 | $0.01 |
| 24 | C12 | 4.7 µF 10V X5R 0603 | TPS62840 input cap | 0603 | 1 | $0.03 | $0.03 |
| 25 | C13-C14 | 12 pF C0G 0402 | 25 MHz crystal load caps | 0402 | 2 | $0.005 | $0.01 |
| 26 | C15-C16 | 12 pF C0G 0402 | 32 MHz crystal load caps | 0402 | 2 | $0.005 | $0.01 |
| 27 | C17-C18 | 18 pF C0G 0402 | 100 MHz crystal load caps | 0402 | 2 | $0.005 | $0.01 |
| 28 | R1 | 10 kΩ 1% 0402 | STM32 BOOT0 pull-down | 0402 | 1 | $0.002 | $0.002 |
| 29 | R2 | 1.5 kΩ 1% 0402 | WS2812B data line series | 0402 | 1 | $0.002 | $0.002 |
| 30 | R3-R4 | 100 Ω 1% 0402 | USB D+/D- series termination | 0402 | 2 | $0.002 | $0.004 |
| 31 | R5-R6 | 5.1 kΩ 1% 0402 | USB-C CC1/CC2 pull-downs (5V/900mA) | 0402 | 2 | $0.002 | $0.004 |
| 32 | R7 | 4.7 kΩ 1% 0402 | I2C SDA pull-up | 0402 | 1 | $0.002 | $0.002 |
| 33 | R8 | 4.7 kΩ 1% 0402 | I2C SCL pull-up | 0402 | 1 | $0.002 | $0.002 |
| 34 | R9 | 0 Ω 0402 | NBOOT0 tie for nRF52832 | 0402 | 1 | $0.002 | $0.002 |
| 35 | R10 | 22 Ω 1% 0402 | PCIe TX diff pair series | 0402 | 1 | $0.002 | $0.002 |
| 36 | R11 | 22 Ω 1% 0402 | PCIe RX diff pair series | 0402 | 1 | $0.002 | $0.002 |
| 37 | R12 | 49.9 Ω 1% 0402 | PCIe 100 MHz refclk termination | 0402 | 1 | $0.002 | $0.002 |
| 38 | R13 | 10 kΩ 1% 0402 | XIO2001 PERST# pull-up | 0402 | 1 | $0.002 | $0.002 |
| 39 | R14 | 10 kΩ 1% 0402 | microSD card detect pull-up | 0402 | 1 | $0.002 | $0.002 |
| 40 | PCB | 4-layer FR4 | 65×30 mm, ENIG finish, impedance controlled | — | 1 | $3.00 | $3.00 |
| | | | | | **Total** | | **~$40** |

> **Note:** BOM cost at qty 100 is ~$40; at qty 1 prototype it approaches ~$80–85. Well under $100 target.

## 2. Pinout Tables

### 2.1 STM32F423CHU6 (U1) — UFQFPN-48

| Pin | Name | Function | Connection |
|-----|------|----------|------------|
| 1 | VBAT | Battery/backup power | 3V3 (TPS62840 output) |
| 2 | PC13 | GPIO / TAMPER | SW1 (mode button, active low) |
| 3 | PC14/OSC32_IN | GPIO | WS2812B data out (R2 series) |
| 4 | PC15/OSC32_OUT | GPIO | XIO2001 PERST# control (active low reset) |
| 5 | PH0/OSC_IN | OSC_IN | Y1 (25 MHz crystal) |
| 6 | PH1/OSC_OUT | OSC_OUT | Y1 (25 MHz crystal) |
| 7 | NRST | Reset | 10 kΩ pull-up + 100 nF cap |
| 8 | PC0 | ADC1_IN10 | VBUS_SENSE (USB VBUS voltage divider) |
| 9 | PC1 | ADC1_IN11 | microSD CARD_DETECT (J2 pin) |
| 10 | PC2 | GPIO | XIO2001 CLKREQ# input |
| 11 | PC3 | GPIO | HD3SS460 CTL1 |
| 12 | PA0 | GPIO / WKUP | nRF52832 RESET# (active low) |
| 13 | PA1 | UART4_RX | nRF52832 UART_TX (C2 link) |
| 14 | PA2 | UART4_TX | nRF52832 UART_RX (C2 link) |
| 15 | PA3 | UART4_RTS | nRF52832 UART_CTS (HW flow) |
| 16 | PA4 | UART4_CTS | nRF52832 UART_RTS (HW flow) |
| 17 | PA5 | SPI4_SCK | W25Q128 SCLK + XIO2001 SPI_CLK |
| 18 | PA6 | SPI4_MISO | W25Q128 DO + XIO2001 SPI_DO |
| 19 | PA7 | SPI4_MOSI | W25Q128 DI + XIO2001 SPI_DI |
| 20 | PC4 | GPIO | XIO2001 SPI_CS# (chip select, active low) |
| 21 | PC5 | GPIO | W25Q128 CS# (SPI flash chip select) |
| 22 | PB0 | I2C1_SCL | R8 (4.7 kΩ pull-up) → HD3SS460 SCL + microSD level shifter |
| 23 | PB1 | I2C1_SDA | R7 (4.7 kΩ pull-up) → HD3SS460 SDA + microSD level shifter |
| 24 | PB2 | GPIO | microSD SD_CMD (via level shifter U7) |
| 25 | PB10 | GPIO / TIM2_CH3 | microSD SD_CLK (via level shifter U7) |
| 26 | PB11 | GPIO | microSD SD_D0 (via level shifter U7) |
| 27 | PB12 | GPIO | microSD SD_D1 (via level shifter U7) |
| 28 | VCAP1 | Internal regulator cap | C10 (10 µF) to GND |
| 29 | VSS | Ground | GND |
| 30 | VDD | 3.3 V power | 3V3 net |
| 31 | PB13 | USB_OTG_FS_VBUS | USB VBUS sense (via voltage divider) |
| 32 | PB14 | USB_OTG_FS_DM | USB-C D- (via R4 100 Ω + D2 TVS) |
| 33 | PB15 | USB_OTG_FS_DP | USB-C D+ (via R3 100 Ω + D2 TVS) |
| 34 | PA8 | MCO1 / GPIO | nRF52832 SWD_CLK (debug/programming) |
| 35 | PA9 | USB_OTG_FS_VBUS | USB VBUS (alternate) |
| 36 | PA10 | USB_OTG_FS_ID | USB OTG ID (pull to GND for device mode) |
| 37 | PA11 | SPI4_NSS (alt) | XIO2001 WAKE# (DMA wake signal) |
| 38 | PA12 | GPIO | HD3SS460 CTL2 |
| 39 | PA13 | SWDIO | SWD debug header |
| 40 | PA14 | SWCLK | SWD debug header |
| 41 | PA15 | JTDI / GPIO | SPI flash WP# (write-protect) |
| 42 | PC6 | GPIO | XIO2001 SERR# (system error) |
| 43 | PC7 | GPIO | XIO2001 INTA# (interrupt) |
| 44 | PC8 | GPIO | HD3SS460 CTL3 |
| 45 | PC9 | GPIO | HD3SS460 CTL4 / FLT# |
| 46 | PA13_ALT | — | (SWD shared) |
| 47 | BOOT0 | Boot mode select | R1 (10 kΩ to GND) |
| 48 | VDD | 3.3 V power | 3V3 net |

### 2.2 nRF52832-QFAA (U2) — QFN-48

| Pin | Name | Function | Connection |
|-----|------|----------|------------|
| 1 | DEC1 | Decoupling | 100 nF to GND |
| 2 | DEC2 | Decoupling | 100 nF + 1 nF to GND |
| 3 | DEC3 | Decoupling | 100 nF to GND |
| 4 | VDD | 3.3 V power | 3V3_LDO (U8 output) |
| 5-8 | P0.00-P0.03 | GPIO | Antenna matching + reserved |
| 9 | P0.04 | UART_TX | STM32 PA2 (UART4_TX) |
| 10 | P0.05 | UART_RX | STM32 PA1 (UART4_RX) |
| 11 | P0.06 | UART_CTS | STM32 PA3 (UART4_RTS) |
| 12 | P0.07 | UART_RTS | STM32 PA4 (UART4_CTS) |
| 13 | P0.08 | GPIO | SW1 (shared mode button, active low) |
| 14 | P0.09 | NFC1 | (unused, antenna not populated) |
| 15 | P0.10 | NFC2 | (unused, antenna not populated) |
| 16-23 | P0.11-P0.18 | GPIO | Reserved for expansion |
| 24 | P0.19 | SWD_CLK | Debug header |
| 25 | P0.20 | SWDIO | Debug header |
| 26 | P0.21 | RESET# | STM32 PA0 (active low) |
| 27 | SWDIO | SWD data | Debug header |
| 28 | SWDCLK | SWD clock | Debug header |
| 29-32 | P0.22-P0.25 | GPIO | Reserved |
| 33 | P0.26 | GPIO | WS2812B (cascade from STM32, daisy) |
| 34 | VDD | 3.3 V | 3V3_LDO |
| 35 | ANT1 | Antenna | PCB trace antenna (2.4 GHz) |
| 36 | ANT2 | Antenna | PCB trace antenna (2.4 GHz) |
| 37 | VDD | 3.3 V | 3V3_LDO |
| 38 | DEC4 | Decoupling | 100 nF to GND |
| 39 | DEC5 | Decoupling | 100 nF to GND |
| 40 | XC1 | 32 MHz XTAL | Y2 |
| 41 | XC2 | 32 MHz XTAL | Y2 |
| 42 | VDD | 3.3 V | 3V3_LDO |
| 43 | DEC6 | Decoupling | 100 nF to GND |
| 44-48 | VDD/GND | Power | 3V3_LDO / GND |

### 2.3 XIO2001 (U3) — nFBGA-144 (Key Signals)

| Ball | Name | Function | Connection |
|------|------|----------|------------|
| A1 | PERST# | PCIe reset | R13 (10 kΩ pull-up) + STM32 PC15 |
| A2 | CLKREQ# | Clock request | STM32 PC2 |
| A3 | REFCLK± | 100 MHz PCIe reference | Y3 (100 MHz) via R12 (49.9 Ω) |
| B1 | TX± | PCIe transmit diff pair | HD3SS460 TX pair |
| B2 | RX± | PCIe receive diff pair | HD3SS460 RX pair |
| C1 | SPI_DI | SPI data in | STM32 PA7 (SPI4_MOSI) |
| C2 | SPI_DO | SPI data out | STM32 PA6 (SPI4_MISO) |
| C3 | SPI_CLK | SPI clock | STM32 PA5 (SPI4_SCK) |
| C4 | SPI_CS# | SPI chip select | STM32 PC4 |
| D1 | INTA# | Interrupt A | STM32 PC7 |
| D2 | SERR# | System error | STM32 PC6 |
| D3 | WAKE# | DMA wake | STM32 PA11 |
| E1-E4 | VDD | 3.3 V core power | 3V3 |
| F1-F4 | VDDIO | 3.3 V I/O power | 3V3 |
| G1-G4 | VSS | Ground | GND |
| H1 | GRST# | Global reset | STM32 PC15 (shared with PERST#) |
| J1-J4 | PWRGD | Power good | (monitored, 10 kΩ pull-up) |

### 2.4 HD3SS460 (U4) — Key Signals

| Pin | Name | Function | Connection |
|-----|------|----------|------------|
| 1 | CC1 | USB-C CC1 | R5 (5.1 kΩ to GND) via J1 |
| 2 | CC2 | USB-C CC2 | R6 (5.1 kΩ to GND) via J1 |
| 3 | CTL1 | Mode control 1 | STM32 PC3 |
| 4 | CTL2 | Mode control 2 | STM32 PA12 |
| 5 | CTL3 | Mode control 3 | STM32 PC8 |
| 6 | CTL4 / FLT# | Mode control 4 | STM32 PC9 |
| 7 | TX1± | SuperSpeed TX pair 1 | J1 (USB-C TX1±) |
| 8 | RX1± | SuperSpeed RX pair 1 | J1 (USB-C RX1±) |
| 9 | TX2± | SuperSpeed TX pair 2 | XIO2001 TX± |
| 10 | RX2± | SuperSpeed RX pair 2 | XIO2001 RX± |
| 11 | SBU1 | Sideband use 1 | J1 (USB-C SBU1) |
| 12 | SBU2 | Sideband use 2 | J1 (USB-C SBU2) |
| 13 | D± | USB 2.0 diff pair | J1 (USB-C D±) via D2 TVS |
| 14 | VDD | 3.3 V | 3V3 |
| 15 | VSS | Ground | GND |

## 3. Netlist (Source Pin → Component → Dest Pin)

| Net Name | Source Pin | Via Component(s) | Dest Pin |
|----------|-----------|-------------------|----------|
| 3V3 | U6 (TPS62840) VOUT | C11, C12 | U1 VDD×6, U2 VDD×5, U3 VDD×8, U4 VDD, U5 VCC, U7 VDDA |
| 3V3_LDO | U8 (TLV70033) OUT | C_bulk | U2 VDD×3 |
| 5V_USB | J1 VBUS | C12 (U6 input cap) | U6 (TPS62840) VIN |
| GND | J1 GND×4 | All GND pins | All IC GND pins |
| USB_DP | U1 PB15 | R3 (100 Ω) → D2 TVS | J1 D+ (via HD3SS460 pass-through) |
| USB_DM | U1 PB14 | R4 (100 Ω) → D2 TVS | J1 D- (via HD3SS460 pass-through) |
| VBUS_SENSE | J1 VBUS | R_div ½ → U1 PC0 | ADC1_IN10 |
| SPI4_SCK | U1 PA5 | — | U5 CLK, U3 SPI_CLK |
| SPI4_MISO | U5 DO | — | U1 PA6, U3 SPI_DO |
| SPI4_MOSI | U1 PA7 | — | U5 DI, U3 SPI_DI |
| FLASH_CS# | U1 PC5 | — | U5 CS# |
| XIO_CS# | U1 PC4 | — | U3 SPI_CS# |
| XIO_PERST# | U1 PC15 | R13 (10 kΩ pull-up) | U3 PERST# |
| XIO_CLKREQ# | U3 CLKREQ# | — | U1 PC2 |
| XIO_INTA# | U3 INTA# | — | U1 PC7 |
| XIO_SERR# | U3 SERR# | — | U1 PC6 |
| XIO_WAKE# | U3 WAKE# | — | U1 PA11 |
| UART_TX | U1 PA2 | — | U2 P0.05 (UART_RX) |
| UART_RX | U1 PA1 | — | U2 P0.04 (UART_TX) |
| UART_RTS | U1 PA3 | — | U2 P0.07 (UART_RTS) |
| UART_CTS | U1 PA4 | — | U2 P0.06 (UART_CTS) |
| nRF_RESET# | U1 PA0 | — | U2 P0.21 (RESET#) |
| HD3SS_CTL1 | U1 PC3 | — | U4 CTL1 |
| HD3SS_CTL2 | U1 PA12 | — | U4 CTL2 |
| HD3SS_CTL3 | U1 PC8 | — | U4 CTL3 |
| HD3SS_CTL4 | U1 PC9 | — | U4 CTL4 |
| I2C_SCL | U1 PB0 | R8 (4.7 kΩ pull-up) | U4 SCL, U7 SCL |
| I2C_SDA | U1 PB1 | R7 (4.7 kΩ pull-up) | U4 SDA, U7 SDA |
| SD_CMD | U1 PB2 | U7 (level shift) | J2 CMD |
| SD_CLK | U1 PB10 | U7 (level shift) | J2 CLK |
| SD_D0 | U1 PB11 | U7 (level shift) | J2 D0 |
| SD_D1 | U1 PB12 | U7 (level shift) | J2 D1 |
| SD_CD | J2 CD | R14 (10 kΩ pull-up) | U1 PC1 |
| WS2812_DATA | U1 PC14 | R2 (1.5 kΩ) | D1 DIN |
| MODE_BTN | SW1 | — | U1 PC13, U2 P0.08 |
| XTAL25_P | U1 PH0 | C13 (12 pF) | Y1 pin 1 |
| XTAL25_N | U1 PH1 | C14 (12 pF) | Y1 pin 3 |
| XTAL32_P | U2 XC1 | C15 (12 pF) | Y2 pin 1 |
| XTAL32_N | U2 XC2 | C16 (12 pF) | Y2 pin 3 |
| XTAL100_P | Y3 out+ | C17 (18 pF) | U3 REFCLK+ |
| XTAL100_N | Y3 out- | C18 (18 pF) | U3 REFCLK- |
| CC1_PULL | J1 CC1 | R5 (5.1 kΩ) | GND |
| CC2_PULL | J1 CC2 | R6 (5.1 kΩ) | GND |

## 4. Decoupling Strategy

| IC | Pin Group | Cap Value | Package | Placement |
|----|-----------|-----------|---------|-----------|
| STM32F423 | VDD (6 pins) | 100 nF × 6 | 0402 | Within 2 mm of each VDD pin |
| STM32F423 | VBAT | 100 nF | 0402 | Adjacent to pin |
| STM32F423 | VCAP1 | 10 µF (C10) | 0603 | Adjacent to VCAP1 pin |
| nRF52832 | DEC1-DEC6 | 100 nF × 6 | 0402 | Adjacent to each DEC pin |
| nRF52832 | VDD | 4.7 µF | 0603 | Near pin 4 |
| XIO2001 | VDD (8 pins) | 100 nF × 8 | 0402 | Within 1 mm of each VDD ball |
| XIO2001 | VDDIO | 100 nF × 4 | 0402 | Within 1 mm of each VDDIO ball |
| HD3SS460 | VDD | 100 nF + 1 µF | 0402 + 0603 | Adjacent to VDD pin |
| W25Q128 | VCC | 100 nF | 0402 | Adjacent to pin 8 |

## 5. Impedance-Controlled Pairs

| Pair | Impedance | Trace Width | Spacing | Layer | Notes |
|------|-----------|-------------|---------|-------|-------|
| USB D+/D- | 90 Ω diff | 0.18 mm | 0.12 mm | Top | Controlled by HD3SS460 internal routing |
| PCIe TX± | 85 Ω diff | 0.15 mm | 0.10 mm | L3 (inner) | AC-coupled via 0.1 µF caps |
| PCIe RX± | 85 Ω diff | 0.15 mm | 0.10 mm | L3 (inner) | AC-coupled at receiver |
| PCIe REFCLK± | 85 Ω diff | 0.15 mm | 0.10 mm | L3 (inner) | Low-jitter path, length-matched |
| BLE Antenna trace | 50 Ω single | 0.30 mm | — | Top | Coplanar waveguide, λ/4 at 2.4 GHz |
| USB-C SS TX1± | 90 Ω diff | 0.18 mm | 0.12 mm | Top | Short path to HD3SS460 |
| USB-C SS RX1± | 90 Ω diff | 0.18 mm | 0.12 mm | Top | Short path to HD3SS460 |

## 6. Power Tree

```
5V_USB (J1 VBUS)
  │
  ├──► TPS62840 (U6) ──► 3V3 (3.3 V @ 1 A)
  │     │                   │
  │     │                   ├── STM32F423 VDD × 6
  │     │                   ├── nRF52832 VDD × 5 (via ferrite bead)
  │     │                   │     └── TLV70033 (U8) ──► 3V3_LDO (clean analog for nRF)
  │     │                   ├── XIO2001 VDD/VDDIO × 12
  │     │                   ├── HD3SS460 VDD
  │     │                   ├── W25Q128 VCC
  │     │                   ├── DA1220 VDDA
  │     │                   └── microSD (via DA1220 level shift)
  │     │
  │     └──► 5V_USB direct ──► J1 VBUS pass-through (Thunderbolt power)
  │
  └──► USB-C CC pull-downs (5.1 kΩ) → 5V @ 900mA default