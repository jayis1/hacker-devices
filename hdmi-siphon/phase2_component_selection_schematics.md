# Phase 2: Component Selection & Schematics — HDMI-Siphon

**Author: jayis1**  
**Version: 1.0.0**

---

## 1. Bill of Materials

| RefDes | Part Number | Description | Package | Qty | Unit Price (100) |
|---|---|---|---|---|---|
| U1 | TFP401APZP | HDMI/DVI Receiver, 165 MHz, 24-bit RGB | HTQFP-100 | 1 | $4.50 |
| U2 | TFP410PAP | HDMI/DVI Transmitter, 165 MHz, 24-bit RGB | HTQFP-64 | 1 | $4.20 |
| U3 | iCE40UP5K-UWG30 | Ultra-low-power FPGA, 5280 LUTs | WLCSP-30 | 1 | $7.80 |
| U4 | ESP32-S3-WROOM-1 | Dual-core MCU, WiFi, BLE | SMD module | 1 | $3.50 |
| U5 | IS42S16160J-6TL | 32 MB SDRAM, 166 MHz | TSOPII-54 | 1 | $2.80 |
| U6 | 24LC02BT-I/OT | 2 Kbit I²C EEPROM (EDID) | SOT-23-5 | 1 | $0.35 |
| U7 | TXS0108EPWR | 8-bit bidirectional level shifter | TSSOP-20 | 1 | $1.20 |
| U8 | TCA9548APWR | 8-channel I²C mux/bus switch | TSSOP-24 | 1 | $1.50 |
| U9 | CH340C | USB-to-UART bridge | SOP-16 | 1 | $0.60 |
| U10 | MCP73831T-2ACI/OT | Li-Po battery charger IC | SOT-23-5 | 1 | $0.40 |
| U11 | XC6206P332MR | 3.3V LDO regulator | SOT-23-3 | 1 | $0.30 |
| U12 | XC6206P182MR | 1.8V LDO regulator | SOT-23-3 | 1 | $0.30 |
| J1, J2 | HDMI-A-F | HDMI Type A female receptacle, SMT | SMT | 2 | $0.80 |
| J3 | USB-C-F-16P | USB-C female connector, 16-pin | SMT | 1 | $0.50 |
| J4 | 101-00397-68 | microSD push-push card slot | SMT | 1 | $0.55 |
| J5 | SMD-ANT-2.4GHz | 2.4 GHz chip antenna, 1.5 dBi | SMD 3216 | 1 | $0.15 |
| BT1 | LP503048 | 3.7V Li-Po battery, 300 mAh | Prismatic | 1 | $2.50 |
| Q1, Q2 | BSS138 | N-channel MOSFET, CEC level shift | SOT-23 | 2 | $0.10 |
| SW1, SW2 | TS-1145 | Tactile switch, 4.5mm, 160gf | SMT | 2 | $0.08 |
| LED1 | SK6812-MINI | RGB LED (NeoPixel-compatible) | SOT-23-6 | 1 | $0.35 |
| R1-R20 | CR0402 series | 0402 thick-film resistors | 0402 | 20 | $0.02 |
| C1-C20 | CC0402 series | 0402 MLCC capacitors | 0402 | 20 | $0.02 |
| L1 | BLM18PG331SN1 | 330Ω ferrite bead, 2A | 0603 | 1 | $0.10 |

## 2. Pin Multiplexing Tables

### 2.1 ESP32-S3 Pin Assignment

| Pin | Function | Connected To | Notes |
|---|---|---|---|
| GPIO1 | SPI_MOSI | FPGA SPI | 3.3V logic |
| GPIO2 | SPI_MISO | FPGA SPI | 3.3V logic |
| GPIO3 | SPI_SCK | FPGA SPI | 3.3V logic, 20 MHz max |
| GPIO4 | SPI_CS | FPGA SPI_SS | Active low, pull-up to 3.3V |
| GPIO5 | FPGA_DONE | FPGA DONE pin | Input, high = configured |
| GPIO6 | FPGA_CRESET | FPGA CRESET_B | Output, active low |
| GPIO7 | FPGA_CDONE | FPGA CDONE | Input, high = config done |
| GPIO8 | I2C_SDA | TCA9548 SDA, 24LC02 SDA | Open-drain, 10k pull-up to 3.3V |
| GPIO9 | I2C_SCL | TCA9548 SCL, 24LC02 SCL | Open-drain, 10k pull-up to 3.3V |
| GPIO10 | SD_CS | microSD card CS | Active low, pull-up to 3.3V |
| GPIO11 | SD_MOSI | microSD card DI | 3.3V logic |
| GPIO12 | SD_MISO | microSD card DO | 3.3V logic |
| GPIO13 | SD_SCK | microSD card CLK | 20 MHz max |
| GPIO14 | CEC_RX | HDMI CEC bus (through BSS138) | Open-drain input, 27k pull-up to 3.3V |
| GPIO15 | CEC_TX | HDMI CEC bus (through BSS138) | Open-drain output |
| GPIO16 | FPGA_IRQ | FPGA interrupt output | Input, active high |
| GPIO17 | FPGA_RDY | FPGA ready signal | Input, high = FPGA ready |
| GPIO18 | TXD | CH340 RXD | UART TX, 3.3V |
| GPIO19 | RXD | CH340 TXD | UART RX, 3.3V |
| GPIO20 | BAT_ADC | BAT voltage divider (100k/47k) | ADC1 channel, 0-2.4V range |
| GPIO21 | LED_DATA | SK6812 data input | NeoPixel protocol, 800 kHz |
| GPIO42 | BTN_CONFIG | SW1 (config) | Pull-up, active low |
| GPIO43 | BTN_CAPTURE | SW2 (capture) | Pull-up, active low |

### 2.2 TFP401 (HDMI RX) Critical Pins

| Pin | Name | Function | Connection |
|---|---|---|---|
| 1-8 | R0-R7 | Red data outputs (0=LSB) | FPGA IO_39-46 |
| 9-16 | G0-G7 | Green data outputs | FPGA IO_47-54 |
| 17-24 | B0-B7 | Blue data outputs | FPGA IO_55-62 |
| 25 | HSYNC | Horizontal sync output | FPGA IO_63 |
| 26 | VSYNC | Vertical sync output | FPGA IO_64 |
| 27 | DE | Data enable output | FPGA IO_65 |
| 28 | PCLK | Pixel clock output | FPGA IO_66 |
| 29 | SCL | DDC I²C clock (from HDMI) | TCA9548 channel 0 |
| 30 | SDA | DDC I²C data (from HDMI) | TCA9548 channel 0 |
| 31 | OE_B | Output enable (active low) | GND (always enabled) |
| 32 | PD_B | Power-down (active low) | 3.3V via 10k pull-up |
| 33-36 | STAG | Stagger control | GND (no stagger) |
| 37-40 | CTL1-3 | Control signals | NC |
| 41 | REXT | External resistor (1%) | 5.1kΩ to GND |
| 42-50 | VCC | Power (3.3V) | Decouple each with 100nF |
| 51-55 | GND | Ground | Common ground plane |
| 56-64 | NC | Not connected | Float |
| 65-76 | GND | Ground | Common ground plane |
| 77-84 | NC | Not connected | Float |
| 85-92 | GND | Ground | Common ground plane |
| 93-100 | NC | Not connected | Float |

### 2.3 TFP410 (HDMI TX) Critical Pins

| Pin | Name | Function | Connection |
|---|---|---|---|
| 1-8 | B0-B7 | Blue data inputs | FPGA IO_83-90 |
| 9-16 | G0-G7 | Green data inputs | FPGA IO_75-82 |
| 17-24 | R0-R7 | Red data inputs | FPGA IO_67-74 |
| 25 | HSYNC | Horizontal sync input | FPGA IO_91 |
| 26 | VSYNC | Vertical sync input | FPGA IO_92 |
| 27 | DE | Data enable input | FPGA IO_93 |
| 28 | PCLK | Pixel clock input | FPGA IO_94 |
| 29 | SCL | DDC I²C clock (to display) | TCA9548 channel 1 |
| 30 | SDA | DDC I²C data (to display) | TCA9548 channel 1 |
| 31 | PD_B | Power-down (active low) | 3.3V via 10k pull-up |
| 32 | REXT | External resistor (1%) | 5.1kΩ to GND |
| 33-36 | CTL1-3 | Control signals | NC |
| 37-40 | GND | Ground | Common ground plane |
| 41-48 | GND | Ground | Common ground plane |
| 49-56 | NC | Not connected | Float |
| 57-64 | VCC | Power (3.3V) | Decouple each with 100nF |

### 2.4 FPGA iCE40UP5K Ball Map (WLCSP-30)

| Ball | Name | Connection |
|---|---|---|
| A1 | VCC_IO | 3.3V |
| A2 | IO_0 | SDRAM DQ0 |
| A3 | IO_1 | SDRAM DQ1 |
| A4 | IO_2 | SDRAM DQ2 |
| A5 | GND | Ground |
| B1 | IO_3 | SDRAM DQ3 |
| B2 | IO_4 | SDRAM DQ4 |
| B3 | IO_5 | SDRAM DQ5 |
| B4 | IO_6 | SDRAM DQ6 |
| B5 | IO_7 | SDRAM DQ7 |
| C1 | IO_8 | SDRAM DQ8 |
| C2 | IO_9 | SDRAM DQ9 |
| C3 | IO_10 | SDRAM DQ10 |
| C4 | IO_11 | SDRAM DQ11 |
| C5 | IO_12 | SDRAM DQ12 |
| D1 | VCC_IO | 3.3V |
| D2 | IO_13 | SDRAM DQ13 |
| D3 | IO_14 | SDRAM DQ14 |
| D4 | IO_15 | SDRAM DQ15 |
| D5 | GND | Ground |
| E1 | IO_16 | SDRAM BA0 |
| E2 | IO_17 | SDRAM BA1 |
| E3 | IO_18 | SDRAM A0 |
| E4 | IO_19 | SDRAM A1 |
| E5 | IO_20 | SDRAM A2 |
| F1 | IO_21 | SDRAM A3 |
| F2 | IO_22 | SDRAM A4 |
| F3 | IO_23 | SDRAM A5 |
| F4 | IO_24 | SDRAM A6 |
| F5 | I/O_25 | SDRAM A7 |

Note: The iCE40UP5K in WLCSP-30 has limited I/O. The pixel data is routed using an external I/O expander or serialization scheme. In practice, this FPGA manages SDRAM addressing and control while pixel data flows through a simpler scheme. For the pin count, we use the FPGA for SDRAM control, CEC, SPI slave, and control signals only; the pixel data path uses the FPGA's limited I/O for pixel clock and syncs, but the actual 24-bit pixel data flows TFP401→TFP410 through a dedicated 24-bit bus buffered by the FPGA's I/O.

## 3. Decoupling Network

| Domain | Cap Value | Qty | Placement |
|---|---|---|---|
| VCC_3V3 | 100 nF 0402 X7R | 6 | One near each power pin of TFP401, TFP410, ESP32, FPGA, CH340, TCA9548 |
| VCC_3V3 | 10 µF 0603 X7R | 2 | One near power input, one center of board |
| VCC_1V8 | 100 nF 0402 X7R | 2 | Near FPGA and SDRAM |
| VCC_1V8 | 10 µF 0603 X7R | 1 | Near LDO output |
| VCC_5V0 | 10 µF 0603 X7R | 2 | Near HDMI connectors, one near USB-C |
| VCC_5V0 | 100 nF 0402 X7R | 2 | Near MCP73831 |

## 4. EDID Netlist

The EDID EEPROM (U6, 24LC02) is connected to the TCA9548 I²C mux. The mux selects which I²C bus the ESP32 communicates with:

- Channel 0: TFP401 DDC (HDMI IN)
- Channel 1: TFP410 DDC (HDMI OUT)  
- Channel 2: EDID EEPROM (24LC02)
- Channels 3-7: Reserved

Default operation:
1. ESP32 opens mux channels 0 and 1 (HDMI IN and OUT DDC)
2. TFP401 reads the EDID from the display via TFP410's I²C bus
3. For EDID injection: ESP32 writes to 24LC02, then opens only channel 0 so TFP401 reads the injected EDID from the local EEPROM instead of the display

## 5. USB-C Power & Debug

The USB-C connector (J3) provides:
- VBUS → MCP73831 (battery charger, up to 500 mA)
- D+/D- → CH340C (USB-to-serial for ESP32 console)
- CC1/CC2 → 5.1kΩ pulldown (advertise as sink)

**Author: jayis1**
