# Infrared Phantom — Phase 2: Component Selection & Schematics

## 1. Bill of Materials

| # | Reference | Part Number | Description | Package | Qty | Unit $ | Total $ |
|---|-----------|-------------|-------------|---------|-----|--------|---------|
| 1 | U1 | STM32H743VIT6 | Cortex-M7 MCU, 480 MHz, 1 MB Flash, 1 MB RAM | LQFP-100 | 1 | 12.50 | 12.50 |
| 2 | U2 | nRF52840-M.2 | BLE 5.0 module, M.2 footprint | M.2 2242 | 1 | 5.20 | 5.20 |
| 3 | U3 | USB3320-EZ | ULPI USB 2.0 HS PHY | QFN-32 | 1 | 1.80 | 1.80 |
| 4 | U4 | W25Q128JVSIQ | 16 MB SPI NOR Flash | SOIC-8 | 1 | 0.65 | 0.65 |
| 5 | U5 | SSD1306 | 128×32 OLED driver (on OLED module) | COG | 1 | 1.50 | 1.50 |
| 6 | U6 | TSMP58000 | IR learning receiver, 30–56 kHz, analog+digital | 3-pin DIP | 1 | 0.95 | 0.95 |
| 7 | U7 | TLV62569 | 4.5–17 V, 2 A buck converter | SOT-23-6 | 1 | 0.75 | 0.75 |
| 8 | U8 | LP3985-1.8 | 1.8 V, 150 mA LDO | SOT-23-5 | 1 | 0.35 | 0.35 |
| 9 | Q1 | Si2312CDS | P-MOSFET, –20 V, –4.2 A, 28 mΩ | SOT-323 | 1 | 0.25 | 0.25 |
| 10 | Q2–Q10 | TSAL6100 | IR LED, 940 nm, 35 mW/sr, 25° | 5 mm THT | 9 | 0.15 | 1.35 |
| 11 | Q11 | BC847C | NPN BJT, 100 mA driver for MOSFET | SOT-23 | 1 | 0.04 | 0.04 |
| 12 | Y1 | ABM8-27.000MHZ | 27.000 MHz crystal, USB PHY reference | HC49/SMD | 1 | 0.30 | 0.30 |
| 13 | Y2 | ECS-250-8-36B | 25.000 MHz crystal, STM32H7 HSE | HC49/SMD | 1 | 0.25 | 0.25 |
| 14 | Y3 | ABS07-32.768KHZ | 32.768 kHz crystal, STM32 RTC | 3.2×1.5 mm | 1 | 0.15 | 0.15 |
| 15 | J1 | USB-A-M-PLUG | USB-A male plug, SMT, right angle | SMT | 1 | 0.40 | 0.40 |
| 16 | J2 | microSD-CARD-CONN | microSD card connector, push-push | SMT | 1 | 0.50 | 0.50 |
| 17 | SW1 | SKQGADE010 | Tactile switch, 6×6 mm, 160 gf | THT | 1 | 0.10 | 0.10 |
| 18 | D1 | PRTR5V0U2X | USB ESD protection, 2-line | SOT-143 | 1 | 0.20 | 0.20 |
| 19 | D2 | PRTR5V0U2X | ESD protection for nRF UART | SOT-143 | 1 | 0.20 | 0.20 |
| 20 | FB1 | BLM21PG221SN1 | Ferrite bead, 220 Ω @ 100 MHz, 500 mA | 0805 | 1 | 0.05 | 0.05 |
| 21 | C1 | 100 µF | Electrolytic, 6.3 V, USB bulk | 0805 (MLCC) | 1 | 0.15 | 0.15 |
| 22 | C2,C3 | 22 pF | 27 MHz crystal load caps | 0402 | 2 | 0.01 | 0.02 |
| 23 | C4,C5 | 22 pF | 25 MHz crystal load caps | 0402 | 2 | 0.01 | 0.02 |
| 24 | C6,C7 | 12.5 pF | 32.768 kHz crystal load caps | 0402 | 2 | 0.01 | 0.02 |
| 25 | C8 | 10 µF | TLV62569 output cap | 0805 | 1 | 0.05 | 0.05 |
| 26 | C9 | 10 µF | LP3985 output cap | 0805 | 1 | 0.05 | 0.05 |
| 27 | C10 | 100 nF | TLV62569 input decoupling | 0402 | 1 | 0.01 | 0.01 |
| 28 | C11 | 100 nF | STM32 VDD decoupling #1 | 0402 | 1 | 0.01 | 0.01 |
| 29 | C12–C20 | 100 nF | STM32 VDD decoupling #2–10 | 0402 | 9 | 0.01 | 0.09 |
| 30 | C21 | 100 nF | nRF52840 decoupling | 0402 | 1 | 0.01 | 0.01 |
| 31 | C22 | 4.7 µF | nRF52840 VDD decoupling | 0402 | 1 | 0.02 | 0.02 |
| 32 | C23 | 100 nF | W25Q128 decoupling | 0402 | 1 | 0.01 | 0.01 |
| 33 | C24 | 100 nF | USB3320 decoupling | 0402 | 1 | 0.01 | 0.01 |
| 34 | C25 | 1 µF | USB3320 VDDA decoupling | 0402 | 1 | 0.02 | 0.02 |
| 35 | C26 | 100 nF | TSMP58000 decoupling | 0402 | 1 | 0.01 | 0.01 |
| 36 | C27 | 10 nF | IR LED array gate drive filter | 0402 | 1 | 0.01 | 0.01 |
| 37 | C28 | 1 µF | STM32 VCAP1 (SMPS) | 0402 | 1 | 0.02 | 0.02 |
| 38 | R1 | 10 kΩ | BOOT0 pull-down | 0402 | 1 | 0.005 | 0.005 |
| 39 | R2 | 100 Ω | nRF52840 RESET pull-up | 0402 | 1 | 0.005 | 0.005 |
| 40 | R3 | 1 kΩ | BC847C base resistor | 0402 | 1 | 0.005 | 0.005 |
| 41 | R4 | 27.4 Ω | USB D+ series (USB3320) | 0402 | 1 | 0.005 | 0.005 |
| 42 | R5 | 27.4 Ω | USB D– series (USB3320) | 0402 | 1 | 0.005 | 0.005 |
| 43 | R6,R7 | 2.2 Ω | IR LED array current limiters (per LED) | 0402 | 2 | 0.005 | 0.01 |
| 44 | R8 | 100 kΩ | TLV62569 feedback upper | 0402 | 1 | 0.005 | 0.005 |
| 45 | R9 | 47 kΩ | TLV62569 feedback lower | 0402 | 1 | 0.005 | 0.005 |
| 46 | R10 | 10 kΩ | TLV62569 enable pull-up | 0402 | 1 | 0.005 | 0.005 |
| 47 | R11 | 330 Ω | SSD1306 LED backlight resistor | 0402 | 1 | 0.005 | 0.005 |
| 48 | R12 | 100 kΩ | microSD card detect pull-up | 0402 | 1 | 0.005 | 0.005 |
| 49 | L1 | VLS201610MT-2R2M | 2.2 µH, 2 A buck inductor | 2016 | 1 | 0.20 | 0.20 |
| 50 | OLED | ER-OLED0.49-1 | 0.49" 128×32 OLED display module | FPC | 1 | 2.00 | 2.00 |
| 51 | — | PCB | 4-layer, 85×34 mm, ENIG | — | 1 | 2.50 | 2.50 |
| | | | | | | **TOTAL** | **$34.92** |

## 2. Pinout Tables

### 2.1 STM32H743VIT6 — LQFP-100 Pin Assignment

| Pin | Name | Function | Net | Notes |
|-----|------|----------|-----|-------|
| 1 | PE1 | GPIO | IR_LED_EN | MOSFET gate enable via BC847C |
| 2 | PE0 | GPIO | nRF_CTS | nRF52840 CTS (UART flow control) |
| 3 | PB9 | I²C1_SDA | I2C1_SDA | (reserved) |
| 4 | PB8 | I²C1_SCL | I2C1_SCL | (reserved) |
| 5 | BOOT0 | — | BOOT0 | 10 kΩ pull-down → boot from Flash |
| 6 | PB7 | GPIO | OLED_DC | SSD1306 data/command select |
| 7 | PB6 | GPIO | OLED_RST | SSD1306 reset |
| 8 | PB5 | SPI3_SCK | SD_CLK | microSD SDMMC1 (alt: SPI3) |
| 9 | PB4 | SPI3_MISO | SD_D0 | SDMMC1_D0 (alt: SPI3) |
| 10 | PB3 | SPI3_MOSI | SD_CMD | SDMMC1_CMD |
| 11 | PD7 | GPIO | SD_D1 | SDMMC1_D1 |
| 12 | PD6 | GPIO | SD_D2 | SDMMC1_D2 |
| 13 | PD5 | GPIO | SD_D3 | SDMMC1_D3 |
| 14 | PD4 | GPIO | SD_CMD_DIR | SD level shifter direction (unused) |
| 15 | PD3 | GPIO | SD_CLK_DIR | SD level shifter direction (unused) |
| 16 | PD2 | GPIO | IR_PWM | TIM3_CH1 alternate → IR LED PWM |
| 17 | PD1 | GPIO | nRF_RTS | nRF52840 RTS |
| 18 | PD0 | GPIO | nRF_RESET | nRF52840 RESET active-low |
| 19 | PC12 | SPI3_MOSI | SPI2_MOSI | (mapped to SPI2 for OLED) |
| 20 | PC11 | SPI3_MISO | — | (unused) |
| 21 | PC10 | SPI3_SCK | SPI2_SCK | (mapped to SPI2 for OLED) |
| 22 | PA14 | SWCLK | SWCLK | Debug SWD clock |
| 23 | PA13 | SWDIO | SWDIO | Debug SWD data |
| 24 | PA12 | USB_DP | USB_DP | USB3320 data+ (via ULPI) |
| 25 | PA11 | USB_DM | USB_DM | USB3320 data– (via ULPI) |
| 26 | PA10 | USART1_RX | nRF_TX | nRF52840 UART TX → STM32 RX |
| 27 | PA9 | USART1_TX | nRF_RX | STM32 TX → nRF52840 UART RX |
| 28 | PA8 | MCO1 | — | (unused, reserved for clock out) |
| 29 | PA7 | ADC1_IN7 | IR_ANALOG | TSMP58000 analog output |
| 30 | PA6 | ADC1_IN6 | IR_DIGITAL | TSMP58000 digital output (TIM3 capture) |
| 31 | PA5 | SPI1_SCK | SPI1_SCK | W25Q128 SPI clock |
| 32 | PA4 | SPI1_NSS | SPI1_NSS | W25Q128 chip select (manual) |
| 33 | PA3 | SPI1_MISO | SPI1_MISO | W25Q128 data out |
| 34 | PA2 | SPI1_MOSI | SPI1_MOSI | W25Q128 data in |
| 35 | PA1 | GPIO | USER_BTN | User button (active-low) |
| 36 | PA0 | GPIO | nRF_INT | nRF52840 interrupt pin |
| 37 | VBAT | — | VBAT | 3.3 V backup domain |
| 38 | VDDA | — | VDDA | 3.3 V analog supply (filtered) |
| 39 | VSSA | — | GND | Analog ground |
| 40 | VCAP1 | — | VCAP1 | 1.2 V SMPS cap (4.7 µF) |
| 41–56 | VDD/VSS | — | VDD/GND | Power/ground pairs (9 pairs) |
| 57 | PH0-OSC_IN | — | HSE_IN | 25 MHz crystal input |
| 58 | PH1-OSC_OUT | — | HSE_OUT | 25 MHz crystal output |
| 59 | NRST | — | NRST | System reset (active-low) |
| 60 | PC0 | ULPI_STP | ULPI_STP | USB3320 ULPI STP |
| 61 | PC1 | ULPI_DIR | ULPI_DIR | USB3320 ULPI DIR |
| 62 | PC2 | ULPI_NXT | ULPI_NXT | USB3320 ULPI NXT |
| 63 | PC3 | ULPI_CLK | ULPI_CLK | USB3320 ULPI 60 MHz clock |
| 64–71 | PD8–PD15 | ULPI_D[0:7] | ULPI_D0–D7 | USB3320 ULPI data bus |
| 72 | PE2 | GPIO | LED_STATUS | Status LED (green) |
| 73 | PE3 | GPIO | LED_ERROR | Error LED (red) |
| 74 | PE4 | TIM3_CH2 | IR_PWM_GATE | TIM3_CH2 alternate → IR LED PWM gate |
| 75 | PE5 | GPIO | OLED_CS | SSD1306 chip select |
| 76 | PE6 | GPIO | SD_CD | microSD card detect |
| 77 | PE7 | ULPI_D[0] alt | — | (alternate ULPI, unused) |
| 78–100 | — | — | — | VDD/VSS pairs, NC, reserved |

### 2.2 nRF52840-M.2 — Module Pin Assignment

| Pin | Function | Net | Notes |
|-----|----------|-----|-------|
| 1 | VDD | 3V3 | Module power |
| 2 | GND | GND | Module ground |
| 3 | P0.05 | UART_TX | nRF_TX → STM32 PA10 (USART1_RX) |
| 4 | P0.06 | UART_RX | nRF_RX ← STM32 PA9 (USART1_TX) |
| 5 | P0.07 | UART_CTS | nRF_CTS ← STM32 PE0 |
| 6 | P0.08 | UART_RTS | nRF_RTS ← STM32 PD1 |
| 7 | P0.11 | RESET | nRF_RESET ← STM32 PD0 |
| 8 | P0.12 | GPIO | nRF_INT → STM32 PA0 |
| 9 | P0.13 | SWDIO | SWDIO | Debug |
| 10 | P0.14 | SWCLK | SWCLK | Debug |
| 11–24 | RF/NC | — | Antenna + unused |

### 2.3 USB3320 — ULPI USB 2.0 HS PHY Pin Assignment

| Pin | Name | Net | Notes |
|-----|------|-----|-------|
| 1 | VDDA33 | VDDA33 | 3.3 V analog supply (filtered) |
| 2 | DP | USB_DP | USB D+ to connector |
| 3 | DM | USB_DM | USB D– to connector |
| 4 | VDDA18 | VDDA18 | 1.8 V core supply |
| 5 | CLK | ULPI_CLK | 60 MHz ULPI clock → STM32 PC3 |
| 6 | STP | ULPI_STP | ULPI STP ← STM32 PC0 |
| 7 | DIR | ULPI_DIR | ULPI DIR → STM32 PC1 |
| 8 | NXT | ULPI_NXT | ULPI NXT → STM32 PC2 |
| 9–16 | D[7:0] | ULPI_D[7:0] | ULPI data → STM32 PD8–PD15 |
| 17 | REFCLK | HSE_IN_USB | 27 MHz crystal (or from STM32 MCO) |
| 18 | XTAL1 | XTAL1_USB | 27 MHz crystal input |
| 19 | XTAL2 | XTAL2_USB | 27 MHz crystal output |
| 20 | RESET | 3V3 | Active-low reset (pull-up) |
| 21 | ID | GND | USB device mode |
| 22 | VBUS | VBUS | USB VBUS detect |
| 23 | GND | GND | Ground |
| 24 | VDD33 | 3V3 | 3.3 V digital supply |

### 2.4 TSMP58000 IR Receiver Pinout

| Pin | Name | Net | Notes |
|-----|------|-----|-------|
| 1 | VOUT | IR_ANALOG | Analog envelope output → STM32 PA7 (ADC1_IN7) |
| 2 | GND | GND | Ground |
| 3 | VCC | 3V3 | 3.3 V supply (100 nF decoupled) |
| 4 | OUT | IR_DIGITAL | Digital demodulated output → STM32 PA6 (TIM3_CH1 capture) |

## 3. Netlist — Critical Signal Paths

### 3.1 IR Receive Path

```
TSMP58000 VOUT (pin 1) → R_filter 100 Ω (EMI filter) → C_dec 100 nF → STM32 PA7 (ADC1_IN7)
  ADC1 config: 12-bit, 2 Msps, DMA1 Stream 0, circular mode
  DMA buffer: 8192 samples in DTCM SRAM (4 ms at 2 Msps)

TSMP58000 OUT (pin 4) → R_series 47 Ω → STM32 PA6 (TIM3_CH1 input capture)
  TIM3 config: 32-bit counter, capture on both edges, DMA timestamp logging
```

### 3.2 IR Transmit Path

```
STM32 PD2 (TIM3_CH1 PWM) → R_gate 1 kΩ → BC847C base
  BC847C collector → Si2312CDS gate (P-MOSFET)
  Si2312CDS source → VBUS (5 V)
  Si2312CDS drain → IR LED array common anode

STM32 PE4 (TIM3_CH2 gate) → TIM3_CH2 DMA pattern → IR burst gating

IR LED array: 9× TSAL6100 in 3×3 parallel-series
  Each LED: If = 100 mA, Vf = 1.35 V
  Series string of 3: Vf_total = 4.05 V (directly from VBUS 5 V)
  3 parallel strings: Itotal = 300 mA continuous, 3 A peak (10% duty)
  Current limiter per string: R = (5.0 – 4.05) / 0.100 = 9.5 Ω → 10 Ω
  Peak power (10% duty, 3 A): 3 W × 0.1 = 300 mW average
```

### 3.3 SPI NOR Flash (W25Q128JVSIQ)

```
STM32 PA5 (SPI1_SCK)    → W25Q128 pin 6 (CLK)
STM32 PA4 (SPI1_NSS)    → W25Q128 pin 1 (/CS)
STM32 PA2 (SPI1_MOSI)   → W25Q128 pin 5 (DI)
STM32 PA3 (SPI1_MISO)   → W25Q128 pin 2 (DO)
W25Q128 pin 3 (/WP)     → 3V3 (write protect off)
W25Q128 pin 7 (/HOLD)   → 3V3 (hold off)
W25Q128 pin 8 (VCC)     → 3V3
W25Q128 pin 4 (GND)     → GND
```

### 3.4 OLED Display (SSD1306, SPI2)

```
STM32 PB3  (SPI2_SCK)    → SSD1306 SCLK
STM32 PB4  (SPI2_MISO)   → (unused, SPI2 has no readback)
STM32 PB5  (SPI2_MOSI)   → SSD1306 SDA (MOSI)
STM32 PE5  (GPIO)        → SSD1306 /CS
STM32 PB7  (GPIO)        → SSD1306 DC (data/command)
STM32 PB6  (GPIO)        → SSD1306 /RES
SSD1306 VCC              → 3V3
SSD1306 GND              → GND
```

### 3.5 microSD Card (SDMMC1)

```
STM32 PC12 (SDMMC1_CK)   → microSD pin 5 (CLK)
STM32 PC10 (SDMMC1_CMD)  → microSD pin 2 (CMD)
STM32 PC11 (SDMMC1_D0)   → microSD pin 7 (D0)
STM32 PD7  (SDMMC1_D1)   → microSD pin 8 (D1)
STM32 PD6  (SDMMC1_D2)   → microSD pin 1 (D2)
STM32 PD5  (SDMMC1_D3)   → microSD pin 2 (D3)
STM32 PE6  (GPIO)        → microSD card detect switch
microSD VCC              → 3V3
microSD GND              → GND
```

### 3.6 nRF52840 UART (BLE C2)

```
STM32 PA9  (USART1_TX)   → nRF52840 P0.06 (UART_RX)
STM32 PA10 (USART1_RX)  → nRF52840 P0.05 (UART_TX)
STM32 PE0  (GPIO)        → nRF52840 P0.07 (UART_CTS)
STM32 PD1  (GPIO)        → nRF52840 P0.08 (UART_RTS)
STM32 PD0  (GPIO)        → nRF52840 P0.11 (RESET, active-low)
STM32 PA0  (GPIO)        → nRF52840 P0.12 (INT, active-high)
nRF52840 VDD             → 3V3
nRF52840 GND             → GND
```

### 3.7 USB3320 ULPI Interface

```
STM32 PC3  (ULPI_CLK)    → USB3320 pin 5 (CLK)
STM32 PC0  (ULPI_STP)    → USB3320 pin 6 (STP)
STM32 PC1  (ULPI_DIR)    → USB3320 pin 7 (DIR)
STM32 PC2  (ULPI_NXT)    → USB3320 pin 8 (NXT)
STM32 PD8  (ULPI_D0)     → USB3320 pin 16 (D0)
STM32 PD9  (ULPI_D1)     → USB33220 pin 15 (D1)
STM32 PD10 (ULPI_D2)     → USB3320 pin 14 (D2)
STM32 PD11 (ULPI_D3)     → USB3320 pin 13 (D3)
STM32 PD12 (ULPI_D4)     → USB3320 pin 12 (D4)
STM32 PD13 (ULPI_D5)     → USB3320 pin 11 (D5)
STM32 PD14 (ULPI_D6)     → USB3320 pin 10 (D6)
STM32 PD15 (ULPI_D7)     → USB3320 pin 9 (D7)
USB3320 pin 2 (DP)       → R4 27.4 Ω → USB connector D+
USB3320 pin 3 (DM)       → R5 27.4 Ω → USB connector D–
USB3320 pin 22 (VBUS)    → VBUS (USB 5 V)
USB3320 pin 21 (ID)      → GND (device mode)
```

## 4. Power Supply Design

### 4.1 Power Tree

```
USB VBUS (5 V, 500 mA guaranteed)
  │
  ├── TLV62569 Buck Converter
  │   Input: 5 V USB
  │   Output: 3.3 V / 1.5 A (VDD domain)
  │   Feedback: R8 = 100 kΩ, R9 = 47 kΩ → Vout = 0.6 × (1 + R8/R9) = 0.6 × 3.128 = 1.877 V
  │   [CORRECTION: For 3.3 V: R8 = 100 kΩ, R9 = 22 kΩ → Vout = 0.6 × (1 + 100/22) = 3.33 V]
  │   Inductor: 2.2 µH (VLS201610MT-2R2M)
  │   C_in: 10 µF (C10), C_out: 10 µF (C8)
  │
  ├── LP3985-1.8 LDO
  │   Input: 3.3 V (from TLV62569)
  │   Output: 1.8 V / 150 mA (VDDCORE domain for STM32H7)
  │   C_out: 10 µF (C9)
  │
  └── IR LED Array (direct from VBUS)
      VBUS → Si2312CDS P-FET → IR LED strings
      Average current: < 100 mA (pulsed operation, 10% duty)
      Peak current: 3 A (microsecond bursts)
```

### 4.2 Power Budget

| Rail | Voltage | Max Current | Source | Loads |
|------|---------|-------------|--------|-------|
| VBUS | 5.0 V | 500 mA | USB | TLV62569, IR LEDs |
| 3V3 | 3.3 V | 400 mA | TLV62569 | STM32, nRF, OLED, SPI NOR, SD |
| 1V8 | 1.8 V | 150 mA | LP3985 | STM32 core (VDDCORE) |
| VDDA | 3.3 V | 20 mA | 3V3 via ferrite | STM32 analog, ADC |
| VBUS_LED | 5.0 V | 3 A peak | VBUS via MOSFET | IR LED array (pulsed only) |

### 4.3 Current Budget (3.3 V Rail)

| Component | Typical | Peak |
|-----------|---------|------|
| STM32H743 | 120 mA | 250 mA |
| nRF52840 | 15 mA | 80 mA |
| USB3320 | 25 mA | 50 mA |
| W25Q128 | 5 mA | 25 mA |
| SSD1306 | 10 mA | 20 mA |
| microSD | 30 mA | 100 mA |
| TSMP58000 | 3 mA | 5 mA |
| Misc (pull-ups, LEDs) | 5 mA | 10 mA |
| **Total** | **213 mA** | **540 mA** |

Note: Peak exceeds USB 500 mA only during microSD writes; firmware staggers operations.

## 5. Decoupling Network

### 5.1 STM32H743VIT6 Decoupling

| Capacitor | Value | Placement | Net |
|-----------|-------|-----------|-----|
| C11 | 100 nF | Pin 41 (VDD) | VDD to GND |
| C12 | 100 nF | Pin 51 (VDD) | VDD to GND |
| C13 | 100 nF | Pin 61 (VDD) | VDD to GND |
| C14 | 100 nF | Pin 71 (VDD) | VDD to GND |
| C15 | 100 nF | Pin 81 (VDD) | VDD to GND |
| C16 | 100 nF | Pin 91 (VDD) | VDD to GND |
| C17 | 100 nF | Pin 56 (VDD) | VDD to GND |
| C18 | 100 nF | Pin 46 (VDD) | VDD to GND |
| C19 | 100 nF | Pin 36 (VDD) | VDD to GND |
| C20 | 4.7 µF | Pin 41 (VDD) | VDD to GND (bulk) |
| C28 | 4.7 µF | Pin 40 (VCAP) | VCAP to GND |

### 5.2 Peripheral Decoupling

| Component | Capacitor | Value | Net |
|-----------|-----------|-------|-----|
| nRF52840 | C21, C22 | 100 nF, 4.7 µF | VDD to GND |
| USB3320 | C24, C25, C29 | 100 nF, 1 µF, 10 nF | VDD, VDDA to GND |
| W25Q128 | C23 | 100 nF | VCC to GND |
| TSMP58000 | C26 | 100 nF | VCC to GND |
| microSD | C30 | 10 µF | VCC to GND |

## 6. Impedance Matching Pairs

| Signal Pair | Impedance | Layer | Notes |
|-------------|-----------|-------|-------|
| USB D+ / D– | 90 Ω diff | Top | Matched to USB 2.0 HS spec |
| ULPI D[7:0] | 50 Ω single | Top | Length-matched within 50 mil |
| SPI1 (SCK/MOSI/MISO) | 50 Ω single | Top | < 2" trace length |
| SPI2 (SCK/MOSI) | 50 Ω single | Top | < 1.5" trace length |
| SDMMC1 CLK | 50 Ω single | Top | < 2" trace length |
| USART1 TX/RX | 50 Ω single | Top | 115200 baud, no critical impedance |
| IR_ANALOG | 50 Ω single | Top | Shielded, < 0.5" |
| IR_PWM | 50 Ω single | Top | Minimize loop area to MOSFET gate |

## 7. Crystal Oscillator Specifications

| Reference | Frequency | Load Caps | Tolerance | Purpose |
|-----------|-----------|-----------|-----------|---------|
| Y2 | 25.000 MHz | C4, C5 = 22 pF | ±30 ppm | STM32H7 HSE (system clock) |
| Y1 | 27.000 MHz | C2, C3 = 22 pF | ±30 ppm | USB3320 ULPI reference |
| Y3 | 32.768 kHz | C6, C7 = 12.5 pF | ±20 ppm | STM32 RTC LSE |

STM32H743 clock tree:
- HSE = 25 MHz → PLL1 → SYSCLK = 480 MHz
- PLL1: M = 5, N = 192, P = 2 → VCO = 960 MHz, SYSCLK = 480 MHz
- AHB = SYSCLK/1 = 480 MHz
- APB1 = SYSCLK/4 = 120 MHz (TIM2, TIM3)
- APB2 = SYSCLK/2 = 240 MHz (USART1, SPI1, ADC)
- APB4 = SYSCLK/4 = 120 MHz (SPI2, SDMMC1)