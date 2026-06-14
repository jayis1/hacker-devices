# NFC Relay Phantom — Phase 2: Component Selection & Schematics

## 1. Bill of Materials

| Ref | Part Number | Description | Qty | Price (1K) | Notes |
|-----|-------------|-------------|-----|-----------|-------|
| U1 | STM32L4S5VIT6 | Main MCU, Cortex-M4 @ 120 MHz, 2 MB Flash, 640 KB SRAM | 1 | $7.20 | LQFP-100 |
| U2 | NRF52832-CIAB | BLE 5.0 SoC, Cortex-M4F @ 64 MHz | 1 | $4.10 | QFN-48 |
| U3 | PN5180A0HN/C2 | NFC Frontend Controller, 13.56 MHz | 1 | $3.80 | HVQFN-40 |
| U4 | EM4095 | 125 kHz RFID Reader IC | 1 | $1.90 | SOIC-16 |
| U5 | W25Q128JVSIQ | 16 MB SPI NOR Flash | 1 | $0.85 | SOIC-8 |
| U6 | BQ25896RTWR | USB Battery Charger, I2C, 3 A | 1 | $1.60 | WQFN-24 |
| U7 | TPS6284010YKR | 3.3 V Buck, 60 nA Iq, 750 mA | 1 | $0.90 | DSBGA-6 |
| U8 | SSD1306Z | 128×64 OLED Controller, I2C | 1 | $2.10 | COG on flex |
| U9 | TLV70033DCKR | 3.3 V LDO for NRF52 VDD | 1 | $0.35 | SOT-23-5 |
| Q1-Q3 | SI1308BDL-T1 | P-MOSFET load switches | 3 | $0.20ea | SOT-23 |
| D1 | USBLC6-2SC6 | USB ESD protection | 1 | $0.25 | SOT-23-6 |
| D2-D3 | RB521S30T1G | Schottky diodes | 2 | $0.03ea | SOD-523 |
| Y1 | ECS-3225S-8MHz | 8 MHz crystal, STM32L4 HSE | 1 | $0.25 | 3.2×2.5mm |
| Y2 | ABS07-32.768KHZ | 32.768 kHz crystal, RTC | 1 | $0.15 | 7.0×2.5mm |
| Y3 | ABS07-32.000KHZ-T | 32 MHz crystal, NRF52832 | 1 | $0.20 | 7.0×2.5mm |
| L1 | 10 µH, NLV32T-100J | EM4095 antenna tuning | 1 | $0.35 | 3.2×2.5mm |
| L2-L3 | 22 nH, LQP15MN22NJ00D | PN5180 RF matching | 2 | $0.10ea | 0402 |
| L4 | 1.0 µH, XGL6045S-102MEC | TPS62840 inductor | 1 | $0.45 | 6.0×4.5mm |
| C1-C6 | 100 nF 0402 | Decoupling, per IC VDD | 15 | $0.002ea | 0402 X5R 16V |
| C7-C10 | 1 µF 0402 | Bulk decoupling | 6 | $0.005ea | 0402 X5R 6.3V |
| C11-C14 | 22 pF 0402 | Crystal load caps | 4 | $0.002ea | C0G NP0 |
| C15 | 10 µF 0603 | VBAT bulk | 1 | $0.03 | X5R 10V |
| C16 | 22 µF 0603 | TPS62840 output | 1 | $0.05 | X5R 6.3V |
| C17 | 1 nF 0402 | PN5180 TX matching | 2 | $0.002ea | C0G |
| R1-R8 | 10 kΩ 0402 | Pull-ups, default states | 10 | $0.001ea | 1% |
| R9 | 33 Ω 0402 | USB D+ series | 1 | — | USB term |
| R10 | 33 Ω 0402 | USB D- series | 1 | — | USB term |
| R11 | 1.5 kΩ 0402 | USB D+ pull-up | 1 | — | D+ pull-up |
| R12-R13 | 4.7 kΩ 0402 | I2C pull-ups | 2 | — | SDA/SCL |
| R14 | 56 kΩ 0402 | BQ25896 PROG resistor | 1 | — | 500 mA charge |
| R15 | 100 kΩ 0402 | EM4095 ANT_DRV resistor | 1 | — | Antenna drive |
| J1 | USB4105-GF-A | USB-C 2.0 receptacle | 1 | $0.45 | SMD, 16-pin |
| J2 | TC2050-NL | 10-pin ARM debug header | 1 | $0.60 | Tag-Connect |
| J3 | 505560-0471 | Molex 1.25mm, 4-pin battery | 1 | $0.30 | JST-PH 4-pin |
| SW1-SW2 | SKQGAFE010 | Tactile switch, 6×3.5mm | 2 | $0.08ea | User buttons |
| LED1 | 150060VS75000 | Green LED 0603 | 1 | $0.04 | Status |
| LED2 | 150060RS75000 | Red LED 0603 | 1 | $0.04 | Error |
| LED3 | 150060BS75000 | Blue LED 0603 | 1 | $0.04 | BLE |
| FB1 | BLM21PG221SN1D | 220 Ω ferrite bead | 1 | $0.02 | USB VBUS |
| ANT_125 | PCB trace | 125 kHz antenna (8 turns, 0.5mm trace) | 1 | — | Embedded |
| ANT_NFC | PCB trace | 13.56 MHz antenna (4 turns, 1mm trace, 40×30mm) | 1 | — | Embedded |
| PCB | — | 4-layer, 85×54mm | 1 | $3.50 | ENIG, 0.8mm |
| BAT | LP502030 | 3.7 V 1200 mAh LiPo | 1 | $4.50 | JST-PH connector |

**Total BOM: ~$38.00 (1K qty)**

---

## 2. Pinout Tables

### 2.1 STM32L4S5VIT6 (U1) — LQFP-100

| Pin | Net | Function | Notes |
|-----|-----|----------|-------|
| 1 | VBAT | Power | Battery backup domain |
| 2 | PC13 | SW1_BTN | User button 1, active low |
| 3 | PC14 | OSC32_IN | 32.768 kHz crystal |
| 4 | PC15 | OSC32_OUT | 32.768 kHz crystal |
| 5 | PH0 | OSC_IN | 8 MHz HSE crystal |
| 6 | PH1 | OSC_OUT | 8 MHz HSE crystal |
| 7 | NRST | NRST | Reset, 10k pull-up + 100nF |
| 8 | PC0 | NRF_IRQ | NRF52832 IRQ input |
| 9 | PC1 | EM4095_SHD | EM4095 shutdown control |
| 10 | PC2 | EM4095_MOD | EM4095 modulation control |
| 11 | PC3 | NFC_ANT_SEL | Antenna select (high=Rx, low=Tx) |
| 12 | PA0 | UART4_TX | UART to NRF52832 |
| 13 | PA1 | UART4_RX | UART from NRF52832 |
| 14 | PA2 | — | GPIO spare |
| 15 | PA3 | — | GPIO spare |
| 16 | PA4 | SPI1_NSS | PN5180 chip select |
| 17 | PA5 | SPI1_SCK | PN5180 SPI clock |
| 18 | PA6 | SPI1_MISO | PN5180 SPI MISO |
| 19 | PA7 | SPI1_MOSI | PN5180 SPI MOSI |
| 20 | PC4 | PN5180_BUSY | PN5180 busy indicator |
| 21 | PC5 | PN5180_RST | PN5180 reset, active low |
| 22 | PB0 | LED_GREEN | Status LED (active high) |
| 23 | PB1 | LED_RED | Error LED (active high) |
| 24 | PB2 | — | GPIO spare |
| 25-32 | PB3-PB10 | — | GPIO spare / JTAG |
| 33 | PB11 | — | GPIO spare |
| 34 | VDD | Power | 3.3V |
| 35 | PB12 | SPI2_NSS | W25Q128 chip select |
| 36 | PB13 | SPI2_SCK | W25Q128 SPI clock |
| 37 | PB14 | SPI2_MISO | W25Q128 SPI MISO |
| 38 | PB15 | SPI2_MOSI | W25Q128 SPI MOSI |
| 39 | PD0 | — | GPIO spare |
| 40 | PD1 | — | GPIO spare |
| 41-46 | PD2-PD7 | — | GPIO spare |
| 47 | PB6 | I2C1_SCL | SSD1306 + BQ25896 |
| 48 | PB7 | I2C1_SDA | SSD1306 + BQ25896 |
| 49 | BOOT0 | BOOT0 | 10k pull-down |
| 50 | PB8 | SW2_BTN | User button 2, active low |
| 51 | PB9 | LED_BLUE | BLE status LED |
| 52 | PE0 | — | GPIO spare |
| 53-68 | PE1-PE16 | — | GPIO spare |
| 69 | PA8 | — | GPIO spare |
| 70 | PA9 | UART1_TX | EM4095 data TX |
| 71 | PA10 | UART1_RX | EM4095 data RX |
| 72 | PA11 | USB_DM | USB-C D- |
| 73 | PA12 | USB_DP | USB-C D+ |
| 74 | PA13 | SWDIO | Debug |
| 75 | PA14 | SWCLK | Debug |
| 76 | PA15 | SPI1_NSS_ALT | Alternate SPI1 NSS |
| 77-78 | PC6-PC7 | — | GPIO spare |
| 79 | PC8 | NRF_RST | NRF52832 reset, active low |
| 80 | PC9 | — | GPIO spare |
| 81-88 | PC10-PC12, PD8-PD12 | — | GPIO spare |
| 89-100 | VDD/VSS | Power | Decoupled 3.3V/GND pairs |

### 2.2 NRF52832-CIAB (U2) — QFN-48

| Pin | Net | Function | Notes |
|-----|-----|----------|-------|
| 1 | DEC1 | Decoupling | 100nF to GND |
| 2 | DCC | DC-DC inductor | 10µH to VDD |
| 3 | DEC2 | Decoupling | NC (internal) |
| 4 | P0.00 | XL1 | 32 MHz crystal |
| 5 | P0.01 | XL2 | 32 MHz crystal |
| 6 | P0.02 | UART_RX | From STM32L4 UART4_TX |
| 7 | P0.03 | UART_TX | To STM32L4 UART4_RX |
| 8 | P0.04 | BLE_CONN | BLE connection status |
| 9 | P0.05 | NRF_READY | Signal to STM32L4 |
| 10 | P0.06 | — | GPIO spare |
| 11 | P0.07 | — | GPIO spare |
| 12 | P0.08 | — | GPIO spare |
| 13 | P0.09 | NFC1 | NFC antenna pair (unused) |
| 14 | P0.10 | NFC2 | NFC antenna pair (unused) |
| 15 | P0.11 | SWDIO | Debug |
| 16 | P0.12 | SWCLK | Debug |
| 17-24 | P0.13-P0.20 | — | GPIO spare |
| 25 | P0.21 | NRF_RST_IN | From STM32L4 PC8 |
| 26 | PORT0 | — | GPIO |
| 27-30 | P0.22-P0.25 | — | GPIO spare |
| 31 | P0.26 | — | GPIO spare |
| 32 | P0.27 | — | GPIO spare |
| 33 | P0.28 | — | GPIO spare |
| 34 | P0.29 | — | GPIO spare |
| 35 | P0.30 | — | GPIO spare |
| 36 | P0.31 | — | GPIO spare |
| 37 | VDD | 3.3V | From LDO |
| 38-39 | VDD | 3.3V | Power |
| 40 | GND | GND | — |
| 41-48 | VSS/GND | GND | Ground |

### 2.3 PN5180A0HN/C2 (U3) — HVQFN-40

| Pin | Net | Function | Notes |
|-----|-----|----------|-------|
| 1 | VIN | 5V (USB VBUS) | Main power |
| 2 | VDD | 3.3V | Digital power |
| 3 | GND | GND | — |
| 4 | SPI_NSS | PA4 (STM32) | SPI chip select |
| 5 | SPI_SCK | PA5 (STM32) | SPI clock |
| 6 | SPI_MOSI | PA7 (STM32) | SPI data in |
| 7 | SPI_MISO | PA6 (STM32) | SPI data out |
| 8 | BUSY | PC4 (STM32) | Busy indicator |
| 9 | IRQ | — | Interrupt (wired-OR) |
| 10 | RST | PC5 (STM32) | Reset, active low |
| 11 | ANT1 | Antenna pad 1 | 13.56 MHz trace |
| 12 | ANT2 | Antenna pad 2 | 13.56 MHz trace |
| 13 | TX1 | Matching net 1 | L2 + C17 |
| 14 | TX2 | Matching net 2 | L3 + C17 |
| 15 | TVDD | 3.3V | TX power supply |
| 16 | AVDD | 3.3V | Analog supply |
| 17 | DVDD | 3.3V | Digital supply |
| 18-24 | GND | GND | Ground pads |
| 25-40 | Antenna/Matching | RF network | See matching circuit |

### 2.4 EM4095 (U4) — SOIC-16

| Pin | Net | Function | Notes |
|-----|-----|----------|-------|
| 1 | VDD | 5V | From USB or boost |
| 2 | AGND | GND | Analog ground |
| 3 | ANT1 | Antenna drive 1 | 125 kHz antenna |
| 4 | ANT2 | Antenna drive 2 | 125 kHz antenna |
| 5 | CDEC_IN | 100nF to GND | Charge pump decoupling |
| 6 | CDEC_OUT | 100nF to GND | Charge pump decoupling |
| 7 | SHD | PC1 (STM32) | Shutdown, active low |
| 8 | MOD | PC2 (STM32) | Modulation input |
| 9 | NC | — | No connect |
| 10 | DGND | GND | Digital ground |
| 11 | RDY/CLK | — | Ready output (unused) |
| 12 | DEMOD_OUT | PA10 (STM32 UART1_RX) | Demodulated data |
| 13 | NC | — | No connect |
| 14 | NC | — | No connect |
| 15 | VSS | GND | — |
| 16 | FEI | — | Free-running (tied low) |

---

## 3. Netlist (Source → Component → Destination)

### 3.1 Power Domain Nets

| Net | Source Pin | Component | Dest Pin | Notes |
|-----|-----------|-----------|----------|-------|
| VBUS_5V | J1 (VBUS) | FB1 → C15 | U6 (VBUS), U3 (VIN), U4 (VDD) | USB 5V, ferrite filtered |
| VBAT | J3 (+) | — | U6 (BAT), U1 (VBAT) | 3.7V LiPo |
| VREG_3V3 | U7 (OUT) | C16 | U1 (VDD×8), U3 (VDD,DVDD,AVDD,TVDD), U8 (VDD), U5 (VDD) | Main 3.3V rail |
| VREG_3V3_NRF | U9 (OUT) | C10 | U2 (VDD×3) | Isolated NRF52 supply |
| VBUS_5V | — | — | Q1 gate (load switch) | Antenna power gate |

### 3.2 SPI1 Bus (STM32L4 → PN5180)

| Net | Source Pin | Component | Dest Pin | Notes |
|-----|-----------|-----------|----------|-------|
| SPI1_NSS | U1-PA4 | R22 (10Ω) | U3-4 (NSS) | With pull-up |
| SPI1_SCK | U1-PA5 | — | U3-5 (SCK) | Direct, <10mm trace |
| SPI1_MOSI | U1-PA7 | — | U3-6 (MOSI) | Direct |
| SPI1_MISO | U1-PA6 | R23 (22Ω) | U3-7 (MISO) | Series term |
| PN5180_BUSY | U3-8 | — | U1-PC4 | Direct GPIO |
| PN5180_RST | U1-PC5 | — | U3-10 (RST) | Active low |

### 3.3 SPI2 Bus (STM32L4 → W25Q128)

| Net | Source Pin | Component | Dest Pin | Notes |
|-----|-----------|-----------|----------|-------|
| SPI2_NSS | U1-PB12 | — | U5-1 (CS) | Active low |
| SPI2_SCK | U1-PB13 | — | U5-6 (CLK) | Direct |
| SPI2_MOSI | U1-PB15 | — | U5-5 (DI) | Direct |
| SPI2_MISO | U1-PB14 | — | U5-2 (DO) | Direct |
| FLASH_WP | U1-PD0 | — | U5-3 (WP) | Write protect |
| FLASH_HOLD | U1-PD1 | — | U5-7 (HOLD) | Hold |

### 3.4 UART4 Bus (STM32L4 ↔ NRF52832)

| Net | Source Pin | Component | Dest Pin | Notes |
|-----|-----------|-----------|----------|-------|
| UART4_TX | U1-PA0 | — | U2-P0.02 (RX) | Cross-wired |
| UART4_RX | U1-PA1 | — | U2-P0.03 (TX) | Cross-wired |
| NRF_RST | U1-PC8 | — | U2-P0.21 | Reset NRF52 |
| NRF_IRQ | U2-P0.05 | — | U1-PC0 | IRQ from NRF52 |

### 3.5 UART1 Bus (STM32L4 ↔ EM4095)

| Net | Source Pin | Component | Dest Pin | Notes |
|-----|-----------|-----------|----------|-------|
| UART1_TX | U1-PA9 | R15 (1kΩ) | U4-8 (MOD) | Modulation data |
| UART1_RX | U1-PA10 | — | U4-12 (DEMOD_OUT) | Received data |
| EM4095_SHD | U1-PC1 | — | U4-7 (SHD) | Shutdown control |

### 3.6 I2C1 Bus (STM32L4 → SSD1306 + BQ25896)

| Net | Source Pin | Component | Dest Pin | Notes |
|-----|-----------|-----------|----------|-------|
| I2C1_SCL | U1-PB6 | R12 (4.7kΩ pull-up) | U8-SCL, U6-SCL | 400 kHz |
| I2C1_SDA | U1-PB7 | R13 (4.7kΩ pull-up) | U8-SDA, U6-SDA | 400 kHz |
| OLED_RST | U1-PE0 | — | U8-RES | SSD1306 reset |

### 3.7 USB-C Interface

| Net | Source Pin | Component | Dest Pin | Notes |
|-----|-----------|-----------|----------|-------|
| USB_DP | U1-PA12 | R9 (33Ω), R11 (1.5k pull-up) | J1-D+ | With ESD |
| USB_DM | U1-PA11 | R10 (33Ω) | J1-D- | With ESD |
| USB_VBUS | J1-VBUS | FB1 | U6-VBUS, U1-VBUS detect | 5V power |
| USB_CC1 | J1-CC1 | R24 (5.1kΩ) | GND | Type-C pull-down |
| USB_CC2 | J1-CC2 | R25 (5.1kΩ) | GND | Type-C pull-down |
| USB_ESD | D1 (USBLC6) | — | USB_DP, USB_DM | ESD protection |

### 3.8 RF Matching Networks

**PN5180 13.56 MHz Antenna Matching:**

| Net | Source Pin | Component | Dest Pin | Notes |
|-----|-----------|-----------|----------|-------|
| TX1 | U3-13 | L2 (22nH) → C17 (1nF) | ANT_NFC pad 1 | PI network |
| TX2 | U3-14 | L3 (22nH) → C18 (1nF) | ANT_NFC pad 2 | PI network |
| ANT1 | U3-11 | C19 (27pF) | GND | Antenna feed |
| ANT2 | U3-12 | C20 (27pF) | GND | Antenna feed |

**EM4095 125 kHz Antenna:**

| Net | Source Pin | Component | Dest Pin | Notes |
|-----|-----------|-----------|----------|-------|
| ANT_DRV1 | U4-3 | R15 (100Ω) | ANT_125 pad 1 | Antenna drive |
| ANT_DRV2 | U4-4 | R16 (100Ω) | ANT_125 pad 2 | Antenna drive |
| ANT_CDEC | U4-5/6 | C21/C22 (100nF each) | GND | Charge pump |

---

## 4. Decoupling Strategy

| IC | VDD Pin | Decoupling | Notes |
|----|---------|------------|-------|
| STM32L4 | Each VDD pair | 100nF + 1µF per pair | 8 pairs, 16 caps total |
| NRF52832 | VDD pins | 100nF per pin + 4.7µF bulk | DEC1, DEC2 per datasheet |
| PN5180 | VDD, DVDD, AVDD, TVDD | 100nF per pin + 10µF bulk | Separate per domain |
| EM4095 | VDD | 100nF + 10µF | Near pin |
| W25Q128 | VDD | 100nF | Single cap |
| BQ25896 | VBUS, BAT, SYS | 1µF per datasheet | Per ref design |
| TPS62840 | VIN, VOUT | 10µF in, 22µF out | Per datasheet |

---

## 5. Impedance-Controlled Pairs

| Pair | Impedance | Routing | Notes |
|------|-----------|---------|-------|
| USB_DP / USB_DM | 90 Ω differential | Controlled, <25mm | Type-C spec |
| SPI1 (SCK/MOSI/MISO) | 50 Ω single-ended | Length-matched, <5mm | 20 MHz |
| SPI2 (SCK/MOSI/MISO) | 50 Ω single-ended | Length-matched, <10mm | 40 MHz |
| UART4 (TX/RX) | — | Short, <20mm | 115200 bps |
| I2C (SCL/SDA) | — | <30mm, 4.7k pull-ups | 400 kHz |
| 13.56 MHz RF | 50 Ω single-ended | Microstrip, calculated width | Antenna feed |
| 125 kHz antenna | Low-Z | Wide trace, 0.5mm | High current drive |

---

## 6. Crystal Oscillator Circuits

### STM32L4 HSE (8 MHz)
- Y1: ECS-3225S-8MHz, 10 ppm, 9 pF load
- C11/C12: 22 pF (C_load = 2×C - C_stray → 2×22 - 5 ≈ 39 pF effective)
- Routing: <5mm to MCU, guard ring around crystal

### STM32L4 LSE (32.768 kHz)
- Y2: ABS07-32.768KHZ, 20 ppm, 12.5 pF load
- C13/C14: 15 pF load caps
- Routing: Adjacent to MCU, shielded from high-speed traces

### NRF52832 (32 MHz)
- Y3: ABS07-32.000KHZ-T, 10 ppm, 9 pF load
- C23/C24: 22 pF load caps
- Routing: Per Nordic reference design, <3mm to pins