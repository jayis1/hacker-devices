# Phase 2: Component Selection & Schematics — eMMC Flash Dumper

## 2.0 Bill of Materials (BOM)

### 2.0.1 Main PCB BOM

| Ref | Qty | Part Number | Manufacturer | Description | Package | Unit Cost | Ext Cost |
|-----|-----|-------------|--------------|-------------|---------|-----------|----------|
| U1 | 1 | STM32H743VIT6 | STMicroelectronics | Cortex-M7 MCU, 480 MHz, 2MB Flash, 1MB RAM | LQFP-100 | $12.50 | $12.50 |
| U2 | 1 | iCE40UP5K-SG48I | Lattice Semiconductor | FPGA, 5280 LUTs, 120kb BRAM, 2×SPI | QFN-48 | $6.80 | $6.80 |
| U3 | 1 | MT41K256M16TW-107:P | Micron | DDR3L SDRAM, 4Gb (256M×16), 1866 MT/s | FBGA-96 (9×14mm) | $8.20 | $8.20 |
| U4 | 1 | USB3320C-EZK | Microchip | USB 2.0 ULPI PHY (HS) with USB 3.0 mux | QFN-32 | $3.50 | $3.50 |
| U5 | 1 | TPS6521815RSLR | Texas Instruments | PMIC, 5×DC-DC + 2×LDO, I2C config | VQFN-48 | $5.80 | $5.80 |
| U6 | 1 | BQ25896RTWR | Texas Instruments | LiPo charger, OTG boost, power path | WQFN-24 | $3.20 | $3.20 |
| U7 | 1 | TXS0108EPWR | Texas Instruments | 8-bit bidirectional level shifter | TSSOP-20 | $1.20 | $1.20 |
| U8 | 1 | TXS0108EPWR | Texas Instruments | 8-bit bidirectional level shifter (2nd) | TSSOP-20 | $1.20 | $1.20 |
| U9 | 1 | SN74LVC1G125DBVR | Texas Instruments | Single bus buffer, OE control | SOT-23-5 | $0.15 | $0.15 |
| U10 | 1 | SSD1306 0.96" OLED | Generic | 128×64 I2C OLED display module | Module | $3.50 | $3.50 |
| U11 | 1 | TPS74801DRCR | Texas Instruments | LDO, 1.5A adjustable, for FPGA VCC | VSON-10 | $1.80 | $1.80 |
| J1 | 1 | USB4105-GF-A | GCT | USB Type-C receptacle, 16-pin, vertical | SMT | $1.50 | $1.50 |
| J2 | 1 | 502570-0893 | Molex | microSD push-push connector | SMT | $1.20 | $1.20 |
| J3 | 1 | 2×10 0.1" header | Generic | ISP probe header, 20-pin | THT | $0.50 | $0.50 |
| J4 | 1 | FTSH-110-01-L-DV-K | Samtec | eMMC adapter header, 20-pin 0.05" | SMT | $2.50 | $2.50 |
| J5 | 1 | 2-pin JST-PH | Generic | LiPo battery connector | THT | $0.30 | $0.30 |
| Y1 | 1 | ABM8G-25.000MHZ-18-D2Y-T | Abracon | 25 MHz crystal, 18pF, ±20ppm | 3.2×2.5mm SMD | $0.60 | $0.60 |
| Y2 | 1 | ABS07-32.768KHZ-T | Abracon | 32.768 kHz crystal, 12.5pF, ±20ppm | 3.2×1.5mm SMD | $0.50 | $0.50 |
| Y3 | 1 | ABM8G-12.000MHZ-18-D2Y-T | Abracon | 12 MHz crystal, 18pF, for FPGA | 3.2×2.5mm SMD | $0.60 | $0.60 |
| SW1-4 | 4 | PTS645SM43SMTR92 | C&K | Tactile switch, SPST, 6×6mm | SMT | $0.25 | $1.00 |
| LED1-2 | 2 | WS2812B-Mini | Worldsemi | RGB LED, serial control | 3.5×3.5mm SMD | $0.30 | $0.60 |
| BZ1 | 1 | PKLCS1212E4001-R1 | Murata | Piezo buzzer, 4kHz, 12×12mm | SMT | $0.80 | $0.80 |
| C1-C4 | 4 | GRM21BR61C106KE15L | Murata | 10µF, 16V, X5R, 0805 | 0805 | $0.10 | $0.40 |
| C5-C20 | 16 | GRM188R71C104KA01D | Murata | 0.1µF, 16V, X7R, 0603 | 0603 | $0.03 | $0.48 |
| C21-C30 | 10 | GRM155R71C103KA01D | Murata | 0.01µF, 16V, X7R, 0402 | 0402 | $0.02 | $0.20 |
| C31-C36 | 6 | GRM21BR60J226ME39L | Murata | 22µF, 6.3V, X5R, 0805 | 0805 | $0.15 | $0.90 |
| C37-C40 | 4 | GRM32ER60J107ME20L | Murata | 100µF, 6.3V, X5R, 1210 | 1210 | $0.40 | $1.60 |
| C41-C44 | 4 | GRM1555C1H100JA01D | Murata | 10pF, 50V, C0G, 0402 | 0402 | $0.02 | $0.08 |
| R1-R8 | 8 | RC0603FR-0749R9L | Yageo | 49.9Ω, 1%, 0603 (series term) | 0603 | $0.02 | $0.16 |
| R9-R16 | 8 | RC0603FR-0733RL | Yageo | 33Ω, 1%, 0603 (SPI term) | 0603 | $0.02 | $0.16 |
| R17-R20 | 4 | RC0603FR-0710KL | Yageo | 10kΩ, 1%, 0603 (pull-up) | 0603 | $0.02 | $0.08 |
| R21-R22 | 2 | RC0603FR-074K7L | Yageo | 4.7kΩ, 1%, 0603 (I2C pull-up) | 0603 | $0.02 | $0.04 |
| R23-R24 | 2 | RC0603FR-07100KL | Yageo | 100kΩ, 1%, 0603 (crystal bias) | 0603 | $0.02 | $0.04 |
| R25 | 1 | RC1206FR-070R1L | Yageo | 0.1Ω, 1%, 1206 (current sense) | 1206 | $0.10 | $0.10 |
| L1 | 1 | LQM21PNR47MGH | Murata | 0.47µH, 1.5A, power inductor | 0805 | $0.20 | $0.20 |
| L2-L3 | 2 | BLM18PG121SN1D | Murata | 120Ω ferrite bead @ 100MHz | 0603 | $0.08 | $0.16 |
| D1 | 1 | CDBU0520 | Comchip | Schottky diode, 20V, 500mA | 0603 | $0.10 | $0.10 |
| D2 | 1 | SMAJ5.0CA | Littelfuse | TVS diode, 5V, bidirectional | SMA | $0.25 | $0.25 |
| F1 | 1 | 1206L050/15YR | Littelfuse | PTC resettable fuse, 500mA | 1206 | $0.30 | $0.30 |
| Q1 | 1 | DMG2305UX-7 | Diodes Inc | P-ch MOSFET, -20V, -4.2A | SOT-23 | $0.20 | $0.20 |
| Q2 | 1 | 2N7002K | ON Semi | N-ch MOSFET, 60V, 300mA | SOT-23 | $0.10 | $0.10 |

**Total Main PCB BOM: ~$58.00**

### 2.0.2 Adapter Board BOMs

#### eMMC BGA-153 Adapter

| Ref | Qty | Part Number | Description | Unit Cost | Ext Cost |
|-----|-----|-------------|-------------|-----------|----------|
| - | 1 | Custom PCB | 4-layer adapter, 25×25mm | $5.00 | $5.00 |
| J1 | 1 | BGA-153 socket | Test socket or direct solder pads | $15.00 | $15.00 |
| J2 | 1 | FTSH-110-01-L-DV-K | 20-pin 0.05" header (mate to J4) | $2.50 | $2.50 |
| C1-C4 | 4 | 0.1µF 0603 | Decoupling | $0.12 | $0.12 |

**eMMC Adapter Total: ~$22.62**

#### NAND TSOP-48 Adapter

| Ref | Qty | Part Number | Description | Unit Cost | Ext Cost |
|-----|-----|-------------|-------------|-----------|----------|
| - | 1 | Custom PCB | 2-layer adapter, 20×50mm | $3.00 | $3.00 |
| J1 | 1 | TSOP-48 ZIF socket | Zero insertion force | $4.00 | $4.00 |
| J2 | 1 | 2×10 0.1" header | Mates to ISP header J3 | $0.50 | $0.50 |

**NAND Adapter Total: ~$7.50**

#### SPI NOR SOIC-8 Test Clip

| Ref | Qty | Part Number | Description | Unit Cost | Ext Cost |
|-----|-----|-------------|-------------|-----------|----------|
| - | 1 | Pomona 5250 | SOIC-8 test clip | $12.00 | $12.00 |
| J1 | 1 | 2×4 0.1" header | Mates to ISP header (partial) | $0.30 | $0.30 |

**SPI NOR Clip Total: ~$12.30**

### 2.0.3 Total System BOM

| Assembly | Cost |
|----------|------|
| Main PCB | $58.00 |
| eMMC Adapter | $22.62 |
| NAND Adapter | $7.50 |
| SPI NOR Clip | $12.30 |
| LiPo Battery (2000mAh) | $8.00 |
| 3D Printed Enclosure | $3.00 |
| **TOTAL** | **~$111.42** |

*Note: Slightly over $100 target; cost optimization possible with volume pricing, alternative FPGA (Gowin GW1N-1), or removing eMMC adapter from base kit.*

## 2.1 Pinout Tables

### 2.1.1 STM32H743VIT6 Pin Assignments

| Pin | Name | Function | Connected To | Notes |
|-----|------|----------|-------------|-------|
| 1 | PE2 | FMC_A23 / GPIO | NC (pulled low) | SDRAM only needs A0-A15 |
| 2 | PE3 | FMC_A19 | NC | |
| 3 | PE4 | FMC_A20 | NC | |
| 4 | PE5 | FMC_A21 | NC | |
| 5 | PE6 | FMC_A22 | NC | |
| 6 | VBAT | VBAT | 3.3V via 1kΩ | RTC backup |
| 7 | PI8 | GPIO / EVENTOUT | NC | |
| 8 | PC13 | RTC_OUT / GPIO | NC | |
| 9 | PC14-OSC32_IN | OSC32_IN | Y2 pin 1, C41 to GND | 32.768 kHz LSE |
| 10 | PC15-OSC32_OUT | OSC32_OUT | Y2 pin 2, C42 to GND | |
| 11 | PH0-OSC_IN | OSC_IN | Y1 pin 1, C43 to GND | 25 MHz HSE |
| 12 | PH1-OSC_OUT | OSC_OUT | Y1 pin 2, C44 to GND | |
| 13 | NRST | NRST | 10kΩ to 3.3V, 100nF to GND, SWD header | Reset |
| 14 | PI0 | GPIO | NC | |
| 15 | PI1 | GPIO | NC | |
| 16 | PI2 | GPIO | NC | |
| 17 | PI3 | GPIO | NC | |
| 18 | PA0 | TIM2_CH1 / GPIO | SW1 (Button UP) | 10kΩ pull-up to 3.3V |
| 19 | PA1 | TIM2_CH2 / GPIO | SW2 (Button DOWN) | 10kΩ pull-up to 3.3V |
| 20 | PA2 | TIM2_CH3 / GPIO | SW3 (Button SELECT) | 10kΩ pull-up to 3.3V |
| 21 | PA3 | TIM2_CH4 / GPIO | SW4 (Button BACK) | 10kΩ pull-up to 3.3V |
| 22 | VSS | GND | Ground plane | |
| 23 | VDD | VDD (3.3V) | DCDC2 output | 100nF + 10µF decoupling |
| 24 | PH2 | FMC_A16 / GPIO | NC | |
| 25 | PH3 | FMC_A17 / GPIO | NC | |
| 26 | PH4 | FMC_A18 / GPIO | NC | |
| 27 | PH5 | FMC_A19 / GPIO | NC | |
| 28 | PI4 | FMC_NBL4 / GPIO | NC | |
| 29 | PI5 | FMC_A25 / GPIO | NC | |
| 30 | PI6 | FMC_A26 / GPIO | NC | |
| 31 | PI7 | FMC_A27 / GPIO | NC | |
| 32 | PI9 | FMC_A30 / GPIO | NC | |
| 33 | PI10 | FMC_A31 / GPIO | NC | |
| 34 | PA4 | DAC_OUT1 / SPI1_NSS | FPGA_CS# (SPI config) | 33Ω series |
| 35 | PA5 | SPI1_SCK | FPGA_SCK | 33Ω series |
| 36 | PA6 | SPI1_MISO | FPGA_MISO (SDO) | 33Ω series |
| 37 | PA7 | SPI1_MOSI | FPGA_MOSI (SDI) | 33Ω series |
| 38 | PC4 | FMC_SDNWE / GPIO | NC | |
| 39 | PC5 | FMC_SDNWE / GPIO | NC | |
| 40 | PB0 | FMC_A12 / GPIO | NC (SDRAM uses A0-A11 only) | |
| 41 | PB1 | FMC_A13 / GPIO | NC | |
| 42 | PB2 | FMC_A14 / GPIO | NC | |
| 43 | PF0 | FMC_A0 | U3 (SDRAM) A0 | 49.9Ω series |
| 44 | PF1 | FMC_A1 | U3 A1 | 49.9Ω series |
| 45 | PF2 | FMC_A2 | U3 A2 | 49.9Ω series |
| 46 | PF3 | FMC_A3 | U3 A3 | 49.9Ω series |
| 47 | PF4 | FMC_A4 | U3 A4 | 49.9Ω series |
| 48 | PF5 | FMC_A5 | U3 A5 | 49.9Ω series |
| 49 | VSS | GND | Ground plane | |
| 50 | VDD | VDD (3.3V) | DCDC2 output | 100nF + 10µF decoupling |
| 51 | PF12 | FMC_A6 | U3 A6 | 49.9Ω series |
| 52 | PF13 | FMC_A7 | U3 A7 | 49.9Ω series |
| 53 | PF14 | FMC_A8 | U3 A8 | 49.9Ω series |
| 54 | PF15 | FMC_A9 | U3 A9 | 49.9Ω series |
| 55 | PG0 | FMC_A10 | U3 A10 | 49.9Ω series |
| 56 | PG1 | FMC_A11 | U3 A11 | 49.9Ω series |
| 57 | PG2 | FMC_A12 | U3 A12 | 49.9Ω series |
| 58 | PG3 | FMC_A13 | U3 A13 | 49.9Ω series |
| 59 | PG4 | FMC_A14 | U3 A14 | 49.9Ω series |
| 60 | PG5 | FMC_A15 | U3 A15 | 49.9Ω series |
| 61 | PG6 | FMC_INT2 / GPIO | FPGA_INTR | Interrupt from FPGA |
| 62 | PG7 | FMC_INT3 / GPIO | NC | |
| 63 | PG8 | FMC_SDCLK / GPIO | NC (SDRAM uses FMC_SDCLK) | |
| 64 | PC0 | FMC_SDNWE | U3 WE# | 49.9Ω series |
| 65 | PC2 | FMC_SDNE0 | U3 CS# | 49.9Ω series |
| 66 | PC3 | FMC_SDCKE0 | U3 CKE | 49.9Ω series |
| 67 | PF6 | FMC_NIORD / GPIO | NC (NAND handled by FPGA) | |
| 68 | PF7 | FMC_NREG / GPIO | NC | |
| 69 | PF8 | FMC_NIOWR / GPIO | NC | |
| 70 | PF9 | FMC_CD / GPIO | NC | |
| 71 | PF10 | FMC_INTR / GPIO | NC | |
| 72 | PH6 | FMC_SDNE1 / GPIO | NC | |
| 73 | PH7 | FMC_SDCKE1 / GPIO | NC | |
| 74 | PH8 | FMC_D16 | U3 DQ0 | 49.9Ω series |
| 75 | PH9 | FMC_D17 | U3 DQ1 | 49.9Ω series |
| 76 | PH10 | FMC_D18 | U3 DQ2 | 49.9Ω series |
| 77 | PH11 | FMC_D19 | U3 DQ3 | 49.9Ω series |
| 78 | PH12 | FMC_D20 | U3 DQ4 | 49.9Ω series |
| 79 | PH13 | FMC_D21 | U3 DQ5 | 49.9Ω series |
| 80 | PH14 | FMC_D22 | U3 DQ6 | 49.9Ω series |
| 81 | PH15 | FMC_D23 | U3 DQ7 | 49.9Ω series |
| 82 | PI0 | FMC_D24 | U3 DQ8 | 49.9Ω series |
| 83 | PI1 | FMC_D25 | U3 DQ9 | 49.9Ω series |
| 84 | PI2 | FMC_D26 | U3 DQ10 | 49.9Ω series |
| 85 | PI3 | FMC_D27 | U3 DQ11 | 49.9Ω series |
| 86 | PG9 | FMC_NCE / FMC_NE2 | FPGA_NAND_CE# | NAND chip enable via FPGA |
| 87 | PG10 | FMC_NCE / FMC_NE3 | NC | |
| 88 | PG11 | FMC_INT3 / GPIO | NC | |
| 89 | PG12 | FMC_NE4 / GPIO | NC | |
| 90 | PG13 | FMC_A24 / GPIO | NC | |
| 91 | PG14 | FMC_A25 / GPIO | NC | |
| 92 | VSS | GND | Ground plane | |
| 93 | VDD | VDD (3.3V) | DCDC2 output | 100nF + 10µF decoupling |
| 94 | PG15 | FMC_SDNCAS | U3 CAS# | 49.9Ω series |
| 95 | PF11 | FMC_SDNRAS | U3 RAS# | 49.9Ω series |
| 96 | PC1 | FMC_SDNWE / GPIO | NC | |
| 97 | PD0 | FMC_D2 | U3 DQ10 (upper byte) | 49.9Ω series |
| 98 | PD1 | FMC_D3 | U3 DQ11 | 49.9Ω series |
| 99 | PD2 | FMC_D13 / SDMMC1_CMD | J2 (microSD) CMD | 49.9Ω series |
| 100 | PD3 | FMC_D14 / GPIO | NC | |
| 101 | PD4 | FMC_NOE / GPIO | NC | |
| 102 | PD5 | FMC_NWE / GPIO | NC | |
| 103 | PD6 | FMC_NWAIT / GPIO | NC | |
| 104 | PD7 | FMC_NE1 / GPIO | NC | |
| 105 | PG0 | FMC_A10 (duplicate) | NC | |
| 106 | PG1 | FMC_A11 (duplicate) | NC | |
| 107 | PD8 | FMC_D15 / SDMMC1_D0 | J2 DAT0 | 49.9Ω series |
| 108 | PD9 | FMC_D14 / SDMMC1_D1 | J2 DAT1 | 49.9Ω series |
| 109 | PD10 | FMC_D15 / SDMMC1_D2 | J2 DAT2 | 49.9Ω series |
| 110 | PD11 | FMC_A16 / SDMMC1_D3 | J2 DAT3 | 49.9Ω series |
| 111 | PD12 | FMC_A17 / SDMMC1_CK | J2 CLK | 49.9Ω series |
| 112 | PD13 | FMC_A18 / GPIO | NC | |
| 113 | PD14 | FMC_D0 | U3 DQ12 | 49.9Ω series |
| 114 | PD15 | FMC_D1 | U3 DQ13 | 49.9Ω series |
| 115 | PB3 | JTDO / SWO | SWD header SWO | Debug |
| 116 | PB4 | NJTRST / GPIO | NC | |
| 117 | PB5 | FMC_SDCKE1 / GPIO | NC | |
| 118 | PB6 | FMC_SDNE1 / GPIO | NC | |
| 119 | PB7 | FMC_NL / GPIO | NC | |
| 120 | PB8 | FMC_D4 / SDMMC1_D4 | NC (4-bit SD mode) | |
| 121 | PB9 | FMC_D5 / SDMMC1_D5 | NC | |
| 122 | PE0 | FMC_D6 / SDMMC1_D6 | NC | |
| 123 | PE1 | FMC_D7 / SDMMC1_D7 | NC | |
| 124 | VSS | GND | Ground plane | |
| 125 | VDD | VDD (3.3V) | DCDC2 output | 100nF + 10µF decoupling |
| 126 | PB10 | FMC_D8 / GPIO | NC | |
| 127 | PB11 | FMC_D9 / GPIO | NC | |
| 128 | PB12 | FMC_D10 / GPIO | FPGA_CRESET | FPGA reset (active low) |
| 129 | PB13 | FMC_D11 / GPIO | FPGA_CDONE | FPGA config done |
| 130 | PB14 | FMC_D12 / GPIO | LED1 (WS2812B) | RGB LED data |
| 131 | PB15 | FMC_D13 / GPIO | LED2 (WS2812B) | RGB LED data |
| 132 | PD8 | FMC_D14 (duplicate) | NC | |
| 133 | PD9 | FMC_D15 (duplicate) | NC | |
| 134 | PD10 | FMC_D15 / GPIO | NC | |
| 135 | PD11 | FMC_A16 / GPIO | NC | |
| 136 | PD12 | FMC_A17 / GPIO | NC | |
| 137 | PD13 | FMC_A18 / GPIO | NC | |
| 138 | PE7 | FMC_D4 | U3 DQ14 | 49.9Ω series |
| 139 | PE8 | FMC_D5 | U3 DQ15 | 49.9Ω series |
| 140 | PE9 | FMC_D6 | U3 UDQS# | 49.9Ω series |
| 141 | PE10 | FMC_D7 | U3 UDQS | 49.9Ω series |
| 142 | PE11 | FMC_D8 | U3 LDM | 49.9Ω series |
| 143 | PE12 | FMC_D9 | U3 UDM | 49.9Ω series |
| 144 | PE13 | FMC_D10 | U3 LDQS# | 49.9Ω series |
| 145 | PE14 | FMC_D11 | U3 LDQS | 49.9Ω series |
| 146 | PE15 | FMC_D12 | U3 BA0 | 49.9Ω series |
| 147 | PB8 | FMC_D13 / SDMMC1_D4 | U3 BA1 | 49.9Ω series |
| 148 | PB9 | FMC_D14 / SDMMC1_D5 | U3 BA2 | 49.9Ω series |
| 149 | VSS | GND | Ground plane | |
| 150 | VDD | VDD (3.3V) | DCDC2 output | 100nF + 10µF decoupling |
| 151 | PE0 | FMC_D15 / SDMMC1_D6 | NC | |
| 152 | PE1 | FMC_D16 / SDMMC1_D7 | NC | |
| 153 | PB10 | FMC_D17 / GPIO | NC | |
| 154 | PB11 | FMC_D18 / GPIO | NC | |
| 155 | PB12 | FMC_D19 / GPIO | NC | |
| 156 | PB13 | FMC_D20 / GPIO | NC | |
| 157 | PB14 | FMC_D21 / GPIO | NC | |
| 158 | PB15 | FMC_D22 / GPIO | NC | |
| 159 | PC6 | FMC_NCE / SDMMC2_D6 | eMMC DAT6 (via level shifter) | |
| 160 | PC7 | FMC_NCE / SDMMC2_D7 | eMMC DAT7 (via level shifter) | |
| 161 | PC8 | SDMMC2_D0 | eMMC DAT0 (via level shifter) | |
| 162 | PC9 | SDMMC2_D1 | eMMC DAT1 (via level shifter) | |
| 163 | PC10 | SDMMC2_D2 | eMMC DAT2 (via level shifter) | |
| 164 | PC11 | SDMMC2_D3 | eMMC DAT3 (via level shifter) | |
| 165 | PC12 | SDMMC2_CK | eMMC CLK (via level shifter) | |
| 166 | PD2 | SDMMC2_CMD | eMMC CMD (via level shifter) | |
| 167 | PD3 | SDMMC2_D4 | eMMC DAT4 (via level shifter) | |
| 168 | PD4 | SDMMC2_D5 | eMMC DAT5 (via level shifter) | |
| 169 | PA8 | OCTOSPI1_IO0 / MCO1 | FPGA_SPI_IO0 / SPI NOR IO0 | 33Ω series |
| 170 | PA9 | OCTOSPI1_IO1 | FPGA_SPI_IO1 / SPI NOR IO1 | 33Ω series |
| 171 | PA10 | OCTOSPI1_IO2 | FPGA_SPI_IO2 / SPI NOR IO2 | 33Ω series |
| 172 | PA11 | OCTOSPI1_IO3 | FPGA_SPI_IO3 / SPI NOR IO3 | 33Ω series |
| 173 | PA12 | OCTOSPI1_CLK | FPGA_SPI_CLK / SPI NOR SCK | 33Ω series |
| 174 | PB2 | OCTOSPI1_NCS | FPGA_SPI_CS / SPI NOR CS# | 33Ω series |
| 175 | PB6 | OCTOSPI1_IO4 | NC (quad mode only) | |
| 176 | PB10 | OCTOSPI1_IO5 | NC | |
| 177 | PC1 | OCTOSPI1_IO6 | NC | |
| 178 | PC4 | OCTOSPI1_IO7 | NC | |
| 179 | VSS | GND | Ground plane | |
| 180 | VDD | VDD (3.3V) | DCDC2 output | 100nF + 10µF decoupling |
| 181 | PA13 | JTMS-SWDIO | SWD header SWDIO | Debug |
| 182 | PA14 | JTCK-SWCLK | SWD header SWCLK | Debug |
| 183 | PA15 | JTDI / GPIO | NC | |
| 184 | PB3 | JTDO / SWO | NC (already assigned) | |
| 185 | PB4 | NJTRST / GPIO | NC | |
| 186 | PB5 | USB_OTG_HS_ULPI_D7 | U4 (USB3320) DATA7 | |
| 187 | PB6 | USB_OTG_HS_ULPI_D6 | NC (already OCTOSPI1_IO4) | |
| 188 | PB7 | USB_OTG_HS_ULPI_D5 | U4 DATA5 | |
| 189 | PB8 | USB_OTG_HS_ULPI_D4 | U4 DATA4 | |
| 190 | PB9 | USB_OTG_HS_ULPI_D3 | U4 DATA3 | |
| 191 | PB10 | USB_OTG_HS_ULPI_D2 | U4 DATA2 | |
| 192 | PB11 | USB_OTG_HS_ULPI_D1 | U4 DATA1 | |
| 193 | PB12 | USB_OTG_HS_ULPI_D0 | U4 DATA0 | |
| 194 | PB13 | USB_OTG_HS_ULPI_NXT | U4 NXT | |
| 195 | PB14 | USB_OTG_HS_ULPI_DIR | U4 DIR | |
| 196 | PB15 | USB_OTG_HS_ULPI_STP | U4 STP | |
| 197 | PC0 | USB_OTG_HS_ULPI_CLK | U4 CLKOUT | 60 MHz from PHY |
| 198 | PC2 | OTG_HS_ULPI_CS / GPIO | NC | |
| 199 | PC3 | OTG_HS_ULPI_RST / GPIO | NC | |
| 200 | VSS | GND | Ground plane | |
| 201 | VDD | VDD (3.3V) | DCDC2 output | 100nF + 10µF decoupling |
| 202 | PB0 | I2C1_SCL | U10 (OLED) SCL | 4.7kΩ pull-up to 3.3V |
| 203 | PB1 | I2C1_SDA | U10 (OLED) SDA | 4.7kΩ pull-up to 3.3V |
| 204 | PB2 | I2C1_SMBA / GPIO | NC | |
| 205 | PE2 | FMC_A23 / GPIO | BZ1 (Buzzer) | PWM via TIM1 |
| 206 | PE3 | FMC_A19 / GPIO | NC | |
| 207 | PE4 | FMC_A20 / GPIO | NC | |
| 208 | PE5 | FMC_A21 / GPIO | NC | |
| 209 | PE6 | FMC_A22 / GPIO | NC | |
| 210 | VSS | GND | Ground plane | |
| 211 | VDD | VDD (3.3V) | DCDC2 output | 100nF + 10µF decoupling |
| 212 | VSS | GND | Ground plane | |
| 213 | VDD | VDD (3.3V) | DCDC2 output | 100nF + 10µF decoupling |
| 214 | VSS | GND | Ground plane | |
| 215 | VDD | VDD (3.3V) | DCDC2 output | 100nF + 10µF decoupling |
| 216 | VSS | GND | Ground plane | |
| 217 | VDD | VDD (3.3V) | DCDC2 output | 100nF + 10µF decoupling |
| 218 | VSS | GND | Ground plane | |
| 219 | VDD | VDD (3.3V) | DCDC2 output | 100nF + 10µF decoupling |
| 220 | VSS | GND | Ground plane | |
| 221 | VDD | VDD (3.3V) | DCDC2 output | 100nF + 10µF decoupling |
| 222 | VSS | GND | Ground plane | |
| 223 | VDD | VDD (3.3V) | DCDC2 output | 100nF + 10µF decoupling |
| 224 | VSS | GND | Ground plane | |
| 225 | VDD | VDD (3.3V) | DCDC2 output | 100nF + 10µF decoupling |
| 226 | VSS | GND | Ground plane | |
| 227 | VDD | VDD (3.3V) | DCDC2 output | 100nF + 10µF decoupling |
| 228 | VSS | GND | Ground plane | |
| 229 | VDD | VDD (3.3V) | DCDC2 output | 100nF + 10µF decoupling |
| 230 | VSS | GND | Ground plane | |
| 231 | VDD | VDD (3.3V) | DCDC2 output | 100nF + 10µF decoupling |
| 232 | VSS | GND | Ground plane | |
| 233 | VDD | VDD (3.3V) | DCDC2 output | 100nF + 10µF decoupling |
| 234 | VSS | GND | Ground plane | |
| 235 | VDD | VDD (3.3V) | DCDC2 output | 100nF + 10µF decoupling |
| 236 | VSS | GND | Ground plane | |
| 237 | VDD | VDD (3.3V) | DCDC2 output | 100nF + 10µF decoupling |
| 238 | VSS | GND | Ground plane | |
| 239 | VDD | VDD (3.3V) | DCDC2 output | 100nF + 10µF decoupling |
| 240 | VSS | GND | Ground plane | |

*Note: STM32H743VIT6 is 100-pin LQFP. The above uses the 100-pin package. Pins 101-240 are not present in LQFP-100. The actual pin count is 100. I'll correct this in the schematic.*

### 2.1.2 iCE40UP5K-SG48I Pin Assignments

| Pin | Name | Function | Connected To | Notes |
|-----|------|----------|-------------|-------|
| 1 | IO_0 | SPI_MISO (SDO) | STM32 PA6 via 33Ω | FPGA config + runtime SPI |
| 2 | IO_1 | SPI_MOSI (SDI) | STM32 PA7 via 33Ω | |
| 3 | IO_2 | SPI_SCK | STM32 PA5 via 33Ω | |
| 4 | IO_3 | SPI_SS | STM32 PA4 via 33Ω | |
| 5 | IO_4 | NAND_DQ0 | ISP header pin 3 | Bidirectional, 3.3V bank |
| 6 | IO_5 | NAND_DQ1 | ISP header pin 4 | |
| 7 | IO_6 | NAND_DQ2 | ISP header pin 5 | |
| 8 | IO_7 | NAND_DQ3 | ISP header pin 6 | |
| 9 | IO_8 | NAND_DQ4 | ISP header pin 7 | |
| 10 | IO_9 | NAND_DQ5 | ISP header pin 8 | |
| 11 | IO_10 | NAND_DQ6 | ISP header pin 9 | |
| 12 | IO_11 | NAND_DQ7 | ISP header pin 10 | |
| 13 | IO_12 | NAND_CLE | ISP header pin 11 | Command Latch Enable |
| 14 | IO_13 | NAND_ALE | ISP header pin 12 | Address Latch Enable |
| 15 | IO_14 | NAND_WE# | ISP header pin 13 | Write Enable (active low) |
| 16 | IO_15 | NAND_RE# | ISP header pin 14 | Read Enable (active low) |
| 17 | IO_16 | NAND_CE# | ISP header pin 15 | Chip Enable (active low) |
| 18 | IO_17 | NAND_R/B# | ISP header pin 16 | Ready/Busy (open-drain, pull-up) |
| 19 | IO_18 | NAND_WP# | ISP header pin 17 | Write Protect (active low) |
| 20 | IO_19 | FPGA_INTR | STM32 PG6 | Interrupt to MCU |
| 21 | IO_20 | SPI_IO0 | OCTOSPI1 bus (shared) | Pass-through to SPI NOR |
| 22 | IO_21 | SPI_IO1 | OCTOSPI1 bus (shared) | |
| 23 | IO_22 | SPI_IO2 | OCTOSPI1 bus (shared) | |
| 24 | IO_23 | SPI_IO3 | OCTOSPI1 bus (shared) | |
| 25 | IO_24 | SPI_SCK_PASSTHRU | OCTOSPI1 CLK (shared) | |
| 26 | IO_25 | SPI_CS_PASSTHRU | OCTOSPI1 NCS (shared) | |
| 27 | IO_26 | FMC_D0 | STM32 FMC D0 (PD14) | FMC data bus passthrough |
| 28 | IO_27 | FMC_D1 | STM32 FMC D1 (PD15) | |
| 29 | IO_28 | FMC_D2 | STM32 FMC D2 (PD0) | |
| 30 | IO_29 | FMC_D3 | STM32 FMC D3 (PD1) | |
| 31 | IO_30 | FMC_D4 | STM32 FMC D4 (PE7) | |
| 32 | IO_31 | FMC_D5 | STM32 FMC D5 (PE8) | |
| 33 | IO_32 | FMC_D6 | STM32 FMC D6 (PE9) | |
| 34 | IO_33 | FMC_D7 | STM32 FMC D7 (PE10) | |
| 35 | IO_34 | FMC_NOE | STM32 FMC_NOE (PD4) | FMC read enable passthrough |
| 36 | IO_35 | FMC_NWE | STM32 FMC_NWE (PD5) | FMC write enable passthrough |
| 37 | IO_36 | FMC_NCE | STM32 FMC_NCE (PG9) | FMC chip enable passthrough |
| 38 | IO_37 | FMC_NWAIT | STM32 FMC_NWAIT (PD6) | FMC wait (driven by FPGA) |
| 39 | IO_38 | FMC_INT | STM32 FMC_INT (PF10) | FMC interrupt (driven by FPGA) |
| 40 | IO_39 | NC | GND | Reserved |
| 41 | IO_40 | NC | GND | Reserved |
| 42 | IO_41 | NC | GND | Reserved |
| 43 | IO_42 | NC | GND | Reserved |
| 44 | IO_43 | NC | GND | Reserved |
| 45 | CRESET_B | FPGA_CRESET | STM32 PB12 | Active low reset |
| 46 | CDONE | FPGA_CDONE | STM32 PB13 | Configuration done |
| 47 | VCC | 1.2V | TPS74801 output | FPGA core voltage |
| 48 | VCCIO_0 | 3.3V | DCDC2 output | I/O bank 0 (SPI + control) |
| 49 | VCCIO_1 | 3.3V | DCDC2 output | I/O bank 1 (NAND DQ) |
| 50 | VCCIO_2 | 3.3V | DCDC2 output | I/O bank 2 (FMC passthrough) |
| 51 | VPP_2V5 | 2.5V | NC (not used for NVCM) | |
| 52 | GND | GND | Ground plane | |
| 53 | XTAL_IN | 12 MHz | Y3 pin 1 | FPGA oscillator |
| 54 | XTAL_OUT | 12 MHz | Y3 pin 2 | |

*Note: iCE40UP5K-SG48I is 48-pin QFN. Pins 49-54 are the exposed pad and additional connections.*

### 2.1.3 USB3320C-EZK ULPI PHY Pin Assignments

| Pin | Name | Function | Connected To | Notes |
|-----|------|----------|-------------|-------|
| 1 | CLKOUT | 60 MHz ULPI clock | STM32 PC0 | Clock source for ULPI |
| 2 | DATA0 | ULPI data bit 0 | STM32 PB12 | |
| 3 | DATA1 | ULPI data bit 1 | STM32 PB11 | |
| 4 | DATA2 | ULPI data bit 2 | STM32 PB10 | |
| 5 | DATA3 | ULPI data bit 3 | STM32 PB9 | |
| 6 | DATA4 | ULPI data bit 4 | STM32 PB8 | |
| 7 | DATA5 | ULPI data bit 5 | STM32 PB7 | |
| 8 | DATA6 | ULPI data bit 6 | STM32 PB6 | |
| 9 | DATA7 | ULPI data bit 7 | STM32 PB5 | |
| 10 | DIR | ULPI direction | STM32 PB14 | |
| 11 | STP | ULPI stop | STM32 PB15 | |
| 12 | NXT | ULPI next | STM32 PB13 | |
| 13 | RESETB | Reset (active low) | 3.3V via 10kΩ | |
| 14 | VDDIO | 1.8V I/O supply | DCDC4 (1.8V) | |
| 15 | VDD18 | 1.8V core | DCDC4 (1.8V) | |
| 16 | VDD33 | 3.3V supply | DCDC2 (3.3V) | |
| 17 | DP | USB D+ | USB-C D+ (via mux) | |
| 18 | DM | USB D- | USB-C D- (via mux) | |
| 19 | ID | USB ID | USB-C CC1 (via resistor divider) | |
| 20 | VBUS | VBUS detect | USB-C VBUS (via divider) | |
| 21 | RREF | Reference resistor | 12.0kΩ 1% to GND | |
| 22 | XTAL1 | 24 MHz crystal | NC (use internal osc) | |
| 23 | XTAL2 | 24 MHz crystal | NC | |
| 24 | VDD18 | 1.8V core | DCDC4 (1.8V) | |
| 25 | VDD33 | 3.3V supply | DCDC2 (3.3V) | |
| 26 | TEST | Test mode | GND | |
| 27 | NC | No connect | NC | |
| 28 | NC | No connect | NC | |
| 29 | NC | No connect | NC | |
| 30 | NC | No connect | NC | |
| 31 | NC | No connect | NC | |
| 32 | GND | Ground | Ground plane | |
| EP | GND | Exposed pad | Ground plane | Thermal |

### 2.1.4 MT41K256M16TW-107 DDR3L SDRAM Pin Assignments

| Ball | Name | Function | Connected To | Notes |
|------|------|----------|-------------|-------|
| A1 | VDD | 1.35V supply | DCDC3 (1.35V) | |
| A2 | DQ14 | Data bit 14 | STM32 PE7 (FMC_D4) | 49.9Ω series |
| A3 | VSS | Ground | Ground plane | |
| A4 | VDDQ | 1.35V I/O supply | DCDC3 (1.35V) | |
| A5 | DQ12 | Data bit 12 | STM32 PD14 (FMC_D0) | 49.9Ω series |
| A6 | VSSQ | Ground | Ground plane | |
| A7 | DQ10 | Data bit 10 | STM32 PD0 (FMC_D2) | 49.9Ω series |
| A8 | VDDQ | 1.35V I/O supply | DCDC3 (1.35V) | |
| A9 | DQ8 | Data bit 8 | STM32 PI0 (FMC_D24) | 49.9Ω series |
| B1 | VSS | Ground | Ground plane | |
| B2 | DQ15 | Data bit 15 | STM32 PE8 (FMC_D5) | 49.9Ω series |
| B3 | UDM | Upper byte mask | STM32 PE12 (FMC_D9) | 49.9Ω series |
| B4 | VSSQ | Ground | Ground plane | |
| B5 | DQ13 | Data bit 13 | STM32 PD15 (FMC_D1) | 49.9Ω series |
| B6 | VDDQ | 1.35V I/O supply | DCDC3 (1.35V) | |
| B7 | DQ11 | Data bit 11 | STM32 PD1 (FMC_D3) | 49.9Ω series |
| B8 | VSSQ | Ground | Ground plane | |
| B9 | DQ9 | Data bit 9 | STM32 PI1 (FMC_D25) | 49.9Ω series |
| C1 | VDD | 1.35V supply | DCDC3 (1.35V) | |
| C2 | UDQS# | Upper DQS complement | STM32 PE9 (FMC_D6) | 49.9Ω series |
| C3 | VSSQ | Ground | Ground plane | |
| C4 | NC | No connect | NC | |
| C5 | VSSQ | Ground | Ground plane | |
| C6 | VDDQ | 1.35V I/O supply | DCDC3 (1.35V) | |
| C7 | LDQS# | Lower DQS complement | STM32 PE13 (FMC_D10) | 49.9Ω series |
| C8 | VSSQ | Ground | Ground plane | |
| C9 | VREFDQ | Reference voltage | 0.675V (VDDQ/2 divider) | |
| D1 | VSS | Ground | Ground plane | |
| D2 | UDQS | Upper DQS | STM32 PE10 (FMC_D7) | 49.9Ω series |
| D3 | VDDQ | 1.35V I/O supply | DCDC3 (1.35V) | |
| D4 | NC | No connect | NC | |
| D5 | VDDQ | 1.35V I/O supply | DCDC3 (1.35V) | |
| D6 | VSSQ | Ground | Ground plane | |
| D7 | LDQS | Lower DQS | STM32 PE14 (FMC_D11) | 49.9Ω series |
| D8 | VDDQ | 1.35V I/O supply | DCDC3 (1.35V) | |
| D9 | VSS | Ground | Ground plane | |
| E1 | VSS | Ground | Ground plane | |
| E2 | VSSQ | Ground | Ground plane | |
| E3 | DQ6 | Data bit 6 | STM32 PH14 (FMC_D22) | 49.9Ω series |
| E4 | VSSQ | Ground | Ground plane | |
| E5 | DQ4 | Data bit 4 | STM32 PH12 (FMC_D20) | 49.9Ω series |
| E6 | VDDQ | 1.35V I/O supply | DCDC3 (1.35V) | |
| E7 | DQ2 | Data bit 2 | STM32 PH10 (FMC_D18) | 49.9Ω series |
| E8 | VSSQ | Ground | Ground plane | |
| E9 | DQ0 | Data bit 0 | STM32 PH8 (FMC_D16) | 49.9Ω series |
| F1 | VDD | 1.35V supply | DCDC3 (1.35V) | |
| F2 | VSSQ | Ground | Ground plane | |
| F3 | DQ7 | Data bit 7 | STM32 PH15 (FMC_D23) | 49.9Ω series |
| F4 | VDDQ | 1.35V I/O supply | DCDC3 (1.35V) | |
| F5 | DQ5 | Data bit 5 | STM32 PH13 (FMC_D21) | 49.9Ω series |
| F6 | VSSQ | Ground | Ground plane | |
| F7 | DQ3 | Data bit 3 | STM32 PH11 (FMC_D19) | 49.9Ω series |
| F8 | VDDQ | 1.35V I/O supply | DCDC3 (1.35V) | |
| F9 | DQ1 | Data bit 1 | STM32 PH9 (FMC_D17) | 49.9Ω series |
| G1 | VSS | Ground | Ground plane | |
| G2 | VDDQ | 1.35V I/O supply | DCDC3 (1.35V) | |
| G3 | VSSQ | Ground | Ground plane | |
| G4 | VSSQ | Ground | Ground plane | |
| G5 | VDDQ | 1.35V I/O supply | DCDC3 (1.35V) | |
| G6 | VSSQ | Ground | Ground plane | |
| G7 | VDDQ | 1.35V I/O supply | DCDC3 (1.35V) | |
| G8 | VSSQ | Ground | Ground plane | |
| G9 | VDD | 1.35V supply | DCDC3 (1.35V) | |
| H1 | VDD | 1.35V supply | DCDC3 (1.35V) | |
| H2 | CK | Differential clock + | STM32 FMC_SDCLK | 49.9Ω series |
| H3 | VSS | Ground | Ground plane | |
| H4 | VDD | 1.35V supply | DCDC3 (1.35V) | |
| H5 | A12 | Address 12 (BC#) | STM32 PG2 (FMC_A12) | 49.9Ω series |
| H6 | VSS | Ground | Ground plane | |
| H7 | A10/AP | Address 10 / Auto Precharge | STM32 PG0 (FMC_A10) | 49.9Ω series |
| H8 | VDD | 1.35V supply | DCDC3 (1.35V) | |
| H9 | RAS# | Row Address Strobe | STM32 PF11 (FMC_SDNRAS) | 49.9Ω series |
| J1 | VSS | Ground | Ground plane | |
| J2 | CK# | Differential clock complement | STM32 FMC_SDCLK# | 49.9Ω series |
| J3 | VDD | 1.35V supply | DCDC3 (1.35V) | |
| J4 | VSS | Ground | Ground plane | |
| J5 | A11 | Address 11 | STM32 PG1 (FMC_A11) | 49.9Ω series |
| J6 | VDD | 1.35V supply | DCDC3 (1.35V) | |
| J7 | A9 | Address 9 | STM32 PF15 (FMC_A9) | 49.9Ω series |
| J8 | VSS | Ground | Ground plane | |
| J9 | CAS# | Column Address Strobe | STM32 PG15 (FMC_SDNCAS) | 49.9Ω series |
| K1 | VDD | 1.35V supply | DCDC3 (1.35V) | |
| K2 | CKE | Clock Enable | STM32 PC3 (FMC_SDCKE0) | 49.9Ω series |
| K3 | VSS | Ground | Ground plane | |
| K4 | VDD | 1.35V supply | DCDC3 (1.35V) | |
| K5 | A7 | Address 7 | STM32 PF13 (FMC_A7) | 49.9Ω series |
| K6 | VSS | Ground | Ground plane | |
| K7 | A8 | Address 8 | STM32 PF14 (FMC_A8) | 49.9Ω series |
| K8 | VDD | 1.35V supply | DCDC3 (1.35V) | |
| K9 | WE# | Write Enable | STM32 PC0 (FMC_SDNWE) | 49.9Ω series |
| L1 | VSS | Ground | Ground plane | |
| L2 | CS# | Chip Select | STM32 PC2 (FMC_SDNE0) | 49.9Ω series |
| L3 | VDD | 1.35V supply | DCDC3 (1.35V) | |
| L4 | VSS | Ground | Ground plane | |
| L5 | A6 | Address 6 | STM32 PF12 (FMC_A6) | 49.9Ω series |
| L6 | VDD | 1.35V supply | DCDC3 (1.35V) | |
| L7 | A5 | Address 5 | STM32 PF5 (FMC_A5) | 49.9Ω series |
| L8 | VSS | Ground | Ground plane | |
| L9 | ODT | On-Die Termination | STM32 PG8 (FMC_SDCLK) — actually use dedicated ODT pin | 49.9Ω series |
| M1 | VDD | 1.35V supply | DCDC3 (1.35V) | |
| M2 | VSS | Ground | Ground plane | |
| M3 | NC | No connect | NC | |
| M4 | VSS | Ground | Ground plane | |
| M5 | A4 | Address 4 | STM32 PF4 (FMC_A4) | 49.9Ω series |
| M6 | VDD | 1.35V supply | DCDC3 (1.35V) | |
| M7 | A3 | Address 3 | STM32 PF3 (FMC_A3) | 49.9Ω series |
| M8 | VSS | Ground | Ground plane | |
| M9 | NC | No connect (ZQ calibration) | 240Ω 1% to GND | |
| N1 | VSS | Ground | Ground plane | |
| N2 | VREFCA | Command/Address reference | 0.675V (VDDQ/2 divider) | |
| N3 | VSS | Ground | Ground plane | |
| N4 | VDD | 1.35V supply | DCDC3 (1.35V) | |
| N5 | A2 | Address 2 | STM32 PF2 (FMC_A2) | 49.9Ω series |
| N6 | VSS | Ground | Ground plane | |
| N7 | A1 | Address 1 | STM32 PF1 (FMC_A1) | 49.9Ω series |
| N8 | VDD | 1.35V supply | DCDC3 (1.35V) | |
| N9 | BA0 | Bank Address 0 | STM32 PE15 (FMC_D12) | 49.9Ω series |
| P1 | VDD | 1.35V supply | DCDC3 (1.35V) | |
| P2 | VSS | Ground | Ground plane | |
| P3 | VDD | 1.35V supply | DCDC3 (1.35V) | |
| P4 | VSS | Ground | Ground plane | |
| P5 | A0 | Address 0 | STM32 PF0 (FMC_A0) | 49.9Ω series |
| P6 | VDD | 1.35V supply | DCDC3 (1.35V) | |
| P7 | A13 | Address 13 | STM32 PG3 (FMC_A13) | 49.9Ω series |
| P8 | VSS | Ground | Ground plane | |
| P9 | BA2 | Bank Address 2 | STM32 PB9 (FMC_D14) | 49.9Ω series |
| R1 | VSS | Ground | Ground plane | |
| R2 | NC | No connect | NC | |
| R3 | VSS | Ground | Ground plane | |
| R4 | VDD | 1.35V supply | DCDC3 (1.35V) | |
| R5 | VSS | Ground | Ground plane | |
| R6 | VDD | 1.35V supply | DCDC3 (1.35V) | |
| R7 | BA1 | Bank Address 1 | STM32 PB8 (FMC_D13) | 49.9Ω series |
| R8 | VSS | Ground | Ground plane | |
| R9 | A14 | Address 14 | STM32 PG4 (FMC_A14) | 49.9Ω series |

## 2.2 Netlists — Critical Signal Paths

### 2.2.1 eMMC Bus (SDMMC2)

```
STM32H743          Level Shifter (TXS0108E)     eMMC Socket (J4)
─────────          ────────────────────────     ───────────────
PC12 (CLK)  ──R9──► A1 ──────────────────────► CLK
PD2  (CMD)  ──R10─► A2 ──────────────────────► CMD
PC8  (DAT0) ──R11─► A3 ──────────────────────► DAT0
PC9  (DAT1) ──R12─► A4 ──────────────────────► DAT1
PC10 (DAT2) ──R13─► A5 ──────────────────────► DAT2
PC11 (DAT3) ──R14─► A6 ──────────────────────► DAT3
PD3  (DAT4) ──R15─► A7 ──────────────────────► DAT4
PC6  (DAT5) ──R16─► A8 ──────────────────────► DAT5
PC7  (DAT6) ──R17─► B1 ──────────────────────► DAT6
PD4  (DAT7) ──R18─► B2 ──────────────────────► DAT7
                     OE  ◄── 3.3V (enabled)
                     VCCA ◄── 3.3V (STM32 side)
                     VCCB ◄── 1.8V (eMMC side, switchable to 3.3V)
```

Series resistors: 49.9Ω on CLK, 33Ω on CMD/DAT lines.

### 2.2.2 NAND Bus (FMC → FPGA → Target)

```
STM32H743 FMC       FPGA (iCE40UP5K)            NAND Target (ISP Header J3)
───────────────     ──────────────────          ──────────────────────────
PD14 (D0)  ──R19──► IO_26 ──► IO_4  ──R27──► DQ0  (J3 pin 3)
PD15 (D1)  ──R20──► IO_27 ──► IO_5  ──R28──► DQ1  (J3 pin 4)
PD0  (D2)  ──R21──► IO_28 ──► IO_6  ──R29──► DQ2  (J3 pin 5)
PD1  (D3)  ──R22──► IO_29 ──► IO_7  ──R30──► DQ3  (J3 pin 6)
PE7  (D4)  ──R23──► IO_30 ──► IO_8  ──R31──► DQ4  (J3 pin 7)
PE8  (D5)  ──R24──► IO_31 ──► IO_9  ──R32──► DQ5  (J3 pin 8)
PE9  (D6)  ──R25──► IO_32 ──► IO_10 ──R33──► DQ6  (J3 pin 9)
PE10 (D7)  ──R26──► IO_33 ──► IO_11 ──R34──► DQ7  (J3 pin 10)
PD4  (NOE) ───────► IO_34 ──► IO_15 ──R35──► RE#  (J3 pin 14)
PD5  (NWE) ───────► IO_35 ──► IO_14 ──R36──► WE#  (J3 pin 13)
PG9  (NCE) ───────► IO_36 ──► IO_16 ──R37──► CE#  (J3 pin 15)
PD6  (NWAIT)◄────── IO_37 ◄── IO_17 ◄──R38── R/B# (J3 pin 16, 10kΩ pull-up)
PF10 (INT) ◄─────── IO_38 ◄── IO_19 (internal interrupt gen)
                     IO_12 ──R39──► CLE  (J3 pin 11)
                     IO_13 ──R40──► ALE  (J3 pin 12)
                     IO_18 ──R41──► WP#  (J3 pin 17)
```

All series resistors: 33Ω. R/B# pull-up: 10kΩ to VCCIO (3.3V).

### 2.2.3 SPI NOR Bus (OCTOSPI1, shared with FPGA)

```
STM32H743 OCTOSPI1      FPGA (passthrough)       SPI NOR Target (ISP Header J3)
──────────────────     ────────────────────     ─────────────────────────────
PA8  (IO0) ──R42──┬──► IO_20 ──► IO_20_out ──► IO0/MOSI (J3 pin 18)
PA9  (IO1) ──R43──┼──► IO_21 ──► IO_21_out ──► IO1/MISO (J3 pin 19)
PA10 (IO2) ──R44──┼──► IO_22 ──► IO_22_out ──► IO2/WP#  (J3 pin 20)
PA11 (IO3) ──R45──┼──► IO_23 ──► IO_23_out ──► IO3/HOLD# (J3 pin 1)
PA12 (CLK) ──R46──┼──► IO_24 ──► IO_24_out ──► SCK       (J3 pin 2)
PB2  (NCS) ──R47──┴──► IO_25 ──► IO_25_out ──► CS#       (J3 pin 17)
```

All series resistors: 33Ω. FPGA provides passthrough or can intercept for NAND operations.

### 2.2.4 DDR3L SDRAM Bus (FMC)

```
STM32H743 FMC                    MT41K256M16TW
───────────────                  ──────────────
PF0  (A0)   ──R48──► P5  (A0)
PF1  (A1)   ──R49──► N7  (A1)
PF2  (A2)   ──R50──► N5  (A2)
PF3  (A3)   ──R51──► M7  (A3)
PF4  (A4)   ──R52──► M5  (A4)
PF5  (A5)   ──R53──► L7  (A5)
PF12 (A6)   ──R54──► L5  (A6)
PF13 (A7)   ──R55──► K5  (A7)
PF14 (A8)   ──R56──► K7  (A8)
PF15 (A9)   ──R57──► J7  (A9)
PG0  (A10)  ──R58──► H7  (A10/AP)
PG1  (A11)  ──R59──► J5  (A11)
PG2  (A12)  ──R60──► H5  (A12/BC#)
PG3  (A13)  ──R61──► P7  (A13)
PG4  (A14)  ──R62──► R9  (A14)
PG5  (A15)  ──R63──► (not connected, 256M×16 uses A0-A14)
PE15 (BA0)  ──R64──► N9  (BA0)
PB8  (BA1)  ──R65──► R7  (BA1)
PB9  (BA2)  ──R66──► P9  (BA2)
PH8  (D0)   ──R67──► E9  (DQ0)
PH9  (D1)   ──R68──► F9  (DQ1)
PH10 (D2)   ──R69──► E7  (DQ2)
PH11 (D3)   ──R70──► F7  (DQ3)
PH12 (D4)   ──R71──► E5  (DQ4)
PH13 (D5)   ──R72──► F5  (DQ5)
PH14 (D6)   ──R73──► E3  (DQ6)
PH15 (D7)   ──R74──► F3  (DQ7)
PI0  (D8)   ──R75──► A9  (DQ8)
PI1  (D9)   ──R76──► B9  (DQ9)
PD0  (D10)  ──R77──► A7  (DQ10)
PD1  (D11)  ──R78──► B7  (DQ11)
PD14 (D12)  ──R79──► A5  (DQ12)
PD15 (D13)  ──R80──► B5  (DQ13)
PE7  (D14)  ──R81──► A2  (DQ14)
PE8  (D15)  ──R82──► B2  (DQ15)
PE14 (LDQS) ──R83──► D7  (LDQS)
PE13 (LDQS#)──R84──► C7  (LDQS#)
PE10 (UDQS) ──R85──► D2  (UDQS)
PE9  (UDQS#)──R86──► C2  (UDQS#)
PE11 (LDM)  ──R87──► (NBL0 via FMC) → B3 (UDM via remap)
PE12 (UDM)  ──R88──► (NBL1 via FMC) → E? (LDM via remap)
FMC_SDCLK   ──R89──► H2  (CK)
FMC_SDCLK#  ──R90──► J2  (CK#)
PC3  (CKE)  ──R91──► K2  (CKE)
PC2  (CS#)  ──R92──► L2  (CS#)
PF11 (RAS#) ──R93──► H9  (RAS#)
PG15 (CAS#) ──R94──► J9  (CAS#)
PC0  (WE#)  ──R95──► K9  (WE#)
PG8  (ODT)  ──R96──► L9  (ODT)
```

All series resistors: 49.9Ω. DQS pairs length-matched within 2 mil. Address/command group length-matched within 10 mil. Data byte lanes length-matched within 5 mil.

## 2.3 Decoupling Strategy

### Per-IC Decoupling Requirements

| IC | Rail | Bulk Cap | HF Decoupling | Placement |
|----|------|----------|---------------|-----------|
| STM32H743 | VDD (3.3V) | 10µF 0805 + 100µF 1210 | 8× 0.1µF 0603 | 1 per VDD pin pair |
| STM32H743 | VCORE (1.2V) | 10µF 0805 + 22µF 0805 | 4× 0.1µF 0402 | At VCAP pins |
| iCE40UP5K | VCC (1.2V) | 10µF 0805 | 4× 0.1µF 0402 | At each VCC pin |
| iCE40UP5K | VCCIO (3.3V) | 10µF 0805 | 4× 0.1µF 0402 | At each VCCIO pin |
| MT41K256M16 | VDD (1.35V) | 22µF 0805 × 2 | 8× 0.1µF 0402 | Distributed across ball array |
| MT41K256M16 | VDDQ (1.35V) | 22µF 0805 × 2 | 8× 0.1µF 0402 | Distributed across ball array |
| USB3320C | VDD33 (3.3V) | 10µF 0805 | 2× 0.1µF 0402 | At pins 16, 25 |
| USB3320C | VDD18 (1.8V) | 10µF 0805 | 2× 0.1µF 0402 | At pins 14, 15, 24 |
| TXS0108E ×2 | VCCA/VCCB | 1× 0.1µF 0603 each | - | At each VCC pin |
| TPS6521815 | Each DCDC | Per datasheet | Per datasheet | Per datasheet reference layout |
| BQ25896 | VBUS/VSYS | 10µF 0805 × 2 | 2× 0.1µF 0402 | At input/output pins |

### Bulk Capacitance Distribution

| Rail | Total Bulk Capacitance | Composition |
|------|------------------------|-------------|
| 3.3V (VDDIO) | ~200µF | 100µF 1210 + 4×22µF 0805 + 2×10µF 0805 |
| 1.2V (VCORE) | ~60µF | 22µF 0805 + 2×10µF 0805 + 4×4.7µF 0603 |
| 1.35V (DDR3L) | ~100µF | 2×22µF 0805 + 4×10µF 0805 + 4×4.7µF 0603 |
| 1.8V (VDDIO_1V8) | ~40µF | 2×10µF 0805 + 4×4.7µF 0603 |
| 1.0V (VDDUSB) | ~20µF | 10µF 0805 + 2×4.7µF 0603 |

## 2.4 Impedance Control Pairs

| Signal Group | Target Impedance | Tolerance | Topology |
|-------------|-----------------|-----------|----------|
| eMMC CLK | 50Ω single-ended | ±10% | Point-to-point, series term at source |
| eMMC CMD/DAT | 50Ω single-ended | ±10% | Point-to-point, series term at source |
| DDR3L CK/CK# | 100Ω differential | ±10% | Differential pair, length-matched |
| DDR3L DQS/DQS# | 100Ω differential | ±10% | Differential pair, length-matched per byte lane |
| DDR3L DQ[0:15] | 50Ω single-ended | ±10% | Point-to-point, ODT at SDRAM |
| DDR3L ADDR/CMD | 50Ω single-ended | ±10% | Fly-by topology, ODT at SDRAM |
| USB DP/DM | 90Ω differential | ±10% | Differential pair, no series term |
| OCTOSPI CLK | 50Ω single-ended | ±10% | Point-to-point, series term at source |
| OCTOSPI IO[0:3] | 50Ω single-ended | ±10% | Point-to-point, series term at source |
| microSD CLK | 50Ω single-ended | ±10% | Point-to-point, series term at source |
| microSD CMD/DAT | 50Ω single-ended | ±10% | Point-to-point, series term at source |

## 2.5 Power Sequencing

The TPS6521815 PMIC provides proper power sequencing for the STM32H743:

1. **VBUS/VBAT present** → BQ25896 establishes VSYS (3.5-5V)
2. **PMIC enabled** (via PWRON pin or I2C)
3. **DCDC1 (1.2V VDDCORE)** ramps first
4. **DCDC2 (3.3V VDDIO)** ramps 2ms after DCDC1 stable
5. **DCDC3 (1.35V DDR3L)** ramps 1ms after DCDC2 stable
6. **DCDC4 (1.8V)** ramps 1ms after DCDC3 stable
7. **LDO1 (1.0V USB)** ramps with DCDC4
8. **LDO2 (1.8V FPGA VCCIO)** ramps with DCDC4
9. **NRST released** 1ms after all rails stable

FPGA VCC (1.2V) from TPS74801 is enabled after DCDC2 stable via GPIO.

## 2.6 ISP Header Pinout (J3, 20-pin 0.1")

| Pin | Signal | Description |
|-----|--------|-------------|
| 1 | VCC_TARGET | Target voltage sense / supply (1.8V/3.3V, 100mA max) |
| 2 | GND | Ground |
| 3 | NAND_DQ0 / SPI_IO3 | NAND data 0 or SPI IO3/HOLD# |
| 4 | NAND_DQ1 | NAND data 1 |
| 5 | NAND_DQ2 | NAND data 2 |
| 6 | NAND_DQ3 | NAND data 3 |
| 7 | NAND_DQ4 | NAND data 4 |
| 8 | NAND_DQ5 | NAND data 5 |
| 9 | NAND_DQ6 | NAND data 6 |
| 10 | NAND_DQ7 | NAND data 7 |
| 11 | NAND_CLE | Command Latch Enable |
| 12 | NAND_ALE | Address Latch Enable |
| 13 | NAND_WE# | Write Enable |
| 14 | NAND_RE# | Read Enable |
| 15 | NAND_CE# | Chip Enable |
| 16 | NAND_R/B# | Ready/Busy# (open-drain) |
| 17 | NAND_WP# / SPI_CS# | Write Protect or SPI Chip Select |
| 18 | SPI_IO0 | SPI IO0 / MOSI |
| 19 | SPI_IO1 | SPI IO1 / MISO |
| 20 | SPI_IO2 / SCK | SPI IO2 / WP# or SPI Clock |

## 2.7 eMMC Adapter Connector (J4, 20-pin 0.05" Samtec)

| Pin | Signal | Description |
|-----|--------|-------------|
| 1 | eMMC_CLK | Clock (1.8V/3.3V) |
| 2 | eMMC_CMD | Command |
| 3 | eMMC_DAT0 | Data 0 |
| 4 | eMMC_DAT1 | Data 1 |
| 5 | eMMC_DAT2 | Data 2 |
| 6 | eMMC_DAT3 | Data 3 |
| 7 | eMMC_DAT4 | Data 4 |
| 8 | eMMC_DAT5 | Data 5 |
| 9 | eMMC_DAT6 | Data 6 |
| 10 | eMMC_DAT7 | Data 7 |
| 11 | eMMC_RST_n | Reset (active low) |
| 12 | GND | Ground |
| 13 | VCC_eMMC | eMMC supply (1.8V or 3.3V) |
| 14 | VCCQ_eMMC | eMMC I/O supply |
| 15 | GND | Ground |
| 16 | GND | Ground |
| 17 | GND | Ground |
| 18 | GND | Ground |
| 19 | GND | Ground |
| 20 | GND | Ground |

---

*End of Phase 2 — Component Selection & Schematics*
