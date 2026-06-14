# Sub-GHz IoT Gateway Implant — Phase 2: Component Selection & Schematics

## 1. Bill of Materials (BOM)

### 1.1 Main Components

| Ref | Part Number | Description | Manufacturer | Qty | Unit Price | Notes |
|-----|------------|-------------|--------------|-----|-----------|-------|
| U1 | CC1352P1F3RGZ | SimpleLink Multi-Protocol SoC, 1MB Flash, 128KB SRAM, Sub-GHz+2.4GHz, VQFN-48 | Texas Instruments | 1 | $7.20 | Main MCU+Radio |
| U2 | IS66WVS256ALL-104BLI | 256Mbit (32MB×8) xSPI SRAM, 104 MHz, BGA-24 | Integrated Silicon (ISSI) | 2 | $3.10 | 64MB total capture buffer |
| U3 | MX25LW1636AMI | 16MB SPI NOR Flash, Dual/Quad SPI, WSON-8 | Macronix | 1 | $1.40 | Config + stored captures |
| U4 | ATECC608A-MAHDA-S | Crypto Authentication Chip, I²C, SOIC-8 | Microchip | 1 | $0.68 | Secure boot + BLE auth |
| U5 | TLV62569DBVR | 2A Buck Converter, 5V→3.3V, SOT-23-5 | Texas Instruments | 1 | $0.45 | Main 3.3V rail |
| U6 | TLV62569DBVR | 2A Buck Converter, 3.3V→1.2V, SOT-23-5 | Texas Instruments | 1 | $0.45 | Core voltage rail |
| U7 | TPS63020DSJR | 2A Buck-Boost, 3.7V→3.3V, VQFN-10 | Texas Instruments | 1 | $1.85 | Battery → 3.3V |
| U8 | MCP73871T-2CCI/ML | LiPo Charger, USB Input, 4.2V, QFN-20 | Microchip | 1 | $0.95 | Battery management |
| U9 | TXB0108PWR | 8-bit Bidirectional Level Shifter, TSSOP-20 | Texas Instruments | 1 | $0.72 | SD card IO translation |
| U10 | TPD4E05U06DQAR | 4-ch ESD Protection, 0.5pF, USON-10 | Texas Instruments | 1 | $0.28 | USB ESD clamp |
| U11 | LP3985IM5-3.3 | 150mA LDO, 3.3V, SOT-23-5 | Texas Instruments | 1 | $0.30 | SDRAM VDD ref rail |
| D1 | WS2812B-2020 | RGB LED, Intelligent Control, 3.5×3.5mm | Worldsemi | 1 | $0.12 | Status indicator |
| J1 | USB4105-GF-A | USB-C Receptacle, 2.0, SMT, 16-pin | GCT | 1 | $0.35 | Data + charge |
| J2 | CON-SMA-EDGE-S | SMA Edge-Launch Connector, 50Ω | Amphenol RF | 1 | $0.85 | External antenna |
| J3 | DM3AT-SF-PEJ2 | MicroSD Card Connector, Push-Push, SMT | Hirose | 1 | $0.40 | Mass storage |
| J4 | 5055700691 | 6-pin FPC Connector, 0.5mm pitch | Molex | 1 | $0.30 | Expansion header |
| SW1, SW2 | KMR421NGULFS | Tactile Switch, 6×3.5mm, SMT | C&K | 2 | $0.08 | User buttons |
| Y1 | ABM8-24.000MHZ-20-B2X-T | 24 MHz Crystal, 20ppm, 3.2×2.5mm | Abracon | 1 | $0.30 | CC1352P main clock |
| Y2 | ABS07-32.768KHZ-T | 32.768 kHz Crystal, 20ppm, 3.2×1.5mm | Abracon | 1 | $0.22 | RTC crystal |
| ANT1 | Custom | 2.4 GHz PCB Trace Antenna (inverted-F) | — | 1 | — | BLE antenna (on PCB) |

**BOM Total (est.)**: ~$22.50 (1K volume)

### 1.2 Passive Components

| Ref | Value | Package | Part Number | Qty | Notes |
|-----|-------|---------|-------------|-----|-------|
| C1, C2 | 12 pF | 0402 | GRM1555C1H120J | 2 | Y1 load caps |
| C3, C4 | 12 pF | 0402 | GRM1555C1H120J | 2 | Y2 load caps |
| C5 | 10 µF | 0603 | GRM188R61A106M | 1 | VDD_MAIN bulk |
| C6 | 22 µF | 0603 | GRM188R60J226M | 1 | VDD_MAIN output |
| C7 | 10 µF | 0603 | GRM188R61A106M | 1 | VDD_CORE input |
| C8 | 22 µF | 0603 | GRM188R60J226M | 1 | VDD_CORE output |
| C9–C16 | 100 nF | 0402 | GRM155R71C104K | 8 | Decoupling per VDD pin |
| C17, C18 | 1 µF | 0402 | GRM155R61E105K | 2 | CC1352P internal reg |
| C19 | 100 nF | 0402 | GRM155R71C104K | 1 | ATECC608A decoupling |
| C20 | 10 µF | 0805 | GRM21BR61A106K | 1 | USB VBUS decoupling |
| C21 | 4.7 µF | 0402 | GRM155R61E475K | 1 | LDO output |
| C22–C25 | 100 nF | 0402 | GRM155R71C104K | 4 | SDRAM decoupling |
| C26 | 1 nF | 0402 | GRM155R71H102K | 1 | PA output coupling |
| C27 | 10 pF | 0402 | GRM1555C1H100D | 1 | Sub-GHz RF match |
| C28 | 0.5 pF | 0402 | GRM1555C1H0R5C | 1 | Sub-GHz RF match |
| C29, C30 | 100 nF | 0402 | GRM155R71C104K | 2 | SD card decoupling |
| L1 | 2.2 µH | 0805 | LQM21PN2R2M | 1 | TLV62569 (5V→3.3V) inductor |
| L2 | 1.0 µH | 0805 | LQM21PN1R0M | 1 | TLV62569 (3.3V→1.2V) inductor |
| L3 | 2.2 µH | 0805 | LQM21PN2R2M | 1 | TPS63020 inductor |
| L4 | 10 nH | 0402 | LQW15AN10N | 1 | Sub-GHz RF match |
| L5 | 4.7 nH | 0402 | LQW15AN4N7 | 1 | Sub-GHz RF match |
| R1, R2 | 10 kΩ | 0402 | RC0402JR-10KL | 2 | CC1352P pull-ups (RESET, SWD) |
| R3 | 100 Ω | 0402 | RC0402JR-100L | 1 | WS2812B data line series |
| R4 | 4.7 kΩ | 0402 | RC0402JR-4K7L | 2 | I²C pull-ups |
| R5 | 330 Ω | 0402 | RC0402JR-330L | 1 | USB D+ pull-up (not needed for FS device) |
| R6, R7 | 1.5 kΩ | 0402 | RC0402JR-1K5L | 2 | CC1352P JTAG pull-ups |
| R8 | 10 kΩ | 0402 | RC0402JR-10KL | 1 | ATECC608A SDA pull-up (if not shared) |
| FB1 | 600 Ω @ 100 MHz | 0402 | BLM15AG601SN | 1 | VDD_PA ferrite bead |

## 2. CC1352P Pinout & Net Assignments

### 2.1 Complete Pin Mapping (VQFN-48)

| Pin | Name | Net | Function |
|-----|------|-----|----------|
| 1 | DIO_0 | SDRAM_DQ0 | SDRAM data bit 0 |
| 2 | DIO_1 | SDRAM_DQ1 | SDRAM data bit 1 |
| 3 | DIO_2 | SDRAM_DQ2 | SDRAM data bit 2 |
| 4 | DIO_3 | SDRAM_DQ3 | SDRAM data bit 3 |
| 5 | DIO_4 | SDRAM_DQ4 | SDRAM data bit 4 |
| 6 | DIO_5 | SDRAM_DQ5 | SDRAM data bit 5 |
| 7 | DIO_6 | SDRAM_DQ6 | SDRAM data bit 6 |
| 8 | DIO_7 | SDRAM_DQ7 | SDRAM data bit 7 |
| 9 | DIO_8 | FLASH_SPI_CLK | External SPI Flash clock (SSI1) |
| 10 | DIO_9 | FLASH_SPI_MOSI | External SPI Flash MOSI (SSI1) |
| 11 | DIO_10 | FLASH_SPI_MISO | External SPI Flash MISO (SSI1) |
| 12 | DIO_11 | FLASH_SPI_CS | External SPI Flash chip select |
| 13 | DIO_12 | SD_SPI_CS | MicroSD chip select |
| 14 | DIO_13 | SD_SPI_CLK | MicroSD SPI clock (SSI3) |
| 14 | DIO_14 | SD_SPI_MOSI | MicroSD SPI MOSI |
| 15 | DIO_15 | SD_SPI_MISO | MicroSD SPI MISO |
| 16 | DIO_16 | WS2812B_DATA | RGB LED data out |
| 17 | DIO_17 | BTN_MODE | Mode button input (active low) |
| 18 | DIO_18 | BTN_ACTION | Action button input (active low) |
| 19 | DIO_19 | I2C_SDA | ATECC608A I²C data |
| 20 | DIO_20 | I2C_SCL | ATECC608A I²C clock |
| 21 | DIO_21 | SDRAM_CLK | SDRAM clock (SSI0) |
| 22 | DIO_22 | SDRAM_CS | SDRAM chip select |
| 23 | DIO_23 | SDRAM_DQS | SDRAM DQS strobe |
| 24 | DIO_24 | UART0_TX | Debug console TX |
| 25 | DIO_25 | UART0_RX | Debug console RX |
| 26 | DIO_26 | UART1_TX | Expansion UART TX |
| 27 | DIO_27 | UART1_RX | Expansion UART RX |
| 28 | DIO_28 | USB_DP | USB Data+ |
| 29 | DIO_29 | USB_DM | USB Data- |
| 30 | DIO_30 | SDRAM_DQ8 | SDRAM data bit 8 (U2) |
| 31 | DIO_31 | SDRAM_DQ9 | SDRAM data bit 9 (U2) |
| 32 | VDDR | VDD_RF (1.7V) | Internal RF regulator output |
| 33 | DCPL | VDDR_CAP | Internal DC-DC decoupling |
| 34 | SUB1 | SUB_GHZ_RF_IN | Sub-GHz LNA input from antenna match |
| 35 | PA1 | SUB_GHZ_PA_OUT | Sub-GHz PA output to antenna match |
| 36 | RF2 | BLE_2.4_RF | 2.4 GHz BLE RF path |
| 37 | VDDS | VDD_PA (3.3V) | PA supply |
| 38 | VDDS2 | VDD_PA (3.3V) | PA supply |
| 39 | DIO_0 (alt) | SDRAM_DQ10 | SDRAM data bit 10 (U2) |
| 40 | DIO_1 (alt) | SDRAM_DQ11 | SDRAM data bit 11 (U2) |
| 41 | DIO_2 (alt) | SDRAM_DQ12 | SDRAM data bit 12 (U2) |
| 42 | DIO_3 (alt) | SDRAM_DQ13 | SDRAM data bit 13 (U2) |
| 43 | DIO_4 (alt) | SDRAM_DQ14 | SDRAM data bit 14 (U2) |
| 44 | DIO_5 (alt) | SDRAM_DQ15 | SDRAM data bit 15 (U2) |
| 45 | JTAG_TCK | SWD_CLK | Debug SWD clock |
| 46 | JTAG_TMS | SWD_IO | Debug SWD data |
| 47 | JTAG_TDI | SWD_TDI | Debug SWD TDI |
| 48 | JTAG_TDO | SWD_TDO | Debug SWD TDO |

**Note**: CC1352P VQFN-48 has configurable DIO mapping. The above shows the primary assignment. Pins 30-44 are multiplexed for the second SDRAM chip data bus.

### 2.2 Power Pin Connections

| Pin | Net | Decoupling |
|-----|-----|-----------|
| VDDS (3 pins) | VDD_MAIN (3.3V) | 100 nF each, close to pin |
| VDDS2 (2 pins) | VDD_MAIN (3.3V) | 100 nF each |
| VDDR | VDD_RF (1.7V, internal LDO) | 1 µF + 100 nF |
| VDD | VDD_MAIN (3.3V) | 100 nF |
| VSS (4 pins) | GND | Via to ground plane |

## 3. Netlist (Source → Component → Destination)

### 3.1 Power Distribution

| Net Name | Source Pin | Component | Dest Pin |
|----------|-----------|-----------|----------|
| VBUS_5V | J1 Pin 4 (VBUS) | C20 (10µF) | GND |
| VBUS_5V | J1 Pin 4 (VBUS) | U5 IN (TLV62569) | U5 Pin 4 |
| VBUS_5V | J1 Pin 4 (VBUS) | U8 IN (MCP73871) | U8 Pin 1 |
| VDD_MAIN | U5 OUT (TLV62569) | C5, C6 | U5 Pin 1 (FB) |
| VDD_MAIN | U7 OUT (TPS63020) | C6 (22µF) | GND |
| VDD_MAIN | L1 (2.2µH) other side | — | U5 Pin 2 (SW) |
| VDD_CORE | U6 OUT (TLV62569) | C8 (22µF) | GND |
| VDD_CORE | L2 (1.0µH) other side | — | U6 Pin 2 (SW) |
| VDD_RF | U1 VDDR (CC1352P internal LDO) | C17 (1µF) | GND |
| VDD_PA | VDD_MAIN via FB1 | C26 (1nF) | GND |
| VDD_SDRAM | VDD_MAIN | C22–C25 (100nF each) | GND |
| BAT+ | U8 BAT (MCP73871) | — | Battery+ terminal |
| GND | All GND pins | — | Ground plane |

### 3.2 SDRAM Interface (U2, U3 = IS66WVS256)

| Net Name | U1 Pin (CC1352P) | Net | U2/U3 Pin |
|----------|-----------------|-----|-----------|
| SDRAM_CLK | DIO_21 (SSI0_CLK) | SDRAM_CLK | U2 CLK, U3 CLK |
| SDRAM_CS | DIO_22 | SDRAM_CS# | U2 CS#, U3 CS# |
| SDRAM_DQS | DIO_23 | SDRAM_DQS | U2 DQS, U3 DQS |
| SDRAM_DQ0 | DIO_0 | SDRAM_DQ0 | U2 DQ0 |
| SDRAM_DQ1 | DIO_1 | SDRAM_DQ1 | U2 DQ1 |
| ... | ... | ... | ... |
| SDRAM_DQ7 | DIO_7 | SDRAM_DQ7 | U2 DQ7 |
| SDRAM_DQ8 | DIO_30 | SDRAM_DQ8 | U3 DQ0 |
| ... | ... | ... | ... |
| SDRAM_DQ15 | DIO_5(alt) | SDRAM_DQ15 | U3 DQ7 |

### 3.3 SPI Flash Interface (U4 = MX25LW1636)

| Net Name | U1 Pin | Net | U4 Pin |
|----------|--------|-----|--------|
| FLASH_CLK | DIO_8 (SSI1_CLK) | FLASH_CLK | U4 Pin 6 (SCK) |
| FLASH_MOSI | DIO_9 (SSI1_TX) | FLASH_MOSI | U4 Pin 5 (SI) |
| FLASH_MISO | DIO_10 (SSI1_RX) | FLASH_MISO | U4 Pin 2 (SO) |
| FLASH_CS | DIO_11 (GPIO) | FLASH_CS# | U4 Pin 1 (CS#) |
| VDD | VDD_MAIN | — | U4 Pin 8 (VCC) |
| GND | GND | — | U4 Pin 4 (GND) |

### 3.4 ATECC608A Interface (U4)

| Net Name | U1 Pin | Net | U4 Pin |
|----------|--------|-----|--------|
| I2C_SDA | DIO_19 | I2C_SDA | U4 Pin 5 (SDA) |
| I2C_SCL | DIO_20 | I2C_SCL | U4 Pin 6 (SCL) |
| VDD | VDD_MAIN | — | U4 Pin 8 (VCC) |
| GND | GND | — | U4 Pin 2,3,4 (GND) |

**Note**: I²C pull-ups R4a (4.7kΩ to VDD_MAIN on SDA) and R4b (4.7kΩ to VDD_MAIN on SCL).

### 3.5 MicroSD Interface (J3 via TXB0108)

| Net Name | U1 Pin | U9 (TXB0108) | J3 Pin |
|----------|--------|---------------|--------|
| SD_CLK | DIO_13 (SSI3_CLK) | U9 A3→B3 | J3 Pin 5 (CLK) |
| SD_MOSI | DIO_14 (SSI3_TX) | U9 A4→B4 | J3 Pin 3 (CMD) |
| SD_MISO | DIO_15 (SSI3_RX) | U9 A5←B5 | J3 Pin 7 (DAT0) |
| SD_CS | DIO_12 (GPIO) | U9 A6→B6 | J3 Pin 2 (CD/DAT3) |
| SD_CD | DIO_28 (GPIO) | Direct | J3 Pin 10 (Card Detect) |
| SD_VDD | VDD_MAIN | — | J3 Pin 4 (VDD) |

**TXB0108 Power**:
- VA = VDD_MAIN (3.3V) — A-side
- VB = VDD_SD (3.3V from VDD_MAIN) — B-side
- OE = VDD_MAIN (always enabled)

### 3.6 USB Interface (J1)

| Net Name | J1 Pin | U10 (TPD4E05U06) | U1 Pin |
|----------|--------|-------------------|--------|
| USB_DP | J1 Pin A6/B6 | U10 Ch1 | U1 DIO_28 |
| USB_DM | J1 Pin A7/B7 | U10 Ch2 | U1 DIO_29 |
| VBUS_5V | J1 Pin A4/B4/A9/B9 | — | U5 Pin 4 (via filter) |
| GND | J1 Pin A1/B1/A12/B12 | U10 GND | GND |

### 3.7 RF Paths

**Sub-GHz (868/915 MHz):**
| Net Name | U1 Pin | Component | Dest |
|----------|--------|-----------|------|
| SUB_PA_OUT | PA1 (Pin 35) | L4 (10nH) → C27 (10pF) → C28 (0.5pF) | Sub-GHz match network |
| SUB_RF_IN | SUB1 (Pin 34) | Direct from match | Sub-GHz LNA input |
| SUB_GHZ_ANT | Match network output | J2 (SMA) | External antenna |

**2.4 GHz BLE:**
| Net Name | U1 Pin | Component | Dest |
|----------|--------|-----------|------|
| BLE_RF | RF2 (Pin 36) | Matching network (on PCB) | PCB trace antenna (inverted-F) |

### 3.8 Status LED (WS2812B)

| Net Name | U1 Pin | Component | Dest |
|----------|--------|-----------|------|
| WS2812_DATA | DIO_16 | R3 (330Ω) | D1 DIN |
| VDD | VDD_MAIN | C29 (100nF) | D1 VDD |
| GND | GND | — | D1 GND |

### 3.9 User Buttons

| Net Name | U1 Pin | Component | Dest |
|----------|--------|-----------|------|
| BTN_MODE | DIO_17 | SW1 (active low to GND) | R1a (10kΩ pull-up to VDD) |
| BTN_ACTION | DIO_18 | SW2 (active low to GND) | R1b (10kΩ pull-up to VDD) |

### 3.10 Debug Interface (SWD)

| Net Name | U1 Pin | Connector | Note |
|----------|--------|-----------|------|
| SWD_CLK | Pin 45 (TCK) | 0.1" header | R6 (1.5kΩ pull-up) |
| SWD_IO | Pin 46 (TMS) | 0.1" header | R7 (1.5kΩ pull-up) |
| SWD_TDI | Pin 47 (TDI) | 0.1" header | — |
| SWD_TDO | Pin 48 (TDO) | 0.1" header | — |
| RESET | DIO_30 (RESET) | 0.1" header | R2 (10kΩ pull-up to VDD) |

## 4. Decoupling Strategy

### 4.1 CC1352P Power Domains

Each VDD pin on the CC1352P requires a 100 nF capacitor placed within 2mm of the pin, via directly to ground plane. Additionally:
- **VDDR domain**: 1 µF (C17) + 100 nF (C9) at the VDDR pin
- **VDDS/VDDS2 (PA supply)**: 10 µF bulk (C5) + 100 nF per pin (C10–C13), FB1 (600Ω ferrite) isolating PA supply from digital
- **VDD (digital)**: 100 nF (C14) at pin + 10 µF bulk (C5) within 10mm

### 4.2 External Memory
- **Each SDRAM (U2, U3)**: 100 nF per VDD pin (C22–C25), placed within 2mm
- **SPI Flash (U4)**: 100 nF (C15) at VCC pin
- **ATECC608A**: 100 nF (C19) at VCC pin

### 4.3 Power Supply Output
- **TLV62569 (3.3V)**: 22 µF (C6) + 10 µF (C5) at output, within 5mm of inductor
- **TLV62569 (1.2V)**: 22 µF (C8) at output
- **TPS63020**: 22 µF (C6 shared) at output
- **LP3985 LDO**: 4.7 µF (C21) at output

## 5. Impedance-Controlled Pairs

| Pair | Impedance | Trace Width | Spacing | Layer |
|------|-----------|-------------|---------|-------|
| USB_DP / USB_DM | 90 Ω diff | 0.18 mm | 0.12 mm | Top |
| SDRAM_DQ[0:7] | 50 Ω single | 0.15 mm | — | Top |
| SDRAM_CLK | 50 Ω single | 0.15 mm | — | Top |
| Sub-GHz RF (PA to SMA) | 50 Ω single | 0.30 mm (coplanar) | — | Top |
| BLE RF (to antenna) | 50 Ω single | 0.25 mm | — | Top |
| SPI Flash | 50 Ω single | 0.15 mm | — | Top |

### Sub-GHz RF Matching Network

For 868/915 MHz operation, the CC1352P PA output requires an impedance match to 50Ω:

```
CC1352P PA ── L4(10nH) ──┬── C27(10pF) ── GND
                          │
                          ├── C28(0.5pF) ── GND
                          │
                          └── 50Ω microstrip ── J2 (SMA) ── Antenna
```

This pi-network matches the CC1352P PA output impedance (~15+j30Ω at 915MHz) to 50Ω. Values are starting points from TI reference design and will be tuned during board Bring-Up with a VNA.

### BLE 2.4 GHz Matching Network

```
CC1352P RF2 ── C_match(1.5pF) ── L_match(2.7nH) ──┬── GND
                                                     └── Inverted-F PCB Antenna
```

The inverted-F antenna trace is λ/4 at 2.45 GHz (≈ 28mm on FR4, εr=4.4), with a ground plane cutout beneath.

## 6. Schematic Sheet Organization

| Sheet | Title | Contents |
|-------|-------|----------|
| 1 | Power | USB-C, LiPo charger, buck converters, buck-boost, LDO, power rails |
| 2 | SoC | CC1352P, crystals, decoupling, SWD header |
| 3 | Memory | SDRAM (×2), SPI Flash, MicroSD + level shifter |
| 4 | RF | Sub-GHz match network, SMA connector, BLE antenna, ESD |
| 5 | Security & IO | ATECC608A, WS2812B, buttons, expansion header |
| 6 | USB | USB-C connector, ESD protection, pull-downs (5.1kΩ on CC pins) |

### USB-C PD Resistors (required for USB 2.0 device)
| Net | J1 Pin | Resistor | Value | Purpose |
|-----|--------|----------|-------|---------|
| CC1 | A5 | R_CC1 | 5.1 kΩ | USB-C device pull-down |
| CC2 | B5 | R_CC2 | 5.1 kΩ | USB-C device pull-down |

These 5.1 kΩ pull-downs on both CC pins signal to the host that this is a USB 2.0 device (not a PD source), ensuring 5V VBUS is provided.