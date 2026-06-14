# Phase 2 — Component Selection & Schematics: CAN Bus Infiltrator

## 1. Bill of Materials

| Ref | Part | Manufacturer | Mfr Part # | Package | Qty | Unit $ | Extended $ |
|---|---|---|---|---|---|---|---|
| U1 | MCU, Cortex-M4 168 MHz | STMicroelectronics | STM32F407VGT6 | LQFP-100 | 1 | 8.50 | 8.50 |
| U2 | BLE SoC, nRF52840 | Nordic Semiconductor | nRF52840-QIAA | QFN-73 7×7 | 1 | 4.20 | 4.20 |
| U3 | CAN Transceiver CH1 | NXP | TJA1050T/VC | SOIC-8 | 1 | 0.85 | 0.85 |
| U4 | CAN Transceiver CH2 | NXP | TJA1050T/VC | SOIC-8 | 1 | 0.85 | 0.85 |
| U5 | Digital Isolator (dual CAN) | Texas Instruments | ISO1050DUB | DUB-8 | 2 | 2.80 | 5.60 |
| U6 | EEPROM 2 Kb | Microchip | AT24C02C-PHM | TSSOP-8 | 1 | 0.28 | 0.28 |
| U7 | LDO 3.3 V, 500 mA | Richtek | RT9193-33GB | SOT-89-3 | 1 | 0.22 | 0.22 |
| U8 | Buck Conv. 12→3.3 V | Monolithic Power | MP2307DN | SOIC-8 | 1 | 0.65 | 0.65 |
| U9 | USB-C PD PHY (optional) | FUSB302 | FUSB302BMPX | WQFN-14 | 1 | 0.55 | 0.55 |
| D1-D4 | TVS Diode Array | Littelfuse | SRV05-4 | SOT-23-6 | 4 | 0.30 | 1.20 |
| D5 | ESD Diode USB-C | Prsemi | PRTR5V0U2X | SOT-143 | 1 | 0.15 | 0.15 |
| LED1-4 | RGB LED | Worldsemi | WS2812B-Mini | 3.5×3.5mm | 4 | 0.18 | 0.72 |
| Y1 | 8 MHz crystal | Abracon | ABM8-8.000MHZ | HC49/SMD | 1 | 0.35 | 0.35 |
| Y2 | 32.768 kHz crystal | Abracon | ABS07-32.768KHZ | 3.2×1.5mm | 1 | 0.30 | 0.30 |
| Y3 | 32 MHz crystal (nRF) | Abracon | ABS07-32.000MHZ | 3.2×1.5mm | 1 | 0.40 | 0.40 |
| J1 | OBD-II 16-pin male | Molex | 345000216 | Through-hole | 1 | 1.20 | 1.20 |
| J2 | USB-C 16-pin receptacle | GCT | USB4105-GF | SMD | 1 | 0.55 | 0.55 |
| J3 | microSD card slot | Molex | 5040771891 | SMD | 1 | 0.65 | 0.65 |
| J4 | SWD 5-pin header | Samtec | FTSH-105-01 | 0.05" | 1 | 0.40 | 0.40 |
| L1 | Inductor 4.7 µH, 3 A | Würth | 74477114 | SMD | 1 | 0.35 | 0.35 |
| FB1-2 | Ferrite bead 600 Ω | Murata | BLM21A601SN1 | 0805 | 2 | 0.03 | 0.06 |
| C1-C6 | 100 nF decoupling | Samsung | CL05B104KO5NN | 0402 | 6 | 0.005 | 0.03 |
| C7-C10 | 4.7 µF bulk | Samsung | CL05A475KQ5NQ | 0402 | 4 | 0.02 | 0.08 |
| C11 | 1 µF LDO output | Samsung | CL05A105KQ5NQ | 0402 | 1 | 0.01 | 0.01 |
| C12 | 22 µF buck output | Samsung | CL10A226MQ8NR | 0603 | 1 | 0.04 | 0.04 |
| C13-C14 | 20 pF (8 MHz xtal) | Samsung | CL05C200JB5NN | 0402 | 2 | 0.005 | 0.01 |
| C15-C16 | 12 pF (32 MHz xtal) | Samsung | CL05C120JB5NN | 0402 | 2 | 0.005 | 0.01 |
| R1-R8 | 4.7 kΩ pull-up | Yageo | RC0402FR-074K7L | 0402 | 8 | 0.003 | 0.02 |
| R9-R10 | 120 Ω CAN termination | Yageo | RC0402FR-07120RL | 0402 | 2 | 0.003 | 0.01 |
| R11-R12 | 10 kΩ nRST pull-up | Yageo | RC0402FR-0710KL | 0402 | 2 | 0.003 | 0.01 |
| R13 | 27.4 Ω USB D+ | Yageo | RC0402FR-0727R4L | 0402 | 1 | 0.003 | 0.003 |
| R14 | 27.4 Ω USB D− | Yageo | RC0402FR-0727R4L | 0402 | 1 | 0.003 | 0.003 |
| R15 | 1.5 kΩ USB D+ pull-up | Yageo | RC0402FR-071K5L | 0402 | 1 | 0.003 | 0.003 |
| | **TOTAL** | | | | | | **~$29.55** |

## 2. Pinout Tables

### 2.1 STM32F407VGT6 — Key Pin Assignments

| Pin | Function | Net Name | Notes |
|---|---|---|---|
| 1 | PE2 | MCU_PE2 | Free GPIO (expansion) |
| 2 | PE3 | MCU_PE3 | Free GPIO (expansion) |
| 7 | PE7 | LED_DIN | WS2812B data in (bit-bang SPI) |
| 10 | PE10 | CAN2_RX_ISO | ISO1050 CH2 RX OUT |
| 11 | PE11 | CAN2_TX_ISO | ISO1050 CH2 TX IN |
| 14 | PB0 | CAN1_RX_ISO | ISO1050 CH1 RX OUT |
| 15 | PB1 | CAN1_TX_ISO | ISO1050 CH1 TX IN |
| 19 | PB5 | SPI3_MOSI | SPI to nRF52840 |
| 20 | PB4 | SPI3_MISO | SPI from nRF52840 |
| 21 | PB3 | SPI3_SCK | SPI clock to nRF52840 |
| 22 | PD7 | SPI3_NSS | SPI chip select (nRF) |
| 28 | PC6 | SDIO_D6 | microSD data line 6 |
| 29 | PC7 | SDIO_D7 | microSD data line 7 |
| 30 | PC8 | SDIO_D0 | microSD data line 0 |
| 31 | PC9 | SDIO_D1 | microSD data line 1 |
| 33 | PA8 | SDIO_D2 | microSD data line 2 |
| 34 | PA9 | SDIO_D3 | microSD data line 3 |
| 35 | PA10 | SDIO_CMD | microSD command |
| 36 | PA11 | USB_DM | USB-C D− |
| 37 | PA12 | USB_DP | USB-C D+ |
| 38 | PA13 | SWDIO | SWD debug |
| 39 | PA14 | SWCLK | SWD debug |
| 40 | PA15 | I2C1_SCL | EEPROM clock |
| 44 | PB6 | I2C1_SDA | EEPROM data |
| 45 | PB7 | nRF_BOOT | nRF52840 BOOTMODE pin |
| 51 | PC10 | SDIO_CK | microSD clock |
| 58 | PD2 | SDIO_CMD_ALT | (alternate SDIO CMD) |
| 69 | PH0 | OSC_IN | 8 MHz crystal |
| 70 | PH1 | OSC_OUT | 8 MHz crystal |
| 71 | VBAT | VBAT | Backup domain power |
| 72 | NRST | nRST | Reset (10 kΩ pull-up) |
| 81 | PC14 | OSC32_IN | 32.768 kHz crystal |
| 82 | PC15 | OSC32_OUT | 32.768 kHz crystal |
| 100 | PE1 | MCU_PE1 | Free GPIO (expansion) |

### 2.2 nRF52840-QIAA — Key Pin Assignments

| Pin | Function | Net Name | Notes |
|---|---|---|---|
| 1 | DEC1 | DEC1 | 0.5 pF + 100 nF decoupling |
| 4 | P0.02/XL2 | nRF_XL2 | 32 MHz crystal |
| 5 | P0.03/XL1 | nRF_XL1 | 32 MHz crystal |
| 10 | P0.08 | SPI_SLAVE_MOSI | SPI slave data out → STM32 |
| 11 | P0.09 | SPI_SLAVE_MISO | SPI slave data in ← STM32 |
| 12 | P0.10 | SPI_SLAVE_SCK | SPI slave clock ← STM32 |
| 13 | P0.11 | SPI_SLAVE_CSN | SPI slave select ← STM32 |
| 14 | P0.12 | nRF_IRQ | Interrupt to STM32 (active low) |
| 21 | P0.19 | ANT | 2.4 GHz antenna feed |
| 30 | P0.22 | nRF_RESET | Reset (10 kΩ pull-up) |
| 32 | VDD | VDD_nRF | 3.3 V supply |
| 39 | P0.29 | nRF_BOOTMODE | Boot mode select from STM32 |
| 40 | P0.30 | nRF_SWCLK | SWD debug clock |
| 41 | P0.31 | nRF_SWDIO | SWD debug data |

### 2.3 TJA1050T/VC — CAN Transceiver (×2)

| Pin | Function | Net Name CH1 | Net Name CH2 |
|---|---|---|---|
| 1 | TXD | CAN1_TXD | CAN2_TXD |
| 2 | GND | GND | GND |
| 3 | VCC | VCC_5V | VCC_5V |
| 4 | RXD | CAN1_RXD | CAN2_RXD |
| 5 | VREF | CAN1_VREF | CAN2_VREF |
| 6 | CANL | CAN1_L | CAN2_L |
| 7 | CANH | CAN1_H | CAN2_H |
| 8 | STB | CAN1_STB | CAN2_STB (standby, active-low) |

### 2.4 ISO1050DUB — Digital Isolator (×2)

| Pin | Function | Net Name CH1 | Net Name CH2 |
|---|---|---|---|
| 1 | VCC1 | VCC_3V3 | VCC_3V3 |
| 2 | TXD_IN | CAN1_TX_ISO | CAN2_TX_ISO |
| 3 | RXD_OUT | CAN1_RX_ISO | CAN2_RX_ISO |
| 4 | GND1 | GND_ISO | GND_ISO |
| 5 | GND2 | GND | GND |
| 6 | TXD_OUT | CAN1_TXD | CAN2_TXD |
| 7 | RXD_IN | CAN1_RXD | CAN2_RXD |
| 8 | VCC2 | VCC_5V | VCC_5V |

## 3. Netlist (Source Pin → Component → Dest Pin)

### 3.1 CAN Channel 1 Signal Path

| Net Name | Source Pin | Component | Dest Pin |
|---|---|---|---|
| CAN1_TX_ISO | STM32 PB1 (bxCAN1_TX) | Direct trace | ISO1050-CH1 pin 2 (TXD_IN) |
| CAN1_RX_ISO | ISO1050-CH1 pin 3 (RXD_OUT) | Direct trace | STM32 PB0 (bxCAN1_RX) |
| CAN1_TXD | ISO1050-CH1 pin 6 (TXD_OUT) | Direct trace | TJA1050-CH1 pin 1 (TXD) |
| CAN1_RXD | TJA1050-CH1 pin 4 (RXD) | Direct trace | ISO1050-CH1 pin 7 (RXD_IN) |
| CAN1_H | TJA1050-CH1 pin 7 (CANH) | 120 Ω term → J1 pin 6 | OBD-II CAN-H |
| CAN1_L | TJA1050-CH1 pin 6 (CANL) | 120 Ω term → J1 pin 14 | OBD-II CAN-L |
| CAN1_STB | STM32 PE2 | Direct trace | TJA1050-CH1 pin 8 (STB) |

### 3.2 CAN Channel 2 Signal Path

| Net Name | Source Pin | Component | Dest Pin |
|---|---|---|---|
| CAN2_TX_ISO | STM32 PE11 (bxCAN2_TX) | Direct trace | ISO1050-CH2 pin 2 (TXD_IN) |
| CAN2_RX_ISO | ISO1050-CH2 pin 3 (RXD_OUT) | Direct trace | STM32 PE10 (bxCAN2_RX) |
| CAN2_TXD | ISO1050-CH2 pin 6 (TXD_OUT) | Direct trace | TJA1050-CH2 pin 1 (TXD) |
| CAN2_RXD | TJA1050-CH2 pin 4 (RXD) | Direct trace | ISO1050-CH2 pin 7 (RXD_IN) |
| CAN2_H | TJA1050-CH2 pin 7 (CANH) | 120 Ω term → J1 pin 3 | OBD-II MS-CAN-H |
| CAN2_L | TJA1050-CH2 pin 6 (CANL) | 120 Ω term → J1 pin 11 | OBD-II MS-CAN-L |
| CAN2_STB | STM32 PE3 | Direct trace | TJA1050-CH2 pin 8 (STB) |

### 3.3 SPI Link (STM32 ↔ nRF52840)

| Net Name | Source | Dest | Notes |
|---|---|---|---|
| SPI3_SCK | STM32 PB3 | nRF P0.10 | SPI clock (STM32 master) |
| SPI3_MOSI | STM32 PB5 | nRF P0.09 | Master-out slave-in |
| SPI3_MISO | STM32 PB4 | nRF P0.08 | Master-in slave-out |
| SPI3_NSS | STM32 PD7 | nRF P0.11 | Chip select (active low) |
| nRF_IRQ | nRF P0.12 | STM32 PE1 | Interrupt (falling edge) |
| nRF_BOOT | STM32 PB7 | nRF P0.29 | Boot mode control |
| nRF_RESET | STM32 (GPIO) | nRF P0.22 | Hardware reset |

### 3.4 Power Netlist

| Net Name | Source | Via | Voltage | Max Current |
|---|---|---|---|---|
| VBAT_OBD | J1 pin 16 | — | 12 V nominal (8–16 V) | 200 mA |
| VBUS_USB | J2 (VBUS) | Schottky OR | 5 V | 500 mA |
| VCC_5V | MP2307DN output | — | 5.0 V | 2 A |
| VCC_3V3 | RT9193-33 output | — | 3.3 V | 500 mA |
| VDD_nRF | VCC_3V3 | — | 3.3 V | 50 mA |
| VCC_ISO | ISO1050 VCC1 | Isolated 3.3 V | 3.3 V | 30 mA |
| GND | Common ground | — | 0 V | — |
| GND_ISO | Isolated ground | — | 0 V (isolated) | — |

## 4. Decoupling Strategy

### 4.1 STM32F407VGT6
- VDD: 4× 100 nF (0402) on VDD pins + 1× 4.7 µF bulk (0402)
- VDDA: 100 nF + 1 µF (analog supply, ferrite bead isolated)
- VBAT: 100 nF (backup domain)

### 4.2 nRF52840
- VDD: 100 nF + 4.7 µF on each VDD pin
- DEC1/DEC2: Per datasheet — 100 nF with 0.5 pF in series
- ANT: PI matching network (2.7 pF series + 1.5 pF shunt to GND)

### 4.3 TJA1050 (each)
- VCC (pin 3): 100 nF + 4.7 µF, placed within 5 mm
- VREF (pin 5): 100 nF to GND (stabilize reference)

### 4.4 MP2307DN Buck Converter
- Input: 2× 10 µF ceramic (0805) + 1× 100 nF
- Output: 22 µF ceramic (0603) + 100 nF
- Bootstrap: 100 nF between BST and SW pins
- Soft-start: 100 nF on SS pin

## 5. Impedance-Controlled Pairs

| Net Pair | Impedance | Layer | Notes |
|---|---|---|---|
| CAN1_H / CAN1_L | 120 Ω diff | Top | OBD-II spec, 120 Ω termination at each end |
| CAN2_H / CAN2_L | 120 Ω diff | Top | Same as above |
| USB_DP / USB_DM | 90 Ω diff | Inner L2 | USB 2.0 full-speed differential pair |
| SPI3_SCK/MOSI/MISO | Single-ended 50 Ω | Top | Short run (<25 mm), not critical |
| nRF ANT | 50 Ω single-ended | Top | With PI match to chip antenna |

## 6. OBD-II Connector Pin Assignment (J1)

| Pin | Net | Function |
|---|---|---|
| 1 | — | Not connected |
| 2 | SAE_J1850_BUS+ | Reserved (future) |
| 3 | CAN2_H | MS-CAN high |
| 4 | GND | Chassis ground |
| 5 | GND_ISO | Signal ground |
| 6 | CAN1_H | HS-CAN high |
| 7 | ISO_9141_K | Reserved (future) |
| 8 | — | Not connected |
| 9 | — | Not connected (battery direct) |
| 10 | CAN2_L | MS-CAN low |
| 11 | — | Not connected |
| 12 | — | Not connected |
| 13 | — | Not connected |
| 14 | CAN1_L | HS-CAN low |
| 15 | ISO_15765_4_L | Reserved (future) |
| 16 | VBAT_OBD | Battery (12 V) |