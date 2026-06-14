# Phase 3 — PCB Blueprints & Layout: CAN Bus Infiltrator

## 1. PCB Stackup

| Layer | Material | Thickness | Copper | Purpose |
|---|---|---|---|---|
| Top (L1) | 1 oz copper | 35 µm | 1 oz | Signal routing, component placement |
| L2 | FR-4 core | 0.2 mm | 1 oz | Ground plane (unsplit) |
| L3 | FR-4 core | 0.2 mm | 1 oz | Power plane (3.3 V, 5 V split) |
| Bottom (L4) | 1 oz copper | 35 µm | 1 oz | Signal routing (short runs), ground pour |

**Total thickness**: 1.6 mm (standard FR-4 4-layer)

### Prepreg Details
- L1–L2: 0.2 mm FR-4 core (high Tg, 170 °C)
- L2–L3: 1.0 mm FR-4 core (structural)
- L3–L4: 0.2 mm FR-4 core (high Tg)

### Impedance Calculations (using JLCPCB 4-layer stackup)
- **USB diff pair** (L1 over L2 ground): 90 Ω ±10% — 0.11 mm trace, 0.15 mm spacing
- **CAN diff pairs** (L1 over L2 ground): 120 Ω ±10% — 0.20 mm trace, 0.12 mm spacing
- **50 Ω single-ended** (L1 over L2): 0.18 mm trace width

## 2. Board Outline

```
    ┌─────────────────────────────────────────────────────────┐
    │  OBD-II Connector (16-pin right-angle, edge-mount)     │
    │  ┌─┐┌─┐┌─┐┌─┐┌─┐┌─┐┌─┐┌─┐┌─┐┌─┐┌─┐┌─┐┌─┐┌─┐┌─┐┌─┐ │
    └──┘  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │   └──┘
         │                                                          │
    65mm │   ┌──────┐  ┌──────┐    ┌───────────┐   ┌──────────┐  │
         │   │TJA1  │  │ISO1  │    │STM32F407  │   │nRF52840 │  │
         │   └──────┘  └──────┘    │  LQFP100  │   │ QFN73   │  │
         │   ┌──────┐  ┌──────┐    └───────────┘   └──────────┘  │
         │   │TJA2  │  │ISO2  │                                    │
         │   └──────┘  └──────┘    ┌────────┐                     │
         │                         │MP2307  │ ┌────┐              │
         │   ┌──────┐              │Buck    │ │LDO │              │
         │   │EEPROM│              └────────┘ └────┘              │
         │   └──────┘                                           35mm│
         │                                                         │
         │   [LED1][LED2][LED3][LED4]                              │
         │                                                         │
         │   ┌──────────┐                    ┌───────────┐        │
         │   │ microSD  │                    │  USB-C    │        │
         │   └──────────┘                    └───────────┘        │
         └─────────────────────────────────────────────────────────┘
```

**Dimensions**: 65.0 mm × 35.0 mm × 1.6 mm  
**Mounting**: 2× M2.5 mounting holes (corners opposite OBD-II connector)  
**Edge connectors**: OBD-II (bottom edge), USB-C (right edge), microSD (right edge)

## 3. Component Placement Strategy

### 3.1 Top Side Placement (primary component side)

| Zone | Components | Rationale |
|---|---|---|
| Zone A (left) | TJA1050-CH1, ISO1050-CH1, TVS array | CAN bus isolation, close to OBD-II connector |
| Zone B (left-lower) | TJA1050-CH2, ISO1050-CH2, TVS array | Second CAN channel, close to OBD-II |
| Zone C (center) | STM32F407VGT6 | Central placement, short equal-length traces |
| Zone D (upper-right) | nRF52840, 32 MHz xtal, antenna matching | RF zone, keep away from switching supplies |
| Zone E (lower-left) | MP2307 buck, LDO, inductor | Power supply zone, away from sensitive analog |
| Zone F (lower-center) | AT24C02 EEPROM, I²C pull-ups | Near STM32 I2C1 pins |
| Zone G (bottom edge) | microSD slot, USB-C connector | Edge-mounted, user-accessible |
| Zone H (top edge) | 4× WS2812B-Mini LEDs | Status indicators, visible through case |

### 3.2 Bottom Side (minimal components)
- Crystal Y1 (8 MHz) — placed under STM32 if space-constrained
- Decoupling capacitors for U1 (beneath IC)
- Test points (4× 1 mm pads): SWDIO, SWCLK, nRST, GND

### 3.3 Keep-Out Zones
- **Antenna keep-out**: 5 mm radius around nRF52840 antenna pin, no copper on any layer
- **Crystal keep-out**: No high-speed traces within 3 mm of Y1, Y2, Y3
- **Buck inductor keep-out**: No sensitive traces within 5 mm of L1

## 4. High-Speed Routing Rules

### 4.1 CAN Bus (TJA1050 ↔ OBD-II)

| Rule | Value |
|---|---|
| Differential impedance | 120 Ω ±10% |
| Trace width | 0.20 mm |
| Trace spacing (intra-pair) | 0.12 mm |
| Spacing to other nets | ≥ 0.5 mm |
| Max skew within pair | ≤ 5 mm |
| Length matching | CAN_H and CAN_L matched to ±0.5 mm |
| Via count | 0 (route entirely on L1) |
| Series termination | None (120 Ω at each bus end per CAN spec) |
| ESD | SRV05-4 TVS at OBD-II connector pins |

### 4.2 USB 2.0 Full-Speed (STM32 ↔ USB-C)

| Rule | Value |
|---|---|
| Differential impedance | 90 Ω ±10% |
| Trace width | 0.11 mm |
| Trace spacing | 0.15 mm |
| Max trace length | ≤ 80 mm |
| Max skew within pair | ≤ 0.5 mm |
| Via count | ≤ 2 (pair transitions together) |
| ESD protection | PRTR5V0U2X at connector |
| Series resistors | 27.4 Ω ±1% on D+ and D− |

### 4.3 SPI Bus (STM32 ↔ nRF52840)

| Rule | Value |
|---|---|
| Max clock | 8 MHz |
| Trace length | ≤ 25 mm |
| Single-ended impedance | 50 Ω |
| Ground guard | L2 ground plane beneath |
| No vias preferred | Route on L1 if possible |

## 5. Via Strategy

| Via Type | Drill | Pad | Use |
|---|---|---|---|
| Standard via | 0.3 mm | 0.6 mm | General signal routing, power |
| Micro via | 0.1 mm | 0.3 mm | Not used (4-layer, no HDI needed) |
| Thermal via | 0.3 mm | 0.6 mm | Under MP2307DN and LDO pads |

### Via Placement Rules
- **Decoupling caps**: Via to ground plane within 0.5 mm of cap ground pad
- **STM32 power pins**: 1–2 thermal vias per VDD/VSS pin
- **Buck converter**: 4× thermal vias under MP2307DN exposed pad
- **Ground stitching**: Via fence every 2 mm along board edges for EMI containment
- **Isolation barrier**: Via fence (0.5 mm pitch) along isolation boundary between GND and GND_ISO planes

## 6. Thermal Management

| Component | Power Dissipation | Cooling Strategy |
|---|---|---|
| STM32F407 | ~200 mW (worst case) | Thermal vias under LQFP pad, ground plane heat spread |
| MP2307DN | ~150 mW (efficiency loss) | Exposed pad → 4× thermal vias → L2 ground → board edge |
| TJA1050 (×2) | ~120 mW each | SOIC-8, convection cooling sufficient |
| nRF52840 | ~40 mW (TX) | QFN exposed pad → thermal vias → ground plane |
| ISO1050 (×2) | ~100 mW each | DIP-8 equivalent, minimal heating |

### Thermal Via Arrays
- Under MP2307DN: 4× 0.3 mm vias on 1 mm pitch
- Under STM32 exposed pad: 9× 0.3 mm vias on 1 mm pitch (3×3 grid)
- Under nRF52840 exposed pad: 4× 0.3 mm vias on 0.8 mm pitch

## 7. Isolation Barriers

### 7.1 Galvanic Isolation (CAN Bus)

The ISO1050 digital isolators create a 2.5 kV isolation barrier between the vehicle CAN bus domain and the MCU domain.

```
Vehicle Side (ISO DOMAIN)          MCU Side (DIGITAL DOMAIN)
┌─────────────────────┐            ┌─────────────────────────┐
│ OBD-II Connector    │            │ STM32F407              │
│   ↓                  │            │                         │
│ TVS (SRV05-4)       │            │                         │
│   ↓                  │            │                         │
│ TJA1050 Transceiver │            │                         │
│   ↓                  │            │                         │
│ ISO1050 (VCC2 side) │──2.5kV───│ ISO1050 (VCC1 side)    │
│                      │  barrier  │   ↓                    │
│ GND_ISO plane        │            │ GND plane              │
│ VCC_5V_ISO           │            │ VCC_3V3                │
└─────────────────────┘            └─────────────────────────┘
```

### 7.2 PCB Isolation Implementation

- **Slot cut**: 1 mm wide slot in PCB between ISO1050 pin 4 (GND1) and pin 5 (GND2) — no copper on any layer in this slot
- **Creepage**: Minimum 4 mm creepage distance between isolated domains on L1
- **Clearance**: Minimum 0.5 mm air gap between domains
- **Via fence**: 0.5 mm pitch ground stitching vias along each side of isolation slot

### 7.3 Power Domain Separation

| Domain | Source | Ground Net | Notes |
|---|---|---|---|
| Digital | RT9193 LDO (3.3 V) | GND | MCU, nRF, SD card, USB |
| Isolated CAN | MP2307 (5 V) → LDO (3.3 V ISO) | GND_ISO | ISO1050 side 1, TJA1050 |
| USB | VBUS (5 V) via Schottky OR | GND | USB-C connector |
| OBD Power | VBAT (12 V) → MP2307 (5 V) | GND | Vehicle battery |

## 8. Ground Plane Strategy

### L2 (Ground Plane)
- **Primary ground**: Solid copper fill on L2
- **Slot**: 1 mm isolation slot across L2 between GND and GND_ISO domains
- **Stitching**: Via fence every 2 mm along slot edges

### L3 (Power Plane)
- Split plane: 3.3 V fills 70% of area, 5.0 V fills 30%
- 2 mm keep-out between voltage domains
- Both domains tied to L2 ground through decoupling caps

### L4 (Bottom Ground Pour)
- Ground pour with 0.3 mm clearance from signal traces
- Stitched to L2 ground every 3 mm

## 9. Design for Manufacturing

| Parameter | Value |
|---|---|
| Minimum trace width | 0.1 mm (4 mil) |
| Minimum trace spacing | 0.1 mm (4 mil) |
| Minimum via drill | 0.3 mm |
| Minimum via pad | 0.6 mm |
| Minimum annular ring | 0.15 mm |
| Solder mask | Green (standard) |
| Silkscreen | White |
| Surface finish | ENIG (for fine-pitch QFNs) |
| Board thickness | 1.6 mm |
| Copper weight | 1 oz (35 µm) all layers |
| Panel size | N/A (single board) |
| Electrical testing | 100% netlist connectivity |

## 10. Design Verification Checklist

- [ ] All differential pairs length-matched within tolerance
- [ ] All decoupling caps within 5 mm of IC power pins
- [ ] No high-speed traces crossing isolation slot
- [ ] Antenna keep-out zone clear on all layers
- [ ] Ground plane uninterrupted under STM32
- [ ] All thermal via arrays in place
- [ ] OBD-II pin assignments verified against SAE J1962
- [ ] USB-C pinout verified against USB 2.0 spec
- [ ] All net names consistent between schematic and layout
- [ ] 120 Ω termination resistors placed near OBD-II connector
- [ ] Ferrite beads on all power domain crossings