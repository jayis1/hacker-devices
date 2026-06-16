# Phase 2: Component Selection & Schematics — CAN Creeper

## 2.0 Bill of Materials (BOM)

| Ref | Qty | Part Number | Manufacturer | Description | Package | Unit Cost (USD) | Ext Cost |
|---|---|---|---|---|---|---|---|
| U1 | 1 | nRF52840-QIAA-R7 | Nordic Semiconductor | BLE 5.2 SoC, Cortex-M4F, 1MB Flash, 256KB RAM | QFN-73 (7×7mm) | $5.20 | $5.20 |
| U2,U3 | 2 | MCP2518FD-I/SL | Microchip | CAN FD Controller, SPI, 31 FIFO | SOIC-14 | $2.85 | $5.70 |
| U4,U5 | 2 | TJA1445AT/0Z | NXP | CAN FD Transceiver, 5 Mbps, Standby | SOIC-8 | $1.10 | $2.20 |
| U6 | 1 | TPS63070RNMR | TI | Buck-Boost Converter, 2A, 3.3V out | VQFN-15 | $2.40 | $2.40 |
| U7 | 1 | TPS7A4701RGWT | TI | Ultra-low-noise LDO, 1A, 1.8V out | VQFN-20 | $3.80 | $3.80 |
| U8 | 1 | AP2112K-5.0TRG1 | Diodes Inc | LDO, 600mA, 5.0V out | SOT-25 | $0.18 | $0.18 |
| U9 | 1 | TP4056 | TPOWER | Li-Ion Charger, 1A, 4.2V | SOP-8 | $0.15 | $0.15 |
| U10 | 1 | DW01-P | Fortune | Li-Ion Protection IC | SOT-23-6 | $0.08 | $0.08 |
| U11 | 1 | FS8205A | Fortune | Dual N-MOSFET (protection) | TSSOP-8 | $0.12 | $0.12 |
| U12 | 1 | APS6404L-3SQR-SN | AP Memory | 8MB QSPI PSRAM, 80 MHz | SOP-8 (150mil) | $1.60 | $1.60 |
| U13 | 1 | W25Q128JVSIQ | Winbond | 16MB NOR Flash, QSPI, 133 MHz | SOIC-8 (208mil) | $1.20 | $1.20 |
| U14 | 1 | SSD1306 | Solomon Systech | 128×64 OLED, I²C | 0.96" Module | $2.50 | $2.50 |
| Y1 | 1 | NX3225SA-32.000000MHZ-B4 | NDK | 32 MHz Crystal, 10ppm, 8pF | 3.2×2.5mm SMD | $0.45 | $0.45 |
| Y2 | 1 | FC-135 32.768kHz 12.5pF | Epson | 32.768 kHz Crystal, 20ppm | 3.2×1.5mm SMD | $0.30 | $0.30 |
| Y3,Y4 | 2 | ABM8G-40.000MHZ-18-D2Y-T | Abracon | 40 MHz Crystal, 20ppm, 18pF | 3.2×2.5mm SMD | $0.55 | $1.10 |
| D1 | 1 | MBR0520LT1G | ON Semi | Schottky Diode, 0.5A, 20V | SOD-123 | $0.10 | $0.10 |
| D2 | 1 | SMAJ5.0CA | Littelfuse | TVS Bidirectional, 5V, 400W | SMA | $0.15 | $0.15 |
| D3,D4 | 2 | PESD1CAN-U | NXP | ESD Protection CAN bus | SOT-323 | $0.25 | $0.50 |
| Q1,Q2 | 2 | 2N7002K | ON Semi | N-MOSFET, 60V, 300mA (termination switch) | SOT-23 | $0.08 | $0.16 |
| Q3 | 1 | DMG3415U-7 | Diodes Inc | P-MOSFET, -20V, -4A (power switch) | SOT-23 | $0.12 | $0.12 |
| LED1 | 1 | LTST-C191KRKT | Lite-On | Red LED, 0603 | 0603 | $0.05 | $0.05 |
| LED2 | 1 | LTST-C191KGKT | Lite-On | Green LED, 0603 | 0603 | $0.05 | $0.05 |
| LED3 | 1 | LTST-C191TBKT | Lite-On | Blue LED, 0603 | 0603 | $0.06 | $0.06 |
| J1 | 1 | USB-C 16-pin Receptacle | GCT USB4135 | USB 2.0 Type-C, 16-pin, SMT | SMT | $0.65 | $0.65 |
| J2 | 1 | DB9 Female, Right-Angle | NorComp 171-009-203L001 | D-Sub 9-pin, PCB mount | D-Sub | $1.20 | $1.20 |
| J3 | 1 | OBD-II J1962 Male | Custom | 16-pin OBD-II plug with pigtail | Pigtail | $3.50 | $3.50 |
| J4 | 1 | JST-PH 2.0mm 2-pin | JST B2B-PH-K-S | Battery connector | TH | $0.15 | $0.15 |
| J5 | 1 | Tag-Connect TC2050-IDC-NL | Tag-Connect | 6-pin SWD debug | Footprint only | $0.00 | $0.00 |
| SW1 | 1 | SKRPACE010 | Alps Alpine | Tactile switch, SPST | SMD 3.5×2.9mm | $0.20 | $0.20 |
| SW2 | 1 | JS202011CQN | C&K | Slide switch, SPDT | SMD | $0.35 | $0.35 |
| R1,R2 | 2 | 120Ω ±1% 0805 | — | CAN termination resistors | 0805 | $0.03 | $0.06 |
| R3,R4 | 2 | 5.1kΩ ±5% 0402 | — | USB CC pull-down | 0402 | $0.01 | $0.02 |
| R5,R6 | 2 | 22Ω ±5% 0402 | — | USB D+/D- series | 0402 | $0.01 | $0.02 |
| R7,R8 | 2 | 10kΩ ±5% 0402 | — | I²C pull-up | 0402 | $0.01 | $0.02 |
| R9,R10 | 2 | 1kΩ ±5% 0402 | — | LED current limit | 0402 | $0.01 | $0.02 |
| R11 | 1 | 100kΩ ±5% 0402 | — | nRESET pull-up | 0402 | $0.01 | $0.01 |
| R12,R13 | 2 | 1MΩ ±5% 0402 | — | 32kHz crystal bias | 0402 | $0.01 | $0.02 |
| R14,R15 | 2 | 10kΩ ±5% 0402 | — | MCP2518FD INT pull-up | 0402 | $0.01 | $0.02 |
| R16 | 1 | 1.2kΩ ±1% 0402 | — | TP4056 PROG (1A charge) | 0402 | $0.01 | $0.01 |
| R17,R18 | 2 | 0Ω jumper 0805 | — | CAN termination enable jumpers | 0805 | $0.01 | $0.02 |
| C1,C2 | 2 | 12pF ±5% 0402 NP0 | — | 32 MHz crystal load | 0402 | $0.02 | $0.04 |
| C3,C4 | 2 | 12.5pF ±5% 0402 NP0 | — | 32.768 kHz crystal load | 0402 | $0.02 | $0.04 |
| C5,C6 | 2 | 18pF ±5% 0402 NP0 | — | 40 MHz crystal load (×2) | 0402 | $0.02 | $0.08 |
| C7-C10 | 4 | 100nF ±10% 0402 X7R | — | Decoupling (nRF52840) | 0402 | $0.01 | $0.04 |
| C11 | 1 | 4.7µF ±10% 0603 X5R | — | nRF52840 DEC4 | 0603 | $0.04 | $0.04 |
| C12 | 1 | 10µF ±10% 0805 X5R | — | nRF52840 DEC5 | 0805 | $0.06 | $0.06 |
| C13,C14 | 2 | 100nF ±10% 0402 X7R | — | MCP2518FD decoupling (×2) | 0402 | $0.01 | $0.04 |
| C15,C16 | 2 | 100nF ±10% 0402 X7R | — | TJA1445 decoupling (×2) | 0402 | $0.01 | $0.04 |
| C17,C18 | 2 | 4.7µF ±10% 0603 X5R | — | TJA1445 VCC bulk (×2) | 0603 | $0.04 | $0.08 |
| C19,C20 | 2 | 10µF ±10% 0805 X5R | — | TPS63070 input/output | 0805 | $0.06 | $0.12 |
| C21 | 1 | 22µF ±20% 0805 X5R | — | TPS63070 output bulk | 0805 | $0.08 | $0.08 |
| C22 | 1 | 100nF ±10% 0402 X7R | — | TPS7A4701 input | 0402 | $0.01 | $0.01 |
| C23 | 1 | 10µF ±10% 0805 X5R | — | TPS7A4701 output | 0805 | $0.06 | $0.06 |
| C24 | 1 | 100nF ±10% 0402 X7R | — | AP2112K input | 0402 | $0.01 | $0.01 |
| C25 | 1 | 1µF ±10% 0402 X5R | — | AP2112K output | 0402 | $0.02 | $0.02 |
| C26 | 1 | 10µF ±10% 0805 X5R | — | TP4056 VCC | 0805 | $0.06 | $0.06 |
| C27,C28 | 2 | 100nF ±10% 0402 X7R | — | PSRAM decoupling | 0402 | $0.01 | $0.02 |
| C29,C30 | 2 | 100nF ±10% 0402 X7R | — | NOR Flash decoupling | 0402 | $0.01 | $0.02 |
| C31 | 1 | 4.7µF ±10% 0603 X5R | — | OLED decoupling | 0603 | $0.04 | $0.04 |
| L1 | 1 | 2.2µH ±20% 3.5A | Bourns SRP4020 | TPS63070 inductor | 4×4mm SMD | $0.45 | $0.45 |
| L2 | 1 | 1.0µH ±20% 2.5A | Murata LQH3NP | TPS7A4701 noise filter | 3×3mm SMD | $0.30 | $0.30 |
| FB1,FB2 | 2 | BLM18AG102SN1D | Murata | Ferrite bead, 1kΩ @ 100MHz | 0603 | $0.05 | $0.10 |

**Total BOM Cost (Qty 1): ~$42.50 USD** — well under $75 target.

## 2.1 Component Justifications

### nRF52840-QIAA-R7 (U1)
Selected for its combination of BLE 5.2, Cortex-M4F @ 64 MHz, 1 MB Flash, 256 KB RAM, hardware SPI/QSPI with EasyDMA, and USB 2.0 Full Speed peripheral. The QIAA package (QFN-73, 7×7mm) provides all 48 GPIOs needed. The integrated DC/DC converter (DCDCEN) reduces active current from ~6mA to ~3mA at 3.3V.

### MCP2518FD-I/SL (U2, U3)
Microchip's standalone CAN FD controller with SPI interface. Key features:
- ISO 11898-1:2015 compliant CAN FD
- 31 flexible FIFOs configurable as TX or RX
- 1 µs timestamping via internal free-running timer (TEF)
- Automatic retransmission on arbitration loss
- Bus monitoring mode (no ACK, no error flags) for passive sniffing
- SPI clock up to 20 MHz (we run at 8 MHz for nRF52840 compatibility)
- 3.0V–5.5V VDD range (we run at 3.3V)

### TJA1445AT/0Z (U4, U5)
NXP's CAN FD transceiver with:
- ISO 11898-2:2016 compliant up to 5 Mbps CAN FD
- VIO pin for 3.3V logic interface (direct connection to nRF52840)
- Standby mode with remote wake-up capability
- ±58V bus fault protection on CAN-H/CAN-L
- Undervoltage detection on VCC and VIO
- SPLIT pin for stabilized recessive level in terminated networks

### TPS63070RNMR (U6)
TI's buck-boost converter handles the wide LiPo voltage range (3.0V–4.2V) and USB VBUS (5V) to produce a stable 3.3V rail. 2A output current is more than sufficient for the ~150mA peak load. The device automatically transitions between buck, boost, and buck-boost modes.

### APS6404L-3SQR-SN (U12)
AP Memory's 8MB QSPI PSRAM provides the frame buffer capacity needed. At 80 MHz QSPI with 4-bit I/O, theoretical throughput is 40 MB/s — far exceeding the ~1 MB/s needed for CAN frame logging. The device supports XIP (execute-in-place) and wraps at 16-byte boundaries.

### W25Q128JVSIQ (U13)
Winbond's 16MB NOR Flash stores DBC files, injection scripts, and OTA firmware images. 133 MHz QSPI with QPI mode. Uniform 4KB sectors for efficient DBC file management.

## 2.2 Pinout Tables

### nRF52840 Pin Assignments

| Pin | Port | Function | Connected To | Notes |
|---|---|---|---|---|
| 1 | P0.00 | XL1 | Y1 (32 MHz) | HF crystal |
| 2 | P0.01 | XL2 | Y1 (32 MHz) | HF crystal |
| 3 | P0.02 | AIN0 | — | NC (analog input available) |
| 4 | P0.03 | AIN1 | — | NC |
| 5 | P0.04 | AIN2 | VBAT_SENSE (via 1:2 divider) | Battery voltage monitor |
| 6 | P0.05 | AIN3 | — | NC |
| 7 | P0.06 | — | — | NC |
| 8 | P0.07 | — | — | NC |
| 9 | P0.08 | — | — | NC |
| 10 | P0.09 | NFC1 | — | NC (NFC not used) |
| 11 | P0.10 | NFC2 | — | NC |
| 12 | P0.11 | SPIM1_MOSI | U3 (MCP2518FD #1) SDI | CAN1 SPI MOSI |
| 13 | P0.12 | SPIM1_MISO | U3 (MCP2518FD #1) SDO | CAN1 SPI MISO |
| 14 | P0.13 | QSPI_CSN0 | U12 (APS6404L) CE# | PSRAM chip select |
| 15 | P0.14 | SPIM1_SCK | U3 (MCP2518FD #1) SCK | CAN1 SPI clock |
| 16 | P0.15 | — | U3 (MCP2518FD #1) INT | CAN1 interrupt (GPIO input) |
| 17 | P0.16 | — | U2 (MCP2518FD #0) INT | CAN0 interrupt (GPIO input) |
| 18 | P0.17 | SPIM0_CSN | U2 (MCP2518FD #0) CS | CAN0 SPI chip select |
| 19 | P0.18 | nRESET | SWD connector, R11 (100k pull-up) | Configurable reset pin |
| 20 | P0.19 | QSPI_CSN1 | U13 (W25Q128JV) CS# | NOR Flash chip select |
| 21 | P0.20 | QSPI_IO0/D0 | U12 IO0, U13 IO0 | QSPI data 0 |
| 22 | P0.21 | QSPI_IO1/D1 | U12 IO1, U13 IO1 | QSPI data 1 |
| 23 | P0.22 | SPIM1_CSN | U3 (MCP2518FD #1) CS | CAN1 SPI chip select |
| 24 | P0.23 | QSPI_IO2/D2 | U12 IO2, U13 IO2 | QSPI data 2 |
| 25 | P0.24 | QSPI_IO3/D3 | U12 IO3, U13 IO3 | QSPI data 3 |
| 26 | P0.25 | QSPI_CLK | U12 CLK, U13 CLK | QSPI clock |
| 27 | P0.26 | TWIM0_SDA | U14 (SSD1306) SDA, R7 (10k pull-up) | I²C data |
| 28 | P0.27 | TWIM0_SCL | U14 (SSD1306) SCL, R8 (10k pull-up) | I²C clock |
| 29 | P0.28 | SPIM0_MISO | U2 (MCP2518FD #0) SDO | CAN0 SPI MISO |
| 30 | P0.29 | SPIM0_MOSI | U2 (MCP2518FD #0) SDI | CAN0 SPI MOSI |
| 31 | P0.30 | SPIM0_SCK | U2 (MCP2518FD #0) SCK | CAN0 SPI clock |
| 32 | P0.31 | — | LED1 (Red, via R9) | Status LED |
| 33 | P1.00 | — | U4 (TJA1445 #0) STB | CAN0 standby control |
| 34 | P1.01 | — | U5 (TJA1445 #1) STB | CAN1 standby control |
| 35 | P1.02 | — | Q1 gate (termination FET CAN0) | CAN0 120Ω term enable |
| 36 | P1.03 | — | Q2 gate (termination FET CAN1) | CAN1 120Ω term enable |
| 37 | P1.04 | — | LED2 (Green) | Activity LED |
| 38 | P1.05 | — | LED3 (Blue) | BLE connected LED |
| 39 | P1.06 | — | SW1 (tactile button) | User button (active low) |
| 40 | P1.07 | — | — | NC |
| 41 | P1.08 | — | — | NC |
| 42 | P1.09 | — | — | NC |
| 43 | P1.10 | — | U4 (TJA1445 #0) EN | CAN0 enable |
| 44 | P1.11 | — | U5 (TJA1445 #1) EN | CAN1 enable |
| 45 | P1.12 | — | — | NC |
| 46 | P1.13 | — | — | NC |
| 47 | P1.14 | — | — | NC |
| 48 | P1.15 | — | — | NC |
| 49 | VDD_nRF | Power | 3.3V rail | Decoupled with C7-C10 |
| 50 | VDD_nRF | Power | 3.3V rail | |
| 51 | DEC4 | Decoupling | C11 (4.7µF) to GND | Internal regulator |
| 52 | DEC5 | Decoupling | C12 (10µF) to GND | Internal regulator |
| 53 | DCDCEN | Power config | 3.3V (enabled) | Enable DC/DC converter |
| 54 | DEC6 | Decoupling | 100nF to GND | |
| 55 | DEC7 | Decoupling | 100nF to GND | |
| 56 | VSS | Ground | GND plane | |
| 57 | VSS | Ground | GND plane | |
| 58 | VBUS | USB power detect | USB-C VBUS (via divider) | USB detection |
| 59 | D+ | USB D+ | USB-C D+ (via R5 22Ω) | USB data |
| 60 | D- | USB D- | USB-C D- (via R6 22Ω) | USB data |
| 61 | VDD_USB | USB power | 3.3V rail | USB PHY supply |
| 62 | DECUSB | Decoupling | 100nF to GND | USB regulator |
| 63 | XC1 | LF crystal | Y2 (32.768 kHz) | LF crystal in |
| 64 | XC2 | LF crystal | Y2 (32.768 kHz) | LF crystal out |
| 65 | SWDIO | Debug | Tag-Connect pin 2 | SWD data |
| 66 | SWDCLK | Debug | Tag-Connect pin 4 | SWD clock |
| 67 | VSS | Ground | GND plane | Exposed pad |
| 68-73 | VSS | Ground | GND plane | Exposed pad (center) |

### MCP2518FD Pin Assignments (×2, identical for U2 and U3)

| Pin | Name | Connected To (U2=CAN0) | Connected To (U3=CAN1) | Notes |
|---|---|---|---|---|
| 1 | VDD | 3.3V rail, C13 | 3.3V rail, C14 | Decouple with 100nF |
| 2 | nCS | P0.17 (SPIM0_CSN) | P0.22 (SPIM1_CSN) | Active-low chip select |
| 3 | SCK | P0.30 (SPIM0_SCK) | P0.14 (SPIM1_SCK) | SPI clock |
| 4 | SDI | P0.29 (SPIM0_MOSI) | P0.11 (SPIM1_MOSI) | SPI data in (MOSI) |
| 5 | SDO | P0.28 (SPIM0_MISO) | P0.12 (SPIM1_MISO) | SPI data out (MISO) |
| 6 | nINT | P0.16 (GPIO) | P0.15 (GPIO) | Interrupt output, R14/R15 10k pull-up to 3.3V |
| 7 | TXCAN | U4 (TJA1445) TXD | U5 (TJA1445) TXD | CAN TX data to transceiver |
| 8 | RXCAN | U4 (TJA1445) RXD | U5 (TJA1445) RXD | CAN RX data from transceiver |
| 9 | CLKO | — | — | NC (clock output not used) |
| 10 | XTAL1 | Y3 (40 MHz) | Y4 (40 MHz) | Crystal input |
| 11 | XTAL2 | Y3 (40 MHz) | Y4 (40 MHz) | Crystal output |
| 12 | VSS | GND | GND | Ground |
| 13 | VSS | GND | GND | Ground |
| 14 | VIO | 3.3V rail | 3.3V rail | I/O supply voltage |

### TJA1445 Pin Assignments (×2, identical for U4 and U5)

| Pin | Name | Connected To (U4=CAN0) | Connected To (U5=CAN1) | Notes |
|---|---|---|---|---|
| 1 | TXD | U2 (MCP2518FD) TXCAN | U3 (MCP2518FD) TXCAN | Transmit data input |
| 2 | GND | GND plane | GND plane | Ground |
| 3 | VCC | 5V_CAN rail, C17 | 5V_CAN rail, C18 | 5V supply, decouple with 4.7µF |
| 4 | RXD | U2 (MCP2518FD) RXCAN | U3 (MCP2518FD) RXCAN | Receive data output |
| 5 | VIO | 3.3V rail | 3.3V rail | I/O level supply |
| 6 | CANL | J2 pin 2 (DB9) / J3 pin 14 (OBD-II) | J3 pin 14 (OBD-II) | CAN low bus line |
| 7 | CANH | J2 pin 7 (DB9) / J3 pin 6 (OBD-II) | J3 pin 6 (OBD-II) | CAN high bus line |
| 8 | STB | P1.00 (GPIO) | P1.01 (GPIO) | Standby control (low=normal, high=standby) |

### TJA1445 Additional Pins (not on SOIC-8, but relevant for variant)

The TJA1445AT variant includes:
- **EN** (enable, active high): Connected to P1.10 (CAN0) / P1.11 (CAN1)
- **nSTB** (standby, active low): Connected to P1.00 (CAN0) / P1.01 (CAN1)
- **SPLIT** (split termination): Connected to center tap of CAN termination network

### QSPI Bus Connections

| Signal | nRF52840 Pin | APS6404L Pin | W25Q128JV Pin |
|---|---|---|---|
| CLK | P0.25 | CLK (pin 6) | CLK (pin 6) |
| CS# | P0.13 (CSN0) | CE# (pin 1) | — |
| CS# | P0.19 (CSN1) | — | CS# (pin 1) |
| IO0 | P0.20 | IO0/SI (pin 5) | IO0/DI (pin 5) |
| IO1 | P0.21 | IO1/SO (pin 2) | IO1/DO (pin 2) |
| IO2 | P0.23 | IO2 (pin 3) | IO2/WP# (pin 3) |
| IO3 | P0.24 | IO3 (pin 7) | IO3/HOLD# (pin 7) |

## 2.3 Netlists

### Power Nets

```
Net: +3V3
  Sources: TPS63070 VOUT
  Loads: nRF52840 VDD_nRF (×2), VDD_USB, DCDCEN, VIO
         MCP2518FD #0 VDD, VIO
         MCP2518FD #1 VDD, VIO
         TJA1445 #0 VIO
         TJA1445 #1 VIO
         APS6404L VDD
         W25Q128JV VDD
         SSD1306 VDD
         Pull-up resistors: R7,R8,R11,R14,R15
         LED current-limit: R9,R10,R_blue

Net: +1V8
  Sources: TPS7A4701 VOUT
  Loads: APS6404L VDDQ (if separate I/O voltage)
         W25Q128JV VDDQ (if separate I/O voltage)
         (Note: APS6404L and W25Q128JV run at 3.3V in this design; 1.8V rail is provisioned for future)

Net: +5V_CAN
  Sources: AP2112K-5.0 VOUT
  Loads: TJA1445 #0 VCC
         TJA1445 #1 VCC

Net: VBAT
  Sources: LiPo battery (J4), TP4056 BAT
  Loads: TPS63070 VIN
         DW01-P VCC
         VBAT_SENSE divider (P0.04)

Net: VBUS
  Sources: USB-C VBUS pins
  Loads: TP4056 VCC
         nRF52840 VBUS (via 1:4.3 divider for detection)

Net: GND
  Common ground plane, all ICs, connectors, passives
```

### CAN Bus Nets

```
Net: CAN0_TXD
  Source: U2 (MCP2518FD #0) pin 7 (TXCAN)
  Destination: U4 (TJA1445 #0) pin 1 (TXD)

Net: CAN0_RXD
  Source: U4 (TJA1445 #0) pin 4 (RXD)
  Destination: U2 (MCP2518FD #0) pin 8 (RXCAN)

Net: CAN0_H
  Source: U4 (TJA1445 #0) pin 7 (CANH)
  Destination: J2 (DB9) pin 7, J3 (OBD-II) pin 6
  Protection: D3 (PESD1CAN-U) to GND

Net: CAN0_L
  Source: U4 (TJA1445 #0) pin 6 (CANL)
  Destination: J2 (DB9) pin 2, J3 (OBD-II) pin 14
  Protection: D3 (PESD1CAN-U) to GND

Net: CAN0_TERM
  Source: Q1 drain (2N7002K)
  Destination: R1 (120Ω) → CAN0_H, R2 (120Ω) → CAN0_L
  Control: Q1 gate ← P1.02 via 10k series resistor

Net: CAN1_TXD
  Source: U3 (MCP2518FD #1) pin 7 (TXCAN)
  Destination: U5 (TJA1445 #1) pin 1 (TXD)

Net: CAN1_RXD
  Source: U5 (TJA1445 #1) pin 4 (RXD)
  Destination: U3 (MCP2518FD #1) pin 8 (RXCAN)

Net: CAN1_H
  Source: U5 (TJA1445 #1) pin 7 (CANH)
  Destination: J3 (OBD-II) pin 6 (shared with CAN0 via jumper)
  Protection: D4 (PESD1CAN-U) to GND

Net: CAN1_L
  Source: U5 (TJA1445 #1) pin 6 (CANL)
  Destination: J3 (OBD-II) pin 14 (shared with CAN0 via jumper)
  Protection: D4 (PESD1CAN-U) to GND
```

### SPI Nets

```
Net: SPI0_SCK
  Source: nRF52840 P0.30
  Destination: U2 (MCP2518FD #0) pin 3 (SCK)

Net: SPI0_MOSI
  Source: nRF52840 P0.29
  Destination: U2 (MCP2518FD #0) pin 4 (SDI)

Net: SPI0_MISO
  Source: U2 (MCP2518FD #0) pin 5 (SDO)
  Destination: nRF52840 P0.28

Net: SPI0_nCS
  Source: nRF52840 P0.17
  Destination: U2 (MCP2518FD #0) pin 2 (nCS)

Net: SPI1_SCK
  Source: nRF52840 P0.14
  Destination: U3 (MCP2518FD #1) pin 3 (SCK)

Net: SPI1_MOSI
  Source: nRF52840 P0.11
  Destination: U3 (MCP2518FD #1) pin 4 (SDI)

Net: SPI1_MISO
  Source: U3 (MCP2518FD #1) pin 5 (SDO)
  Destination: nRF52840 P0.12

Net: SPI1_nCS
  Source: nRF52840 P0.22
  Destination: U3 (MCP2518FD #1) pin 2 (nCS)
```

### QSPI Nets

```
Net: QSPI_CLK
  Source: nRF52840 P0.25
  Destination: U12 (APS6404L) pin 6, U13 (W25Q128JV) pin 6
  Series termination: 33Ω at source

Net: QSPI_IO0
  Source: nRF52840 P0.20
  Destination: U12 pin 5, U13 pin 5

Net: QSPI_IO1
  Source: nRF52840 P0.21
  Destination: U12 pin 2, U13 pin 2

Net: QSPI_IO2
  Source: nRF52840 P0.23
  Destination: U12 pin 3, U13 pin 3

Net: QSPI_IO3
  Source: nRF52840 P0.24
  Destination: U12 pin 7, U13 pin 7

Net: QSPI_CSN_PSRAM
  Source: nRF52840 P0.13
  Destination: U12 (APS6404L) pin 1

Net: QSPI_CSN_FLASH
  Source: nRF52840 P0.19
  Destination: U13 (W25Q128JV) pin 1
```

### I²C Nets

```
Net: I2C_SDA
  Source: nRF52840 P0.26
  Destination: U14 (SSD1306) SDA
  Pull-up: R7 (10kΩ) to +3V3

Net: I2C_SCL
  Source: nRF52840 P0.27
  Destination: U14 (SSD1306) SCL
  Pull-up: R8 (10kΩ) to +3V3
```

### USB Nets

```
Net: USB_DPLUS
  Source: nRF52840 D+ (pin 59)
  Destination: USB-C D+ (J1 pin A6/B6)
  Series: R5 (22Ω)

Net: USB_DMINUS
  Source: nRF52840 D- (pin 60)
  Destination: USB-C D- (J1 pin A7/B7)
  Series: R6 (22Ω)

Net: USB_VBUS
  Source: USB-C VBUS (J1 pins A4,A9,B4,B9)
  Destination: TP4056 VCC, nRF52840 VBUS (via divider)
  Protection: D2 (SMAJ5.0CA TVS) to GND

Net: USB_CC1
  Source: USB-C CC1 (J1 pin A5)
  Destination: R3 (5.1kΩ) to GND

Net: USB_CC2
  Source: USB-C CC2 (J1 pin B5)
  Destination: R4 (5.1kΩ) to GND
```

## 2.4 Decoupling Strategy

### Per-IC Decoupling Requirements

| IC | High-Freq (100nF X7R 0402) | Bulk (≥4.7µF X5R) | Placement |
|---|---|---|---|
| nRF52840 | 4× (VDD_nRF ×2, DECUSB, DEC6) | 2× (DEC4=4.7µF, DEC5=10µF) | Within 2mm of pin pairs |
| MCP2518FD (each) | 1× (VDD) | — | Within 3mm of VDD pin |
| TJA1445 (each) | 1× (VIO) | 1× (VCC=4.7µF) | Within 3mm |
| TPS63070 | 1× (VIN) | 2× (VIN=10µF, VOUT=22µF) | Within 5mm |
| TPS7A4701 | 1× (IN) | 1× (OUT=10µF) | Within 5mm |
| AP2112K | 1× (IN) | 1× (OUT=1µF) | Within 3mm |
| APS6404L | 2× (VDD) | — | Within 3mm |
| W25Q128JV | 2× (VDD) | — | Within 3mm |
| SSD1306 | — | 1× (VDD=4.7µF) | Within 5mm |

### Decoupling Placement Rules
- 100nF capacitors: Place on same layer as IC, as close as possible to power pin, with short wide trace to pin and direct via to GND plane
- Bulk capacitors: Can be placed slightly farther (up to 5mm), but still prioritize proximity
- nRF52840 DEC4/DEC5: Follow Nordic reference layout exactly — these are critical for internal LDO stability
- TJA1445 VCC bulk: Place between VCC pin and CAN bus connector to also serve as bus noise filter

## 2.5 Impedance-Controlled Pairs

| Pair | Impedance Target | Tolerance | Notes |
|---|---|---|---|
| USB D+/D- | 90Ω differential | ±10% | USB 2.0 FS requirement |
| CAN-H/CAN-L | 120Ω differential | ±5% | CAN bus characteristic impedance |
| QSPI CLK + IO signals | 50Ω single-ended | ±15% | For 80 MHz operation, length-matched within 5mm |

## 2.6 Termination Networks

### CAN Bus Termination (per channel)

```
CAN_H ──┬── Rterm (120Ω) ──┬── CAN_L
        │                  │
        ├── Rsplit1 (60Ω)  │
        │                  │
        └── Csplit ────────┘
             (4.7nF)
                    │
                   GND (via TJA1445 SPLIT pin)
```

- Rterm: Two 120Ω 0805 resistors, one at each end of the bus
- For CAN Creeper: Software-switchable termination via 2N7002K MOSFET
  - When P1.02/P1.03 = HIGH: FET on, 120Ω connected between CAN-H and CAN-L
  - When P1.02/P1.03 = LOW: FET off, termination disconnected (for mid-bus tapping)
- Split termination: 2× 60Ω + 4.7nF to GND via TJA1445 SPLIT pin for improved EMC

### USB Termination
- D+ pull-up: Internal to nRF52840 USBD peripheral (software-controlled for enumeration)
- Series resistors: 22Ω on D+ and D- for impedance matching and edge-rate control
- CC1/CC2: 5.1kΩ pull-down to GND (indicates default USB power, no PD)

## 2.7 Protection Devices

| Device | Location | Purpose |
|---|---|---|
| PESD1CAN-U (×2) | CAN0_H/L, CAN1_H/L | ±15kV ESD protection per IEC 61000-4-2, <1pF capacitance |
| SMAJ5.0CA | USB VBUS | ±400W TVS for VBUS overvoltage |
| MBR0520LT1G | TP4056 input | Reverse current protection from battery to USB |
| DW01-P + FS8205A | Battery | Overcharge (4.25V), overdischarge (2.4V), overcurrent (3A), short-circuit protection |
| Ferrite beads (×2) | 3.3V rail, 5V_CAN rail | High-frequency noise suppression on power rails |

## 2.8 Crystal Oscillator Circuits

### 32 MHz HF Crystal (nRF52840)
- Crystal: NX3225SA-32.000000MHZ-B4 (10ppm, 8pF load)
- Load caps: C1=C2=12pF (accounting for ~4pF stray)
- Layout: Symmetric, guard ring to GND, no digital traces crossing

### 32.768 kHz LF Crystal (nRF52840)
- Crystal: FC-135 32.768kHz 12.5pF
- Load caps: C3=C4=12.5pF
- Bias resistors: R12=R13=1MΩ (may not be needed for FC-135 but provisioned)
- Layout: Keep away from switching nodes, guard ring

### 40 MHz CAN Controller Crystals (×2)
- Crystal: ABM8G-40.000MHZ-18-D2Y-T (20ppm, 18pF load)
- Load caps: C5=C6=18pF (for U2), C7=C8=18pF (for U3) — wait, renumbering needed
- Actually: C5,C6 for U2 (CAN0), C7,C8 for U3 (CAN1) — but we already used C7-C10 for nRF decoupling
- Corrected: C5,C6 for CAN0 crystal, C13,C14 for CAN1 crystal (reassigning numbers)
- Let me renumber: C5,C6 = CAN0 40MHz load caps (18pF), C15,C16 = CAN1 40MHz load caps (18pF)

## 2.9 Power Sequencing

1. **Battery insertion** or **USB plug-in**: VBAT or VBUS present
2. **TP4056**: If VBUS present, begins CC/CV charge cycle; BAT output always connected to battery
3. **DW01-P**: Monitors battery voltage; enables FS8205A MOSFETs if within safe range (2.5V–4.25V)
4. **TPS63070**: EN pin tied to VBAT via 100k pull-up; starts when VIN > 2.0V; soft-start ~1ms
5. **+3V3 rail stable**: nRF52840 POR released at ~1.7V threshold
6. **nRF52840 boot**: HFCLK starts, firmware initializes peripherals
7. **TPS7A4701**: EN controlled by nRF52840 GPIO (P1.07 if available, else always-on); 1.8V rail for QSPI if needed
8. **AP2112K-5.0**: Always-on from +3V3; 5V_CAN rail for transceivers
9. **CAN transceivers**: EN pins (P1.10, P1.11) controlled by firmware; held low during init, then enabled
10. **MCP2518FD**: Reset via SPI command after power stable; configuration loaded

## 2.10 Current Budget

| Component | Active Current (3.3V) | Sleep Current |
|---|---|---|
| nRF52840 (64 MHz, DC/DC on, BLE TX 0dBm) | 3.4 mA | 0.6 µA (SYSTEM_ON) |
| nRF52840 BLE Radio (1 Mbps, 0dBm) | 5.3 mA | — |
| MCP2518FD (×2, active) | 5 mA each = 10 mA | 5 µA each (sleep) |
| TJA1445 (×2, normal mode) | 3 mA each = 6 mA | 10 µA each (standby) |
| APS6404L (active, 80 MHz QSPI) | 15 mA | 50 µA (standby) |
| W25Q128JV (active read) | 15 mA | 1 µA (power-down) |
| SSD1306 OLED (all pixels on) | 20 mA | 10 µA (display off) |
| TPS63070 (quiescent) | 50 µA | 50 µA |
| TPS7A4701 (quiescent) | 30 µA | 30 µA |
| AP2112K (quiescent) | 55 µA | 55 µA |
| LEDs (×3, each ~2mA) | 6 mA | 0 |
| **TOTAL ACTIVE** | **~80 mA** | — |
| **TOTAL SLEEP** | — | **~160 µA** |

With 2000 mAh battery: ~25 hours active sniffing, ~500 days sleep.
