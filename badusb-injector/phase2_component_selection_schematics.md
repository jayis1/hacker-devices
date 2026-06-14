# PHANTOM — Phase 2: Component Selection & Schematics

## 1. Bill of Materials (BOM)

### 1.1 Main Components

| Ref | Part Number | Description | Manufacturer | Qty | Unit Price | Notes |
|-----|------------|-------------|--------------|-----|-----------|-------|
| U1 | RP2040 | Dual Cortex-M0+ MCU, 133MHz, 264KB SRAM, QFN-56 | Raspberry Pi Foundation | 1 | $1.00 | Main MCU, USB FS device |
| U2 | W25Q128JVSIQ | 16MB SPI NOR Flash, SOIC-8 | Winbond | 1 | $0.85 | Payload storage |
| U3 | ESP32-C3-MINI-1 | WiFi 6 + BLE 5 Module, M.2 stick | Espressif | 1 | $3.50 | Wireless control |
| U4 | SSD1306 | 128×64 OLED Display, I²C, 0.49" | Solomon Systech | 1 | $2.50 | Status display |
| U5 | MCP73831T-2ACI/OT | LiPo Charger, 500mA, SOT-23-5 | Microchip | 1 | $0.45 | Battery charging |
| U6 | AP2112K-3.3 | 3.3V LDO, 600mA, SOT-23-5 | Diodes Inc | 1 | $0.25 | Main 3.3V rail |
| U7 | AP2112K-1.8 | 1.8V LDO, 300mA, SOT-23-5 | Diodes Inc | 1 | $0.25 | RP2040 core voltage |
| U8 | TS3USB221A | USB 2:1 Analog Switch, VQFN-10 | Texas Instruments | 1 | $0.75 | Kill switch data mux |
| U9 | W25Q16JVSIQ | 16Mb QSPI Boot Flash, SOIC-8 | Winbond | 1 | $0.40 | RP2040 boot flash |
| D1 | WS2812B-2020 | RGB LED, 3.5×3.5mm, Intelligent | Worldsemi | 1 | $0.12 | Status indicator |
| J1 | USB-A Plug | USB-A Male PCB Edge, SMT | Various | 1 | $0.35 | Host connection |
| SW1 | Slide Switch | SPDT Slide Switch, 2.5mm pitch | C&K | 1 | $0.15 | Hardware kill switch |
| ENC1 | EC11 | Rotary Encoder w/ Push, 6mm shaft | ALPS/Bourns | 1 | $0.80 | Profile navigation |
| Y1 | ABM8-12.000MHZ | 12 MHz Crystal, 20ppm, 3.2×2.5mm | Abracon | 1 | $0.25 | RP2040 main clock |
| BAT1 | 502030 | 3.7V 500mAh LiPo, 2-pin JST | Various | 1 | $3.00 | Portable power |
| FB1 | 600Ω @ 100MHz | Ferrite Bead, 0402 | Murata | 1 | $0.02 | VBUS filtering |

**BOM Total (est.)**: ~$17.72 (1K volume)

### 1.2 Passive Components

| Ref | Value | Package | Part Number | Qty | Notes |
|-----|-------|---------|-------------|-----|-------|
| C1 | 10 µF | 0603 | GRM188R61A106M | 1 | VBUS bulk decoupling |
| C2 | 1 µF | 0402 | GRM155R61E105K | 1 | VDD_3V3 decoupling |
| C3 | 100 nF | 0402 | GRM155R71C104K | 1 | RP2040 VDD decoupling |
| C4 | 100 nF | 0402 | GRM155R71C104K | 1 | RP2040 VDD decoupling |
| C5 | 100 nF | 0402 | GRM155R71C104K | 1 | RP2040 VDD decoupling |
| C6 | 100 nF | 0402 | GRM155R71C104K | 1 | RP2040 VDD decoupling |
| C7 | 100 nF | 0402 | GRM155R71C104K | 1 | RP2040 VDD decoupling |
| C8 | 100 nF | 0402 | GRM155R71C104K | 1 | RP2040 VDD decoupling |
| C9 | 10 µF | 0603 | GRM188R61A106M | 1 | VDD_3V3 bulk |
| C10 | 10 µF | 0603 | GRM188R61A106M | 1 | VDD_1V8 bulk |
| C11 | 100 nF | 0402 | GRM155R71C104K | 1 | ESP32-C3 decoupling |
| C12 | 100 nF | 0402 | GRM155R71C104K | 1 | ESP32-C3 decoupling |
| C13 | 10 µF | 0603 | GRM188R61A106M | 1 | ESP32-C3 bulk |
| C14 | 100 nF | 0402 | GRM155R71C104K | 1 | W25Q128 decoupling |
| C15 | 100 nF | 0402 | GRM155R71C104K | 1 | W25Q16 decoupling |
| C16 | 27 pF | 0402 | GRM1555C1H270J | 2 | Y1 load caps |
| C17 | 27 pF | 0402 | GRM1555C1H270J | — | Y1 load cap (paired with C16) |
| C18 | 1 µF | 0402 | GRM155R61E105K | 1 | MCP73831 timing |
| C19 | 4.7 µF | 0603 | GRM188R61E475K | 1 | MCP73831 VBAT |
| R1 | 10 kΩ | 0402 | RC0402JR-10KL | 1 | RUN pull-up |
| R2 | 10 kΩ | 0402 | RC0402JR-10KL | 1 | BOOT_BTN pull-up |
| R3 | 1 kΩ | 0402 | RC0402JR-1KL | 1 | ADC divider upper |
| R4 | 3.3 kΩ | 0402 | RC0402JR-3K3L | 1 | ADC divider lower (3.3:1) |
| R5 | 27.4 kΩ | 0402 | RC0402FR-27K4L | 1 | MCP73831 charge current (470mA) |
| R6 | 100 Ω | 0402 | RC0402JR-100L | 1 | WS2812B series resistor |
| R7 | 4.7 kΩ | 0402 | RC0402JR-4K7L | 1 | I²C SDA pull-up |
| R8 | 4.7 kΩ | 0402 | RC0402JR-4K7L | 1 | I²C SCL pull-up |
| R9 | 10 kΩ | 0402 | RC0402JR-10KL | 1 | KILL_SW pull-up |
| R10 | 10 kΩ | 0402 | RC0402JR-10KL | 1 | ENC_BTN pull-up |
| R11 | 10 kΩ | 0402 | RC0402JR-10KL | 1 | ESP_RST pull-up |
| R12 | 10 kΩ | 0402 | RC0402JR-10KL | 1 | FLASH_WP pull-up |

### 1.3 No-Stuff / Optional Components

| Ref | Value | Notes |
|-----|-------|-------|
| R13 | 0 Ω | Jumper for VBUS current sense (DNP) |
| C20 | DNP | Optional bulk cap for WiFi TX burst |

## 2. RP2040 Pinout & Net Assignments

### 2.1 Complete Pin Map (QFN-56)

| Pin | GPIO | Net | Function | Direction | Notes |
|-----|------|-----|----------|-----------|-------|
| 1 | — | GND | Ground | — | |
| 2 | GPIO0 | QSPI_SCLK | Boot flash clock | O | 3.3V, QSPI |
| 3 | GPIO1 | QSPI_SS | Boot flash select | O | 3.3V, QSPI |
| 4 | GPIO2 | QSPI_SD0 | Boot flash data 0 | IO | 3.3V, QSPI |
| 5 | GPIO3 | QSPI_SD1 | Boot flash data 1 | IO | 3.3V, QSPI |
| 6 | GPIO4 | QSPI_SD2 | Boot flash data 2 | IO | 3.3V, QSPI |
| 7 | GPIO5 | QSPI_SD3 | Boot flash data 3 | IO | 3.3V, QSPI |
| 8 | — | RUN | Reset | I | Active-low, 10k pull-up |
| 9 | — | GND | Ground | — | |
| 10 | GPIO6 | ENC_A | Encoder phase A | I | Rotary encoder, interrupt |
| 11 | GPIO7 | ENC_B | Encoder phase B | I | Rotary encoder, interrupt |
| 12 | GPIO8 | ENC_BTN | Encoder button | I | Active-low, 10k pull-up |
| 13 | GPIO9 | KILL_SW_N | Kill switch | I | Active-low = kill mode |
| 14 | GPIO10 | I2C0_SDA | OLED data | IO | I²C0, 4.7k pull-up |
| 15 | GPIO11 | I2C0_SCL | OLED clock | O | I²C0, 4.7k pull-up |
| 16 | GPIO12 | UART0_TX | ESP UART TX | O | 115200 baud, AT commands |
| 17 | GPIO13 | UART0_RX | ESP UART RX | I | 115200 baud, AT responses |
| 18 | GPIO14 | ESP_RST_N | ESP32-C3 reset | O | Active-low |
| 19 | GPIO15 | LED_DATA | WS2812B data | O | Single-NeoPixel data line |
| 20 | GPIO16 | SPI0_CSn | Ext flash CS | O | W25Q128 chip select |
| 21 | GPIO17 | SPI0_SCK | Ext flash clock | O | W25Q128 clock |
| 22 | GPIO18 | SPI0_TX | Ext flash MOSI | O | W25Q128 data in |
| 23 | GPIO19 | SPI0_RX | Ext flash MISO | I | W25Q128 data out |
| 24 | GPIO20 | USB_MUX_CTRL | USB mode switch | O | HIGH=HID, LOW=MSC |
| 25 | GPIO21 | BAT_SENSE | Battery ADC | I | ADC1, voltage divider |
| 26 | GPIO22 | LED_EN | LED enable | O | HIGH=LED on, LOW=stealth |
| 27 | GPIO23 | SPI0_CSn_1 | Reserved | O | Second SPI CS (future) |
| 28 | — | GND | Ground | — | |
| 29 | GPIO24 | BOOT_BTN | Boot button | I | Active-low, 10k pull-up |
| 30 | GPIO25 | GPIO25 | User GPIO | IO | Breakout pad |
| 31 | GPIO26/ADC0 | ADC0 | Spare ADC | I | Breakout pad |
| 32 | GPIO27/ADC1 | ADC1 | Spare ADC | I | Breakout pad |
| 33 | GPIO28/ADC2 | ADC2 | Spare ADC | I | Breakout pad |
| 34 | — | GND | Ground | — | |
| 35 | — | 3V3 | 3.3V output | — | RP2040 regulator output |
| 36 | — | 1V8 | 1.8V core | — | RP2040 core voltage |
| 37 | — | USB_DP | USB D+ | IO | Via TS3USB221A mux |
| 38 | — | USB_DM | USB D- | IO | Via TS3USB221A mux |
| 39 | — | VBUS | USB 5V input | — | Power input |
| 40 | — | GND | Ground | — | |

*Note: RP2040 QFN-56 has additional power/ground pins not shown. See full datasheet for complete pinout.*

### 2.2 Power Net Assignments

| Net | Voltage | Source | Load | Max Current |
|-----|---------|--------|------|-------------|
| VBUS_5V | 4.75–5.25V | USB-A connector | MCP73831, AP2112K-3.3 | 500 mA |
| VBAT | 3.0–4.2V | LiPo battery | AP2112K-3.3 (via diode OR) | 300 mA |
| VDD_3V3 | 3.3V ±3% | AP2112K-3.3 | RP2040, ESP32-C3, W25Q128, SSD1306, TS3USB221A | 400 mA peak |
| VDD_1V8 | 1.8V ±3% | AP2112K-1.8 | RP2040 core | 100 mA |
| VDD_FLASH | 3.3V | VDD_3V3 | W25Q16 boot flash | 20 mA |

### 2.3 Decoupling Requirements

| IC | Pin | Cap Value | Package | Notes |
|----|-----|----------|---------|-------|
| RP2040 | VDD (each) | 100 nF × 6 | 0402 | One per VDD pin, placed within 2mm |
| RP2040 | VDD_3V3 bulk | 10 µF | 0603 | Near pin 35 |
| RP2040 | VDD_1V8 bulk | 10 µF | 0603 | Near pin 36 |
| RP2040 | USB_VBUS | 10 µF | 0603 | Near pin 39 |
| ESP32-C3 | VDD | 100 nF × 2 | 0402 | One per VDD pin |
| ESP32-C3 | VDD bulk | 10 µF | 0603 | Near module VDD pin |
| W25Q128 | VCC | 100 nF | 0402 | Near pin 8 |
| W25Q16 | VCC | 100 nF | 0402 | Near pin 8 |
| TS3USB221A | VCC | 100 nF | 0402 | Near pin 10 |

## 3. Schematic Design

### 3.1 RP2040 Core Circuit

```
                    RP2040 (QFN-56)
              ┌─────────────────────────┐
              │                         │
   VDD_3V3 ──┤ VDD        GPIO0  QSPI_SCLK ├──► U9 (W25Q16)
   VDD_3V3 ──┤ VDD        GPIO1  QSPI_SS   ├──► U9 (W25Q16)
   VDD_3V3 ──┤ VDD        GPIO2  QSPI_SD0  ├──► U9 (W25Q16)
   VDD_3V3 ──┤ VDD        GPIO3  QSPI_SD1  ├──► U9 (W25Q16)
   VDD_3V3 ──┤ VDD        GPIO4  QSPI_SD2  ├──► U9 (W25Q16)
   VDD_3V3 ──┤ VDD        GPIO5  QSPI_SD3  ├──► U9 (W25Q16)
              │                         │
   10kΩ ───┤ RUN        GPIO6  ENC_A   ├──► ENC1
   VDD_3V3 ──┤ VDD        GPIO7  ENC_B   ├──► ENC1
              │            GPIO8  ENC_BTN ├──► ENC1
   12MHz ───┤ XIN         GPIO9  KILL_SW ├──► SW1
   12MHz ───┤ XOUT       GPIO10 I2C0_SDA ├──► U4 (SSD1306)
              │            GPIO11 I2C0_SCL ├──► U4 (SSD1306)
   VDD_1V8 ──┤ VDD_CORE  GPIO12 UART0_TX ├──► U3 (ESP32-C3)
              │            GPIO13 UART0_RX ├──► U3 (ESP32-C3)
              │            GPIO14 ESP_RST  ├──► U3 (ESP32-C3)
   VDD_3V3 ──┤ VDD        GPIO15 LED_DATA ├──► D1 (WS2812B)
              │            GPIO16 SPI0_CSn  ├──► U2 (W25Q128)
              │            GPIO17 SPI0_SCK  ├──► U2 (W25Q128)
              │            GPIO18 SPI0_TX   ├──► U2 (W25Q128)
              │            GPIO19 SPI0_RX  ├──► U2 (W25Q128)
              │            GPIO20 USB_MUX   ├──► U8 (TS3USB221A)
              │            GPIO21 BAT_SENSE ├──► R3/R4 divider
              │            GPIO22 LED_EN    ├──► D1 enable
              │                         │
              │  USB_DP  ───────────────────► U8 (TS3USB221A)
              │  USB_DM  ───────────────────► U8 (TS3USB221A)
              │                         │
              │  VBUS    ───────────────────► VBUS_5V
              │                         │
              └─────────────────────────┘
```

### 3.2 USB Kill Switch Circuit

```
                VBUS_5V
                   │
                   │
              ┌────┴────┐
              │  C1     │  10µF
              │  (decoup)│
              └────┬────┘
                   │
              ┌────┴────┐
              │ FB1     │  600Ω@100MHz
              │ (ferrite)│
              └────┬────┘
                   │
          ┌────────┴────────┐
          │   VBUS_FILTERED │
          └────────┬────────┘
                   │
    ┌──────────────┼──────────────┐
    │              │              │
    │    ┌─────────┴─────────┐    │
    │    │  TS3USB221A      │    │
    │    │  (USB Analog Mux)│    │
    │    │                  │    │
    │    │  D1+ ← RP2040    │    │
    │    │  D1- ← RP2040   │    │
    │    │  D2+ ← NC        │    │
    │    │  D2- ← NC       │    │
    │    │                  │    │
    │    │  COM+ → USB-A D+ │    │
    │    │  COM- → USB-A D- │    │
    │    │                  │    │
    │    │  SEL ← KILL_SW   │    │
    │    │  (LOW=pass data, │    │
    │    │   HIGH=disconnect)│   │
    │    └─────────────────┘    │
    │              │              │
    │    ┌─────────┴─────────┐    │
    │    │  SW1 (Kill Switch)│    │
    │    │  SPDT slide       │    │
    │    │  NO → GPIO9       │    │
    │    │  COM → VDD_3V3    │    │
    │    └───────────────────┘    │
    │                             │
    └─────────────────────────────┘
```

### 3.3 USB Mode Switching

The USB mode switch (GPIO20) controls the TS3USB221A analog switch:

| GPIO20 (USB_MUX_CTRL) | Device Mode | USB Enumeration |
|----------------------|------------|-----------------|
| LOW | Mass Storage Class | USB MSC (flash drive) |
| HIGH | HID Composite | Keyboard + Mouse + Consumer |

When the kill switch (SW1) is in the OFF position, the TS3USB221A disconnects USB D+/D- regardless of GPIO20 state. This provides a hardware guarantee that no USB data can flow.

### 3.4 Power Supply Circuit

```
  USB VBUS (5V)                        LiPo Battery (3.7V)
       │                                      │
       │                                 ┌────┴────┐
       │                                 │  BAT1   │
       │                                 │ 500mAh  │
       │                                 └────┬────┘
       │                                      │
  ┌────┴────┐                          ┌──────┴──────┐
  │  C1     │                          │  MCP73831   │
  │  10µF   │                          │  Charger    │
  └────┬────┘                          │  (500mA)    │
       │                               └──────┬──────┘
  ┌────┴────┐                                 │
  │ FB1     │                          ┌──────┴──────┐
  │ 600Ω    │                          │  R5 27.4kΩ  │
  │ ferrite │                          │  (charge    │
  └────┬────┘                          │   current   │
       │                               │   set)      │
  ┌────┴────┐                          └─────────────┘
  │  D1     │                                 │
  │ Schottky│                                 │
  │  (OR)   │                                 │
  └────┬────┘                                 │
       │                                      │
       └────────────┬─────────────────────────┘
                    │
              ┌─────┴─────┐
              │  VSYS     │  (4.2V max, 3.0V min)
              └─────┬─────┘
                    │
              ┌─────┴─────┐
              │ AP2112K-3.3│  3.3V LDO
              │ (600mA)    │
              └─────┬─────┘
                    │
              ┌─────┴─────┐
              │  VDD_3V3  │
              └─────┬─────┘
                    │
         ┌──────────┼──────────┬──────────┐
         │          │          │          │
    ┌────┴────┐ ┌───┴───┐ ┌───┴───┐ ┌───┴────┐
    │ RP2040  │ │ESP32  │ │W25Q128│ │SSD1306 │
    │         │ │-C3    │ │+W25Q16│ │        │
    └─────────┘ └───────┘ └───────┘ └────────┘
         │
    ┌────┴────┐
    │AP2112K  │  1.8V LDO
    │-1.8     │  (300mA)
    └────┬────┘
         │
    ┌────┴────┐
    │ VDD_1V8 │
    └────┬────┘
         │
    ┌────┴────┐
    │ RP2040  │
    │ core    │
    └─────────┘
```

### 3.5 ESP32-C3-MINI-1 Interface

```
  ESP32-C3-MINI-1 (WiFi 6 + BLE 5 Module)
  ┌─────────────────────────────────────┐
  │                                     │
  │  TXD0 ────────── GPIO12 (RP2040)   │  UART TX
  │  RXD0 ────────── GPIO13 (RP2040)   │  UART RX
  │  EN ───────────── GPIO14 (RP2040)   │  Reset (active-low)
  │                                     │
  │  GPIO8 ──── WIFI_STATUS (LED)       │  WiFi connected indicator
  │  GPIO9 ──── BLE_STATUS (LED)       │  BLE connected indicator
  │                                     │
  │  VDD ──────── VDD_3V3              │  3.3V power
  │  GND ──────── GND                  │  Ground
  │                                     │
  │  ANT ──────── PCB trace antenna    │  2.4 GHz on-board
  │                                     │
  │  (AT-command firmware mode)        │
  │  Baud: 115200                       │
  │  Flow control: None                 │
  └─────────────────────────────────────┘
```

### 3.6 W25Q128JVSIQ SPI Flash (Payload Storage)

```
  W25Q128JVSIQ (16MB SPI NOR Flash)
  ┌─────────────────────────────────┐
  │  SOIC-8                         │
  │                                 │
  │  Pin 1: CS   ─── GPIO16 (CSn) │  Chip select
  │  Pin 2: DO   ─── GPIO19 (RX)  │  Data out (MISO)
  │  Pin 3: WP   ─── VDD_3V3     │  Write protect (always protected)
  │  Pin 4: GND  ─── GND         │  Ground
  │  Pin 5: DI   ─── GPIO18 (TX) │  Data in (MOSI)
  │  Pin 6: CLK  ─── GPIO17 (SCK) │  Clock
  │  Pin 7: HOLD ─── VDD_3V3     │  Hold off (always enabled)
  │  Pin 8: VCC  ─── VDD_3V3     │  Power
  │                                 │
  │  C14 (100nF) near VCC pin      │  Decoupling
  │  R12 (10kΩ) on WP pin          │  Pull-up for write protect
  └─────────────────────────────────┘
```

### 3.7 SSD1306 OLED Display (I²C)

```
  SSD1306 128×64 OLED Module
  ┌───────────────────────────────────┐
  │  I²C address: 0x3C               │
  │                                   │
  │  VDD ──────── VDD_3V3           │  Power (3.3V)
  │  GND ──────── GND               │  Ground
  │  SCL ──────── GPIO11 (I2C0_SCL) │  Clock (4.7kΩ pull-up)
  │  SDA ──────── GPIO10 (I2C0_SDA) │  Data (4.7kΩ pull-up)
  │                                   │
  │  Display: 128×64 pixels           │
  │  Controller: SSD1306              │
  │  Interface: I²C @ 400kHz         │
  └───────────────────────────────────┘
```

### 3.8 EC11 Rotary Encoder Circuit

```
  EC11 Rotary Encoder with Push Button
  ┌───────────────────────────────────┐
  │                                   │
  │  A ──────── GPIO6  (ENC_A)      │  Phase A (interrupt)
  │  B ──────── GPIO7  (ENC_B)      │  Phase B (interrupt)
  │  C ──────── GND                 │  Common
  │  SW ─────── GPIO8  (ENC_BTN)    │  Push button
  │  SW_COM ─── GND                 │  Button common
  │                                   │
  │  R10 (10kΩ) ENC_BTN → VDD_3V3  │  Pull-up
  │  C_filter (100nF) on A, B       │  Debounce filter
  └───────────────────────────────────┘
```

## 4. Netlist

### 4.1 Power Nets

| Net Name | Pins Connected |
|-----------|----------------|
| VBUS_5V | J1.1 (VBUS), FB1.1, C1.1, U5.4 (VIN), U9.4 (unused) |
| VBUS_FILTERED | FB1.2, U8.5 (VBUS), C20.1 |
| VBAT | BAT1.+, U5.3 (VBAT), U6.1 (VIN), C19.1 |
| VDD_3V3 | U6.5 (VOUT), C9.1, C2.1, RP2040.VDD×6, ESP32-C3.VDD, U2.VCC, U4.VDD, R1.2, R2.2, R7.2, R8.2, R9.2, R10.2, R11.2, R12.2 |
| VDD_1V8 | U7.5 (VOUT), C10.1, RP2040.VDD_CORE |
| GND | All IC ground pins, J1.4, BAT1.-, C1.2, C2.2, C3-C19.2, C20.2 |

### 4.2 Signal Nets

| Net Name | Source | Destination(s) |
|----------|--------|-----------------|
| QSPI_SCLK | RP2040.GPIO0 | U9.6 (W25Q16 CLK) |
| QSPI_SS | RP2040.GPIO1 | U9.1 (W25Q16 CS) |
| QSPI_SD0 | RP2040.GPIO2 | U9.5 (W25Q16 D0) |
| QSPI_SD1 | RP2040.GPIO3 | U9.2 (W25Q16 D1) |
| QSPI_SD2 | RP2040.GPIO4 | U9.3 (W25Q16 D2) |
| QSPI_SD3 | RP2040.GPIO5 | U9.7 (W25Q16 D3) |
| ENC_A | RP2040.GPIO6 | ENC1.A |
| ENC_B | RP2040.GPIO7 | ENC1.B |
| ENC_BTN | RP2040.GPIO8 | ENC1.SW |
| KILL_SW_N | RP2040.GPIO9 | SW1.NO, U8.SEL |
| I2C0_SDA | RP2040.GPIO10 | U4.SDA, R7.1 |
| I2C0_SCL | RP2040.GPIO11 | U4.SCL, R8.1 |
| UART0_TX | RP2040.GPIO12 | U3.RXD0 (ESP32-C3) |
| UART0_RX | RP2040.GPIO13 | U3.TXD0 (ESP32-C3) |
| ESP_RST_N | RP2040.GPIO14 | U3.EN (ESP32-C3) |
| LED_DATA | RP2040.GPIO15 | D1.DIN (WS2812B) |
| SPI0_CSn | RP2040.GPIO16 | U2.1 (W25Q128 CS) |
| SPI0_SCK | RP2040.GPIO17 | U2.6 (W25Q128 CLK) |
| SPI0_TX | RP2040.GPIO18 | U2.5 (W25Q128 DI) |
| SPI0_RX | RP2040.GPIO19 | U2.2 (W25Q128 DO) |
| USB_MUX_CTRL | RP2040.GPIO20 | U8.SEL (TS3USB221A) |
| BAT_SENSE | RP2040.GPIO21 | R3/R4 junction |
| LED_EN | RP2040.GPIO22 | D1.VDD (via MOSFET) |
| USB_DP | RP2040.USB_DP | U8.D1+ (TS3USB221A) |
| USB_DM | RP2040.USB_DM | U8.D1- (TS3USB221A) |
| USB_HOST_DP | U8.COM+ | J1.D+ (USB-A) |
| USB_HOST_DM | U8.COM- | J1.D- (USB-A) |

### 4.3 I²C Bus

| Device | I²C Address | Function |
|--------|------------|----------|
| SSD1306 | 0x3C | OLED display controller |

### 4.4 SPI Bus 0 (Payload Flash)

| Signal | Pin (RP2040) | Pin (W25Q128) |
|--------|-------------|---------------|
| CS | GPIO16 | Pin 1 (CS) |
| SCK | GPIO17 | Pin 6 (CLK) |
| MOSI | GPIO18 | Pin 5 (DI) |
| MISO | GPIO19 | Pin 2 (DO) |

### 4.5 QSPI Bus (Boot Flash)

| Signal | Pin (RP2040) | Pin (W25Q16) |
|--------|-------------|---------------|
| SCLK | GPIO0 | Pin 6 (CLK) |
| SS | GPIO1 | Pin 1 (CS) |
| SD0 | GPIO2 | Pin 5 (D0) |
| SD1 | GPIO3 | Pin 2 (D1) |
| SD2 | GPIO4 | Pin 3 (D2) |
| SD3 | GPIO5 | Pin 7 (D3) |

### 4.6 UART (ESP32-C3)

| Signal | Pin (RP2040) | Pin (ESP32-C3) | Direction |
|--------|-------------|----------------|-----------|
| TX | GPIO12 | RXD0 | RP2040 → ESP32-C3 |
| RX | GPIO13 | TXD0 | ESP32-C3 → RP2040 |
| RST | GPIO14 | EN | RP2040 → ESP32-C3 |

## 5. Design Notes

### 5.1 USB Data Path

The USB data path includes a hardware kill switch that physically disconnects D+ and D- lines:

```
RP2040 USB_DP/DM → TS3USB221A → USB-A D+/D-
                       ↑
                   KILL_SW (SW1)
                   LOW = pass (normal operation)
                   HIGH = disconnect (kill mode)
```

When SW1 is in the KILL position:
- TS3USB221A connects COM to D2 (NC), disconnecting USB data
- VBUS power remains connected for LiPo charging
- RP2040 GPIO9 reads LOW, firmware enters safe mode
- No USB enumeration possible regardless of software state

### 5.2 Battery Charging

The MCP73831 charges the LiPo from USB VBUS:
- Charge current: 470 mA (R5 = 27.4 kΩ)
- Charge voltage: 4.2 V (CC/CV profile)
- Trickle charge: 50 mA (for deeply discharged batteries)
- Charge complete indication via STAT pin (GPIO, optional)
- When USB disconnected, battery powers system via diode OR

### 5.3 Power Selection

Power is OR'd between USB VBUS and battery:
- USB connected: VBUS powers system and charges battery
- USB disconnected: Battery powers system via Schottky diode
- LDO efficiency: ~60% at 500 mA load (acceptable for USB-powered device)
- Battery life: ~8 hours active HID, ~250 hours standby

### 5.4 Crystal Oscillator

RP2040 uses an external 12 MHz crystal (Y1) as the primary clock source:
- Internal PLL multiplies to 133 MHz for core
- USB FS requires 48 MHz (derived from PLL)
- Crystal load capacitors: C16, C17 = 27 pF (for 12.5 pF load capacitance)
- Crystal ESR < 100 Ω for reliable startup

### 5.5 ESP32-C3 Antenna

The ESP32-C3-MINI-1 module includes an on-board PCB antenna:
- 2.4 GHz inverted-F antenna
- No external antenna connector needed
- Keep copper-free area around module per datasheet
- Module antenna end must extend beyond PCB edge or have ground clearance