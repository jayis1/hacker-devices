# Phase 2: Component Selection & Schematics - Volt-Glitcher

## Bill of Materials (BOM)

| Designator | Description | Part Number | Manufacturer | Approx. Price |
|------------|-------------|-------------|--------------|---------------|
| U1 | Main MCU (480MHz Cortex-M7) | STM32H723VGT6 | STMicroelectronics | $12.50 |
| U2 | High-Speed FPGA | ICE40UP5K-SG48ITR | Lattice Semi | $7.80 |
| U3 | 8-bit 100MSPS ADC | AD9283BRSZ-100 | Analog Devices | $9.20 |
| Q1 | Glitch MOSFET (N-Channel) | Si7116DN-T1-GE3 | Vishay | $0.85 |
| U4 | High-Speed Gate Driver | MIC4427ZM | Microchip | $1.15 |
| U5 | Adjustable Buck Regulator | TPS62130RGTR | TI | $1.60 |
| U6 | USB-C Controller/ESD | TPD4S012DRYR | TI | $0.45 |
| U7 | Level Shifter (8-ch) | TXB0108PWR | TI | $0.90 |
| U8 | Bluetooth LE Module | NRF52832-QFAA | Nordic Semi | $3.50 |
| J1 | USB-C Connector | 10118194-0001LF | Amphenol | $0.60 |
| XTAL1 | 24MHz Crystal (MCU/FPGA) | ABM8G-24.000MHZ | Abracon | $0.35 |

**Total Estimated BOM Cost**: ~$42.00 (Excluding passives and PCB)

## Pinout Table (STM32H723VGT6)

| Pin | Name | Function | Destination |
|-----|------|----------|-------------|
| PA0 | FMC_D0 | Parallel Data 0 | FPGA_IO0 |
| PA1 | FMC_D1 | Parallel Data 1 | FPGA_IO1 |
| ... | ... | ... | ... |
| PB0 | ADC_IN | Power Trace Input | U3_VOUT |
| PC0 | GLITCH_EN | Manual Glitch Enable | U4_IN_A |
| PD0 | SPI1_SCK | Control SPI | FPGA_SCK |
| PD1 | SPI1_MOSI | Control SPI | FPGA_SDI |
| PD2 | SPI1_MISO | Control SPI | FPGA_SDO |
| PE0 | TRIGGER_IN | External Trigger | J2_PIN1 |

## Netlist (Critical Paths)

1. **Glitch Path**:
   - `FPGA_OUT_GLITCH` (Pin 12) → `U4_IN_A` (Pin 2)
   - `U4_OUT_A` (Pin 7) → `Q1_GATE` (Pin 4)
   - `Q1_DRAIN` (Pins 5-8) → `TARGET_VCC` (Net)
   - `Q1_SOURCE` (Pins 1-3) → `GND`

2. **Trigger Path**:
   - `TARGET_GPIO_0` → `U7_A1` (Level Shifter) → `U7_B1` → `FPGA_TRIG_0` (Pin 20)
   - `FPGA_MATCH_SIG` → `MCU_EXTI0` (Pin PE0)

3. **Power Sensing**:
   - `TARGET_VCC` → `R_SENSE` (0.1 Ohm) → `U3_VIN` (ADC)
   - `U3_D[0..7]` → `FPGA_DATA[0..7]`

## Decoupling Strategy
- **STM32H7**: 100nF per VDD/VDDA pin + 4.7uF bulk per side.
- **ICE40**: 100nF per VCC/VCCIO pin + 10uF on VCC_CORE (1.2V).
- **Target Rail**: 10uF + 100nF low-ESR ceramic near Q1 to handle high-current transients.

## Impedance Pairs
- **USB_DP / USB_DM**: 90 Ohm differential impedance.
- **FMC_DATA[0..7]**: Length matched to within 50mil.
- **ADC_CLK / ADC_DATA**: Length matched for 100MHz timing closure.
