# Phase 3: PCB Blueprints & Layout — RF Transceiver Tool

## 1. Board Specifications

| Parameter | Value |
|---|---|
| Dimensions | 85 mm × 54 mm |
| Layers | 4 |
| Finish | ENIG (Electroless Nickel Immersion Gold) |
| Thickness | 1.6 mm |
| Copper weight | 1 oz (35 µm) all layers |
| Min trace width | 0.15 mm (6 mil) |
| Min trace spacing | 0.15 mm (6 mil) |
| Min via diameter | 0.3 mm drill / 0.6 mm pad |
| Min drill | 0.3 mm |
| Surface mount | 0402 min passive, 0.5mm pitch IC |
| Solder mask | Green (or black for RF) |
| Silkscreen | White |
| Impedance control | 50 Ω single-ended, 90 Ω diff pair |

## 2. Stackup Definition

4-layer stackup (standard FR4, 1.6mm total):

```
Layer    | Material           | Thickness  | Function
---------|-------------------|------------|------------------
Top      | Copper (1 oz)     | 35 µm      | Signal + component pads
L2       | Prepreg (FR4)     | 200 µm     | ----------------
         | Copper (1 oz)     | 35 µm      | Solid GND plane
L3       | Core (FR4)        | 1140 µm    | ----------------
         | Copper (1 oz)     | 35 µm      | 3V3 power plane
L4       | Prepreg (FR4)     | 200 µm     | ----------------
Bottom   | Copper (1 oz)     | 35 µm      | Signal (USB, debug)
---------|-------------------|------------|------------------
Total                       | ~1.6 mm
```

**Impedance calculations (FR4, Er=4.2):**

- **50 Ω single-ended (Top to L2):** H=0.2mm, W=0.38mm, Er_eff≈3.2 → Z₀≈50.2 Ω ✓
- **90 Ω differential (Top to L2):** H=0.2mm, W=0.22mm, S=0.15mm, Er_eff≈3.1 → Z_diff≈89.6 Ω ✓

## 3. Board Outline

```
    0,0                                             85,0
     ┌──────────────────────────────────────────────┐
     │  USB-C                                      ││
     │   ┌──┐                                      ││
     │   │J1│  ┌─────┐                  ┌──┐ ┌──┐ ││
     │   └──┘  │ U4  │  ┌───────────┐  │J2│ │J3│ ││
     │         │LDO  │  │   U1      │  │  │ │  │ ││
     │         └─────┘  │  STM32    │  └──┘ └──┘ ││
     │                  │  F401     │    ▲    ▲   │
     │   ┌──┐ ┌──┐     └───────────┘    │    │   │
     │   │SW1│ │SW2│  ┌──────────┐      │    │   │
     │   └──┘ └──┘     │  U2      │◄────┘    │   │
     │   LED1 LED2     │  CC1101  │          │   │
     │                  └──────────┘          │   │
     │                  ┌──────────┐          │   │
     │   ┌──────┐      │  U3      │    ┌─────┘   │
     │   │ SSD  │      │ nRF24L01+│◄───┘         │
     │   │1306  │      └──────────┘              │
     │   └──────┘  ┌─┐ ┌─┐ ┌─┐ ┌─┐             │
     │             │Y1│ │Y2│ │C │ │C │             │
     │             └─┘ └─┘ └─┘ └─┘             │
     └──────────────────────────────────────────────┘
    0,54                                           85,54

    J2 = SMA (Sub-GHz antenna, top edge, right side)
    J3 = SMA (2.4GHz antenna, top edge, far right)
    J1 = USB-C (left edge, centered vertically)
    J4 = SWD debug header (bottom edge)
```

## 4. Component Placement — Thermal Zone Map

```
┌─────────────────────────────────────────────────────────────┐
│                    THERMAL ZONES                              │
│                                                               │
│  ┌──────────────────────────────────────────────────────────┐│
│  │  ZONE A: USB & Power (low heat)                          ││
│  │  J1 (USB-C), U4 (AMS1117), C13-C15, FB1, U6 (ESD)       ││
│  └──────────────────────────────────────────────────────────┘│
│                                                               │
│  ┌──────────────────────────────────────────────────────────┐│
│  │  ZONE B: MCU (moderate heat, ground pour)                ││
│  │  U1 (STM32F401), Y1, C1-C4, C11-C12, R8-R9             ││
│  │  Thermal pad → GND pour on L2                            ││
│  └──────────────────────────────────────────────────────────┘│
│                                                               │
│  ┌──────────────────────────────────────────────────────────┐│
│  │  ZONE C: Sub-GHz RF (moderate heat, RF isolation)        ││
│  │  U2 (CC1101), Y2, C5-C6, C9, L1-L2, R10, R1            ││
│  │  Thermal pad → GND pour on L2                            ││
│  │  RF trace → J2 (50Ω controlled impedance)                ││
│  └──────────────────────────────────────────────────────────┘│
│                                                               │
│  ┌──────────────────────────────────────────────────────────┐│
│  │  ZONE D: 2.4 GHz RF (low heat, RF isolation)             ││
│  │  U3 (nRF24L01+), C7-C8, L3                              ││
│  │  RF trace → J3 (50Ω controlled impedance)                ││
│  └──────────────────────────────────────────────────────────┘│
│                                                               │
│  ┌──────────────────────────────────────────────────────────┐│
│  │  ZONE E: User Interface (no heat)                        ││
│  │  U5 (SSD1306), SW1, SW2, D1 (LED), D2 (LED), R6-R7     ││
│  └──────────────────────────────────────────────────────────┘│
│                                                               │
│  NOTE: Zone C and Zone D must be physically separated by    │
│  at least 10mm or a grounded guard trace to prevent          │
│  Sub-GHz / 2.4 GHz intermodulation.                          │
└─────────────────────────────────────────────────────────────┘
```

## 5. Routing Rules

### 5.1 High-Speed Signals

| Signal | Max Length | Matched Length | Via Count | Notes |
|---|---|---|---|---|
| USB_DM | < 50 mm | Match to DP ±0.5 mm | ≤ 2 | 90 Ω diff pair |
| USB_DP | < 50 mm | Match to DM ±0.5 mm | ≤ 2 | 90 Ω diff pair |
| CC1101_RF | < 20 mm | N/A | 0 | 50 Ω coplanar, no vias |
| NRF24_ANT | < 15 mm | N/A | 0 | 50 Ω coplanar, no vias |
| SPI1_SCK | < 30 mm | N/A | ≤ 1 | Route away from RF |
| SPI1_MOSI | < 30 mm | N/A | ≤ 1 | Route away from RF |
| SPI1_MISO | < 30 mm | N/A | ≤ 1 | Route away from RF |
| SPI2_SCK | < 30 mm | N/A | ≤ 1 | Route away from RF |
| I2C1_SCL | < 30 mm | N/A | ≤ 1 | Route away from RF |
| I2C1_SDA | < 30 mm | N/A | ≤ 1 | Route away from RF |

### 5.2 RF Routing Rules

1. **CC1101 RF output** (pin 11 PA_PD) to SMA connector J2:
   - Route on top layer only, no vias
   - 50 Ω coplanar waveguide with grounded adjacent ground pour
   - Minimum 0.5 mm clearance from other signals
   - Keep trace as short as possible (< 20 mm ideal)
   - Place matching network (L1, L2) as close to CC1101 pin as possible

2. **nRF24L01+ ANT output** (pin 15 ANT1) to SMA connector J3:
   - Route on top layer only, no vias
   - 50 Ω coplanar waveguide
   - Keep trace < 15 mm
   - Place L3 (2.7 nH) as close to pin as possible

3. **Guard traces:** Place grounded guard vias (stitching) around RF sections at 2 mm intervals to create a Faraday cage effect between Zone C and Zone D

### 5.3 Crystal Routing

1. **8 MHz XTAL (Y1):** Route to STM32 PH0/PH1 (pins 5-6), load caps C16/C17 adjacent, guard ring of GND vias
2. **26 MHz XTAL (Y2):** Route to CC1101 XOSC_Q1/Q2 (pins 8-9), load caps C18/C19 adjacent, guard ring of GND vias
3. Both crystals: keep traces short (< 5 mm), no other signals crossing, solid GND pour underneath

### 5.4 Power Routing

1. **3V3 power plane:** L3 (inner layer 3) is a solid 3.3V plane with splits only where needed for thermal isolation
2. **VBATT (5V):** Route as a thick trace (0.5 mm) from J1 to U4 input, with C13 (47µF) near J1
3. **Ground plane:** L2 (inner layer 2) is a solid unbroken ground plane — no splits, no routing on this layer except GND vias
4. **Via stitching:** Place GND stitching vias at 2 mm intervals around board perimeter and around each RF zone boundary

## 6. Via Strategy

| Via Type | Drill | Pad | Usage |
|---|---|---|---|
| Standard | 0.3 mm | 0.6 mm | Signal routing, component pads |
| Power | 0.5 mm | 1.0 mm | Power connections (VBATT, 3V3) |
| Thermal | 0.3 mm | 0.6 mm | Thermal pads on U1, U2 (4-9 vias in pad) |
| Stitching | 0.3 mm | 0.6 mm | GND via stitching (perimeter, zone boundaries) |

**Via-in-pad rules:**
- STM32F401 UFQFPN-48 thermal pad: 9 vias (3×3 grid) in exposed pad, connected to L2 GND
- CC1101 VQFN-20 thermal pad: 4 vias (2×2 grid) in exposed pad, connected to L2 GND
- nRF24L01+ QFN-20: No thermal pad (not required for this package variant)

## 7. Thermal Management

| Component | Max Power Dissipation | Thermal Solution |
|---|---|---|
| STM32F401 | ~200 mW | Thermal pad → 9 vias → L2 GND plane |
| CC1101 (TX) | ~100 mW | Thermal pad → 4 vias → L2 GND plane |
| AMS1117-3.3 | ~350 mW (150 mA × 1.7V dropout) | SOT-223 tab → top copper pour → GND |
| nRF24L01+ | ~30 mW | No special thermal needed |

**Ambient operating range:** −20 °C to +70 °C  
**Max junction temp:** STM32F401 = 105 °C, CC1101 = 125 °C  
**PCB acts as primary heatsink** — no external heatsinks required at these power levels

## 8. Design for Manufacturing (DFM) Notes

1. **Minimum feature size:** 0402 passives, 0.5 mm pitch ICs — well within standard fab capability
2. **No BGA components** — all QFN/QFP/TSOP packages, stencil-friendly
3. **Stencil aperture:** 1:1 for 0402 pads, 0.5 mm round for QFN thermal pads
4. **Solder paste:** Type 4 (25-45 µm particle) for 0402 and 0.5 mm pitch
5. **Reflow profile:** Standard SnAgCu (SAC305) lead-free profile
6. **Test points:** Provide 1 mm test pads for: 3V3, GND, SWDIO, SWCLK, NRST, BOOT0, SPI1 bus, SPI2 bus
7. **Panelization:** V-score panel, 5 mm rails, fiducials on each panel corner
8. **No buried vias** — through-vias only, keeps cost down
9. **All components on top side** — single-sided assembly reduces cost
10. **OLED module:** Uses pre-assembled SSD1306 4-pin module (I2C), soldered as a daughterboard with 4 through-hole pads or a 4-pin 1.27mm header

## 9. Board Outline Dimensions (mm)

```
Width:   85.000
Height:  54.000
Corner radius: 1.500 (rounded corners for pocket carry)

Mounting holes: 4× M2.5 (Ø2.7 drill, Ø5.5 pad)
  - (5.0, 5.0)
  - (80.0, 5.0)
  - (5.0, 49.0)
  - (80.0, 49.0)

USB-C cutout: Left edge, centered at Y=27.0, width=9.0, depth=5.5
SMA connectors: Top edge, J2 at X=55.0, J3 at X=70.0
SWD header: Bottom edge, centered at X=42.5
```