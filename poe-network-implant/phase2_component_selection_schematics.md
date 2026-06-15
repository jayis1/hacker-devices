# PoE Network Implant — Phase 2: Component Selection & Schematics

## 1. Component Selection Rationale

### 1.1 MCU: STM32H743ZIT6
- Cortex-M7 @ 480 MHz, 1 MB SRAM, 2 MB Flash
- Dual ETH MAC (one for RGMII to switch, one reserved)
- FMC for SDRAM, SPI/I2C/UART peripherals
- LQFP-144 package (10mm × 10mm) — fits tight PCB
- $12.50 @ 1ku

### 1.2 Ethernet Switch: KSZ9897R
- 7-port managed gigabit switch with RGMII CPU port
- Hardware port mirroring (ingress/egress/both per port)
- VLAN (802.1Q), QoS, IGMP snooping
- SPI/SMI management interface
- QFN-128 (12mm × 12mm)
- $8.20 @ 1ku

### 1.3 PoE PD Controller: TPS2378DDW
- IEEE 802.3at Type 1 (Class 0–3) PD controller
- Integrated hot-swap MOSFET, auto-classification
- I2C interface for status/telemetry
- TSSOP-20 (6.5mm × 6.5mm)
- $2.80 @ 1ku

### 1.4 PoE Pass-Through: TPS23781DDWR
- Second PD controller for downstream PD pass-through negotiation
- Maintains PoE signature on the output side
- TSSOP-20
- $2.80 @ 1ku

### 1.5 DC-DC Converters
| Rail | Part | Topology | Input | Output | Package | Price |
|------|------|----------|-------|--------|---------|-------|
| 3.3V main | TPS62A02AMDR | Buck | 48V PoE | 3.3V/2A | SOT-563 | $1.20 |
| 1.2V core | TLV62569DBV | Buck | 3.3V | 1.2V/1A | SOT-23-5 | $0.60 |
| 1.8V IO | TLV62568DBV | Buck | 3.3V | 1.8V/1A | SOT-23-5 | $0.60 |
| 2.5V SDRAM | TLV62565DBV | Buck | 3.3V | 2.5V/0.5A | SOT-23-5 | $0.55 |

### 1.6 SDRAM: IS42S32800G-6BLI
- 128 Mb (4M × 32-bit) SDRAM, 166 MHz
- FMC interface, 54-ball VFBGA (8mm × 13mm)
- $3.50 @ 1ku

### 1.7 SPI Flash: W25Q128JVSIQ
- 128 Mb (16 MB) SPI NOR Flash
- 133 MHz quad SPI
- SOIC-8 (5mm × 6mm)
- $0.90 @ 1ku

### 1.8 BLE Module: nRF52832-QFAA
- BLE 5.0, 64 KB RAM, 512 KB Flash
- QFN-48 (6mm × 6mm)
- UART interface to main MCU
- $3.40 @ 1ku

### 1.9 Ethernet Magnetics: HX6096FNL
- Dual-port gigabit magnetics module with center taps for PoE
- 16-pin DIP (21mm × 13mm)
- $2.10 @ 1ku

### 1.10 RJ45 Connectors: SJ45520N
- Shielded 8P8C with integrated LEDs (disabled)
- Side-entry, shield can
- $1.50 each × 2 = $3.00

## 2. Complete BOM

| # | Ref | Part Number | Description | Qty | Unit $ | Total $ |
|---|-----|-------------|-------------|-----|--------|---------|
| 1 | U1 | STM32H743ZIT6 | Cortex-M7 MCU | 1 | 12.50 | 12.50 |
| 2 | U2 | KSZ9897R | 7-port GbE switch | 1 | 8.20 | 8.20 |
| 3 | U3 | TPS2378DDW | PoE PD controller (input) | 1 | 2.80 | 2.80 |
| 4 | U4 | TPS23781DDWR | PoE PD controller (output) | 1 | 2.80 | 2.80 |
| 5 | U5 | TPS62A02AMDR | 48V→3.3V buck | 1 | 1.20 | 1.20 |
| 6 | U6 | TLV62569DBV | 3.3V→1.2V buck | 1 | 0.60 | 0.60 |
| 7 | U7 | TLV62568DBV | 3.3V→1.8V buck | 1 | 0.60 | 0.60 |
| 8 | U8 | TLV62565DBV | 3.3V→2.5V buck | 1 | 0.55 | 0.55 |
| 9 | U9 | IS42S32800G-6BLI | 128Mb SDRAM | 1 | 3.50 | 3.50 |
| 10 | U10 | W25Q128JVSIQ | 128Mb SPI flash | 1 | 0.90 | 0.90 |
| 11 | U11 | nRF52832-QFAA | BLE 5.0 module | 1 | 3.40 | 3.40 |
| 12 | T1 | HX6096FNL | Dual magnetics | 2 | 2.10 | 4.20 |
| 13 | J1,J2 | SJ45520N | RJ45 shielded | 2 | 1.50 | 3.00 |
| 14 | Y1 | ABM8AIG-48.000MHZ | 48 MHz crystal (switch) | 1 | 0.45 | 0.45 |
| 15 | Y2 | ABM8AIG-25.000MHZ | 25 MHz crystal (MCU ETH) | 1 | 0.40 | 0.40 |
| 16 | Y3 | ABS07AIG-32.768KHZ | 32.768 kHz RTC crystal | 1 | 0.30 | 0.30 |
| 17 | Y4 | ABM8AIG-32.000MHZ | 32 MHz crystal (BLE) | 1 | 0.40 | 0.40 |
| 18 | C1-C8 | CL05B104KO5NNNC | 100nF 0402 decoupling (8x) | 8 | 0.01 | 0.08 |
| 19 | C9-C24 | GRM21BR71E106K | 10µF 0805 bulk (16x) | 16 | 0.02 | 0.32 |
| 20 | C25-C32 | C0402C104J5RACTU | 100nF 0402 near ICs (8x) | 8 | 0.01 | 0.08 |
| 21 | L1-L4 | SRN3015-4R7M | 4.7µH power inductor (4x) | 4 | 0.25 | 1.00 |
| 22 | L5-L8 | BLM18PG121SN1 | 120Ω ferrite bead (4x) | 4 | 0.03 | 0.12 |
| 23 | R1-R50 | Various 0402 | Pull-up/down, divider, sense | 50 | 0.005 | 0.25 |
| 24 | D1 | SMBJ48A | TVS 48V PoE input | 1 | 0.30 | 0.30 |
| 25 | D2-D3 | PRTR5V0U2X | ESD protection USB/UART | 2 | 0.15 | 0.30 |
| 26 | FB1 | BLM31PG121SN1L | 120Ω ferrite PoE rail | 1 | 0.08 | 0.08 |
| 27 | SW1 | SKQGAKE010 | tactile switch (reset) | 1 | 0.10 | 0.10 |
| 28 | SW2 | SKQGAKE010 | tactile switch (boot) | 1 | 0.10 | 0.10 |
| | | | **TOTAL** | | | **$48.43** |

## 3. Pinout Tables

### 3.1 STM32H743ZIT6 Key Pin Assignments (LQFP-144)

| Pin | Name | Function | Net |
|-----|------|----------|-----|
| 17 | PA0 | ETH_MDC | U2_MDC |
| 18 | PA1 | ETH_MDIO | U2_MDIO |
| 25 | PA7 | ETH_RGMII_CRSDV | U2_CRSDV |
| 30 | PA8 | ETH_RGMII_TXCLK | U2_TXCLK |
| 31 | PA9 | ETH_RGMII_TXD0 | U2_TXD0 |
| 32 | PA10 | ETH_RGMII_TXD1 | U2_TXD1 |
| 33 | PA11 | ETH_RGMII_TXD2 | U2_TXD2 |
| 34 | PA12 | ETH_RGMII_TXD3 | U2_TXD3 |
| 35 | PA13 | ETH_RGMII_TXCTL | U2_TXCTL |
| 44 | PB0 | ETH_RGMII_RXD0 | U2_RXD0 |
| 45 | PB1 | ETH_RGMII_RXD1 | U2_RXD1 |
| 46 | PB2 | ETH_RGMII_RXD2 | U2_RXD2 |
| 47 | PB3 | ETH_RGMII_RXD3 | U2_RXD3 |
| 48 | PB4 | ETH_RGMII_RXCTL | U2_RXCTL |
| 49 | PB5 | ETH_RGMII_RXCLK | U2_RXCLK |
| 51 | PB7 | FMC_NL | SDRAM_NL |
| 56 | PB14 | SPI2_SCK | FLASH_SCK |
| 57 | PB15 | SPI2_MOSI | FLASH_MOSI |
| 63 | PC4 | SPI2_MISO | FLASH_MISO |
| 64 | PC5 | FMC_SDCKE0 | SDRAM_CKE |
| 78 | PC0 | FMC_SDNWE | SDRAM_NWE |
| 79 | PC1 | UART4_TX | BLE_UART_RX |
| 80 | PC2 | UART4_RX | BLE_UART_TX |
| 81 | PC3 | I2C1_SCL | POE_SCL |
| 82 | PC4 | I2C1_SDA | POE_SDA |
| 85 | PD0 | FMC_D2 | SDRAM_D2 |
| 86 | PD1 | FMC_D3 | SDRAM_D3 |
| 87 | PD2 | FMC_D0 | SDRAM_D0 |
| 88 | PD3 | FMC_D1 | SDRAM_D1 |
| 89 | PD4 | FMC_D4 | SDRAM_D4 |
| 90 | PD5 | FMC_D5 | SDRAM_D5 |
| 91 | PD6 | FMC_D6 | SDRAM_D6 |
| 92 | PD7 | FMC_D7 | SDRAM_D7 |
| 93 | PD8 | FMC_D8 | SDRAM_D8 |
| 94 | PD9 | FMC_D9 | SDRAM_D9 |
| 95 | PD10 | FMC_D10 | SDRAM_D10 |
| 96 | PD11 | FMC_D11 | SDRAM_D11 |
| 97 | PD12 | FMC_D12 | SDRAM_D12 |
| 98 | PD13 | FMC_D13 | SDRAM_D13 |
| 99 | PD14 | FMC_D14 | SDRAM_D14 |
| 100 | PD15 | FMC_D15 | SDRAM_D15 |
| 105 | PE0 | FMC_D16 | SDRAM_D16 |
| 106 | PE1 | FMC_D17 | SDRAM_D17 |
| 107 | PE2 | FMC_D18 | SDRAM_D18 |
| 108 | PE3 | FMC_D19 | SDRAM_D19 |
| 109 | PE4 | FMC_D20 | SDRAM_D20 |
| 110 | PE5 | FMC_D21 | SDRAM_D21 |
| 111 | PE6 | FMC_D22 | SDRAM_D22 |
| 112 | PE7 | FMC_D23 | SDRAM_D23 |
| 113 | PE8 | FMC_D24 | SDRAM_D24 |
| 114 | PE9 | FMC_D25 | SDRAM_D25 |
| 115 | PE10 | FMC_D26 | SDRAM_D26 |
| 116 | PE11 | FMC_D27 | SDRAM_D27 |
| 117 | PE12 | FMC_D28 | SDRAM_D28 |
| 118 | PE13 | FMC_D29 | SDRAM_D29 |
| 119 | PE14 | FMC_D30 | SDRAM_D30 |
| 120 | PE15 | FMC_D31 | SDRAM_D31 |
| 121 | PE2 | FMC_A0 | SDRAM_A0 |
| 122 | PE3 | FMC_A1 | SDRAM_A1 |
| 125 | PE4 | FMC_A2 | SDRAM_A2 |
| 126 | PE5 | FMC_A3 | SDRAM_A3 |
| 127 | PE6 | FMC_A4 | SDRAM_A4 |
| 128 | PE7 | FMC_A5 | SDRAM_A5 |
| 129 | PE8 | FMC_A6 | SDRAM_A6 |
| 130 | PE9 | FMC_A7 | SDRAM_A7 |
| 131 | PE10 | FMC_A8 | SDRAM_A8 |
| 132 | PE11 | FMC_A9 | SDRAM_A9 |
| 133 | PE12 | FMC_A10 | SDRAM_A10 |
| 134 | PE13 | FMC_A11 | SDRAM_A11 |
| 135 | PE14 | FMC_BA0 | SDRAM_BA0 |
| 136 | PE15 | FMC_BA1 | SDRAM_BA1 |
| 19 | NRST | Reset | SW1_RESET |
| 20 | BOOT0 | Boot mode | SW2_BOOT |
| 7 | VDD | 3.3V power | VDD_3V3 |
| 8 | VSS | Ground | GND |
| 72 | VDDA | 3.3V analog | VDDA_3V3 |
| 73 | VSSA | Analog ground | AGND |

### 3.2 KSZ9897R Key Pin Assignments (QFN-128)

| Pin | Name | Function | Net |
|-----|------|----------|-----|
| 3 | PORT1_TRXP | Port 1 TX+ | T1_P1_TX+ |
| 4 | PORT1_TRXM | Port 1 TX- | T1_P1_TX- |
| 5 | PORT1_RXP | Port 1 RX+ | T1_P1_RX+ |
| 6 | PORT1_RXM | Port 1 RX- | T1_P1_RX- |
| 11 | PORT2_TRXP | Port 2 TX+ | T1_P2_TX+ |
| 12 | PORT2_TRXM | Port 2 TX- | T1_P2_TX- |
| 13 | PORT2_RXP | Port 2 RX+ | T1_P2_RX+ |
| 14 | PORT2_RXM | Port 2 RX- | T1_P2_RX- |
| 65 | RGMII_TXD0 | CPU TX Data 0 | MCU_ETH_TXD0 |
| 66 | RGMII_TXD1 | CPU TX Data 1 | MCU_ETH_TXD1 |
| 67 | RGMII_TXD2 | CPU TX Data 2 | MCU_ETH_TXD2 |
| 68 | RGMII_TXD3 | CPU TX Data 3 | MCU_ETH_TXD3 |
| 69 | RGMII_TXCTL | CPU TX Control | MCU_ETH_TXCTL |
| 70 | RGMII_TXCLK | CPU TX Clock | MCU_ETH_TXCLK |
| 71 | RGMII_RXD0 | CPU RX Data 0 | MCU_ETH_RXD0 |
| 72 | RGMII_RXD1 | CPU RX Data 1 | MCU_ETH_RXD1 |
| 73 | RGMII_RXD2 | CPU RX Data 2 | MCU_ETH_RXD2 |
| 74 | RGMII_RXD3 | CPU RX Data 3 | MCU_ETH_RXD3 |
| 75 | RGMII_RXCTL | CPU RX Control | MCU_ETH_RXCTL |
| 76 | RGMII_RXCLK | CPU RX Clock | MCU_ETH_RXCLK |
| 80 | MDC | Mgmt Clock | MCU_ETH_MDC |
| 81 | MDIO | Mgmt Data | MCU_ETH_MDIO |
| 100 | SDA | I2C Data | SWITCH_I2C_SDA |
| 101 | SCL | I2C Clock | SWITCH_I2C_SCL |
| 120 | XI | 48 MHz crystal | Y1_XI |
| 121 | XO | 48 MHz crystal | Y1_XO |
| 15 | VDDA1 | 3.3V analog | VDDA_3V3 |
| 40 | VDDIO | 3.3V I/O | VDD_3V3 |
| 50 | VDDC | 1.2V core | VDD_1V2 |
| 128 | VSS | Ground | GND |

### 3.3 TPS2378DDW Pin Assignments (TSSOP-20)

| Pin | Name | Function | Net |
|-----|------|----------|-----|
| 1 | VDD | PoE input voltage | POE_VIN |
| 2 | DEN | Detection enable | POE_DEN (pulled low) |
| 3 | CLS | Classification | POECLS_RES (24.9kΩ to GND = Class 0) |
| 4 | RTN | PoE return | POE_RTN |
| 5 | T2P | Type 2 detect | NC (Type 1 only) |
| 6 | PG | Power good output | POE_PG → MCU_GPIO |
| 7 | CDB | Cadence bypass | POE_CDB (cap to GND) |
| 8 | SDA | I2C data | POE_SDA |
| 9 | SCL | I2C clock | POE_SCL |
| 10 | ADDR | I2C address | GND (addr = 0x20) |
| 11 | VSS | Ground | GND |
| 12 | GATE | Gate drive output | POE_GATE → Q1_GATE |
| 13 | CSN | Current sense - | POE_CSN |
| 14 | CSP | Current sense + | POE_CSP |
| 15 | VDD1 | Internal supply | POE_VDD1 |
| 16 | VDD2 | Internal supply | POE_VDD2 |
| 17 | APD | Auto power down | NC |
| 18 | DET | Detection resistor | POE_DET (25.5kΩ) |
| 19 | CLASS | Class resistor | POE_CLASS |
| 20 | VDD3 | PoE voltage sense | POE_VIN |

### 3.4 nRF52832-QFAA Key Pin Assignments (QFN-48)

| Pin | Name | Function | Net |
|-----|------|----------|-----|
| 1 | DEC1 | Decoupling | BLE_DEC1 |
| 2 | P0.02 | UART RX | BLE_UART_TX_FROM_MCU |
| 3 | P0.03 | UART TX | BLE_UART_RX_TO_MCU |
| 4 | P0.04 | UART CTS | BLE_UART_CTS |
| 5 | P0.05 | UART RTS | BLE_UART_RTS |
| 6 | P0.06 | Status LED (IR, hidden) | BLE_STATUS |
| 7 | P0.07 | SWDIO | SWD_SWDIO |
| 8 | P0.08 | SWCLK | SWD_SWCLK |
| 9 | VDD | 3.3V power | VDD_3V3 |
| 10 | GND | Ground | GND |
| 11 | DEC2 | Decoupling | BLE_DEC2 |
| 12 | XC1 | 32 MHz crystal | Y4_XC1 |
| 13 | XC2 | 32 MHz crystal | Y4_XC2 |
| 14 | ANT | Antenna | BLE_ANT |
| 15 | VDD | 3.3V power | VDD_3V3 |
| 46 | DEC3 | Decoupling | BLE_DEC3 |
| 47 | P0.30 | Reset | BLE_RESET |
| 48 | P0.31 | Boot mode | BLE_BOOT |

## 4. Netlist (Source Pin → Component → Dest Pin)

### 4.1 RGMII Bus (MCU ↔ KSZ9897)
```
U1.PA8  (ETH_RGMII_TXCLK)  → U2.70  (RGMII_TXCLK)
U1.PA9  (ETH_RGMII_TXD0)   → U2.65  (RGMII_TXD0)
U1.PA10 (ETH_RGMII_TXD1)   → U2.66  (RGMII_TXD1)
U1.PA11 (ETH_RGMII_TXD2)   → U2.67  (RGMII_TXD2)
U1.PA12 (ETH_RGMII_TXD3)   → U2.68  (RGMII_TXD3)
U1.PA13 (ETH_RGMII_TXCTL)  → U2.69  (RGMII_TXCTL)
U2.71  (RGMII_RXD0)   → U1.PB0  (ETH_RGMII_RXD0)
U2.72  (RGMII_RXD1)   → U1.PB1  (ETH_RGMII_RXD1)
U2.73  (RGMII_RXD2)   → U1.PB2  (ETH_RGMII_RXD2)
U2.74  (RGMII_RXD3)   → U1.PB3  (ETH_RGMII_RXD3)
U2.75  (RGMII_RXCTL)  → U1.PB4  (ETH_RGMII_RXCTL)
U2.76  (RGMII_RXCLK)  → U1.PB5  (ETH_RGMII_RXCLK)
U1.PA0  (ETH_MDC)     → U2.80  (MDC)
U1.PA1  (ETH_MDIO)    → U2.81  (MDIO) [bidir, 4.7kΩ pull-up]
```

### 4.2 FMC SDRAM Bus (MCU → IS42S32800G)
```
U1.PD0  (FMC_D2)   → U9.D2
U1.PD1  (FMC_D3)   → U9.D3
U1.PD2  (FMC_D0)   → U9.D0
U1.PD3  (FMC_D1)   → U9.D1
U1.PD4  (FMC_D4)   → U9.D4
U1.PD5  (FMC_D5)   → U9.D5
U1.PD6  (FMC_D6)   → U9.D6
U1.PD7  (FMC_D7)   → U9.D7
U1.PD8  (FMC_D8)   → U9.D8
U1.PD9  (FMC_D9)   → U9.D9
U1.PD10 (FMC_D10)  → U9.D10
U1.PD11 (FMC_D11)  → U9.D11
U1.PD12 (FMC_D12)  → U9.D12
U1.PD13 (FMC_D13)  → U9.D13
U1.PD14 (FMC_D14)  → U9.D14
U1.PD15 (FMC_D15)  → U9.D15
U1.PE0  (FMC_D16)  → U9.D16
U1.PE1  (FMC_D17)  → U9.D17
... (through D31)
U1.PE15 (FMC_D31)  → U9.D31
U1.PE2  (FMC_A0)   → U9.A0
... (through A11)
U1.PE13 (FMC_A11)  → U9.A11
U1.PE14 (FMC_BA0)  → U9.BA0
U1.PE15 (FMC_BA1)  → U9.BA1
U1.PC5  (FMC_SDCKE0) → U9.CKE
U1.PC0  (FMC_SDNWE)  → U9.NWE
U1.PB7  (FMC_NL)     → U9.NLAS
```

### 4.3 SPI Flash (MCU → W25Q128JVSIQ)
```
U1.PB14 (SPI2_SCK)  → U10.6 (CLK)
U1.PB15 (SPI2_MOSI) → U10.5 (DI)
U1.PC4  (SPI2_MISO) → U10.2 (DO)
U1.PC6  (GPIO)      → U10.1 (CS#) [active low]
```

### 4.4 UART BLE (MCU ↔ nRF52832)
```
U1.PC1 (UART4_TX) → U11.P0.03 (UART RX)
U1.PC2 (UART4_RX) → U11.P0.02 (UART TX)
U1.PC6 (GPIO)      → U11.P0.30 (BLE_RESET)
```

### 4.5 I2C PoE (MCU → TPS2378)
```
U1.PC3 (I2C1_SCL) → U3.9 (SCL) [4.7kΩ pull-up to 3.3V]
U1.PC4 (I2C1_SDA) → U3.8 (SDA) [4.7kΩ pull-up to 3.3V]
```

### 4.6 Ethernet PHY (Magnetics ↔ KSZ9897 ↔ RJ45)
```
J1.P1 (Tip1)  → T1.P1_TX_CT  → U2.PORT1_TRXP (Port 1 = IN)
J1.P2 (Ring1) → T1.P1_TX_CT  → U2.PORT1_TRXM
J1.P3 (Tip2)  → T1.P1_RX_CT  → U2.PORT1_RXP
J1.P6 (Ring2) → T1.P1_RX_CT  → U2.PORT1_RXM
J2.P1 (Tip1)  → T1.P2_TX_CT  → U2.PORT2_TRXP (Port 2 = OUT)
J2.P2 (Ring1) → T1.P2_TX_CT  → U2.PORT2_TRXM
J2.P3 (Tip2)  → T1.P2_RX_CT  → U2.PORT2_RXP
J2.P6 (Ring2) → T1.P2_RX_CT  → U2.PORT2_RXM
```

### 4.7 PoE Power Path
```
J1.P4,P5 (Spare pair A) → D1 (TVS 48V) → L5 (ferrite) → U3.VDD (TPS2378 input)
J1.P7,P8 (Spare pair B) → D1            → L5           → U3.VDD
U3.GATE → Q1 (MOSFET gate) → VPOE (48V switched) → U5.VIN (TPS62A02A input)
U5.VOUT → VDD_3V3 rail
U5.VOUT → U6.VIN → U6.VOUT → VDD_1V2 rail
U5.VOUT → U7.VIN → U7.VOUT → VDD_1V8 rail
U5.VOUT → U8.VIN → U8.VOUT → VDD_2V5 rail
VPOE (48V) → U4.VDD (TPS23781 pass-through PD) → J2.P4,P5,P7,P8 (PoE output to victim)
```

## 5. Decoupling Strategy

| IC | Cap Value | Package | Placement | Count |
|----|-----------|---------|-----------|-------|
| U1 (STM32H743) | 100nF | 0402 | Each VDD pin pair | 12 |
| U1 (STM32H743) | 10µF | 0805 | Near VDDA | 2 |
| U2 (KSZ9897) | 100nF | 0402 | Each VDD pin | 8 |
| U2 (KSZ9897) | 10µF | 0805 | VDDC (1.2V) | 2 |
| U9 (SDRAM) | 100nF | 0402 | Each VDD pin | 4 |
| U9 (SDRAM) | 10µF | 0805 | Bulk | 1 |
| U11 (nRF52832) | 100nF | 0402 | DEC pins | 3 |
| U11 (nRF52832) | 10µF | 0805 | VDD | 1 |

Total decoupling capacitors: ~33

## 6. Impedance Pairs

| Signal Group | Impedance | Tolerance | Reference |
|-------------|-----------|-----------|-----------|
| Ethernet pairs (100Ω diff) | 100Ω | ±10% | HX6096FNL datasheet |
| RGMII single-ended (50Ω) | 50Ω | ±10% | KSZ9897 datasheet |
| RGMII diff clocks (100Ω) | 100Ω | ±10% | KSZ9897 datasheet |
| SDRAM data (50Ω single) | 50Ω | ±15% | IS42S32800G datasheet |
| SDRAM address (50Ω single) | 50Ω | ±15% | IS42S32800G datasheet |
| SPI (50Ω single) | 50Ω | ±15% | W25Q128JV datasheet |
| PoE center tap (25Ω single) | 25Ω | ±20% | TPS2378 datasheet |
| BLE antenna (50Ω) | 50Ω | ±5% | nRF52832 datasheet |