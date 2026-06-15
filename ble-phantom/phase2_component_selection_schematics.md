# BLE Phantom — Phase 2: Component Selection & Schematics

## 1. Component Selection Rationale

### 1.1 Main MCU — STM32F401CCU6

The STM32F401CCU6 is the central coordinator, handling USB communication, SPI master for both radios, DMA-based packet relay, and application logic. Selected because:

- **Cortex-M4F** with FPU — fast enough for real-time BLE PDU parsing and modification
- **256 KB Flash / 64 KB SRAM** — sufficient for firmware, dual ring buffers, and relay logic
- **UFQFPN-48** package — compact 7×7 mm, ideal for dongle form factor
- **USB 2.0 Full-Speed** device — native USB with dedicated SRAM (1.5 KB)
- **Dual SPI** (SPI1 + SPI2) with DMA — simultaneous radio communication
- **Price**: ~$2.80 at qty 100

| Parameter | Value |
|-----------|-------|
| Core | ARM Cortex-M4F |
| Max Clock | 84 MHz |
| Flash | 256 KB |
| SRAM | 64 KB + 16 KB CCM |
| Package | UFQFPN-48 (7×7 mm) |
| USB | 2.0 Full-Speed Device |
| SPI | 3× (SPI1, SPI2, SPI3) |
| GPIO | 36 |
| ADC | 1× 12-bit |
| Price | $2.80 |

### 1.2 BLE Radio — nRF52832-QFAA (×2)

Two identical nRF52832 chips serve as the BLE transceivers. Selected because:

- **BLE 5.x** with 2 Mbps coded PHY support
- **Programmable radio** — direct register access for channel selection, TX power, and raw PDU manipulation
- **64 MHz Cortex-M4** onboard — can run autonomous radio firmware while responding to SPI commands
- **64 KB RAM / 512 KB Flash** — ample for radio stack + capture buffer
- **QFN-48 (6×6 mm)** — compact, routable
- **Price**: ~$3.50 each at qty 100

| Parameter | Value |
|-----------|-------|
| Core | ARM Cortex-M4F |
| Max Clock | 64 MHz |
| Flash | 512 KB |
| RAM | 64 KB |
| Package | QFN-48 (6×6 mm) |
| Radio | BLE 5.x, 2 Mbps, coded PHY |
| TX Power | +4 dBm max |
| RX Sensitivity | −96 dBm (1 Mbps) |
| SPI Slave | 3× (SPI0, SPI1, SPI2) |
| Price | $3.50 |

### 1.3 Battery Charger — MCP73831T-2ACI/OT

Single-cell LiPo charger with USB VBUS input.

| Parameter | Value |
|-----------|-------|
| Input | 4.2–6.0 V (VBUS) |
| Charge Current | 500 mA (prog resistor) |
| Charge Voltage | 4.2 V |
| Package | SOT-23-5 |
| Price | $0.45 |

### 1.4 LDO Regulator — TLV1117LV33DCYR

3.3 V LDO for system power from VBUS or battery.

| Parameter | Value |
|-----------|-------|
| Input | 2.7–5.5 V |
| Output | 3.3 V fixed |
| Max Current | 800 mA |
| Dropout | 500 mV @ 800 mA |
| Package | SOT-223 |
| Price | $0.50 |

### 1.5 ESD Protection — PRTR5V0U2X,215

USB ESD protection for D+/D− lines.

| Parameter | Value |
|-----------|-------|
| Channels | 2 (D+, D−) |
| Clamping | 6 V |
| Capacitance | 1 pF |
| Package | SOT-143 |
| Price | $0.20 |

### 1.6 Crystal Oscillators

| Ref | Part | Frequency | Tolerance | Package | Price |
|-----|------|-----------|-----------|---------|-------|
| Y1 | ABS07-32.768KHZ-T | 32.768 kHz | ±20 ppm | 3.2×1.5 mm | $0.25 |
| Y2 | ECS-250-8-36-CKM-TR | 8 MHz | ±30 ppm | HC-49/S | $0.40 |

### 1.7 Antenna — 2.4 GHz Chip Antenna (×2)

| Ref | Part | Type | Gain | Impedance | Package | Price |
|-----|------|------|------|-----------|---------|-------|
| ANT1, ANT2 | Johanson 2450AT18A100E | Chip | +0.5 dBi | 50 Ω | 0805 | $0.30 |

## 2. Bill of Materials (BOM)

| # | Ref | Part Number | Description | Qty | Unit Price | Ext Price |
|---|-----|-------------|-------------|-----|-----------|-----------|
| 1 | U1 | STM32F401CCU6 | Main MCU, Cortex-M4F | 1 | $2.80 | $2.80 |
| 2 | U2 | nRF52832-QFAA | BLE Radio A | 1 | $3.50 | $3.50 |
| 3 | U3 | nRF52832-QFAA | BLE Radio B | 1 | $3.50 | $3.50 |
| 4 | U4 | MCP73831T-2ACI/OT | LiPo Charger | 1 | $0.45 | $0.45 |
| 5 | U5 | TLV1117LV33DCYR | 3.3V LDO | 1 | $0.50 | $0.50 |
| 6 | U6 | PRTR5V0U2X,215 | USB ESD Protection | 1 | $0.20 | $0.20 |
| 7 | Y1 | ABS07-32.768KHZ-T | 32.768 kHz crystal | 1 | $0.25 | $0.25 |
| 8 | Y2 | ECS-250-8-36-CKM-TR | 8 MHz crystal | 1 | $0.40 | $0.40 |
| 9 | Y3 | ABS07-32.768KHZ-T | 32.768 kHz crystal (Radio B) | 1 | $0.25 | $0.25 |
| 10 | Y4 | ABS07-32.768KHZ-T | 32.768 kHz crystal (Radio A) | 1 | $0.25 | $0.25 |
| 11 | ANT1 | 2450AT18A100E | 2.4 GHz chip antenna | 1 | $0.30 | $0.30 |
| 12 | ANT2 | 2450AT18A100E | 2.4 GHz chip antenna | 1 | $0.30 | $0.30 |
| 13 | J1 | USB-A Plug Right Angle | USB-A Male connector | 1 | $0.60 | $0.60 |
| 14 | J2 | Molex 5034800700 | FPC 7-pin battery conn | 1 | $0.35 | $0.35 |
| 15 | SW1 | SKQGAFE010 | Tactile switch, 6×3.5mm | 1 | $0.10 | $0.10 |
| 16 | D1-D4 | LTST-C191KRKT | Red LED 0603 | 4 | $0.03 | $0.12 |
| 17 | D5 | RB521S30T1G | Schottky diode 0603 | 1 | $0.05 | $0.05 |
| 18 | L1, L2 | LQG15HS10NJ02D | 10 nH 0402 inductor | 2 | $0.03 | $0.06 |
| 19 | L3 | LQG15HS3N3B02D | 3.3 nH 0402 inductor | 1 | $0.03 | $0.03 |
| 20 | C1 | 4.7 µF 0603 6.3V | VBUS bulk cap | 1 | $0.02 | $0.02 |
| 21 | C2 | 1 µF 0402 6.3V | MCP73831 VBAT cap | 1 | $0.01 | $0.01 |
| 22 | C3 | 4.7 µF 0603 6.3V | LDO output cap | 1 | $0.02 | $0.02 |
| 23 | C4-C9 | 100 nF 0402 6.3V | Decoupling caps (×6) | 6 | $0.005 | $0.03 |
| 24 | C10-C15 | 100 nF 0402 6.3V | nRF decoupling caps (×6) | 6 | $0.005 | $0.03 |
| 25 | C16-C19 | 1 µF 0402 6.3V | nRF VDD caps (×4) | 4 | $0.01 | $0.04 |
| 26 | C20 | 12 pF 0402 NP0 | Y1 load cap | 1 | $0.005 | $0.005 |
| 27 | C21 | 12 pF 0402 NP0 | Y2 load cap | 1 | $0.005 | $0.005 |
| 28 | C22 | 12 pF 0402 NP0 | Y3 load cap (Radio A) | 1 | $0.005 | $0.005 |
| 29 | C23 | 12 pF 0402 NP0 | Y3 load cap (Radio A) | 1 | $0.005 | $0.005 |
| 30 | C24 | 12 pF 0402 NP0 | Y4 load cap (Radio B) | 1 | $0.005 | $0.005 |
| 31 | C25 | 12 pF 0402 NP0 | Y4 load cap (Radio B) | 1 | $0.005 | $0.005 |
| 32 | C26 | 0.5 pF 0402 NP0 | Antenna match | 1 | $0.005 | $0.005 |
| 33 | C27 | 0.5 pF 0402 NP0 | Antenna match | 1 | $0.005 | $0.005 |
| 34 | R1 | 2.0 kΩ 0402 | MCP73831 PROG (500 mA) | 1 | $0.005 | $0.005 |
| 35 | R2 | 10 kΩ 0402 | BOOT0 pull-down | 1 | $0.005 | $0.005 |
| 36 | R3-R6 | 330 Ω 0402 | LED resistors (×4) | 4 | $0.005 | $0.02 |
| 37 | R7, R8 | 22 Ω 0402 | USB D+/D− series | 2 | $0.005 | $0.01 |
| 38 | R9 | 1.5 kΩ 0402 | nRF SWDIO pull-up | 2 | $0.005 | $0.01 |
| 39 | R10, R11 | 4.7 kΩ 0402 | I2C pull-ups | 2 | $0.005 | $0.01 |
| 40 | RN1 | 4×10 kΩ 0402 array | nRF SPI pull-ups | 1 | $0.03 | $0.03 |

**Total BOM: ~$13.95** (well under $100 target)

## 3. Pinout Tables

### 3.1 STM32F401CCU6 Pin Assignment

| Pin | Name | Function | Connected To | Notes |
|-----|------|----------|-------------|-------|
| 1 | VBAT | Power | 3.3 V | |
| 2 | PC13 | GPIO | SW1 (user button) | Active low, internal pull-up |
| 3 | PC14 | GPIO | D3 (status LED) | |
| 4 | PC15 | GPIO | D4 (power LED) | |
| 5 | PH0 | OSC_IN | Y2 (8 MHz) | HSE oscillator |
| 6 | PH1 | OSC_OUT | Y2 (8 MHz) | HSE oscillator |
| 7 | NRST | Reset | SWD header + RC | 10 kΩ pull-up + 100 nF cap |
| 8 | VSSA | Analog GND | GND | |
| 9 | VDDA | Analog Power | 3.3 V (filtered) | LC filter from VDD |
| 10 | PA0 | GPIO / EXTI0 | nRF52832 Radio A IRQ | Active low |
| 11 | PA1 | GPIO / EXTI1 | nRF52832 Radio B IRQ | Active low |
| 12 | PA2 | USART2_TX | Debug UART | |
| 13 | PA3 | USART2_RX | Debug UART | |
| 14 | PA4 | GPIO | nRF Radio A RESET | Active low |
| 15 | PA5 | SPI1_SCK | nRF Radio A SPI SCK | |
| 16 | PA6 | SPI1_MISO | nRF Radio A SPI MISO | |
| 17 | PA7 | SPI1_MOSI | nRF Radio A SPI MOSI | |
| 18 | PB0 | GPIO | D1 (Radio A LED) | |
| 19 | PB1 | GPIO | D2 (Radio B LED) | |
| 20 | PB2 | GPIO | nRF Radio B RESET | Active low |
| 21 | PB10 | SPI2_SCK | nRF Radio B SPI SCK | |
| 22 | PB11 | SPI2_MISO | nRF Radio B SPI MISO | |
| 23 | PB12 | SPI2_NSS | nRF Radio B SPI CS | |
| 24 | VDD | Power | 3.3 V | |
| 25 | VSS | Ground | GND | |
| 26 | PB13 | — | NC | |
| 27 | PB14 | — | NC | |
| 28 | PB15 | SPI2_MOSI | nRF Radio B SPI MOSI | |
| 29 | PA8 | GPIO | nRF Radio A SPI CS | |
| 30 | PA9 | GPIO | USB VBUS detect | |
| 31 | PA10 | GPIO | nRF Radio B CSN (debug) | |
| 32 | PA11 | USB_DM | USB D− via R7 | |
| 33 | PA12 | USB_DP | USB D+ via R8 | |
| 34 | PA13 | SWDIO | SWD debug header | |
| 35 | PA14 | SWCLK | SWD debug header | |
| 36 | PA15 | SPI1_NSS | nRF Radio A SPI CS (alt) | |
| 37 | PB3 | GPIO | SPI1_NSS (manual) | |
| 38 | PB4 | GPIO | Battery gauge INT | |
| 39 | PB5 | I2C1_SDA | Battery gauge SDA | |
| 40 | PB6 | I2C1_SCL | Battery gauge SCL | |
| 41 | PB7 | GPIO | BOOT1 | Tie to GND |
| 42 | BOOT0 | Boot config | R2 (10 kΩ to GND) | |
| 43 | PB8 | GPIO | USB LED | |
| 44 | PB9 | GPIO | NC | |
| 45 | VSS | Ground | GND | |
| 46 | VDD | Power | 3.3 V | |
| 47 | VDD | Power | 3.3 V | |
| 48 | VSS | Ground | GND | |

### 3.2 nRF52832-QFAA Pin Assignment (Radio A and Radio B identical)

| Pin | Name | Function | Connected To | Notes |
|-----|------|----------|-------------|-------|
| 1 | DEC1 | Decoupling | 1 µF to GND | |
| 2 | P0.00 | XLC1 | Y3/Y4 (32.768 kHz) | 32k crystal |
| 3 | P0.01 | XLC2 | Y3/Y4 (32.768 kHz) | 32k crystal |
| 4 | P0.02 | AIN0 | — | NC |
| 5 | P0.03 | AIN1 | — | NC |
| 6 | P0.04 | AIN2 | — | NC |
| 7 | VDD | Power | 3.3 V | |
| 8 | VDD | Power | 3.3 V | |
| 9 | P0.05 | GPIO | SPI CS (from STM32) | Active low |
| 10 | P0.06 | GPIO | SPI MISO (to STM32) | |
| 11 | P0.07 | GPIO | SPI MOSI (from STM32) | |
| 12 | P0.08 | GPIO | SPI SCK (from STM32) | |
| 13 | P0.09 | GPIO | IRQ (to STM32 EXTI) | Active low |
| 14 | P0.10 | GPIO | RESET (from STM32) | Active low |
| 15 | P0.11 | GPIO | — | NC |
| 16 | P0.12 | GPIO | — | NC |
| 17 | P0.13 | — | — | NC |
| 18 | P0.14 | — | — | NC |
| 19 | P0.15 | — | — | NC |
| 20 | P0.16 | — | — | NC |
| 21 | P0.17 | — | — | NC |
| 22 | P0.18 | — | — | NC |
| 23 | P0.19 | — | — | NC |
| 24 | P0.20 | — | — | NC |
| 25 | P0.21 | — | — | NC |
| 26 | P0.22 | — | — | NC |
| 27 | P0.23 | — | — | NC |
| 28 | P0.24 | — | — | NC |
| 29 | SWDCLK | Debug | SWD header | |
| 30 | SWDIO | Debug | SWD header | |
| 31 | DEC2 | Decoupling | NC (internal) | |
| 32 | VDD | Power | 3.3 V | |
| 33 | DEC3 | Decoupling | 100 nF to GND | |
| 34 | DCC | Internal | 100 nF to GND | |
| 35 | ANT1 | RF | Matching network → ANT1/2 | |
| 36 | ANT2 | RF | GND (single-ended) | |
| 37 | VSS | Ground | GND | |
| 38 | VSS | Ground | GND | |
| 39 | XC1 | — | — | NC |
| 40 | XC2 | — | — | NC |
| 41 | VDD | Power | 3.3 V | |
| 42 | DEC4 | Decoupling | 100 nF to GND | |
| 43 | DEC5 | Decoupling | 1 µF to GND | |
| 44 | VDD | Power | 3.3 V | |
| 45 | VSS | Ground | GND | |
| 46 | VSS | Ground | GND | |
| 47 | P0.25 | — | — | NC |
| 48 | P0.26 | — | — | NC |

## 4. Netlists

### 4.1 Power Nets

| Net Name | Source Pin | Via Component | Dest Pin |
|----------|-----------|---------------|----------|
| VBUS | J1.1 (VBUS) | — | U4.4 (MCP73831 VDD) |
| VBUS | J1.1 (VBUS) | — | U5.3 (LDO VIN) |
| VBUS | J1.1 (VBUS) | — | STM32 PA9 (VBUS detect) |
| VBATT | J2.1 (BAT+) | D5 (Schottky) | U4.3 (MCP73831 VBAT) |
| VBATT | J2.1 (BAT+) | — | U5.1 (LDO EN) |
| 3V3 | U5.2 (LDO VOUT) | — | U1.VDD (STM32) |
| 3V3 | U5.2 (LDO VOUT) | — | U2.VDD (nRF A) |
| 3V3 | U5.2 (LDO VOUT) | — | U3.VDD (nRF B) |
| 3V3 | U5.2 (LDO VOUT) | — | R3-R6 (LED anodes) |
| GND | J1.4 (GND) | — | All GND pins |

### 4.2 SPI0 — STM32 ↔ nRF Radio A

| Net Name | Source Pin | Via Component | Dest Pin |
|----------|-----------|---------------|----------|
| SPIA_SCK | STM32.PA5 (SPI1_SCK) | — | nRF_A.P0.08 (SCK) |
| SPIA_MISO | nRF_A.P0.06 (MISO) | — | STM32.PA6 (SPI1_MISO) |
| SPIA_MOSI | STM32.PA7 (SPI1_MOSI) | — | nRF_A.P0.07 (MOSI) |
| SPIA_CS | STM32.PA8 (GPIO) | — | nRF_A.P0.05 (CS) |
| SPIA_IRQ | nRF_A.P0.09 (IRQ) | — | STM32.PA0 (EXTI0) |
| SPIA_RST | STM32.PA4 (GPIO) | — | nRF_A.P0.10 (RESET) |

### 4.3 SPI1 — STM32 ↔ nRF Radio B

| Net Name | Source Pin | Via Component | Dest Pin |
|----------|-----------|---------------|----------|
| SPIB_SCK | STM32.PB10 (SPI2_SCK) | — | nRF_B.P0.08 (SCK) |
| SPIB_MISO | nRF_B.P0.06 (MISO) | — | STM32.PB11 (SPI2_MISO) |
| SPIB_MOSI | STM32.PB15 (SPI2_MOSI) | — | nRF_B.P0.07 (MOSI) |
| SPIB_CS | STM32.PB12 (SPI2_NSS) | — | nRF_B.P0.05 (CS) |
| SPIB_IRQ | nRF_B.P0.09 (IRQ) | — | STM32.PA1 (EXTI1) |
| SPIB_RST | STM32.PB2 (GPIO) | — | nRF_B.P0.10 (RESET) |

### 4.4 USB

| Net Name | Source Pin | Via Component | Dest Pin |
|----------|-----------|---------------|----------|
| USB_DP | STM32.PA12 | R8 (22Ω) | J1.3 (D+) |
| USB_DM | STM32.PA11 | R7 (22Ω) | J1.2 (D−) |
| USB_SHIELD | J1.5 (shield) | — | GND |

### 4.5 I2C — Battery Gauge

| Net Name | Source Pin | Via Component | Dest Pin |
|----------|-----------|---------------|----------|
| I2C_SCL | STM32.PB6 (I2C1_SCL) | R10 (4.7kΩ pull-up) | 3V3 |
| I2C_SDA | STM32.PB5 (I2C1_SDA) | R11 (4.7kΩ pull-up) | 3V3 |
| BAT_INT | STM32.PB4 (GPIO) | — | U4 status |

### 4.6 Antenna Matching Network (Radio A — identical for Radio B)

| Net Name | Source Pin | Via Component | Dest Pin |
|----------|-----------|---------------|----------|
| RF_A | nRF_A.ANT1 | L1 (10 nH) | C26 (0.5 pF) |
| RF_A_MATCH | L1/C26 junction | L2 (10 nH) | ANT1 (chip ant) |
| RF_A_GND | nRF_A.ANT2 | — | GND |

## 5. Decoupling Strategy

### 5.1 STM32F401 Decoupling

| Cap | Value | Placement | Purpose |
|-----|-------|-----------|---------|
| C4 | 100 nF | VDD pin 24 → GND | Core decoupling |
| C5 | 100 nF | VDD pin 46 → GND | Core decoupling |
| C6 | 100 nF | VDD pin 47 → GND | Core decoupling |
| C7 | 100 nF | VBAT pin 1 → GND | VBAT decoupling |
| C8 | 100 nF | VDDA pin 9 → GND | Analog decoupling |
| C9 | 1 µF | VDDA pin 9 → GND | Analog bulk |

### 5.2 nRF52832 Decoupling (per radio, ×2)

| Cap | Value | Placement | Purpose |
|-----|-------|-----------|---------|
| C10 | 100 nF | DEC1 pin 1 → GND | DEC1 decoupling |
| C11 | 100 nF | DEC3 pin 33 → GND | DEC3 decoupling |
| C12 | 100 nF | DCC pin 34 → GND | DCC decoupling |
| C13 | 100 nF | DEC4 pin 42 → GND | DEC4 decoupling |
| C14 | 1 µF | DEC5 pin 43 → GND | DEC5 bulk |
| C15 | 1 µF | VDD pin 7 → GND | VDD decoupling |
| C16 | 1 µF | VDD pin 32 → GND | VDD decoupling |
| C17 | 1 µF | VDD pin 44 → GND | VDD decoupling |
| C18 | 100 nF | VDD pin 41 → GND | VDD decoupling |

## 6. Impedance-Controlled Pairs

| Signal | Impedance | Reference | Notes |
|--------|-----------|-----------|-------|
| USB D+/D− | 90 Ω differential | GND plane | Length-matched ±0.5 mm |
| RF (ANT) | 50 Ω single-ended | GND plane | Short run (< 10 mm) |
| SPI clock/data | 50 Ω single-ended | GND plane | Series terminations not needed at 8 MHz |

## 7. Schematic Notes

1. **nRF52832 SPI slave firmware**: Each nRF runs custom firmware exposing SPI command interface. The SPI operates in Mode 0 (CPOL=0, CPHA=0) at 8 MHz.
2. **Radio A/B independence**: Both radios can operate on different BLE channels simultaneously — Radio A on advertising channel 37 while Radio B is connected on data channel.
3. **Power path**: VBUS takes priority. When VBUS present, LiPo charges and system runs from VBUS via LDO. When VBUS absent, LiPo powers system through Schottky D5 to LDO.
4. **Crystal selection**: STM32 uses 8 MHz HSE (Y2). nRF52832s use internal RC for HFCLK and external 32.768 kHz (Y3, Y4) for LFCLK.
5. **USB ESD**: PRTR5V0U2X placed directly at USB connector to protect D+/D−.
6. **Boot mode**: BOOT0 tied to GND via R2 (10 kΩ) for normal boot from Flash. BOOT0 can be pulled high via SWD for DFU mode.