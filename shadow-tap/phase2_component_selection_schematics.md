# ShadowTap — Phase 2: Component Selection & Schematics

## 1. Bill of Materials

| Ref | Part Number | Description | Qty | Unit $ | Total $ |
|---|---|---|---|---|---|
| U1 | MIMXRT1062DVL6B | i.MX RT1062 Cortex-M7 600 MHz, 256 KB L2, ENET×2, 100-LQFP | 1 | 10.50 | 10.50 |
| U2 | 88E1510-A0-NNB2I000 | Marvell ETH PHY, RGMII, 1000Base-T, 48-QFN | 2 | 3.80 | 7.60 |
| U3 | TPS2378DDA-R-P | TI PoE PD controller, 802.3af/at, 8-SOIC | 1 | 2.10 | 2.10 |
| U4 | TPS562201DDCR | TI 5V/3A step-down converter, SOT-563 | 1 | 0.65 | 0.65 |
| U5 | TLV62569DBVR | TI 3.3V/3A step-down converter, SOT-563 | 1 | 0.55 | 0.55 |
| U6 | TLV62569DBVR | TI 1.2V/2A step-down converter, SOT-563 | 1 | 0.55 | 0.55 |
| U7 | IS25LP016D-JBLE | ISSI 16 MB NOR Flash, SPI, 133 MHz, 8-SOIC | 1 | 0.95 | 0.95 |
| U8 | AT24C02C-SSHL-T | Microchip 2 Kb I²C EEPROM, 8-SOIC | 1 | 0.18 | 0.18 |
| U9 | nRF52840-M2 | Fanstel BLE5.0 M.2 module, FCC cert | 1 | 7.50 | 7.50 |
| U10 | TLV1117LV33DCYR | TI 3.3V LDO 800 mA, SOT-223 (PHY IO) | 1 | 0.35 | 0.35 |
| J1, J2 | LPJG16314A4NL | Pulse RJ45 MagJack w/ PoE center taps, 1:1, 8P8C | 2 | 2.20 | 4.40 |
| J3 | M2-KEY-E-50CM | M.2 Key E socket (for nRF52840 module) | 1 | 1.20 | 1.20 |
| J4 | DM3AT-SF-PEJM5S | Hirose microSD card slot, push-push | 1 | 0.85 | 0.85 |
| Y1 | ECS-250BX-120M | 24 MHz crystal, 20 ppm, 3225 | 1 | 0.30 | 0.30 |
| Y2, Y3 | ECS-250BX-250M | 25 MHz crystal for PHY, 3225 | 2 | 0.30 | 0.60 |
| SW1 | SKQGAKE010 | ALPS tact switch 6×6 mm, reset | 1 | 0.10 | 0.10 |
| D1–D4 | WS2812B-Mini | RGB LED, 3535 | 4 | 0.20 | 0.80 |
| L1 | XAL6060-472MEB | 4.7 µH 6 A inductor, 6×6 mm (5V buck) | 1 | 0.60 | 0.60 |
| L2 | XAL4020-222MEB | 2.2 µH 4 A inductor, 4×4 mm (3.3V buck) | 1 | 0.45 | 0.45 |
| L3 | XAL4020-222MEB | 2.2 µH 4 A inductor, 4×4 mm (1.2V buck) | 1 | 0.45 | 0.45 |
| C_pool | Various | 0402 caps: 100nF×20, 10µF×10, 22µF×4, 1µF×6 | 40 | 0.01 | 0.40 |
| R_pool | Various | 0402 resistors: 49.9Ω×8, 10kΩ×6, 1kΩ×4, 100Ω×4 | 22 | 0.005 | 0.11 |
| T1 | PT61020PEL | Ethernet isolation transformer (in MagJack) | — | — | — |
| — | PCB | 4-layer, 60×40 mm, ENIG | 1 | 2.00 | 2.00 |
| | | | | **Total** | **$44.89** |

## 2. IC Pinout Tables

### 2.1 U1: MIMXRT1062DVL6B (100-LQFP)

| Pin | Name | Function | Net |
|---|---|---|---|
| 7 | GPIO_AD_B0_02 | ENET1_MDIO | U2_MDIO |
| 8 | GPIO_AD_B0_03 | ENET1_MDC | U2_MDC |
| 9 | GPIO_AD_B0_04 | ENET1_RXD0 | U2_RXD0 |
| 10 | GPIO_AD_B0_05 | ENET1_RXD1 | U2_RXD1 |
| 11 | GPIO_AD_B0_06 | ENET1_RXD2 | — (RGMII unused, tie low) |
| 12 | GPIO_AD_B0_07 | ENET1_RXD3 | — (RGMII unused, tie low) |
| 13 | GPIO_AD_B0_08 | ENET1_RX_CLK | U2_RX_CLK |
| 14 | GPIO_AD_B0_09 | ENET1_RXDV | U2_RX_DV |
| 15 | GPIO_AD_B0_10 | ENET1_TX_CLK | U2_TX_CLK |
| 16 | GPIO_AD_B0_11 | ENET1_TXD0 | U2_TXD0 |
| 17 | GPIO_AD_B0_12 | ENET1_TXD1 | U2_TXD1 |
| 18 | GPIO_AD_B0_13 | ENET1_TXD2 | — (RGMII unused, tie low) |
| 19 | GPIO_AD_B0_14 | ENET1_TXD3 | — (RGMII unused, tie low) |
| 20 | GPIO_AD_B0_15 | ENET1_TXEN | U2_TX_EN |
| 30 | GPIO_AD_B1_00 | ENET2_MDIO | U3_MDIO |
| 31 | GPIO_AD_B1_01 | ENET2_MDC | U3_MDC |
| 32 | GPIO_AD_B1_02 | ENET2_RXD0 | U3_RXD0 |
| 33 | GPIO_AD_B1_03 | ENET2_RXD1 | U3_RXD1 |
| 34 | GPIO_AD_B1_04 | ENET2_RX_CLK | U3_RX_CLK |
| 35 | GPIO_AD_B1_05 | ENET2_RXDV | U3_RX_DV |
| 36 | GPIO_AD_B1_06 | ENET2_TX_CLK | U3_TX_CLK |
| 37 | GPIO_AD_B1_07 | ENET2_TXD0 | U3_TXD0 |
| 38 | GPIO_AD_B1_08 | ENET2_TXD1 | U3_TXD1 |
| 39 | GPIO_AD_B1_09 | ENET2_TXEN | U3_TX_EN |
| 44 | GPIO_B0_00 | LPUART1_TX | BLE_UART_TX |
| 45 | GPIO_B0_01 | LPUART1_RX | BLE_UART_RX |
| 48 | GPIO_B0_04 | LPSPI1_SCK | FLASH_SCK |
| 49 | GPIO_B0_05 | LPSPI1_SDO | FLASH_MOSI |
| 50 | GPIO_B0_06 | LPSPI1_SDI | FLASH_MISO |
| 51 | GPIO_B0_07 | LPSPI1_PCS0 | FLASH_CS |
| 55 | GPIO_B0_11 | LPI2C1_SDA | EEPROM_SDA |
| 56 | GPIO_B0_12 | LPI2C1_SCL | EEPROM_SCL |
| 57 | GPIO_B0_13 | LPI2C2_SDA | POE_SDA |
| 58 | GPIO_B0_14 | LPI2C2_SCL | POE_SCL |
| 62 | GPIO_SD_B0_00 | uSDHC1_CMD | SD_CMD |
| 63 | GPIO_SD_B0_01 | uSDHC1_CLK | SD_CLK |
| 64 | GPIO_SD_B0_02 | uSDHC1_D0 | SD_D0 |
| 65 | GPIO_SD_B0_03 | uSDHC1_D1 | SD_D1 |
| 66 | GPIO_SD_B0_04 | uSDHC1_D2 | SD_D2 |
| 67 | GPIO_SD_B0_05 | uSDHC1_D3 | SD_D3 |
| 70 | GPIO_SD_B0_06 | uSDHC1_CD | SD_CD |
| 72 | ONCHIP_CLK1_P | 24 MHz XTAL+ | XTAL_P |
| 73 | ONCHIP_CLK1_N | 24 MHz XTAL− | XTAL_N |
| 80 | POR_B | Power-on reset | RESET_SW |
| 85 | GPIO_EMC_04 | LED_D1_DATA | LED_DIN |
| 86 | GPIO_EMC_05 | Button input | BTN_IN |
| 1 | VDD_SOC_IN | Core 1.2V | VCC_1V2 |
| 3, 24, 42, 60, 76, 88 | VDD_HIGH_IN | I/O 3.3V | VCC_3V3 |
| 5, 22, 40, 58, 74, 90 | VSS | Ground | GND |

### 2.2 U2/U3: 88E1510-A0-NNB2I000 (48-QFN)

| Pin | Name | Function | Net (U2) | Net (U3) |
|---|---|---|---|---|
| 1 | TXD0 | RGMII TXD0 | U2_TXD0 | U3_TXD0 |
| 2 | TXD1 | RGMII TXD1 | U2_TXD1 | U3_TXD1 |
| 3 | TXD2 | RGMII TXD2 | — | — |
| 4 | TXD3 | RGMII TXD3 | — | — |
| 5 | TX_CLK | RGMII TX CLK | U2_TX_CLK | U3_TX_CLK |
| 6 | TX_EN | RGMII TX Enable | U2_TX_EN | U3_TX_EN |
| 7 | RXD0 | RGMII RXD0 | U2_RXD0 | U3_RXD0 |
| 8 | RXD1 | RGMII RXD1 | U2_RXD1 | U3_RXD1 |
| 9 | RXD2 | RGMII RXD2 | — | — |
| 10 | RXD3 | RGMII RXD3 | — | — |
| 11 | RX_CLK | RGMII RX CLK | U2_RX_CLK | U3_RX_CLK |
| 12 | RX_DV | RGMII RX Data Valid | U2_RX_DV | U3_RX_DV |
| 13 | MDIO | Management Data I/O | U2_MDIO | U3_MDIO |
| 14 | MDC | Management Clock | U2_MDC | U3_MDC |
| 15 | INTn | Interrupt (active low) | U2_INTn | U3_INTn |
| 16 | RSTn | Reset (active low) | U2_RSTn | U3_RSTn |
| 17 | XTAL1 | 25 MHz crystal+ | Y2_XTAL1 | Y3_XTAL1 |
| 18 | XTAL2 | 25 MHz crystal− | Y2_XTAL2 | Y3_XTAL2 |
| 19 | VDDIO | I/O 3.3V | VCC_3V3 | VCC_3V3 |
| 20 | VDDA | Analog 3.3V | VCC_3V3A | VCC_3V3A |
| 21–28 | TXP/N, RXP/N | Diff pairs to MagJack | J1_TXP/N, J1_RXP/N | J2_TXP/N, J2_RXP/N |
| 29 | VDD_CORE | 1.2V core | VCC_1V2 | VCC_1V2 |
| 30 | GND | Ground | GND | GND |

### 2.3 U9: nRF52840-M.2 (M.2 Key E)

| Pin | Name | Function | Net |
|---|---|---|---|
| 2 | 3V3 | Power 3.3V | VCC_3V3 |
| 4 | GND | Ground | GND |
| 6 | UART_TX | BLE UART TX → SoC RX | BLE_UART_RX |
| 8 | UART_RX | BLE UART RX ← SoC TX | BLE_UART_TX |
| 10 | UART_CTS | Flow control | BLE_CTS |
| 12 | UART_RTS | Flow control | BLE_RTS |
| 14 | SWDIO | Debug | BLE_SWDIO |
| 16 | SWCLK | Debug | BLE_SWCLK |
| 18 | RESET | Module reset | BLE_RSTn |

## 3. Netlist (Source → Component → Destination)

### 3.1 ENET1 RGMII (U1 ↔ U2)

| Net | Source Pin | → | Dest Pin | Notes |
|---|---|---|---|---|
| U2_TXD0 | U1.16 (ENET1_TXD0) | → | U2.1 (TXD0) | 49.9Ω series term |
| U2_TXD1 | U1.17 (ENET1_TXD1) | → | U2.2 (TXD1) | 49.9Ω series term |
| U2_TX_EN | U1.20 (ENET1_TXEN) | → | U2.6 (TX_EN) | 49.9Ω series term |
| U2_TX_CLK | U1.15 (ENET1_TX_CLK) | → | U2.5 (TX_CLK) | 49.9Ω series term |
| U2_RXD0 | U2.7 (RXD0) | → | U1.9 (ENET1_RXD0) | Direct |
| U2_RXD1 | U2.8 (RXD1) | → | U1.10 (ENET1_RXD1) | Direct |
| U2_RX_DV | U2.12 (RX_DV) | → | U1.14 (ENET1_RXDV) | Direct |
| U2_RX_CLK | U2.11 (RX_CLK) | → | U1.13 (ENET1_RX_CLK) | Direct |
| U2_MDIO | U1.7 (ENET1_MDIO) | ↔ | U2.13 (MDIO) | Bidir, 10kΩ pull-up |
| U2_MDC | U1.8 (ENET1_MDC) | → | U2.14 (MDC) | Direct |

### 3.2 ENET2 RGMII (U1 ↔ U3)

| Net | Source Pin | → | Dest Pin | Notes |
|---|---|---|---|---|
| U3_TXD0 | U1.37 (ENET2_TXD0) | → | U3.1 (TXD0) | 49.9Ω series term |
| U3_TXD1 | U1.38 (ENET2_TXD1) | → | U3.2 (TXD1) | 49.9Ω series term |
| U3_TX_EN | U1.39 (ENET2_TXEN) | → | U3.6 (TX_EN) | 49.9Ω series term |
| U3_TX_CLK | U1.36 (ENET2_TX_CLK) | → | U3.5 (TX_CLK) | 49.9Ω series term |
| U3_RXD0 | U3.7 (RXD0) | → | U1.32 (ENET2_RXD0) | Direct |
| U3_RXD1 | U3.8 (RXD1) | → | U1.33 (ENET2_RXD1) | Direct |
| U3_RX_DV | U3.12 (RX_DV) | → | U1.35 (ENET2_RXDV) | Direct |
| U3_RX_CLK | U3.11 (RX_CLK) | → | U1.34 (ENET2_RX_CLK) | Direct |
| U3_MDIO | U1.30 (ENET2_MDIO) | ↔ | U3.13 (MDIO) | Bidir, 10kΩ pull-up |
| U3_MDC | U1.31 (ENET2_MDC) | → | U3.14 (MDC) | Direct |

### 3.3 SPI Flash (U1 ↔ U7)

| Net | Source Pin | → | Dest Pin | Notes |
|---|---|---|---|---|
| FLASH_SCK | U1.48 (LPSPI1_SCK) | → | U7.6 (SCK) | Direct |
| FLASH_MOSI | U1.49 (LPSPI1_SDO) | → | U7.5 (SI) | Direct |
| FLASH_MISO | U7.2 (SO) | → | U1.50 (LPSPI1_SDI) | Direct |
| FLASH_CS | U1.51 (LPSPI1_PCS0) | → | U7.1 (CS#) | 10kΩ pull-up |

### 3.4 BLE UART (U1 ↔ U9)

| Net | Source Pin | → | Dest Pin | Notes |
|---|---|---|---|---|
| BLE_UART_TX | U1.44 (LPUART1_TX) | → | U9.8 (UART_RX) | 1kΩ series |
| BLE_UART_RX | U9.6 (UART_TX) | → | U1.45 (LPUART1_RX) | 1kΩ series |

### 3.5 SD Card (U1 ↔ J4)

| Net | Source Pin | → | Dest Pin | Notes |
|---|---|---|---|---|
| SD_CMD | U1.62 (uSDHC1_CMD) | → | J4.2 (CMD) | 10kΩ pull-up |
| SD_CLK | U1.63 (uSDHC1_CLK) | → | J4.5 (CLK) | Direct |
| SD_D0 | U1.64 (uSDHC1_D0) | ↔ | J4.7 (D0) | Direct |
| SD_D1 | U1.65 (uSDHC1_D1) | ↔ | J4.8 (D1) | Direct |
| SD_D2 | U1.66 (uSDHC1_D2) | ↔ | J4.9 (D2) | Direct |
| SD_D3 | U1.67 (uSDHC1_D3) | ↔ | J4.1 (D3) | 10kΩ pull-up |
| SD_CD | J4.10 (CD) | → | U1.70 (uSDHC1_CD) | 10kΩ pull-up |

## 4. Decoupling Requirements

| IC | Pin | Cap Value | Cap Type | Placement |
|---|---|---|---|---|
| U1 | VDD_SOC_IN (1.2V) | 4× 10µF + 2× 100nF | 0402 X5R | Within 5 mm of pin |
| U1 | VDD_HIGH_IN (3.3V) | 4× 100nF | 0402 X5R | Per supply pin pair |
| U2/U3 | VDD_CORE (1.2V) | 1× 10µF + 1× 100nF | 0402 X5R | Adjacent to pin |
| U2/U3 | VDDA (3.3V analog) | 1× 10µF + 1× 100nF | 0402 X5R | Adjacent, with ferrite |
| U2/U3 | VDDIO (3.3V I/O) | 1× 100nF | 0402 X5R | Adjacent to pin |
| U9 | 3V3 | 1× 10µF + 1× 100nF | 0402 X5R | At M.2 socket pin |
| U7 | VCC | 1× 100nF | 0402 X5R | Adjacent to pin |
| U8 | VCC | 1× 100nF | 0402 X5R | Adjacent to pin |

## 5. Impedance-Controlled Pairs

| Signal Group | Impedance | Layer Pair | Notes |
|---|---|---|---|
| Ethernet TX/RX diff pairs | 100Ω differential | L1/L4 to L2 ground | Within MagJack + PHY path |
| RGMII single-ended | 50Ω single-ended | L1 to L2 ground | 49.9Ω series term at source |
| SDIO (SDR50) | 50Ω single-ended | L1 to L2 ground | Length-matched within 50 mils |
| SPI (60 MHz) | 50Ω single-ended | L1 to L2 ground | Keep < 2 inch total |

## 6. Ethernet MagJack Wiring

Each MagJack (J1 uplink, J2 target) contains integrated magnetics and PoE center taps:

| MagJack Pin | Signal | Net | Notes |
|---|---|---|---|
| 1 | TX+ | U2.21 TXP / U3.21 TXP | Diff pair |
| 2 | TX− | U2.22 TXN / U3.22 TXN | Diff pair |
| 3 | RX+ | U2.23 RXP / U3.23 RXP | Diff pair |
| 6 | RX− | U2.24 RXN / U3.24 RXN | Diff pair |
| 4,5 | CT (center tap) | POE_CT_A / POE_CT_B | To TPS2378 |
| 7,8 | CT (center tap) | POE_CT_C / POE_CT_D | To TPS2378 |
| 9–12 | LEDs | LED_ link/activity | 3.3V via 100Ω R |
| 13 | Shield | GND | Chassis ground |

## 7. Power Supply Schematics

### 7.1 PoE PD Front-End (U3: TPS2378)

```
PoE CT Pairs ──▶ Bridge Rect (internal) ──▶ VPP (48V nominal) ──▶ TPS2378 ──▶ VDD (5.0V)
                                                                        │
                                                              Classification: Rclass = 127Ω → Class 3 (12.95W)
                                                              Detection: 24.9kΩ signature resistor
```

### 7.2 5V Buck (U4: TPS562201)

```
VDD (5V from TPS2378) ──▶ VIN ──▶ TPS562201 ──▶ SW ──▶ L1 (4.7µH) ──┬──▶ 5V_SYS
                                                                     │
                                                              Cout: 22µF × 2
                                                              FB divider: R1=100kΩ, R2=100kΩ → Vout=5V (pass-through mode)
```

Note: TPS2378 already regulates to 5V; TPS562201 serves as secondary regulation for load transient response.

### 7.3 3.3V Buck (U5: TLV62569)

```
5V_SYS ──▶ VIN ──▶ TLV62569 ──▶ SW ──▶ L2 (2.2µH) ──┬──▶ VCC_3V3
                                                      │
                                               Cout: 22µF × 2
                                               FB: R1=100kΩ, R2=47kΩ → Vout=3.35V
```

### 7.4 1.2V Buck (U6: TLV62569)

```
5V_SYS ──▶ VIN ──▶ TLV62569 ──▶ SW ──▶ L3 (2.2µH) ──┬──▶ VCC_1V2
                                                      │
                                               Cout: 22µF × 2
                                               FB: R1=100kΩ, R2=22kΩ → Vout=1.22V
```

### 7.5 1.5V LDO for PHY VDDA (U10: TLV1117LV33)

```
VCC_3V3 ──▶ VIN ──▶ TLV1117LV33 ──▶ VOUT ──┬──▶ VCC_3V3A (PHY analog)
                                            │
                                     Cout: 10µF
                                     Ferrite bead: BLM21PG221SN1 in series
```