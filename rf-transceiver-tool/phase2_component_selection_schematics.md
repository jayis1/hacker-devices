# Phase 2: Component Selection & Schematics — RF Transceiver Tool

## 1. Bill of Materials

| Ref | Part Number | Description | Package | Qty | Unit Cost | Extended | Source |
|---|---|---|---|---|---|---|---|
| U1 | STM32F401CCU6 | MCU, Cortex-M4, 256KB Flash, 84MHz | UFQFPN-48 | 1 | $3.20 | $3.20 | LCSC / Mouser |
| U2 | CC1101RGT | Sub-GHz transceiver, 300-928MHz | VQFN-20 (4×4) | 1 | $2.80 | $2.80 | LCSC / Mouser |
| U3 | nRF24L01+ | 2.4GHz transceiver | QFN-20 (4×4) | 1 | $1.50 | $1.50 | LCSC / AliExpress |
| U4 | AMS1117-3.3 | LDO, 5V→3.3V, 1A | SOT-223 | 1 | $0.15 | $0.15 | LCSC |
| U5 | SSD1306 | OLED controller, 128×64, I2C | Module (4-pin) | 1 | $1.20 | $1.20 | AliExpress |
| U6 | USBLC6-2SC6 | USB ESD protection, 2-channel | SOT-23-6 | 1 | $0.25 | $0.25 | LCSC |
| Y1 | 8.000MHz XTAL | HSE for STM32 | HC49-SMD | 1 | $0.20 | $0.20 | LCSC |
| Y2 | 26.000MHz XTAL | Reference for CC1101 | HC49-SMD | 1 | $0.30 | $0.30 | LCSC |
| J1 | USB-C 16P | USB-C 2.0 receptacle, SMD | SMD | 1 | $0.40 | $0.40 | LCSC |
| J2 | SMA edge-launch | SMA connector for Sub-GHz | Edge-launch | 1 | $0.35 | $0.35 | LCSC |
| J3 | SMA edge-launch | SMA connector for 2.4GHz | Edge-launch | 1 | $0.35 | $0.35 | LCSC |
| J4 | 2×4 pin header | Debug SWD connector | 2.54mm | 1 | $0.10 | $0.10 | LCSC |
| C1–C4 | 100nF 0402 | VDD decouple STM32 (4×) | 0402 | 4 | $0.005 | $0.02 | LCSC |
| C5–C6 | 100nF 0402 | VDD decouple CC1101 (2×) | 0402 | 2 | $0.005 | $0.01 | LCSC |
| C7–C8 | 100nF 0402 | VDD decouple nRF24L01+ (2×) | 0402 | 2 | $0.005 | $0.01 | LCSC |
| C9 | 100nF 0402 | VDD decouple SSD1306 | 0402 | 1 | $0.005 | $0.005 | LCSC |
| C10 | 100nF 0402 | USB ESD decouple | 0402 | 1 | $0.005 | $0.005 | LCSC |
| C11–C12 | 10µF 0802 | Bulk 3.3V rail (2×) | 0805 | 2 | $0.05 | $0.10 | LCSC |
| C13 | 47µF 1206 | Bulk 5V USB rail | 1206 | 1 | $0.15 | $0.15 | LCSC |
| C14 | 1µF 0402 | AMS1117 output cap | 0402 | 1 | $0.01 | $0.01 | LCSC |
| C15 | 10µF 0805 | AMS1117 output cap | 0805 | 1 | $0.05 | $0.05 | LCSC |
| C16 | 20pF 0402 | Y1 load cap | 0402 | 1 | $0.005 | $0.005 | LCSC |
| C17 | 20pF 0402 | Y1 load cap | 0402 | 1 | $0.005 | $0.005 | LCSC |
| C18 | 20pF 0402 | Y2 load cap | 0402 | 1 | $0.005 | $0.005 | LCSC |
| C19 | 20pF 0402 | Y2 load cap | 0402 | 1 | $0.005 | $0.005 | LCSC |
| L1 | 8.2nH 0402 | CC1101 RF match | 0402 | 1 | $0.02 | $0.02 | LCSC |
| L2 | 5.6nH 0402 | CC1101 RF match | 0402 | 1 | $0.02 | $0.02 | LCSC |
| L3 | 2.7nH 0402 | nRF24L01+ ANT match | 0402 | 1 | $0.02 | $0.02 | LCSC |
| R1 | 1.5kΩ 0402 | CC1101 external bias | 0402 | 1 | $0.005 | $0.005 | LCSC |
| R2–R3 | 22Ω 0402 | USB D+/D- series | 0402 | 2 | $0.005 | $0.01 | LCSC |
| R4 | 5.1kΩ 0402 | USB-C CC1 pull-down | 0402 | 1 | $0.005 | $0.005 | LCSC |
| R5 | 5.1kΩ 0402 | USB-C CC2 pull-down | 0402 | 1 | $0.005 | $0.005 | LCSC |
| R6 | 330Ω 0402 | LED1 current limit | 0402 | 1 | $0.005 | $0.005 | LCSC |
| R7 | 330Ω 0402 | LED2 current limit | 0402 | 1 | $0.005 | $0.005 | LCSC |
| R8 | 10kΩ 0402 | BTN1 pull-up | 0402 | 1 | $0.005 | $0.005 | LCSC |
| R9 | 10kΩ 0402 | BTN2 pull-up | 0402 | 1 | $0.005 | $0.005 | LCSC |
| R10 | 56kΩ 0402 | CC1101 RSET | 0402 | 1 | $0.005 | $0.005 | LCSC |
| D1 | LED Green 0603 | Sub-GHz status LED | 0603 | 1 | $0.02 | $0.02 | LCSC |
| D2 | LED Blue 0603 | 2.4 GHz status LED | 0603 | 1 | $0.02 | $0.02 | LCSC |
| SW1 | Tactile switch 4×4mm | Mode button | 4×4mm SMD | 1 | $0.05 | $0.05 | LCSC |
| SW2 | Tactile switch 4×4mm | Action button | 4×4mm SMD | 1 | $0.05 | $0.05 | LCSC |
| FB1 | 600Ω@100MHz 0402 | Ferrite bead, 3V3 rail | 0402 | 1 | $0.02 | $0.02 | LCSC |
| | | | | **Total** | | **~$12.00** | |

## 2. Pinout Tables

### 2.1 STM32F401CCU6 (U1) — UFQFPN-48

| Pin | Name | Function | Connected To |
|---|---|---|---|
| 1 | VBAT | Power | 3V3 |
| 2 | PC13 | GPIO | (NC, free) |
| 3 | PC14 | GPIO | (NC, free) |
| 4 | PC15 | GPIO | (NC, free) |
| 5 | PH0/OSC_IN | HSE | Y1 (8 MHz XTAL) |
| 6 | PH1/OSC_OUT | HSE | Y1 (8 MHz XTAL) |
| 7 | NRST | Reset | SWD connector, 10k pull-up |
| 8 | VSSA | Ground | GND |
| 9 | VDDA | Analog power | 3V3 (via FB1) |
| 10 | PA0 | GPIO_IN | BTN1 (mode select) |
| 11 | PA1 | GPIO_IN | BTN2 (action) |
| 12 | PA2 | GPIO_OUT | (NC, free) |
| 13 | PA3 | GPIO_OUT | CC1101 RESETn |
| 14 | PA4 | SPI1_NSS | CC1101 CSN (chip select) |
| 15 | PA5 | SPI1_SCK | CC1101 SCLK |
| 16 | PA6 | SPI1_MISO | CC1101 SO (MISO) |
| 17 | PA7 | SPI1_MOSI | CC1101 SI (MOSI) |
| 18 | PB0 | GPIO_OUT | LED1 (Sub-GHz status) |
| 19 | PB1 | GPIO_OUT | LED2 (2.4GHz status) |
| 20 | PB2 | GPIO | (NC, free) |
| 21 | PB10 | I2C2_SCL | (NC, free) |
| 22 | PB11 | (NC) | — |
| 23 | VSS | Ground | GND |
| 24 | VDD | Power | 3V3 |
| 25 | PB12 | SPI2_NSS | nRF24L01+ CSN |
| 26 | PB13 | SPI2_SCK | nRF24L01+ SCK |
| 27 | PB14 | SPI2_MISO | nRF24L01+ MISO |
| 28 | PB15 | SPI2_MOSI | nRF24L01+ MOSI |
| 29 | PA8 | GPIO_OUT | CC1101 GDO0 |
| 30 | PA9 | GPIO_OUT | CC1101 GDO2 |
| 31 | PA10 | (NC) | — |
| 32 | PA11 | USB_DM | USB-C DM (via U6, R2) |
| 33 | PA12 | USB_DP | USB-C DP (via U6, R3) |
| 34 | PA13 | SWDIO | SWD connector |
| 35 | VDD | Power | 3V3 |
| 36 | PA14 | SWCLK | SWD connector |
| 37 | PA15 | GPIO_OUT | nRF24L01+ CE |
| 38 | PB3 | GPIO_IN | nRF24L01+ IRQ |
| 39 | PB4 | (NC) | — |
| 40 | PB5 | (NC) | — |
| 41 | PB6 | I2C1_SCL | SSD1306 SCL |
| 42 | PB7 | I2C1_SDA | SSD1306 SDA |
| 43 | BOOT0 | Boot mode | 10k pull-down → GND |
| 44 | PB8 | (NC) | — |
| 45 | PB9 | (NC) | — |
| 46 | VSS | Ground | GND |
| 47 | VDD | Power | 3V3 |
| 48 | VSS | Ground | GND |

### 2.2 CC1101RGT (U2) — VQFN-20

| Pin | Name | Function | Connected To |
|---|---|---|---|
| 1 | SCLK | SPI clock | STM32 PA5 (SPI1_SCK) |
| 2 | SO | SPI MISO | STM32 PA6 (SPI1_MISO) |
| 3 | GDO2 | General purpose output | STM32 PA9 |
| 4 | GDO0 | General purpose output | STM32 PA8 |
| 5 | GDO1 | SPI status (MISO/IRQ) | (tied high via 10k) |
| 6 | CSn | SPI chip select (active low) | STM32 PA4 (SPI1_NSS) |
| 7 | GND | Ground | GND |
| 8 | XOSC_Q1 | XTAL input | Y2 (26 MHz) pin 1, C18 |
| 9 | XOSC_Q2 | XTAL output | Y2 (26 MHz) pin 2, C19 |
| 10 | AVDD | Analog power | 3V3 (via FB1 + C5) |
| 11 | PA_PD | Power amp PD output | L1 (RF match → J2 SMA) |
| 12 | DVDD | Digital power | 3V3 (via C6) |
| 13 | GND | Ground | GND |
| 14 | SI | SPI MOSI | STM32 PA7 (SPI1_MOSI) |
| 15 | GND | Ground | GND |
| 16 | DCOUPL | Regulator decouple | C9 (100nF to GND) |
| 17 | AVDD | Analog power | 3V3 (via FB1 + C5) |
| 18 | R_BIAS | Bias resistor | R10 (56kΩ to GND) |
| 19 | GND | Ground | GND |
| 20 | RESETn | Reset (active low) | STM32 PA3 |

**CC1101 RF Output Matching Network (868 MHz, 50Ω):**

```
CC1101 PA_PD ──► L1 (8.2nH) ──┬──► C_par (stray) ──► J2 (SMA, 50Ω)
                               │
                              L2 (5.6nH) ──► GND

Note: L1 and L2 form a pi-network matching 50Ω. Exact values per
TI DN023 (SWRA161D). For 433 MHz, substitute L1=12nH, L2=10nH.
```

### 2.3 nRF24L01+ (U3) — QFN-20

| Pin | Name | Function | Connected To |
|---|---|---|---|
| 1 | CE | Chip Enable (TX/RX) | STM32 PA15 |
| 2 | CSN | SPI chip select | STM32 PB12 (SPI2_NSS) |
| 3 | SCK | SPI clock | STM32 PB13 (SPI2_SCK) |
| 4 | MOSI | SPI MOSI | STM32 PB15 (SPI2_MOSI) |
| 5 | MISO | SPI MISO | STM32 PB14 (SPI2_MISO) |
| 6 | IRQ | Active-low interrupt | STM32 PB3 |
| 7 | VDD | Power | 3V3 (via C7, C8) |
| 8 | GND | Ground | GND |
| 9–12 | GND | Ground | GND |
| 13 | VDD | Power | 3V3 |
| 14 | GND | Ground | GND |
| 15 | ANT1 | Antenna output | L3 (2.7nH) → J3 (SMA) |
| 16 | ANT2 | Antenna output | (cap coupled to ANT1) |
| 17–20 | VDD/GND | Power/Ground | 3V3 / GND |

## 3. Decoupling Networks

### 3.1 STM32F401CCU6 Decoupling

| Cap | Value | Package | Net | Placement |
|---|---|---|---|---|
| C1 | 100nF | 0402 | VDD pin 1 (VBAT) → GND | Adjacent to pin 1 |
| C2 | 100nF | 0402 | VDD pin 24 → GND | Adjacent to pin 24 |
| C3 | 100nF | 0402 | VDD pin 35 → GND | Adjacent to pin 35 |
| C4 | 100nF | 0402 | VDD pin 47 → GND | Adjacent to pin 47 |
| C11 | 10µF | 0805 | 3V3 bulk → GND | Near MCU center |
| C12 | 10µF | 0805 | 3V3 bulk → GND | Near MCU center |

### 3.2 CC1101 Decoupling

| Cap | Value | Package | Net | Placement |
|---|---|---|---|---|
| C5 | 100nF | 0402 | AVDD pin 10 → GND | Adjacent to pin 10 |
| C6 | 100nF | 0402 | DVDD pin 12 → GND | Adjacent to pin 12 |
| C9 | 100nF | 0402 | DCOUPL pin 16 → GND | Adjacent to pin 16 |

### 3.3 nRF24L01+ Decoupling

| Cap | Value | Package | Net | Placement |
|---|---|---|---|---|
| C7 | 100nF | 0402 | VDD pin 7 → GND | Adjacent to pin 7 |
| C8 | 100nF | 0402 | VDD pin 13 → GND | Adjacent to pin 13 |

### 3.4 Power Supply Decoupling

| Cap | Value | Package | Net | Placement |
|---|---|---|---|---|
| C13 | 47µF | 1206 | 5V VBATT → GND | Near USB connector |
| C14 | 1µF | 0402 | AMS1117 ADJ → GND | Adjacent to U4 |
| C15 | 10µF | 0805 | AMS1117 VOUT → GND | Adjacent to U4 output |
| C10 | 100nF | 0402 | USBLC6 VCC → GND | Adjacent to U6 |

## 4. Netlist

### 4.1 Power Nets

| Net Name | Source | Destination(s) |
|---|---|---|
| VBATT | J1 (USB VBUS) | U4 VIN, C13 pos, U6 VCC |
| 3V3 | U4 VOUT | U1 VDD×4, U2 AVDD×2, U2 DVDD, U3 VDD×2, U5 VCC, FB1 |
| 3V3A | FB1 out | U1 VDDA, U2 AVDD (filtered) |
| GND | J1 GND, U4 GND | All IC GND pins, C– neg |

### 4.2 SPI1 Bus (STM32 ↔ CC1101)

| Net Name | Source Pin | Dest Pin |
|---|---|---|
| SPI1_SCK | U1 PA5 | U2 SCLK (pin 1) |
| SPI1_MISO | U2 SO (pin 2) | U1 PA6 |
| SPI1_MOSI | U1 PA7 | U2 SI (pin 14) |
| CC1101_CSN | U1 PA4 | U2 CSn (pin 6) |
| CC1101_GDO0 | U2 GDO0 (pin 4) | U1 PA8 |
| CC1101_GDO2 | U2 GDO2 (pin 3) | U1 PA9 |
| CC1101_RESETn | U1 PA3 | U2 RESETn (pin 20) |

### 4.3 SPI2 Bus (STM32 ↔ nRF24L01+)

| Net Name | Source Pin | Dest Pin |
|---|---|---|
| SPI2_SCK | U1 PB13 | U3 SCK (pin 3) |
| SPI2_MISO | U3 MISO (pin 5) | U1 PB14 |
| SPI2_MOSI | U1 PB15 | U3 MOSI (pin 4) |
| NRF24_CSN | U1 PB12 | U3 CSN (pin 2) |
| NRF24_CE | U1 PA15 | U3 CE (pin 1) |
| NRF24_IRQ | U3 IRQ (pin 6) | U1 PB3 |

### 4.4 I2C1 Bus (STM32 ↔ SSD1306)

| Net Name | Source Pin | Dest Pin |
|---|---|---|
| I2C1_SCL | U1 PB6 | U5 SCL |
| I2C1_SDA | U1 PB7 | U5 SDA |

### 4.5 USB Bus

| Net Name | Source Pin | Dest Pin |
|---|---|---|
| USB_DM | J1 D− | U6 D1−→U1 PA11 (via R2 22Ω) |
| USB_DP | J1 D+ | U6 D1+→U1 PA12 (via R3 22Ω) |
| USB_CC1 | J1 CC1 | R4 (5.1kΩ) → GND |
| USB_CC2 | J1 CC2 | R5 (5.1kΩ) → GND |

### 4.6 GPIO / Misc

| Net Name | Source Pin | Dest Pin |
|---|---|---|
| LED1 | U1 PB0 | R6 (330Ω) → D1 anode → GND |
| LED2 | U1 PB1 | R7 (330Ω) → D2 anode → GND |
| BTN1 | SW1 one terminal | U1 PA0, R8 (10kΩ) → 3V3 |
| BTN2 | SW2 one terminal | U1 PA1, R9 (10kΩ) → 3V3 |
| SW1_GND | SW1 other terminal | GND |
| SW2_GND | SW2 other terminal | GND |

### 4.7 RF Output Nets

| Net Name | Source Pin | Dest |
|---|---|---|
| CC1101_RF | U2 PA_PD (pin 11) | L1 (8.2nH) → J2 SMA center |
| CC1101_RF_GND | L2 (5.6nH) | GND |
| NRF24_ANT | U3 ANT1 (pin 15) | L3 (2.7nH) → J3 SMA center |

## 5. Impedance-Matched Pairs

### 5.1 RF Traces (50 Ω)

| Net | Frequency | Impedance | Width (4-layer) | Reference |
|---|---|---|---|---|
| CC1101_RF | 868 MHz | 50 Ω ± 10% | 0.38 mm (coplanar) | Top layer, L2 GND ref |
| NRF24_ANT | 2.4 GHz | 50 Ω ± 10% | 0.38 mm (coplanar) | Top layer, L2 GND ref |

Coplanar waveguide: Er=4.2 (FR4), H=0.2mm (L1-L2), W=0.38mm, G=0.25mm gap → Z≈50Ω.

### 5.2 USB Traces (90 Ω differential)

| Net | Frequency | Impedance | Width/Spacing | Reference |
|---|---|---|---|---|
| USB_DM/DP | 480 MHz | 90 Ω diff | 0.22mm / 0.15mm | Top layer, L2 GND ref |

### 5.3 Crystal Load Capacitors

| Crystal | Frequency | CL (crystal) | C_load | C_ext |
|---|---|---|---|---|
| Y1 | 8.000 MHz | 10 pF | 2 × C_ext + 5 pF (stray) | C16=C17=20 pF → C_load≈15 pF |
| Y2 | 26.000 MHz | 12 pF | 2 × C_ext + 4 pF (stray) | C18=C19=20 pF → C_load≈14 pF |