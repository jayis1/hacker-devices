# Phase 2: Component Selection & Schematics — PCIe Screamer

## 1. Master Bill of Materials (BOM)

### 1.1 Primary Active Components

| Designator | Part Number | Manufacturer | Description | Package | Qty | Unit Cost | Extended |
|------------|-------------|--------------|-------------|---------|-----|-----------|----------|
| U1 | LFE5U-45F-6BG381C | Lattice Semiconductor | ECP5 FPGA, 45K LUTs, 381-ball caBGA | caBGA-381 (17×17 mm, 0.8 mm pitch) | 1 | $22.50 | $22.50 |
| U2, U3 | PI3EQX12908ZHE | Diodes Inc. | PCIe 3.0 8-Channel Redriver, 8 Gbps/ch | TQFN-42 (9×3.5 mm) | 2 | $4.80 | $9.60 |
| U4 | FT601Q-B-T | FTDI/Bridgetek | USB 3.0 to 32-bit FIFO Bridge | QFN-76 (9×9 mm, 0.5 mm pitch) | 1 | $8.50 | $8.50 |
| U5 | MT41K256M16TW-107:P | Micron | DDR3L SDRAM, 4 Gb (256M×16), 1866 MT/s | FBGA-96 (9×14 mm, 0.8 mm pitch) | 1 | $4.20 | $4.20 |
| U6 | W25Q128JVSIQ | Winbond | 128 Mb SPI NOR Flash, 104 MHz | SOIC-8 (208 mil) | 1 | $0.85 | $0.85 |
| U7 | Si5332A-D-GM1 | Skyworks/Silicon Labs | 6-output Any-Frequency Clock Generator | QFN-32 (5×5 mm) | 1 | $3.50 | $3.50 |
| U8, U9, U10, U11 | TPS62825DMQR | Texas Instruments | 3A Step-Down Converter, 2.4-5.5V in, adj out | VSON-6 (1.5×1.5 mm) | 4 | $1.20 | $4.80 |
| U12 | TLV75533PDBVR | Texas Instruments | 3.3V 500mA LDO Regulator | SOT-23-5 | 1 | $0.25 | $0.25 |
| U13 | TLV75518PDBVR | Texas Instruments | 1.8V 500mA LDO Regulator (USB backup) | SOT-23-5 | 1 | $0.25 | $0.25 |
| J1 | M.2 Card Edge | (PCB Gold Fingers) | M.2 M-Key edge connector, 67 positions | PCB Edge | 1 | $0.00 | $0.00 |
| J2 | 10132797-0011LF | Amphenol/FCI | M.2 M-Key Socket, top-mount, 3.2mm height | SMT | 1 | $2.80 | $2.80 |
| J3 | TYPE-C-31-M-12 | Korean Hroparts | USB 3.1 Type-C Receptacle, 16-pin, mid-mount | SMT | 1 | $0.60 | $0.60 |

### 1.2 Passive Components

| Designator | Value | Type | Package | Qty | Unit Cost | Extended |
|------------|-------|------|---------|-----|-----------|----------|
| C1-C10, C20-C29, C40-C49, C60-C69 | 100 nF | X7R MLCC, 16V | 0402 | 40 | $0.008 | $0.32 |
| C11-C14, C30-C33, C50-C53, C70-C73 | 10 µF | X7R MLCC, 16V | 0805 | 16 | $0.05 | $0.80 |
| C15-C16, C34-C35, C54-C55 | 100 µF | Tantalum Polymer, 6.3V | 1210 (Kemet T520) | 6 | $0.45 | $2.70 |
| C17-C19, C36-C39 | 1 µF | X7R MLCC, 25V | 0603 | 7 | $0.02 | $0.14 |
| C80-C83 | 22 pF | C0G/NP0, 50V | 0402 | 4 | $0.01 | $0.04 |
| C84-C87 | 2.2 µF | X7R MLCC, 10V | 0603 | 4 | $0.03 | $0.12 |
| R1-R20 | 10 kΩ | ±1% Thick Film | 0402 | 20 | $0.003 | $0.06 |
| R21-R30 | 100 Ω | ±1% Thick Film | 0402 | 10 | $0.003 | $0.03 |
| R31-R38 | 49.9 Ω | ±1% Thin Film (PCIe termination) | 0402 | 8 | $0.02 | $0.16 |
| R39-R42 | 240 Ω | ±1% Thick Film (DDR3 VTT) | 0402 | 4 | $0.003 | $0.012 |
| R43-R46 | 0 Ω | Jumper | 0402 | 4 | $0.003 | $0.012 |
| R47-R50 | 4.7 kΩ | ±1% Thick Film | 0402 | 4 | $0.003 | $0.012 |
| R51-R54 | 1 kΩ | ±1% Thick Film (LED current limit) | 0402 | 4 | $0.003 | $0.012 |
| L1-L4 | 1 µH | Power Inductor, 3.5A Isat | 0805 (TDK VLS252010ET) | 4 | $0.15 | $0.60 |
| L5-L8 | 600 Ω @ 100 MHz | Ferrite Bead (BLM18KG601SN1D) | 0603 | 4 | $0.04 | $0.16 |
| FB1-FB4 | 120 Ω @ 100 MHz | Ferrite Bead (BLM18AG121SN1D) | 0603 | 4 | $0.03 | $0.12 |
| D1 | Green | LED, 0603 | 0603 | 1 | $0.05 | $0.05 |
| D2 | Blue | LED, 0603 | 0603 | 1 | $0.08 | $0.08 |
| D3 | Amber | LED, 0603 | 0603 | 1 | $0.05 | $0.05 |
| D4 | Red | LED, 0603 | 0603 | 1 | $0.05 | $0.05 |
| Y1 | 32.768 kHz | Crystal, 12.5 pF, ±20 ppm | 3215 (2-pad SMT) | 1 | $0.30 | $0.30 |

### 1.3 Total BOM Summary

| Category | Extended Cost |
|----------|---------------|
| Active Components | $56.80 |
| Passives | $5.93 |
| Connectors | $3.40 |
| PCB (8-layer HDI, qty 1 proto) | $12.00 |
| Assembly (qty 1 proto) | $15.00 |
| **Grand Total (1 prototype)** | **$93.13** |

## 2. FPGA Pinout — LFE5U-45F-6BG381C

### 2.1 Bank Assignments

| Bank | VCCIO | Function | Key Pins |
|------|-------|----------|----------|
| Bank 0 | 3.3V | Configuration SPI, LEDs, Debug UART, GPIO | CSPI_CSN, CSPI_CLK, CSPI_MOSI, CSPI_MISO, LED[3:0], UART_TX, UART_RX, PERST#_IN, CLKREQ#_IN, PEWAKE#_IN |
| Bank 1 | 3.3V | FT601 Control Signals | FT_TXE#, FT_RXF#, FT_WR#, FT_RD#, FT_OE#, FT_SIWU#, FT_RESET#, FT_CLK |
| Bank 2 | 3.3V | FT601 Data[15:0] | FT_D[15:0] |
| Bank 3 | 3.3V | FT601 Data[31:16], BE[3:0] | FT_D[31:16], FT_BE[3:0] |
| Bank 4 | 1.2V | PCIe Hard IP SERDES (Lanes 0-1) | PCIE_RX0_P/N, PCIE_TX0_P/N, PCIE_RX1_P/N, PCIE_TX1_P/N |
| Bank 5 | 1.2V | PCIe Hard IP SERDES (Lanes 2-3) | PCIE_RX2_P/N, PCIE_TX2_P/N, PCIE_RX3_P/N, PCIE_TX3_P/N |
| Bank 6 | 1.35V (SSTL135) | DDR3 Interface | DDR3_A[14:0], DDR3_BA[2:0], DDR3_DQ[15:0], DDR3_DQS[1:0], DDR3_DM[1:0], DDR3_CK_P/N, DDR3_CKE, DDR3_CS#, DDR3_RAS#, DDR3_CAS#, DDR3_WE#, DDR3_ODT, DDR3_RESET# |
| Bank 7 | 1.2V | PCIe Reference Clock, Control Signals | PCIE_REFCLK_P/N, PCIE_WAKE#, PCIE_PERST#, PCIE_CLKREQ# |

### 2.2 Detailed Pin Mapping — Bank 0 (3.3V, Configuration & GPIO)

| Ball | Pin Name | Function | Connected To |
|------|----------|----------|--------------|
| A3 | CSPI_CSN | SPI Flash Chip Select | U6 (W25Q128) Pin 1 (CS#) |
| B3 | CSPI_CLK | SPI Flash Clock | U6 Pin 6 (CLK) |
| C3 | CSPI_MOSI | SPI Flash MOSI | U6 Pin 5 (DI) |
| D3 | CSPI_MISO | SPI Flash MISO | U6 Pin 2 (DO) |
| A4 | GPIO_0 | LED0 (Green) — PCIe Link Up | D1 anode via R51 (1kΩ) |
| B4 | GPIO_1 | LED1 (Blue) — USB Enumeration | D2 anode via R52 (1kΩ) |
| C4 | GPIO_2 | LED2 (Amber) — Capture Active | D3 anode via R53 (1kΩ) |
| D4 | GPIO_3 | LED3 (Red) — Error/Fault | D4 anode via R54 (1kΩ) |
| A5 | GPIO_4 | Debug UART TX | Test point TP1 |
| B5 | GPIO_5 | Debug UART RX | Test point TP2 |
| C5 | GPIO_6 | PERST# Monitor Input | M.2 Host Pin 50 (PERST#) via 10k series R |
| D5 | GPIO_7 | CLKREQ# Monitor Input | M.2 Host Pin 56 (CLKREQ#) via 10k series R |
| A6 | GPIO_8 | PEWAKE# Monitor Input | M.2 Host Pin 58 (PEWAKE#) via 10k series R |
| B6 | GPIO_9 | Si5332 I2C SCL | U7 Pin 25 (SCL) |
| C6 | GPIO_10 | Si5332 I2C SDA | U7 Pin 26 (SDA) |
| D6 | GPIO_11 | FT601 Interrupt (optional) | U4 Pin 9 (INT#) |
| A7 | GPIO_12 | Redriver Control SCL (shared) | U2, U3 SCL pins |
| B7 | GPIO_13 | Redriver Control SDA (shared) | U2, U3 SDA pins |
| C7 | GPIO_14 | Power Good Monitor | TPS62825 PG outputs (wire-OR) |
| D7 | GPIO_15 | Reserved / Test Point | TP3 |

### 2.3 Detailed Pin Mapping — Bank 1 (3.3V, FT601 Control)

| Ball | Pin Name | Function | Connected To |
|------|----------|----------|--------------|
| E1 | FT_TXE# | Transmit FIFO Empty | U4 Pin 38 (TXE#) |
| F1 | FT_RXF# | Receive FIFO Full | U4 Pin 39 (RXF#) |
| G1 | FT_WR# | Write Strobe | U4 Pin 40 (WR#) |
| H1 | FT_RD# | Read Strobe | U4 Pin 41 (RD#) |
| J1 | FT_OE# | Output Enable | U4 Pin 42 (OE#) |
| K1 | FT_SIWU# | Send Immediate / Wake Up | U4 Pin 37 (SIWU#) |
| L1 | FT_RESET# | FT601 Reset | U4 Pin 36 (RESET#) |
| M1 | FT_CLK | 100 MHz Clock Output | U4 Pin 35 (CLK) |
| E2 | FT_WAKEUP | Wakeup signal (optional) | U4 Pin 8 (WAKEUP) |
| F2 | FT_GPIO0 | FT601 GPIO0 | U4 Pin 7 (GPIO0) |
| G2 | FT_GPIO1 | FT601 GPIO1 | U4 Pin 6 (GPIO1) |
| H2 | FT_SUSPEND | Suspend indicator | U4 Pin 5 (SUSPEND#) |

### 2.4 Detailed Pin Mapping — Bank 2 (3.3V, FT601 Data[15:0])

| Ball | Pin Name | Function | Connected To |
|------|----------|----------|--------------|
| E3 | FT_D0 | FT601 Data Bit 0 | U4 Pin 19 (D0) |
| F3 | FT_D1 | FT601 Data Bit 1 | U4 Pin 20 (D1) |
| G3 | FT_D2 | FT601 Data Bit 2 | U4 Pin 21 (D2) |
| H3 | FT_D3 | FT601 Data Bit 3 | U4 Pin 22 (D3) |
| J3 | FT_D4 | FT601 Data Bit 4 | U4 Pin 23 (D4) |
| K3 | FT_D5 | FT601 Data Bit 5 | U4 Pin 24 (D5) |
| L3 | FT_D6 | FT601 Data Bit 6 | U4 Pin 25 (D6) |
| M3 | FT_D7 | FT601 Data Bit 7 | U4 Pin 26 (D7) |
| E4 | FT_D8 | FT601 Data Bit 8 | U4 Pin 27 (D8) |
| F4 | FT_D9 | FT601 Data Bit 9 | U4 Pin 28 (D9) |
| G4 | FT_D10 | FT601 Data Bit 10 | U4 Pin 29 (D10) |
| H4 | FT_D11 | FT601 Data Bit 11 | U4 Pin 30 (D11) |
| J4 | FT_D12 | FT601 Data Bit 12 | U4 Pin 31 (D12) |
| K4 | FT_D13 | FT601 Data Bit 13 | U4 Pin 32 (D13) |
| L4 | FT_D14 | FT601 Data Bit 14 | U4 Pin 33 (D14) |
| M4 | FT_D15 | FT601 Data Bit 15 | U4 Pin 34 (D15) |

### 2.5 Detailed Pin Mapping — Bank 3 (3.3V, FT601 Data[31:16] + BE)

| Ball | Pin Name | Function | Connected To |
|------|----------|----------|--------------|
| E5 | FT_D16 | FT601 Data Bit 16 | U4 Pin 10 (D16) |
| F5 | FT_D17 | FT601 Data Bit 17 | U4 Pin 11 (D17) |
| G5 | FT_D18 | FT601 Data Bit 18 | U4 Pin 12 (D18) |
| H5 | FT_D19 | FT601 Data Bit 19 | U4 Pin 13 (D19) |
| J5 | FT_D20 | FT601 Data Bit 20 | U4 Pin 14 (D20) |
| K5 | FT_D21 | FT601 Data Bit 21 | U4 Pin 15 (D21) |
| L5 | FT_D22 | FT601 Data Bit 22 | U4 Pin 16 (D22) |
| M5 | FT_D23 | FT601 Data Bit 23 | U4 Pin 17 (D23) |
| E6 | FT_D24 | FT601 Data Bit 24 | U4 Pin 1 (D24) |
| F6 | FT_D25 | FT601 Data Bit 25 | U4 Pin 2 (D25) |
| G6 | FT_D26 | FT601 Data Bit 26 | U4 Pin 3 (D26) |
| H6 | FT_D27 | FT601 Data Bit 27 | U4 Pin 4 (D27) |
| J6 | FT_D28 | FT601 Data Bit 28 | U4 Pin 75 (D28) |
| K6 | FT_D29 | FT601 Data Bit 29 | U4 Pin 74 (D29) |
| L6 | FT_D30 | FT601 Data Bit 30 | U4 Pin 73 (D30) |
| M6 | FT_D31 | FT601 Data Bit 31 | U4 Pin 72 (D31) |
| E7 | FT_BE0 | Byte Enable 0 | U4 Pin 18 (BE0) |
| F7 | FT_BE1 | Byte Enable 1 | U4 Pin 76 (BE1) |
| G7 | FT_BE2 | Byte Enable 2 | U4 Pin 71 (BE2) |
| H7 | FT_BE3 | Byte Enable 3 | U4 Pin 70 (BE3) |

### 2.6 Detailed Pin Mapping — Bank 4 (1.2V, PCIe SERDES Lanes 0-1)

| Ball | Pin Name | Function | Connected To |
|------|----------|----------|--------------|
| N1 | PCIE_RX0_P | Lane 0 Receive Positive | U2 (PI3EQX12908) Ch0 B+ (device-side RX) |
| N2 | PCIE_RX0_N | Lane 0 Receive Negative | U2 Ch0 B- |
| P1 | PCIE_TX0_P | Lane 0 Transmit Positive | U2 Ch0 A+ (host-side TX) |
| P2 | PCIE_TX0_N | Lane 0 Transmit Negative | U2 Ch0 A- |
| N3 | PCIE_RX1_P | Lane 1 Receive Positive | U2 Ch1 B+ |
| N4 | PCIE_RX1_N | Lane 1 Receive Negative | U2 Ch1 B- |
| P3 | PCIE_TX1_P | Lane 1 Transmit Positive | U2 Ch1 A+ |
| P4 | PCIE_TX1_N | Lane 1 Transmit Negative | U2 Ch1 A- |

### 2.7 Detailed Pin Mapping — Bank 5 (1.2V, PCIe SERDES Lanes 2-3)

| Ball | Pin Name | Function | Connected To |
|------|----------|----------|--------------|
| R1 | PCIE_RX2_P | Lane 2 Receive Positive | U3 (PI3EQX12908) Ch2 B+ |
| R2 | PCIE_RX2_N | Lane 2 Receive Negative | U3 Ch2 B- |
| T1 | PCIE_TX2_P | Lane 2 Transmit Positive | U3 Ch2 A+ |
| T2 | PCIE_TX2_N | Lane 2 Transmit Negative | U3 Ch2 A- |
| R3 | PCIE_RX3_P | Lane 3 Receive Positive | U3 Ch3 B+ |
| R4 | PCIE_RX3_N | Lane 3 Receive Negative | U3 Ch3 B- |
| T3 | PCIE_TX3_P | Lane 3 Transmit Positive | U3 Ch3 A+ |
| T4 | PCIE_TX3_N | Lane 3 Transmit Negative | U3 Ch3 A- |

### 2.8 Detailed Pin Mapping — Bank 6 (1.35V SSTL135, DDR3)

| Ball | Pin Name | Function | Connected To |
|------|----------|----------|--------------|
| U1 | DDR3_A0 | Address Bit 0 | U5 Pin N7 (A0) |
| U2 | DDR3_A1 | Address Bit 1 | U5 Pin M7 (A1) |
| U3 | DDR3_A2 | Address Bit 2 | U5 Pin M2 (A2) |
| V1 | DDR3_A3 | Address Bit 3 | U5 Pin N2 (A3) |
| V2 | DDR3_A4 | Address Bit 4 | U5 Pin M3 (A4) |
| V3 | DDR3_A5 | Address Bit 5 | U5 Pin N3 (A5) |
| W1 | DDR3_A6 | Address Bit 6 | U5 Pin P8 (A6) |
| W2 | DDR3_A7 | Address Bit 7 | U5 Pin P3 (A7) |
| W3 | DDR3_A8 | Address Bit 8 | U5 Pin R8 (A8) |
| Y1 | DDR3_A9 | Address Bit 9 | U5 Pin R3 (A9) |
| Y2 | DDR3_A10 | Address Bit 10 | U5 Pin L7 (A10/AP) |
| Y3 | DDR3_A11 | Address Bit 11 | U5 Pin R7 (A11) |
| AA1 | DDR3_A12 | Address Bit 12 | U5 Pin T8 (A12/BC#) |
| AA2 | DDR3_A13 | Address Bit 13 | U5 Pin N8 (A13) |
| AA3 | DDR3_A14 | Address Bit 14 | U5 Pin T3 (A14) |
| AB1 | DDR3_BA0 | Bank Address 0 | U5 Pin M6 (BA0) |
| AB2 | DDR3_BA1 | Bank Address 1 | U5 Pin P2 (BA1) |
| AB3 | DDR3_BA2 | Bank Address 2 | U5 Pin P7 (BA2) |
| AC1 | DDR3_DQ0 | Data Bit 0 | U5 Pin G8 (DQ0) |
| AC2 | DDR3_DQ1 | Data Bit 1 | U5 Pin G7 (DQ1) |
| AC3 | DDR3_DQ2 | Data Bit 2 | U5 Pin F7 (DQ2) |
| AD1 | DDR3_DQ3 | Data Bit 3 | U5 Pin F8 (DQ3) |
| AD2 | DDR3_DQ4 | Data Bit 4 | U5 Pin E7 (DQ4) |
| AD3 | DDR3_DQ5 | Data Bit 5 | U5 Pin D7 (DQ5) |
| AE1 | DDR3_DQ6 | Data Bit 6 | U5 Pin E8 (DQ6) |
| AE2 | DDR3_DQ7 | Data Bit 7 | U5 Pin C8 (DQ7) |
| AE3 | DDR3_DQ8 | Data Bit 8 | U5 Pin C3 (DQ8) |
| AF1 | DDR3_DQ9 | Data Bit 9 | U5 Pin C2 (DQ9) |
| AF2 | DDR3_DQ10 | Data Bit 10 | U5 Pin A2 (DQ10) |
| AF3 | DDR3_DQ11 | Data Bit 11 | U5 Pin A3 (DQ11) |
| AG1 | DDR3_DQ12 | Data Bit 12 | U5 Pin B1 (DQ12) |
| AG2 | DDR3_DQ13 | Data Bit 13 | U5 Pin A7 (DQ13) |
| AG3 | DDR3_DQ14 | Data Bit 14 | U5 Pin A8 (DQ14) |
| AH1 | DDR3_DQ15 | Data Bit 15 | U5 Pin B8 (DQ15) |
| AH2 | DDR3_DQS0_P | Data Strobe 0 Positive | U5 Pin F2 (UDQS) |
| AH3 | DDR3_DQS0_N | Data Strobe 0 Negative | U5 Pin F3 (UDQS#) |
| AJ1 | DDR3_DQS1_P | Data Strobe 1 Positive | U5 Pin B2 (LDQS) |
| AJ2 | DDR3_DQS1_N | Data Strobe 1 Negative | U5 Pin B3 (LDQS#) |
| AJ3 | DDR3_DM0 | Data Mask 0 | U5 Pin E3 (UDM) |
| AK1 | DDR3_DM1 | Data Mask 1 | U5 Pin D1 (LDM) |
| AK2 | DDR3_CK_P | Differential Clock Positive | U5 Pin K7 (CK) |
| AK3 | DDR3_CK_N | Differential Clock Negative | U5 Pin J7 (CK#) |
| AL1 | DDR3_CKE | Clock Enable | U5 Pin K9 (CKE) |
| AL2 | DDR3_CS# | Chip Select | U5 Pin L2 (CS#) |
| AL3 | DDR3_RAS# | Row Address Strobe | U5 Pin J3 (RAS#) |
| AM1 | DDR3_CAS# | Column Address Strobe | U5 Pin K3 (CAS#) |
| AM2 | DDR3_WE# | Write Enable | U5 Pin L3 (WE#) |
| AM3 | DDR3_ODT | On-Die Termination | U5 Pin K1 (ODT) |
| AN1 | DDR3_RESET# | DDR3 Reset | U5 Pin T2 (RESET#) |

### 2.9 Detailed Pin Mapping — Bank 7 (1.2V, PCIe Reference Clock & Control)

| Ball | Pin Name | Function | Connected To |
|------|----------|----------|--------------|
| AP1 | PCIE_REFCLK_P | PCIe Reference Clock Positive | U7 (Si5332) OUT0+ via AC-coupling cap (100 nF) |
| AP2 | PCIE_REFCLK_N | PCIe Reference Clock Negative | U7 OUT0- via AC-coupling cap (100 nF) |
| AR1 | PCIE_PERST# | PCIe Reset Output (to device) | J2 (M.2 Socket) Pin 50 (PERST#) |
| AR2 | PCIE_CLKREQ# | PCIe Clock Request (to device) | J2 Pin 56 (CLKREQ#) |
| AR3 | PCIE_WAKE# | PCIe Wake (to device) | J2 Pin 58 (PEWAKE#) |

## 3. PCIe Redriver Configuration — PI3EQX12908ZHE

### 3.1 U2 (Lanes 0-1) Pinout

| Pin | Name | Function | Connected To |
|-----|------|----------|--------------|
| 1 | VDD | 3.3V Supply | VCC3V3 rail |
| 2 | CH0_A+ | Channel 0 Host-Side Positive | M.2 Host Edge PETp0 |
| 3 | CH0_A- | Channel 0 Host-Side Negative | M.2 Host Edge PETn0 |
| 4 | CH0_B+ | Channel 0 Device-Side Positive | FPGA PCIE_RX0_P |
| 5 | CH0_B- | Channel 0 Device-Side Negative | FPGA PCIE_RX0_N |
| 6 | GND | Ground | GND plane |
| 7 | CH1_A+ | Channel 1 Host-Side Positive | M.2 Host Edge PETp1 |
| 8 | CH1_A- | Channel 1 Host-Side Negative | M.2 Host Edge PETn1 |
| 9 | CH1_B+ | Channel 1 Device-Side Positive | FPGA PCIE_RX1_P |
| 10 | CH1_B- | Channel 1 Device-Side Negative | FPGA PCIE_RX1_N |
| 11 | VDD | 3.3V Supply | VCC3V3 rail |
| 12 | CH2_A+ | Channel 2 Host-Side Positive | FPGA PCIE_TX0_P |
| 13 | CH2_A- | Channel 2 Host-Side Negative | FPGA PCIE_TX0_N |
| 14 | CH2_B+ | Channel 2 Device-Side Positive | M.2 Host Edge PERp0 |
| 15 | CH2_B- | Channel 2 Device-Side Negative | M.2 Host Edge PERn0 |
| 16 | GND | Ground | GND plane |
| 17 | CH3_A+ | Channel 3 Host-Side Positive | FPGA PCIE_TX1_P |
| 18 | CH3_A- | Channel 3 Host-Side Negative | FPGA PCIE_TX1_N |
| 19 | CH3_B+ | Channel 3 Device-Side Positive | M.2 Host Edge PERp1 |
| 20 | CH3_B- | Channel 3 Device-Side Negative | M.2 Host Edge PERn1 |
| 21 | VDD | 3.3V Supply | VCC3V3 rail |
| 22 | CH4_A+ | Channel 4 Host-Side Positive | FPGA PCIE_TX0_P (loopback for device-side) |
| 23 | CH4_A- | Channel 4 Host-Side Negative | FPGA PCIE_TX0_N |
| 24 | CH4_B+ | Channel 4 Device-Side Positive | J2 (Device Socket) PETp0 |
| 25 | CH4_B- | Channel 4 Device-Side Negative | J2 PETn0 |
| 26 | GND | Ground | GND plane |
| 27 | CH5_A+ | Channel 5 Host-Side Positive | FPGA PCIE_TX1_P |
| 28 | CH5_A- | Channel 5 Host-Side Negative | FPGA PCIE_TX1_N |
| 29 | CH5_B+ | Channel 5 Device-Side Positive | J2 PETp1 |
| 30 | CH5_B- | Channel 5 Device-Side Negative | J2 PETn1 |
| 31 | VDD | 3.3V Supply | VCC3V3 rail |
| 32 | CH6_A+ | Channel 6 Host-Side Positive | J2 PERp0 |
| 33 | CH6_A- | Channel 6 Host-Side Negative | J2 PERn0 |
| 34 | CH6_B+ | Channel 6 Device-Side Positive | FPGA PCIE_RX0_P |
| 35 | CH6_B- | Channel 6 Device-Side Negative | FPGA PCIE_RX0_N |
| 36 | GND | Ground | GND plane |
| 37 | CH7_A+ | Channel 7 Host-Side Positive | J2 PERp1 |
| 38 | CH7_A- | Channel 7 Host-Side Negative | J2 PERn1 |
| 39 | CH7_B+ | Channel 7 Device-Side Positive | FPGA PCIE_RX1_P |
| 40 | CH7_B- | Channel 7 Device-Side Negative | FPGA PCIE_RX1_N |
| 41 | SCL | I2C Clock | FPGA GPIO_12 (shared with U3) |
| 42 | SDA | I2C Data | FPGA GPIO_13 (shared with U3) |
| EP | GND | Exposed Pad | GND plane (thermal + electrical) |

### 3.2 U3 (Lanes 2-3) Pinout

U3 follows the same pattern as U2 but for lanes 2-3:

| Pin | Name | Function | Connected To |
|-----|------|----------|--------------|
| 1 | VDD | 3.3V Supply | VCC3V3 rail |
| 2 | CH0_A+ | Channel 0 Host-Side Positive | M.2 Host Edge PETp2 |
| 3 | CH0_A- | Channel 0 Host-Side Negative | M.2 Host Edge PETn2 |
| 4 | CH0_B+ | Channel 0 Device-Side Positive | FPGA PCIE_RX2_P |
| 5 | CH0_B- | Channel 0 Device-Side Negative | FPGA PCIE_RX2_N |
| 6 | GND | Ground | GND plane |
| 7 | CH1_A+ | Channel 1 Host-Side Positive | M.2 Host Edge PETp3 |
| 8 | CH1_A- | Channel 1 Host-Side Negative | M.2 Host Edge PETn3 |
| 9 | CH1_B+ | Channel 1 Device-Side Positive | FPGA PCIE_RX3_P |
| 10 | CH1_B- | Channel 1 Device-Side Negative | FPGA PCIE_RX3_N |
| 11 | VDD | 3.3V Supply | VCC3V3 rail |
| 12 | CH2_A+ | Channel 2 Host-Side Positive | FPGA PCIE_TX2_P |
| 13 | CH2_A- | Channel 2 Host-Side Negative | FPGA PCIE_TX2_N |
| 14 | CH2_B+ | Channel 2 Device-Side Positive | M.2 Host Edge PERp2 |
| 15 | CH2_B- | Channel 2 Device-Side Negative | M.2 Host Edge PERn2 |
| 16 | GND | Ground | GND plane |
| 17 | CH3_A+ | Channel 3 Host-Side Positive | FPGA PCIE_TX3_P |
| 18 | CH3_A- | Channel 3 Host-Side Negative | FPGA PCIE_TX3_N |
| 19 | CH3_B+ | Channel 3 Device-Side Positive | M.2 Host Edge PERp3 |
| 20 | CH3_B- | Channel 3 Device-Side Negative | M.2 Host Edge PERn3 |
| 21 | VDD | 3.3V Supply | VCC3V3 rail |
| 22 | CH4_A+ | Channel 4 Host-Side Positive | FPGA PCIE_TX2_P |
| 23 | CH4_A- | Channel 4 Host-Side Negative | FPGA PCIE_TX2_N |
| 24 | CH4_B+ | Channel 4 Device-Side Positive | J2 PETp2 |
| 25 | CH4_B- | Channel 4 Device-Side Negative | J2 PETn2 |
| 26 | GND | Ground | GND plane |
| 27 | CH5_A+ | Channel 5 Host-Side Positive | FPGA PCIE_TX3_P |
| 28 | CH5_A- | Channel 5 Host-Side Negative | FPGA PCIE_TX3_N |
| 29 | CH5_B+ | Channel 5 Device-Side Positive | J2 PETp3 |
| 30 | CH5_B- | Channel 5 Device-Side Negative | J2 PETn3 |
| 31 | VDD | 3.3V Supply | VCC3V3 rail |
| 32 | CH6_A+ | Channel 6 Host-Side Positive | J2 PERp2 |
| 33 | CH6_A- | Channel 6 Host-Side Negative | J2 PERn2 |
| 34 | CH6_B+ | Channel 6 Device-Side Positive | FPGA PCIE_RX2_P |
| 35 | CH6_B- | Channel 6 Device-Side Negative | FPGA PCIE_RX2_N |
| 36 | GND | Ground | GND plane |
| 37 | CH7_A+ | Channel 7 Host-Side Positive | J2 PERp3 |
| 38 | CH7_A- | Channel 7 Host-Side Negative | J2 PERn3 |
| 39 | CH7_B+ | Channel 7 Device-Side Positive | FPGA PCIE_RX3_P |
| 40 | CH7_B- | Channel 7 Device-Side Negative | FPGA PCIE_RX3_N |
| 41 | SCL | I2C Clock | FPGA GPIO_12 (shared with U2) |
| 42 | SDA | I2C Data | FPGA GPIO_13 (shared with U2) |
| EP | GND | Exposed Pad | GND plane |

### 3.3 Redriver Channel Architecture

Each PI3EQX12908 provides 8 bidirectional channels. The mapping is:

```
U2 (Lanes 0-1):
  Ch0: Host PET[0] → FPGA RX[0]  (Host-to-Device direction, lane 0)
  Ch1: Host PET[1] → FPGA RX[1]  (Host-to-Device direction, lane 1)
  Ch2: FPGA TX[0] → Host PER[0]  (Device-to-Host direction, lane 0)
  Ch3: FPGA TX[1] → Host PER[1]  (Device-to-Host direction, lane 1)
  Ch4: FPGA TX[0] → Device PET[0] (FPGA-to-Device direction, lane 0)
  Ch5: FPGA TX[1] → Device PET[1] (FPGA-to-Device direction, lane 1)
  Ch6: Device PER[0] → FPGA RX[0] (Device-to-FPGA direction, lane 0)
  Ch7: Device PER[1] → FPGA RX[1] (Device-to-FPGA direction, lane 1)

U3 (Lanes 2-3):
  Ch0: Host PET[2] → FPGA RX[2]
  Ch1: Host PET[3] → FPGA RX[3]
  Ch2: FPGA TX[2] → Host PER[2]
  Ch3: FPGA TX[3] → Host PER[3]
  Ch4: FPGA TX[2] → Device PET[2]
  Ch5: FPGA TX[3] → Device PET[3]
  Ch6: Device PER[2] → FPGA RX[2]
  Ch7: Device PER[3] → FPGA RX[3]
```

This architecture means the FPGA sits in the middle of every PCIe lane, receiving both directions simultaneously. The redrivers provide signal conditioning in both directions through the FPGA.

## 4. FT601Q USB 3.0 Bridge Pinout

| Pin | Name | Type | Function | Connected To |
|-----|------|------|----------|--------------|
| 1 | D24 | I/O | Data Bit 24 | FPGA FT_D24 |
| 2 | D25 | I/O | Data Bit 25 | FPGA FT_D25 |
| 3 | D26 | I/O | Data Bit 26 | FPGA FT_D26 |
| 4 | D27 | I/O | Data Bit 27 | FPGA FT_D27 |
| 5 | SUSPEND# | O | Suspend Indicator | FPGA FT_SUSPEND |
| 6 | GPIO1 | I/O | General Purpose I/O 1 | FPGA FT_GPIO1 |
| 7 | GPIO0 | I/O | General Purpose I/O 0 | FPGA FT_GPIO0 |
| 8 | WAKEUP | I | Wakeup Input | FPGA FT_WAKEUP |
| 9 | INT# | O | Interrupt Output | FPGA GPIO_11 |
| 10 | D16 | I/O | Data Bit 16 | FPGA FT_D16 |
| 11 | D17 | I/O | Data Bit 17 | FPGA FT_D17 |
| 12 | D18 | I/O | Data Bit 18 | FPGA FT_D18 |
| 13 | D19 | I/O | Data Bit 19 | FPGA FT_D19 |
| 14 | D20 | I/O | Data Bit 20 | FPGA FT_D20 |
| 15 | D21 | I/O | Data Bit 21 | FPGA FT_D21 |
| 16 | D22 | I/O | Data Bit 22 | FPGA FT_D22 |
| 17 | D23 | I/O | Data Bit 23 | FPGA FT_D23 |
| 18 | BE0 | I | Byte Enable 0 | FPGA FT_BE0 |
| 19 | D0 | I/O | Data Bit 0 | FPGA FT_D0 |
| 20 | D1 | I/O | Data Bit 1 | FPGA FT_D1 |
| 21 | D2 | I/O | Data Bit 2 | FPGA FT_D2 |
| 22 | D3 | I/O | Data Bit 3 | FPGA FT_D3 |
| 23 | D4 | I/O | Data Bit 4 | FPGA FT_D4 |
| 24 | D5 | I/O | Data Bit 5 | FPGA FT_D5 |
| 25 | D6 | I/O | Data Bit 6 | FPGA FT_D6 |
| 26 | D7 | I/O | Data Bit 7 | FPGA FT_D7 |
| 27 | D8 | I/O | Data Bit 8 | FPGA FT_D8 |
| 28 | D9 | I/O | Data Bit 9 | FPGA FT_D9 |
| 29 | D10 | I/O | Data Bit 10 | FPGA FT_D10 |
| 30 | D11 | I/O | Data Bit 11 | FPGA FT_D11 |
| 31 | D12 | I/O | Data Bit 12 | FPGA FT_D12 |
| 32 | D13 | I/O | Data Bit 13 | FPGA FT_D13 |
| 33 | D14 | I/O | Data Bit 14 | FPGA FT_D14 |
| 34 | D15 | I/O | Data Bit 15 | FPGA FT_D15 |
| 35 | CLK | I | 100 MHz Clock Input | FPGA FT_CLK |
| 36 | RESET# | I | Reset Input (active low) | FPGA FT_RESET# |
| 37 | SIWU# | I | Send Immediate / Wake Up | FPGA FT_SIWU# |
| 38 | TXE# | O | Transmit FIFO Empty | FPGA FT_TXE# |
| 39 | RXF# | O | Receive FIFO Full | FPGA FT_RXF# |
| 40 | WR# | I | Write Strobe | FPGA FT_WR# |
| 41 | RD# | I | Read Strobe | FPGA FT_RD# |
| 42 | OE# | I | Output Enable | FPGA FT_OE# |
| 43 | VCCIO | PWR | 3.3V I/O Supply | VCC3V3 |
| 44 | VCCIO | PWR | 3.3V I/O Supply | VCC3V3 |
| 45 | GND | PWR | Ground | GND |
| 46 | GND | PWR | Ground | GND |
| 47 | VCCD | PWR | 1.2V Digital Core | VCC1V2 |
| 48 | VCCD | PWR | 1.2V Digital Core | VCC1V2 |
| 49 | VCCA | PWR | 3.3V Analog Supply | VCC3V3 (via ferrite bead FB1) |
| 50 | VCCA | PWR | 3.3V Analog Supply | VCC3V3 (via ferrite bead FB1) |
| 51 | GND | PWR | Ground | GND |
| 52 | GND | PWR | Ground | GND |
| 53 | USBDM | I/O | USB 2.0 D- | J3 (USB-C) D- |
| 54 | USBDP | I/O | USB 2.0 D+ | J3 (USB-C) D+ |
| 55 | VBUS | PWR | USB VBUS Sense | J3 VBUS via 100k/200k divider |
| 56 | ID | I | USB ID Pin | J3 CC1 via 5.1k to GND |
| 57 | RTUNE | I | Reference Resistor | 12.0 kΩ ±1% to GND |
| 58 | XTALIN | I | Crystal Input | 30 MHz crystal (or leave NC, use internal osc) |
| 59 | XTALOUT | O | Crystal Output | 30 MHz crystal (or leave NC) |
| 60 | GND | PWR | Ground | GND |
| 61 | TEST | I | Test Mode (tie to GND) | GND |
| 62 | VCCD | PWR | 1.2V Digital Core | VCC1V2 |
| 63 | VCCD | PWR | 1.2V Digital Core | VCC1V2 |
| 64 | GND | PWR | Ground | GND |
| 65 | GND | PWR | Ground | GND |
| 66 | SSRXM | I | SuperSpeed RX- | J3 (USB-C) SSTX- (cross-connected) |
| 67 | SSRXM_S | I | SuperSpeed RX- Shield | GND via 100 nF |
| 68 | SSRXP | I | SuperSpeed RX+ | J3 (USB-C) SSTX+ (cross-connected) |
| 69 | SSRXP_S | I | SuperSpeed RX+ Shield | GND via 100 nF |
| 70 | BE3 | I | Byte Enable 3 | FPGA FT_BE3 |
| 71 | BE2 | I | Byte Enable 2 | FPGA FT_BE2 |
| 72 | D31 | I/O | Data Bit 31 | FPGA FT_D31 |
| 73 | D30 | I/O | Data Bit 30 | FPGA FT_D30 |
| 74 | D29 | I/O | Data Bit 29 | FPGA FT_D29 |
| 75 | D28 | I/O | Data Bit 28 | FPGA FT_D28 |
| 76 | BE1 | I | Byte Enable 1 | FPGA FT_BE1 |
| EP | GND | PWR | Exposed Pad | GND plane |

## 5. DDR3 SDRAM Pinout — MT41K256M16TW-107

| Ball | Name | Type | Function | Connected To |
|------|------|------|----------|--------------|
| K7 | CK | I | Clock Positive | FPGA DDR3_CK_P |
| J7 | CK# | I | Clock Negative | FPGA DDR3_CK_N |
| K9 | CKE | I | Clock Enable | FPGA DDR3_CKE |
| L2 | CS# | I | Chip Select | FPGA DDR3_CS# |
| J3 | RAS# | I | Row Address Strobe | FPGA DDR3_RAS# |
| K3 | CAS# | I | Column Address Strobe | FPGA DDR3_CAS# |
| L3 | WE# | I | Write Enable | FPGA DDR3_WE# |
| K1 | ODT | I | On-Die Termination | FPGA DDR3_ODT |
| T2 | RESET# | I | Reset | FPGA DDR3_RESET# |
| N7 | A0 | I | Address 0 | FPGA DDR3_A0 |
| M7 | A1 | I | Address 1 | FPGA DDR3_A1 |
| M2 | A2 | I | Address 2 | FPGA DDR3_A2 |
| N2 | A3 | I | Address 3 | FPGA DDR3_A3 |
| M3 | A4 | I | Address 4 | FPGA DDR3_A4 |
| N3 | A5 | I | Address 5 | FPGA DDR3_A5 |
| P8 | A6 | I | Address 6 | FPGA DDR3_A6 |
| P3 | A7 | I | Address 7 | FPGA DDR3_A7 |
| R8 | A8 | I | Address 8 | FPGA DDR3_A8 |
| R3 | A9 | I | Address 9 | FPGA DDR3_A9 |
| L7 | A10/AP | I | Address 10 / Auto Precharge | FPGA DDR3_A10 |
| R7 | A11 | I | Address 11 | FPGA DDR3_A11 |
| T8 | A12/BC# | I | Address 12 / Burst Chop | FPGA DDR3_A12 |
| N8 | A13 | I | Address 13 | FPGA DDR3_A13 |
| T3 | A14 | I | Address 14 | FPGA DDR3_A14 |
| M6 | BA0 | I | Bank Address 0 | FPGA DDR3_BA0 |
| P2 | BA1 | I | Bank Address 1 | FPGA DDR3_BA1 |
| P7 | BA2 | I | Bank Address 2 | FPGA DDR3_BA2 |
| G8 | DQ0 | I/O | Data 0 | FPGA DDR3_DQ0 |
| G7 | DQ1 | I/O | Data 1 | FPGA DDR3_DQ1 |
| F7 | DQ2 | I/O | Data 2 | FPGA DDR3_DQ2 |
| F8 | DQ3 | I/O | Data 3 | FPGA DDR3_DQ3 |
| E7 | DQ4 | I/O | Data 4 | FPGA DDR3_DQ4 |
| D7 | DQ5 | I/O | Data 5 | FPGA DDR3_DQ5 |
| E8 | DQ6 | I/O | Data 6 | FPGA DDR3_DQ6 |
| C8 | DQ7 | I/O | Data 7 | FPGA DDR3_DQ7 |
| C3 | DQ8 | I/O | Data 8 | FPGA DDR3_DQ8 |
| C2 | DQ9 | I/O | Data 9 | FPGA DDR3_DQ9 |
| A2 | DQ10 | I/O | Data 10 | FPGA DDR3_DQ10 |
| A3 | DQ11 | I/O | Data 11 | FPGA DDR3_DQ11 |
| B1 | DQ12 | I/O | Data 12 | FPGA DDR3_DQ12 |
| A7 | DQ13 | I/O | Data 13 | FPGA DDR3_DQ13 |
| A8 | DQ14 | I/O | Data 14 | FPGA DDR3_DQ14 |
| B8 | DQ15 | I/O | Data 15 | FPGA DDR3_DQ15 |
| F2 | UDQS | I/O | Upper Data Strobe | FPGA DDR3_DQS0_P |
| F3 | UDQS# | I/O | Upper Data Strobe Complement | FPGA DDR3_DQS0_N |
| B2 | LDQS | I/O | Lower Data Strobe | FPGA DDR3_DQS1_P |
| B3 | LDQS# | I/O | Lower Data Strobe Complement | FPGA DDR3_DQS1_N |
| E3 | UDM | I | Upper Data Mask | FPGA DDR3_DM0 |
| D1 | LDM | I | Lower Data Mask | FPGA DDR3_DM1 |
| J1 | VREFDQ | PWR | Reference Voltage for DQ | 0.675V (VDD/2) from resistor divider |
| H1, H9, R1, R9 | VDD | PWR | 1.35V Core Supply | VCC1V35 |
| B7, D3, D8, E1, F9, G1, G9, H2, H8, J2, J8, K2, K8, L1, L9, M1, M9, N1, N9, P1, P9, R2 | VDDQ | PWR | 1.35V I/O Supply | VCC1V35 |
| A1, A9, B9, C1, C9, D2, D9, E9, F1, H3, H7, J9, L8, M8, T1, T9 | VSS | PWR | Ground | GND |
| G2, G3 | VSSQ | PWR | DQ Ground | GND |
| T7 | ZQ | I/O | ZQ Calibration | 240 Ω ±1% to VSSQ (R39) |
| K7, J7 | CK, CK# | I | Differential Clock | 100 Ω differential termination at FPGA |

## 6. Power Tree Schematic

### 6.1 VCC3V3 Rail (3.3V Primary)

```
M.2 Host 3.3V (Pins 2,4,70,72,74)
    │
    ├── Ferrite Bead FB1 (120Ω @ 100 MHz) ── VCC3V3_ANALOG (for PLLs, redrivers analog)
    │
    ├── Direct ── VCC3V3 (main digital rail)
    │   ├── U2, U3 (PI3EQX12908) VDD pins
    │   ├── U4 (FT601Q) VCCIO, VCCA
    │   ├── U6 (W25Q128) VCC
    │   ├── U7 (Si5332) VDD
    │   ├── FPGA Banks 0,1,2,3 VCCIO
    │   ├── LEDs (via current-limit resistors)
    │   └── Pull-up resistors (PROGRAMN, INITN, DONE, CFG pins)
    │
    └── TLV75533 LDO (U12) ── VCC3V3_BACKUP (from USB VBUS when M.2 absent)
```

### 6.2 VCC1V35 Rail (1.35V DDR3)

```
VCC3V3 ── TPS62825 (U8) ── VCC1V35 @ 0.8A
    │   ├── Input: 3.3V, C80 (22 pF), C81 (10 µF)
    │   ├── Output: 1.35V, L1 (1 µH), C82 (22 pF), C83-C84 (10 µF ×2), C85 (100 µF tantalum)
    │   ├── FB: R21 (100k) + R22 (10k) → VOUT = 0.6V × (1 + 100k/10k) = 6.6V... WRONG
    │   │   CORRECT: R21 = 12.5k, R22 = 10k → VOUT = 0.6V × (1 + 12.5k/10k) = 1.35V
    │   └── EN: Tied to VCC3V3 via 100k pull-up, delayed by RC (100k + 1 µF = 100 ms)
    │
    └── Loads:
        ├── U5 (MT41K256M16) VDD, VDDQ
        ├── FPGA Bank 6 VCCIO (SSTL135)
        └── VREFDQ divider: 1k + 1k from VCC1V35 → 0.675V at U5 VREFDQ
```

### 6.3 VCC1V2 Rail (1.2V FPGA I/O + FT601 Core)

```
VCC3V3 ── TPS62825 (U9) ── VCC1V2 @ 0.5A
    │   ├── Input: 3.3V, C86 (22 pF), C87 (10 µF)
    │   ├── Output: 1.2V, L2 (1 µH), C88 (22 pF), C89-C90 (10 µF ×2)
    │   ├── FB: R23 (10k) + R24 (10k) → VOUT = 0.6V × (1 + 10k/10k) = 1.2V
    │   └── EN: Tied to VCC1V8_PGOOD (sequenced after 1.8V)
    │
    └── Loads:
        ├── FPGA Banks 4,5,7 VCCIO
        ├── U4 (FT601Q) VCCD (1.2V digital core)
        └── U7 (Si5332) VDD core (1.2V)
```

### 6.4 VCC1V1 Rail (1.1V FPGA Core)

```
VCC3V3 ── TPS62825 (U10) ── VCC1V1 @ 1.2A
    │   ├── Input: 3.3V, C91 (22 pF), C92 (10 µF)
    │   ├── Output: 1.1V, L3 (1 µH), C93 (22 pF), C94-C95 (10 µF ×2), C96 (100 µF tantalum)
    │   ├── FB: R25 (8.33k) + R26 (10k) → VOUT = 0.6V × (1 + 8.33k/10k) = 1.1V
    │   └── EN: Tied to VCC1V2_PGOOD (sequenced after 1.2V)
    │
    └── Loads:
        └── FPGA VCC (core voltage, all banks internal logic)
```

### 6.5 VCC1V8 Rail (1.8V FPGA AUX + PLL)

```
VCC3V3 ── TPS62825 (U11) ── VCC1V8 @ 0.3A
    │   ├── Input: 3.3V, C97 (22 pF), C98 (10 µF)
    │   ├── Output: 1.8V, L4 (1 µH), C99 (22 pF), C100-C101 (10 µF ×2)
    │   ├── FB: R27 (20k) + R28 (10k) → VOUT = 0.6V × (1 + 20k/10k) = 1.8V
    │   └── EN: Tied to VCC3V3 (first rail to come up)
    │
    └── Loads:
        ├── FPGA VCCAUX
        ├── FPGA PLL analog supplies (VCCA_PLL0, VCCA_PLL1)
        └── Ferrite bead FB2 isolates VCCA_PLL from VCCAUX
```

### 6.6 USB Backup Power

```
USB VBUS (5V from J3 Type-C)
    │
    ├── TLV75533 (U12) ── VCC3V3_BACKUP (500 mA)
    │   └── Diode-OR with main VCC3V3 (Schottky BAT54C dual diode)
    │
    └── TLV75518 (U13) ── VCC1V8_BACKUP (500 mA)
        └── Diode-OR with main VCC1V8
```

### 6.7 Power Sequencing Circuit

```
VCC3V3 ── RC delay (100k + 1µF, τ=100ms) ── U11 EN (VCC1V8)
VCC1V8_PGOOD (U11 PG pin, open-drain, pull-up to VCC3V3) ── U9 EN (VCC1V2)
VCC1V2_PGOOD (U9 PG pin) ── U10 EN (VCC1V1)
VCC1V1_PGOOD (U10 PG pin) ── U8 EN (VCC1V35)
VCC1V35_PGOOD (U8 PG pin) ── FPGA PROGRAMN release (via open-drain buffer)
```

## 7. M.2 Connector Pin Mapping

### 7.1 Host-Side M.2 Edge Connector (J1 — PCB Gold Fingers)

M.2 M-Key pinout (67 positions, top side odd 1-75, bottom side even 2-74):

| Pin | Signal | Direction (relative to interposer) | Connected To |
|-----|--------|-----------------------------------|--------------|
| 1 | GND | — | GND plane |
| 2 | 3.3V | Input (power) | VCC3V3 rail |
| 3 | GND | — | GND plane |
| 4 | 3.3V | Input (power) | VCC3V3 rail |
| 5 | PETn3 | Input (from host) | U3 Ch1 A- |
| 7 | PETp3 | Input (from host) | U3 Ch1 A+ |
| 9 | GND | — | GND plane |
| 11 | PERn3 | Output (to host) | U3 Ch3 B- |
| 13 | PERp3 | Output (to host) | U3 Ch3 B+ |
| 15 | GND | — | GND plane |
| 17 | PETn2 | Input (from host) | U3 Ch0 A- |
| 19 | PETp2 | Input (from host) | U3 Ch0 A+ |
| 21 | GND | — | GND plane |
| 23 | PERn2 | Output (to host) | U3 Ch2 B- |
| 25 | PERp2 | Output (to host) | U3 Ch2 B+ |
| 27 | GND | — | GND plane |
| 29 | PETn1 | Input (from host) | U2 Ch1 A- |
| 31 | PETp1 | Input (from host) | U2 Ch1 A+ |
| 33 | GND | — | GND plane |
| 35 | PERn1 | Output (to host) | U2 Ch3 B- |
| 37 | PERp1 | Output (to host) | U2 Ch3 B+ |
| 39 | GND | — | GND plane |
| 41 | PETn0 | Input (from host) | U2 Ch0 A- |
| 43 | PETp0 | Input (from host) | U2 Ch0 A+ |
| 45 | GND | — | GND plane |
| 47 | PERn0 | Output (to host) | U2 Ch2 B- |
| 49 | PERp0 | Output (to host) | U2 Ch2 B+ |
| 51 | GND | — | GND plane |
| 53 | REFCLKn | Input (from host) | U7 IN1- (Si5332) |
| 55 | REFCLKp | Input (from host) | U7 IN1+ (Si5332) |
| 57 | GND | — | GND plane |
| 59 | (M-Key notch) | — | — |
| 61 | (M-Key notch) | — | — |
| 63 | (M-Key notch) | — | — |
| 65 | (M-Key notch) | — | — |
| 67 | (M-Key notch) | — | — |
| 69 | PEDET | NC (float) | — |
| 71 | GND | — | GND plane |
| 73 | GND | — | GND plane |
| 75 | GND | — | GND plane |
| — | — | — | — |
| 2 (bot) | 3.3V | Input (power) | VCC3V3 rail |
| 4 (bot) | 3.3V | Input (power) | VCC3V3 rail |
| 6 (bot) | (Notch area) | — | — |
| 8 (bot) | (Notch area) | — | — |
| 10 (bot) | DAS/DSS# | NC (not used) | — |
| 12 (bot) | (Notch area) | — | — |
| 14 (bot) | (Notch area) | — | — |
| 16 (bot) | (Notch area) | — | — |
| 18 (bot) | (Notch area) | — | — |
| 20 (bot) | (Notch area) | — | — |
| 22 (bot) | (Notch area) | — | — |
| 24 (bot) | (Notch area) | — | — |
| 26 (bot) | (Notch area) | — | — |
| 28 (bot) | (Notch area) | — | — |
| 30 (bot) | (Notch area) | — | — |
| 32 (bot) | (Notch area) | — | — |
| 34 (bot) | (Notch area) | — | — |
| 36 (bot) | (Notch area) | — | — |
| 38 (bot) | DEVSLP | NC (not used) | — |
| 40 (bot) | (Notch area) | — | — |
| 42 (bot) | (Notch area) | — | — |
| 44 (bot) | (Notch area) | — | — |
| 46 (bot) | (Notch area) | — | — |
| 48 (bot) | (Notch area) | — | — |
| 50 (bot) | PERST# | Bidirectional (pass-through + monitor) | J2 Pin 50 + FPGA GPIO_6 (via 10k) |
| 52 (bot) | CLKREQ# | Bidirectional (pass-through + monitor) | J2 Pin 56 + FPGA GPIO_7 (via 10k) |
| 54 (bot) | PEWAKE# | Bidirectional (pass-through + monitor) | J2 Pin 58 + FPGA GPIO_8 (via 10k) |
| 56 (bot) | (Notch area) | — | — |
| 58 (bot) | (Notch area) | — | — |
| 60 (bot) | (Notch area) | — | — |
| 62 (bot) | (Notch area) | — | — |
| 64 (bot) | (Notch area) | — | — |
| 66 (bot) | (Notch area) | — | — |
| 68 (bot) | SUSCLK | Pass-through | J2 Pin 68 (32.768 kHz) |
| 70 (bot) | 3.3V | Input (power) | VCC3V3 rail |
| 72 (bot) | 3.3V | Input (power) | VCC3V3 rail |
| 74 (bot) | 3.3V | Input (power) | VCC3V3 rail |

### 7.2 Device-Side M.2 Socket (J2 — Amphenol 10132797-0011LF)

The device-side socket mirrors the host-side pinout but connects to the redriver device-side channels and FPGA pass-through signals. All PCIe lane signals route through the redrivers. Sideband signals (PERST#, CLKREQ#, PEWAKE#, SUSCLK) pass through directly with FPGA monitoring taps.

## 8. Decoupling Capacitor Placement

### 8.1 Per-Component Decoupling Map

| Component | Rail | 100 nF (0402) | 10 µF (0805) | 100 µF (Tantalum) | Placement |
|-----------|------|---------------|--------------|--------------------|-----------|
| U1 (FPGA) | VCC1V1 (Core) | 10 | 4 | 2 | Under BGA, on bottom layer |
| U1 (FPGA) | VCC1V2 (Banks 4,5,7) | 12 (4/bank) | 3 (1/bank) | 0 | Perimeter of BGA |
| U1 (FPGA) | VCC3V3 (Banks 0,1,2,3) | 16 (4/bank) | 4 (1/bank) | 0 | Perimeter of BGA |
| U1 (FPGA) | VCC1V35 (Bank 6) | 4 | 1 | 0 | Near Bank 6 balls |
| U1 (FPGA) | VCC1V8 (AUX/PLL) | 4 | 2 | 0 | Near AUX/PLL balls |
| U2, U3 (Redrivers) | VCC3V3 | 4 (2 each) | 2 (1 each) | 0 | Adjacent to VDD pins |
| U4 (FT601Q) | VCC3V3 (VCCIO) | 4 | 2 | 0 | Adjacent to VCCIO pins |
| U4 (FT601Q) | VCC1V2 (VCCD) | 2 | 1 | 0 | Adjacent to VCCD pins |
| U4 (FT601Q) | VCCA (3.3V analog) | 2 | 1 | 0 | Adjacent to VCCA pins, after FB |
| U5 (DDR3) | VCC1V35 | 8 | 4 | 2 | Perimeter of BGA |
| U6 (SPI Flash) | VCC3V3 | 1 | 0 | 0 | Adjacent to VCC pin |
| U7 (Si5332) | VCC3V3 | 2 | 1 | 0 | Adjacent to VDD pins |
| U7 (Si5332) | VCC1V2 (core) | 1 | 1 | 0 | Adjacent to VDDC pin |
| U8-U11 (Bucks) | Input (3.3V) | 1 each | 1 each | 0 | Adjacent to VIN pin |
| U8-U11 (Bucks) | Output | 1 each | 2 each | 1 each (U8, U10) | Adjacent to VOUT + inductor |

## 9. Impedance Control Requirements

### 9.1 Differential Pairs

| Signal Group | Impedance | Tolerance | Pair Spacing | Notes |
|-------------|-----------|-----------|--------------|-------|
| PCIe TX/RX (all lanes) | 85 Ω differential | ±10% | 0.15 mm intra-pair | 100 Ω common-mode option per PCIe spec |
| PCIe REFCLK | 100 Ω differential | ±10% | 0.15 mm intra-pair | HCSL signaling |
| DDR3 CK/CK# | 100 Ω differential | ±10% | 0.15 mm intra-pair | SSTL135 differential clock |
| DDR3 DQS/DQS# | 100 Ω differential | ±10% | 0.15 mm intra-pair | SSTL135 differential strobe |
| USB 3.0 SSRX/SSTX | 90 Ω differential | ±10% | 0.15 mm intra-pair | SuperSpeed signaling |
| USB 2.0 DP/DM | 90 Ω differential | ±10% | 0.15 mm intra-pair | High-Speed USB signaling |

### 9.2 Single-Ended Impedance

| Signal Group | Impedance | Tolerance | Notes |
|-------------|-----------|-----------|-------|
| DDR3 Address/Command/Control | 40 Ω | ±10% | SSTL135, ODT to VTT at FPGA |
| DDR3 DQ | 40 Ω | ±10% | SSTL135, ODT at both ends |
| FT601 Data/Control | 50 Ω | ±10% | LVCMOS33, series termination at driver |
| SPI Flash | 50 Ω | ±10% | LVCMOS33, short traces |
| I2C (Redriver, Si5332) | 50 Ω | ±10% | Open-drain, pulled up to 3.3V |
| LED/GPIO | 50 Ω | ±10% | Non-critical |

## 10. Netlist Summary

### 10.1 Power Nets

| Net Name | Voltage | Source(s) | Loads |
|----------|---------|-----------|-------|
| VCC3V3 | 3.3V | M.2 Host, U12 (backup) | U1 Banks 0-3, U2, U3, U4 VCCIO/VCCA, U6, U7 VDD, LEDs, pull-ups |
| VCC3V3_ANALOG | 3.3V | VCC3V3 via FB1 | U4 VCCA (post-FB), U7 VDDA |
| VCC1V35 | 1.35V | U8 (TPS62825) | U1 Bank 6, U5 VDD/VDDQ |
| VREFDQ | 0.675V | Resistor divider from VCC1V35 | U5 VREFDQ |
| VCC1V2 | 1.2V | U9 (TPS62825) | U1 Banks 4,5,7, U4 VCCD, U7 VDDC |
| VCC1V1 | 1.1V | U10 (TPS62825) | U1 VCC (core) |
| VCC1V8 | 1.8V | U11 (TPS62825), U13 (backup) | U1 VCCAUX, VCCA_PLL |
| VCCA_PLL | 1.8V | VCC1V8 via FB2 | U1 PLL analog |
| VCC5V0 | 5.0V | USB VBUS | U12, U13 inputs |
| GND | 0V | Common | All components |

### 10.2 Critical Signal Nets

| Net Name | From | To | Type | Length Constraint |
|----------|------|----|------|-------------------|
| PCIE_RX0_P/N | U2 Ch0 B± | U1 PCIE_RX0_P/N | Differential, 85Ω | < 25 mm |
| PCIE_TX0_P/N | U1 PCIE_TX0_P/N | U2 Ch2 A± | Differential, 85Ω | < 25 mm |
| PCIE_RX1_P/N | U2 Ch1 B± | U1 PCIE_RX1_P/N | Differential, 85Ω | < 25 mm |
| PCIE_TX1_P/N | U1 PCIE_TX1_P/N | U2 Ch3 A± | Differential, 85Ω | < 25 mm |
| PCIE_RX2_P/N | U3 Ch0 B± | U1 PCIE_RX2_P/N | Differential, 85Ω | < 25 mm |
| PCIE_TX2_P/N | U1 PCIE_TX2_P/N | U3 Ch2 A± | Differential, 85Ω | < 25 mm |
| PCIE_RX3_P/N | U3 Ch1 B± | U1 PCIE_RX3_P/N | Differential, 85Ω | < 25 mm |
| PCIE_TX3_P/N | U1 PCIE_TX3_P/N | U3 Ch3 A± | Differential, 85Ω | < 25 mm |
| PCIE_REFCLK_P/N | U7 OUT0± | U1 PCIE_REFCLK_P/N | Differential, 100Ω | < 30 mm |
| DDR3_CK_P/N | U1 DDR3_CK_P/N | U5 CK/CK# | Differential, 100Ω | < 20 mm |
| DDR3_DQS0_P/N | U1 DDR3_DQS0_P/N | U5 UDQS/UDQS# | Differential, 100Ω | < 20 mm |
| DDR3_DQS1_P/N | U1 DDR3_DQS1_P/N | U5 LDQS/LDQS# | Differential, 100Ω | < 20 mm |
| DDR3_DQ[15:0] | U1 DDR3_DQ[15:0] | U5 DQ[15:0] | Single-ended, 40Ω | < 20 mm, matched ±2 mm |
| DDR3_A[14:0] | U1 DDR3_A[14:0] | U5 A[14:0] | Single-ended, 40Ω | < 20 mm, matched ±5 mm |
| FT_D[31:0] | U1 FT_D[31:0] | U4 D[31:0] | Single-ended, 50Ω | < 40 mm, matched ±5 mm |
| FT_CLK | U1 FT_CLK | U4 CLK | Single-ended, 50Ω | < 30 mm |
| USB_SSRX_P/N | U4 SSRX± | J3 SSTX± | Differential, 90Ω | < 15 mm |
| USB_SSTX_P/N | U4 SSTX± | J3 SSRX± | Differential, 90Ω | < 15 mm |
| USB_DP/DM | U4 USBDP/USBDM | J3 D+/D- | Differential, 90Ω | < 15 mm |

---

*End of Phase 2 — Component Selection & Schematics. This document provides complete BOM, pinout tables for every component, netlists, decoupling strategy, impedance requirements, and power tree design for the PCIe Screamer.*
