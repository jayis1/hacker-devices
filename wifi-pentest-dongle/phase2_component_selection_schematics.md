# WFP-100 — Portable WiFi Pentesting Dongle
## Phase 2: Component Selection & Schematics

---

## 2.1 Bill of Materials (BOM)

| # | Reference | Part Number | Description | Qty | Unit Price | Extended | Notes |
|---|---|---|---|---|---|---|---|
| 1 | U1 | StarFive JH7110 | RISC-V SoC, 4×U74+S7, 1.5 GHz, PCIe Gen3 | 1 | $12.00 | $12.00 | Main processor |
| 2 | U2 | Micron MT53E256M32D2DS-046 | 2GB LPDDR4X, 3200 MT/s, PoP-compatible | 1 | $6.50 | $6.50 | DDR on top of SoC (PoP) |
| 3 | U3 | Intel AX210NGW | WiFi 6E (802.11ax) 2×2 MIMO, M.2 2230 E-key | 1 | $8.00 | $8.00 | Tri-band radio |
| 4 | U4 | X-Powers AXP2101 | PMIC, 6-channel DCDC+LDO, I2C control | 1 | $1.20 | $1.20 | Power management |
| 5 | U5 | Samsung KLMBG2GATB-B041 | 16GB eMMC 5.1, 153-ball FBGA | 1 | $3.50 | $3.50 | Boot + data storage |
| 6 | U6 | NXP PCF8563TS | RTC, I2C, coin-cell backup | 1 | $0.40 | $0.40 | Timekeeping |
| 7 | U7 | Microchip AT97SC3204T-I3MA3 | TPM 2.0, SPI, 28-VQFN | 1 | $1.80 | $1.80 | Secure boot, key storage |
| 8 | U8 | TI BQ25895RTW | USB-C battery charger, 5V boost, I2C | 1 | $1.50 | $1.50 | LiPo charging |
| 9 | U9 | GigaDevice GD25LQ128E | 16MB SPI NOR Flash, QSPI, 3V | 1 | $0.60 | $0.60 | Bootloader (U-Boot SPL) |
| 10 | U10 | Realtek RTL8152B | USB 3.0 to 10/100 Ethernet (CDC-ECM backup) | 1 | $1.20 | $1.20 | Optional secondary NIC |
| 11 | Y1 | Abracon ABM8-27.000MHz | 27 MHz crystal, 20ppm, 3.2×2.5mm | 1 | $0.25 | $0.25 | PCIe ref clock for AX210 |
| 12 | Y2 | Tianfa TF3225-26.000MHz | 26 MHz crystal, SoC main oscillator | 1 | $0.20 | $0.20 | JH7110 main clock |
| 13 | Y3 | ECS-327KECX-TR | 32.768 kHz crystal, RTC | 1 | $0.15 | $0.15 | PCF8563 RTC |
| 14 | F1 | Littlefuse NANO2SMDC035F | Resettable fuse, 0.35A hold, USB-C | 1 | $0.30 | $0.30 | USB overcurrent |
| 15 | J1 | Amphenol 12401832E422A | USB-C receptacle, 3.1A, SMT | 1 | $0.80 | $0.80 | Host interface |
| 16 | J2 | Molex 734010010 | microSD card slot, push-push | 1 | $0.50 | $0.50 | Removable storage |
| 17 | J3, J4 | Amphenol RF 132351 | RP-SMA PCB receptacle, 50Ω | 2 | $0.80 | $1.60 | WiFi antennas |
| 18 | J5 | M.2 2230 E-key | M.2 card edge connector, 75-pos | 1 | $1.50 | $1.50 | AX210 socket |
| 19 | SW1 | Alps SKQGADE010 | Tactile switch, 160gF, 4.2×3.2mm | 1 | $0.10 | $0.10 | Mode button |
| 20 | D1 | LED red 0603 | PWR indicator | 1 | $0.02 | $0.02 | Power LED |
| 21 | D2 | LED green 0603 | ACT indicator | 1 | $0.02 | $0.02 | Activity LED |
| 22 | D3 | LED blue 0603 | MON indicator | 1 | $0.02 | $0.02 | Monitor mode LED |
| 23 | BT1 | Energizer CR1220 holder | 3V coin cell for RTC | 1 | $0.30 | $0.30 | RTC backup |
| 24 | L1 | 1000 mAh LiPo | 3.7V nominal, 302030 form factor | 1 | $4.00 | $4.00 | Battery |
| 25 | — | PCB, 6-layer, ENIG | 85mm × 35mm, HDI | 1 | $3.00 | $3.00 | PCB fabrication |
| | | | | | **TOTAL** | **$51.34** | Under $75 target |

---

## 2.2 SoC Pinout & MUX Configuration (JH7110)

The JH7110 uses a BGA-484 package. Key pin assignments:

### 2.2.1 GPIO/Function Pin Table

| Pin # | Ball | Default Function | Alt Function | Used As | Notes |
|---|---|---|---|---|---|
| A1 | GPIO0 | GPIO | I2C0_SDA | I2C0_SDA | PMIC + RTC |
| A2 | GPIO1 | GPIO | I2C0_SCL | I2C0_SCL | PMIC + RTC |
| A3 | GPIO2 | GPIO | I2C1_SDA | I2C1_SDA | TPM (AT97SC3204T) |
| A4 | GPIO3 | GPIO | I2C1_SCL | I2C1_SCL | TPM |
| B1 | GPIO4 | GPIO | SPI0_SCK | SPI0_SCK | TPM SPI clock |
| B2 | GPIO5 | GPIO | SPI0_MOSI | SPI0_MOSI | TPM SPI MOSI |
| B3 | GPIO6 | GPIO | SPI0_MISO | SPI0_MISO | TPM SPI MISO |
| B4 | GPIO7 | GPIO | SPI0_CS0 | SPI0_CS0 | TPM SPI chip select |
| C1 | GPIO10 | GPIO | UART0_TX | UART0_TX | Debug console |
| C2 | GPIO11 | GPIO | UART0_RX | UART0_RX | Debug console |
| C3 | GPIO14 | GPIO | SDIO0_CMD | eMMC_CMD | eMMC command |
| C4 | GPIO15 | GPIO | SDIO0_CLK | eMMC_CLK | eMMC clock |
| D1-D8 | GPIO16-23 | GPIO | SDIO0_D0-D7 | eMMC_D0-D7 | eMMC data (8-bit) |
| E1 | GPIO30 | GPIO | — | USB_ID | USB-C OTG ID |
| E2 | GPIO31 | GPIO | — | WIFI_KILL | AX210 PDn (power down) |
| E3 | GPIO32 | GPIO | — | LED_PWR | Power LED GPIO |
| E4 | GPIO33 | GPIO | — | LED_ACT | Activity LED GPIO |
| E5 | GPIO34 | GPIO | — | LED_MON | Monitor LED GPIO |
| E6 | GPIO35 | GPIO | — | BTN_MODE | Mode button input |
| F1-F4 | — | DDR_PAD | — | DDR_DQ0-DQ15 | LPDDR4X data bus (PoP) |
| F5-F8 | — | DDR_PAD | — | DDR_CA0-CA5 | LPDDR4X command/address |
| G1 | — | PCIe_TXP | — | PCIe_TX+ | AX210 PCIe TX+ |
| G2 | — | PCIe_TXN | — | PCIe_TX- | AX210 PCIe TX- |
| G3 | — | PCIe_RXP | — | PCIe_RX+ | AX210 PCIe RX+ |
| G4 | — | PCIe_RXN | — | PCIe_RX- | AX210 PCIe RX- |
| H1 | — | USB_DP | — | USB_DP | USB-C D+ |
| H2 | — | USB_DN | — | USB_DN | USB-C D- |
| H3 | — | USB_SSTXP | — | USB_SSTX+ | USB-C SS TX+ |
| H4 | — | USB_SSTXN | — | USB_SSTX- | USB-C SS TX- |
| H5 | — | USB_SSRXP | — | USB_SSRX+ | USB-C SS RX+ |
| H6 | — | USB_SSRXN | — | USB_SSRX- | USB-C SS RX- |

### 2.2.2 QSPI Flash Pin Table (U9: GD25LQ128E)

| JH7110 Pin | GD25LQ128E Pin | Net Name | Function |
|---|---|---|---|
| QSPI0_SCK (A8) | Pin 6 (CLK) | QSPI_CLK | SPI clock |
| QSPI0_CS0 (A7) | Pin 1 (CS#) | QSPI_CS# | Chip select (active low) |
| QSPI0_DQ0 (B7) | Pin 5 (SI) | QSPI_DQ0 | Data in (MOSI) |
| QSPI0_DQ1 (B8) | Pin 2 (SO) | QSPI_DQ1 | Data out (MISO) |
| QSPI0_DQ2 (C7) | Pin 3 (WP#) | QSPI_DQ2 | Write protect / quad SPI |
| QSPI0_DQ3 (C8) | Pin 7 (HOLD#) | QSPI_DQ3 | Hold / quad SPI |

---

## 2.3 LPDDR4X Memory Interface

### 2.3.1 LPDDR4X Configuration (U2: MT53E256M32D2DS-046)

| Parameter | Value |
|---|---|
| Density | 8Gb (256Mb × 32) |
| Organization | 32-bit data bus, single rank |
| Package | PoP (Package-on-Package) — mounted on top of JH7110 |
| Speed Grade | 3200 MT/s (LPDDR4X) |
| VDD2 | 1.1V |
| VDDQ | 0.6V (LPDDR4X) |
| VDDCA | 1.8V |
| tCK (min) | 0.625 ns (3200 MHz clock) |
| CAS Latency | CL22 @ 3200 MT/s |
| Burst Length | BL16 (fixed) |
| Refresh | 8192 refreshes / 32ms |

### 2.3.2 DDR Net Group Assignments

| Net Group | Signal Count | Termination | Impedance | Notes |
|---|---|---|---|---|
| DQ[31:0] | 32 | ODT 240Ω to VDDQ | 50Ω single-ended | Byte-swapped per byte lane |
| DMI[3:0] | 4 | None | 50Ω | Data mask inversion |
| DQS_t/DQS_c[3:0] | 8 | ODT 240Ω to VDDQ | 50Ω diff (100Ω diff) | Data strobe, diff pairs |
| CA[5:0] | 6 | Pull-up 4.7kΩ to VDDCA | 50Ω | Command/address |
| CK_t/CK_c | 2 | None | 50Ω diff (100Ω diff) | Clock, diff pair |
| CS_n | 1 | Pull-up 10kΩ to VDDCA | 50Ω | Chip select |
| CKE | 1 | Pull-up 10kΩ to VDDCA | 50Ω | Clock enable |

---

## 2.4 PCIe Interface to AX210

### 2.4.1 M.2 2230 E-Key Pin Assignment

| M.2 Pin | Signal | JH7110 Ball | Net Name | Notes |
|---|---|---|---|---|
| 23 | PETn0 | G2 (PCIe_TXN) | PCIE_TXN | PCIe TX- |
| 21 | PETp0 | G1 (PCIe_TXP) | PCIE_TXP | PCIe TX+ |
| 26 | PERn0 | G4 (PCIe_RXN) | PCIE_RXN | PCIe RX- |
| 24 | PERp0 | G3 (PCIe_RXP) | PCIE_RXP | PCIe RX+ |
| 36 | REFCLKn | Y1 (27MHz XTAL) | PCIE_REFCLKN | 100MHz ref (generated) |
| 38 | REFCLKp | Y1 (27MHz XTAL) | PCIE_REFCLKP | 100MHz ref (generated) |
| 43 | PERST# | GPIO31 (E2) | WIFI_KILL | PCIe reset + power-down |
| 33 | CLKREQ# | GPIO40 | PCIE_CLKREQ | Clock request |
| 28 | WAKE# | GPIO41 | PCIE_WAKE | Wake from L1.2 |
| 1,2,3,4 | GND | GND | GND | Ground |
| 55,57,59,61 | +3.3V | VDD_3V3 | VCC3V3 | PCIe 3.3V supply |
| 69 | +1.8V | VDD_WIFI_IO | VCC1V8 | PCIe 1.8V (or 3.3V, configurable) |
| 75 | +3.3Vaux | VDD_3V3 | VCC3V3AUX | PCIe aux power |

### 2.4.2 PCIe Clock Architecture

The JH7110 does not have an integrated PCIe clock generator. A 27 MHz crystal feeds the SoC's internal PLL, which generates a 100 MHz PCIe reference clock output on dedicated balls. The AX210 requires a 100 MHz differential reference clock.

| Component | Value | Notes |
|---|---|---|
| Y1 | 27.000 MHz, 20 ppm | JH7110 OSC0 input |
| SoC PLL | Fractional-N PLL | Generates 100 MHz from 27 MHz reference |
| PCIe REFCLK | 100 MHz, 100 ppm | Output to M.2 slot pins 36/38 |
| Trace length match | ±5 mil | REFCLKP/N length-matched |

---

## 2.5 USB-C Interface

### 2.5.1 USB-C Connector Pinout (J1: Amphenol 12401832E422A)

| USB-C Pin | Signal | JH7110 Ball | Net Name | Notes |
|---|---|---|---|---|
| A1, B12 | GND | GND | GND | Ground |
| A4, B9 | VBUs | VBUS_5V | VBUS_5V | 5V from host or charger |
| A5 | CC1 | — | USB_CC1 | Configuration channel 1 |
| B5 | CC2 | — | USB_CC2 | Configuration channel 2 |
| A6 | D+ | H1 (USB_DP) | USB_DP | USB 2.0 D+ |
| A7 | D- | H2 (USB_DN) | USB_DN | USB 2.0 D- |
| A2 | SSTX+1 | H3 (USB_SSTXP) | USB_SSTXP | SuperSpeed TX+ |
| A3 | SSTX-1 | H4 (USB_SSTXN) | USB_SSTXN | SuperSpeed TX- |
| A10 | SSRX+2 | H5 (USB_SSRXP) | USB_SSRXP | SuperSpeed RX+ |
| A11 | SSRX-2 | H6 (USB_SSRXN) | USB_SSRXN | SuperSpeed RX- |
| B2 | SSRX+1 | H5 (USB_SSRXP) | USB_SSRXP | SuperSpeed RX+ (mirror) |
| B3 | SSRX-1 | H6 (USB_SSRXN) | USB_SSRXN | SuperSpeed RX- (mirror) |
| B10 | SSTX+2 | H3 (USB_SSTXP) | USB_SSTXP | SuperSpeed TX+ (mirror) |
| B11 | SSTX-2 | H4 (USB_SSTXN) | USB_SSTXN | SuperSpeed TX- (mirror) |

### 2.5.2 USB-C CC Configuration

| Component | Value | Net | Notes |
|---|---|---|---|
| R1 | 5.1kΩ 1% | CC1 → GND | 1.5A default pull-down (UFP) |
| R2 | 5.1kΩ 1% | CC2 → GND | 1.5A default pull-down (UFP) |
| R3 | 56kΩ 1% | CC1 → VCONN | Debug accessory mode (optional) |
| F1 | NANO2SMDC035F | VBUS → VBUS_5V | 0.35A hold resettable fuse |

---

## 2.6 Power Management Schematic

### 2.6.1 PMIC: X-Powers AXP2101 Configuration

| Channel | Type | Output | Load | Enable | Notes |
|---|---|---|---|---|---|
| DCDC1 | Buck | 0.9V / 2A | VDD_CORE (SoC core) | Always-on | JH7110 core supply |
| DCDC2 | Buck | 1.5V / 2A | VDD_DDR (LPDDR4X VDD2) | Always-on | DDR core |
| DCDC3 | Buck | 1.8V / 1.5A | VDD_DDR_IO (LPDDR4X VDDQ) | Always-on | DDR IO |
| DCDC4 | Buck | 3.3V / 1A | VDD_3V3 (peripherals) | GPIO0 | USB PHY, GPIO, SPI |
| LDO1 | LDO | 1.8V / 300mA | VDD_SOC (SoC IO) | Always-on | eMMC, peripheral IO |
| LDO2 | LDO | 1.8V / 200mA | VDD_WIFI_IO | GPIO1 | AX210 PCIe IO |
| LDO3 | LDO | 0.9V / 200mA | VDD_PLL (SoC PLL) | Always-on | Analog PLL supply |

### 2.6.2 Battery Charger: BQ25895RTW

| Parameter | Value | Notes |
|---|---|---|
| Input | 5V USB-C VBUS | Up to 3.25A input |
| Charge Current | 500 mA (configurable) | 1000 mAh / 500 mA = 2 hr charge |
| Battery | 3.7V LiPo, 1000 mAh | 302030 form factor |
| Boost Output | 5V / 1A | USB-C VBUS when host disconnected |
| I2C Address | 0x6A | Control register interface |
| Charge Voltage | 4.208V | 4.2V CV phase |
| Precharge | 100 mA | Below 3.0V precharge |

### 2.6.3 Power Sequencing

```
Time (ms)  Event
─────────  ──────────────────────────────────────────
  0.0      VBUS_5V applied (USB-C insertion)
  1.0      BQ25895 boost enabled → VBUS_5V stable
  5.0      AXP2101 LDO1 (1.8V SOC) enabled
  5.5      AXP2101 LDO3 (0.9V PLL) enabled
 10.0      AXP2101 DCDC1 (0.9V CORE) enabled
 10.5      AXP2101 DCDC2 (1.5V DDR) enabled
 11.0      AXP2101 DCDC3 (1.8V DDR_IO) enabled
 15.0      JH7110 RESETn de-asserted
 15.5      JH7110 boots from QSPI (U-Boot SPL)
 20.0      AXP2101 DCDC4 (3.3V peripherals) enabled
 20.5      AXP2101 LDO2 (1.8V WIFI_IO) enabled
100.0      PCIe REFCLK stable → AX210 PERST# de-asserted
200.0      USB DWC3 controller initialized
500.0      Linux kernel boot complete
```

---

## 2.7 Decoupling Networks

### 2.7.1 SoC Decoupling (JH7110)

| Power Pin | Rail | Decoupling | Notes |
|---|---|---|---|
| VDD_CORE (0.9V) | 4× 100nF + 2× 10µF | One 100nF per core, bulk near BGA center |
| VDD_DDR (1.5V) | 2× 100nF + 1× 10µF | Near PoP balls |
| VDD_DDR_IO (1.8V) | 4× 100nF | One per byte lane |
| VDD_SOC (1.8V) | 2× 100nF + 1× 4.7µF | Peripheral IO |
| VDD_PLL (0.9V) | 1× 100nF + 1× 10µF | Low-ESR ceramic, minimize loop area |

### 2.7.2 WiFi Module Decoupling (AX210)

| Rail | Decoupling | Notes |
|---|---|---|
| 3.3V (VCC3V3) | 3× 100nF + 1× 22µF | M.2 slot pins 55/57/59/61 |
| 1.8V (VDD_WIFI_IO) | 2× 100nF | M.2 slot pin 69 |
| 3.3Vaux | 1× 100nF | M.2 slot pin 75 |

---

## 2.8 Complete Netlist (Source → Component → Destination)

### 2.8.1 Power Nets

| Net Name | Source Pin | Component | Dest Pin | Notes |
|---|---|---|---|---|
| VBUS_5V | J1 (A4,B9) | F1 (0.35A PTC) | BQ25895 VBUS | USB 5V input |
| VBUS_5V | BQ25895 BOOST | — | J1 (A4,B9) | 5V boost output (battery mode) |
| VCC5V | F1 output | C1 (10µF) → GND | AXP2101 VIN | PMIC input |
| VDD_CORE | AXP2101 DCDC1 | C2-C5 (100nF×4), C6-C7 (10µF×2) | JH7110 VDD_CORE | 0.9V core |
| VDD_DDR | AXP2101 DCDC2 | C8-C9 (100nF×2), C10 (10µF) | MT53E256 VDD2 | 1.5V DDR |
| VDD_DDR_IO | AXP2101 DCDC3 | C11-C14 (100nF×4) | MT53E256 VDDQ | 1.8V DDR IO |
| VDD_SOC | AXP2101 LDO1 | C15-C16 (100nF×2), C17 (4.7µF) | JH7110 VDD_SOC | 1.8V SOC IO |
| VDD_3V3 | AXP2101 DCDC4 | C18-C19 (100nF×2), C20 (10µF) | Peripherals | 3.3V rail |
| VDD_WIFI_IO | AXP2101 LDO2 | C21-C22 (100nF×2) | AX210 (1.8V) | WiFi IO |
| VDD_PLL | AXP2101 LDO3 | C23 (100nF), C24 (10µF) | JH7110 VDD_PLL | 0.9V analog |
| VBAT | BQ25895 BAT | — | LiPo (+) | Battery terminal |

### 2.8.2 PCIe Signal Nets

| Net Name | Source Pin | Destination Pin | Notes |
|---|---|---|---|
| PCIE_TXP | JH7110 G1 | M.2 pin 21 | TX+ (50Ω diff, 100Ω diff) |
| PCIE_TXN | JH7110 G2 | M.2 pin 23 | TX- |
| PCIE_RXP | JH7110 G3 | M.2 pin 24 | RX+ |
| PCIE_RXN | JH7110 G4 | M.2 pin 26 | RX- |
| PCIE_REFCLKP | JH7110 CLK_P | M.2 pin 38 | 100 MHz ref+ |
| PCIE_REFCLKN | JH7110 CLK_N | M.2 pin 36 | 100 MHz ref- |
| PCIE_RST# | JH7110 GPIO31 | M.2 pin 43 | PERST# |
| PCIE_CLKREQ# | JH7110 GPIO40 | M.2 pin 33 | CLKREQ# |
| PCIE_WAKE# | M.2 pin 28 | JH7110 GPIO41 | WAKE# |

### 2.8.3 USB Signal Nets

| Net Name | Source Pin | Destination Pin | Notes |
|---|---|---|---|
| USB_DP | JH7110 H1 | J1 A6, B6 | USB 2.0 D+ |
| USB_DN | JH7110 H2 | J1 A7, B7 | USB 2.0 D- |
| USB_SSTXP | JH7110 H3 | J1 A2, B11 | SS TX+ |
| USB_SSTXN | JH7110 H4 | J1 A3, B10 | SS TX- |
| USB_SSRXP | JH7110 H5 | J1 A10, B3 | SS RX+ |
| USB_SSRXN | JH7110 H6 | J1 A11, B2 | SS RX- |
| USB_CC1 | J1 A5 | R1 (5.1kΩ) → GND | CC pull-down |
| USB_CC2 | J1 B5 | R2 (5.1kΩ) → GND | CC pull-down |

### 2.8.4 SPI/QSPI Nets

| Net Name | Source Pin | Dest Pin (U9 GD25LQ128E) | Notes |
|---|---|---|---|
| QSPI_CLK | JH7110 A8 | Pin 6 (CLK) | QSPI clock |
| QSPI_CS# | JH7110 A7 | Pin 1 (CS#) | Chip select |
| QSPI_DQ0 | JH7110 B7 | Pin 5 (SI) | MOSI |
| QSPI_DQ1 | JH7110 B8 | Pin 2 (SO) | MISO |
| QSPI_DQ2 | JH7110 C7 | Pin 3 (WP#) | Quad SPI data 2 |
| QSPI_DQ3 | JH7110 C8 | Pin 7 (HOLD#) | Quad SPI data 3 |

### 2.8.5 I2C Nets

| Net Name | Source Pin | Dest Pin | Notes |
|---|---|---|---|
| I2C0_SDA | JH7110 A1 | AXP2101 SDA (Pin 14), PCF8563 SDA (Pin 5) | PMIC + RTC bus |
| I2C0_SCL | JH7110 A2 | AXP2101 SCL (Pin 13), PCF8563 SCL (Pin 6) | PMIC + RTC bus |
| I2C1_SDA | JH7110 A3 | AT97SC3204T SDA (Pin 12) | TPM bus |
| I2C1_SCL | JH7110 A4 | AT97SC3204T SCL (Pin 11) | TPM bus |

### 2.8.6 GPIO/LED/Button Nets

| Net Name | Source Pin | Dest Pin | Notes |
|---|---|---|---|
| LED_PWR | JH7110 GPIO32 (E3) | R4 (330Ω) → D1 (Red LED) → GND | Power indicator |
| LED_ACT | JH7110 GPIO33 (E4) | R5 (330Ω) → D2 (Green LED) → GND | Activity indicator |
| LED_MON | JH7110 GPIO34 (E5) | R6 (330Ω) → D3 (Blue LED) → GND | Monitor mode indicator |
| BTN_MODE | JH7110 GPIO35 (E6) | SW1 → GND (with R7 10kΩ pull-up to 3.3V) | Mode cycle button |
| WIFI_KILL | JH7110 GPIO31 (E2) | M.2 pin 43 (PERST#), R8 (4.7kΩ pull-up) | WiFi power down |

---

## 2.9 Impedance-Matched Differential Pairs

| Pair | Net | Impedance | Spacing | Width | Length Match | Reference |
|---|---|---|---|---|---|---|
| PCIe TX | PCIE_TXP/TXN | 50Ω SE / 100Ω diff | 6 mil | 4 mil | ±5 mil | GND plane L2 |
| PCIe RX | PCIE_RXP/RXN | 50Ω SE / 100Ω diff | 6 mil | 4 mil | ±5 mil | GND plane L2 |
| PCIe REFCLK | PCIE_REFCLKP/N | 50Ω SE / 100Ω diff | 6 mil | 4 mil | ±2 mil | GND plane L2 |
| USB SS TX | USB_SSTXP/N | 50Ω SE / 90Ω diff | 5 mil | 3.5 mil | ±10 mil | GND plane L2 |
| USB SS RX | USB_SSRXP/N | 50Ω SE / 90Ω diff | 5 mil | 3.5 mil | ±10 mil | GND plane L2 |
| DDR DQS | DQS_t/DQS_c[3:0] | 50Ω SE / 100Ω diff | 6 mil | 4 mil | ±2 mil | VDDQ plane |
| DDR CK | CK_t/CK_c | 50Ω SE / 100Ω diff | 6 mil | 4 mil | ±2 mil | VDDQ plane |

---

## 2.10 Crystal & Clock Circuits

### 2.10.1 27 MHz PCIe Reference (Y1)

```
                    ┌───────────┐
  JH7110 OSC0_P ───┤ XTAL 27MHz ├─── GND
                    │  20ppm    │
  JH7110 OSC0_N ───┤   3.2×2.5 ├─── GND
                    └───────────┘
         │                    │
        C25 (12pF)          C26 (12pF)
         │                    │
        GND                  GND
```

Load capacitance: CL = (C25 × C26) / (C25 + C26) + Cstray ≈ 6pF + 3pF = 9pF (matches crystal spec)

### 2.10.2 32.768 kHz RTC Crystal (Y3)

```
  PCF8563 OSCI ──┤ XTAL 32.768kHz ├─── GND
                  │  12.5pF       │
  PCF8563 OSCO ──┤               ├─── GND
                  └───────────────┘
       │                    │
      C27 (15pF)          C28 (15pF)
       │                    │
      GND                  GND
```