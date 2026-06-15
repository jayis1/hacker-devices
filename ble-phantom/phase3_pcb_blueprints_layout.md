# BLE Phantom — Phase 3: PCB Blueprints & Layout

## 1. PCB Specifications

| Parameter | Value |
|-----------|-------|
| Board dimensions | 80 mm × 40 mm |
| Board outline | USB-A dongle with right-angle USB-A plug extending 12 mm |
| Layer count | 4 |
| Material | FR-4, TG 170°C |
| Copper weight | 1 oz (35 µm) outer, 0.5 oz (17.5 µm) inner |
| Minimum trace width | 0.15 mm (6 mil) |
| Minimum via diameter | 0.3 mm drill / 0.6 mm pad |
| Minimum clearance | 0.15 mm (6 mil) |
| Surface finish | ENIG (Au over Ni over Cu) |
| Solder mask | Green (both sides) |
| Silkscreen | White (both sides) |
| Board thickness | 1.6 mm |

## 2. PCB Stackup

| Layer | Name | Material | Thickness | Copper | Notes |
|-------|------|----------|-----------|--------|-------|
| Top | L1 - Signal | FR-4 | 0.035 mm | 1 oz | Component side, RF, high-speed |
| Core 1 | — | FR-4 | 0.20 mm | — | Thin core for L1-L2 coupling |
| Inner 1 | L2 - GND | FR-4 | 0.0175 mm | 0.5 oz | Solid ground plane |
| Prepreg | — | FR-4 | 0.50 mm | — | Standard prepreg |
| Inner 2 | L3 - Power | FR-4 | 0.0175 mm | 0.5 oz | 3V3, VBUS split plane |
| Core 2 | — | FR-4 | 0.20 mm | — | Thin core for L3-L4 coupling |
| Bottom | L4 - Signal | FR-4 | 0.035 mm | 1 oz | Bottom side routes, USB pads |

**Total board thickness**: ~1.2 mm (within USB spec tolerance)

### Impedance Calculations

| Signal | Target Z | Stackup | Width | Gap | Calc Z |
|--------|----------|---------|-------|-----|--------|
| USB D+/D− diff | 90 Ω diff | L1 over L2 | 0.18 mm | 0.15 mm | 89.6 Ω |
| RF 50 Ω | 50 Ω SE | L1 over L2 | 0.38 mm | — | 49.8 Ω |
| SPI SE | 50 Ω SE | L1 over L2 | 0.15 mm | — | ~54 Ω (acceptable) |

## 3. Board Outline

```
        USB-A Plug (12mm)
    ┌───────┐────────────────────────────────────┐
    │       │                                    │
    │ D+ D− │   Main Board Area (68mm × 40mm)   │
    │ GND   │                                    │
    │ VBUS  │                                    │
    └───────┘────────────────────────────────────┘
    
    Bottom view (component side = top):
    
    ┌──────────────────────────────────────────────────┐
    │  USB Plug  │  [ANT1]  [nRF A]  [STM32]  [nRF B]  │  80mm
    │            │                    [ANT2]             │
    │   [MCP]    │  [LDO]  [LEDs]  [SW]  [Battery]     │
    └──────────────────────────────────────────────────┘
                          40mm
```

### Mechanical Constraints
- USB-A plug: 12 mm wide, 15 mm tall (including shield), extending 12 mm from board edge
- USB-A pads: 4 signal + 2 shield tabs on bottom edge of board
- Max component height: 5 mm (for USB port clearance)
- No components within 3 mm of USB plug area

## 4. Component Placement

### 4.1 Top Side Placement

```
    USB Plug
        │
   ┌────┴─────────────────────────────────────────┐
   │                                              │
   │  [U6 ESD]    [R7 R8]                         │
   │                                              │
   │  [Y2 8MHz]  [C20 C21]                       │
   │                                              │
   │  ┌────────┐              ┌──────────┐        │
   │  │nRF A   │   ┌────────┐ │ nRF B    │        │
   │  │U2      │   │STM32   │ │ U3       │        │
   │  │QFN-48  │   │U1      │ │ QFN-48   │        │
   │  └────────┘   │QFN-48  │ └──────────┘        │
   │               └────────┘                      │
   │  [ANT1]                     [ANT2]           │
   │  [Y3 32k] [L1 L2 C26]      [Y4 32k] [L3]    │
   │                                              │
   │  [D1][D2][D3][D4]  [SW1]                    │
   │                                              │
   │  [U4 MCP]  [U5 LDO]  [C1 C2 C3]            │
   │                                              │
   └──────────────────────────────────────────────┘
```

### 4.2 Bottom Side Placement

- USB-A connector pads (edge-mounted)
- Battery connector J2
- Decoupling capacitors (where needed for short paths)
- Test points (SWD debug, UART)

### 4.3 Placement Rules

1. **STM32 (U1)**: Center of board, oriented with USB pins toward USB plug
2. **nRF A (U2)**: Left side, ANT1 at board edge for optimal radiation
3. **nRF B (U3)**: Right side, ANT2 at board edge for optimal radiation
4. **Crystal Y2**: As close to STM32 PH0/PH1 as possible (< 5 mm)
5. **Antenna matching networks**: Directly adjacent to ANT1/ANT2 pins
6. **ESD (U6)**: Within 5 mm of USB connector pads
7. **LDO (U5)**: Near VBUS input, output caps adjacent
8. **Charger (U4)**: Near battery connector

## 5. High-Speed Routing

### 5.1 USB 2.0 Full-Speed (12 Mbit/s)

| Rule | Value |
|------|-------|
| Differential impedance | 90 Ω ±10% |
| Trace width | 0.18 mm |
| Trace spacing | 0.15 mm |
| Max length mismatch | 0.5 mm |
| Max via count | 2 per signal |
| ESD placement | < 5 mm from connector |
| Series resistors | 22 Ω ±1% at STM32 end |
| Length | Minimize, < 50 mm total |

**Routing**: USB D+ and D− routed as differential pair on L1, referenced to L2 ground plane. No crossing of split planes. ESD protection placed at connector entry.

### 5.2 SPI Traces (8 MHz)

| Rule | Value |
|------|-------|
| Trace width | 0.15 mm |
| Max length | 40 mm (STM32 to nRF) |
| Spacing to other signals | 0.3 mm minimum |
| Length matching | Not required at 8 MHz |

**Routing**: SPI0 (Radio A) routed on L1, SPI1 (Radio B) routed on L4 (bottom). This minimizes crosstalk between the two SPI buses.

### 5.3 RF Traces (2.4 GHz)

| Rule | Value |
|------|-------|
| Impedance | 50 Ω ±5% |
| Trace width | 0.38 mm (coplanar waveguide) |
| Gap to ground pour | 0.25 mm |
| Max length | 10 mm |
| Via stitching | Along RF trace edge |

**Routing**: Each nRF52832 ANT1 pin connects through a pi-network matching (L1-C26-L2 for Radio A) directly to the chip antenna. The RF trace is a coplanar waveguide with via stitching to the ground plane on L2.

### 5.4 Antenna Layout

Each 2.4 GHz chip antenna (Johanson 2450AT18A100E) requires:
- **Keep-out area**: 5 mm radius around antenna element, no copper on any layer
- **Ground plane cutout**: 3 mm × 2 mm notch at antenna feedpoint
- **Via fence**: Row of ground vias along the ground plane edge nearest the antenna
- **Feed trace**: 50 Ω coplanar waveguide, < 5 mm from matching network to antenna pad

```
              Keep-out zone
         ┌─────────────────────┐
         │     No Copper       │
         │                     │
    ─────┤   [Antenna Pad]     │
    50Ω  │                     │
    ─────┘                     │
         └─────────────────────┘
    Via fence: ● ● ● ● ● ● ● ●
```

## 6. Via Strategy

### 6.1 Via Types

| Via Type | Drill | Pad | Use |
|----------|-------|-----|-----|
| Standard | 0.3 mm | 0.6 mm | Signal routing, thermal |
| Thermal | 0.3 mm | 0.8 mm | Power delivery to inner planes |
| RF Stitch | 0.2 mm | 0.5 mm | Ground stitching near RF traces |
| USB | 0.3 mm | 0.6 mm | USB differential pair transitions |

### 6.2 Via Placement Rules

1. **Ground vias**: Place within 1 mm of every IC ground pin
2. **Decoupling vias**: Place within 0.5 mm of decoupling cap ground pad
3. **RF stitching**: Via fence along RF trace, spacing ≤ 0.5 mm (λ/20 at 2.4 GHz)
4. **Thermal vias**: 4× under each QFN thermal pad (0.8 mm pitch)
5. **No vias**: In RF trace paths, under antenna keep-out zones, or in USB differential pair

## 7. Thermal Management

### 7.1 Thermal Sources

| Component | Power Dissipation | Thermal Pad | Junction Temp Limit |
|-----------|------------------|-------------|-------------------|
| STM32F401 | ~165 mW (84 MHz) | QFN exposed pad | 105°C |
| nRF52832 A | ~60 mW (TX at +4 dBm) | QFN exposed pad | 90°C |
| nRF52832 B | ~60 mW (TX at +4 dBm) | QFN exposed pad | 90°C |
| LDO (3V3) | ~300 mW worst case | SOT-223 tab | 125°C |
| MCP73831 | ~250 mW (charging) | SOT-23-5 | 125°C |

### 7.2 Thermal Strategy

1. **QFN thermal pads**: Each IC has exposed thermal pad connected to L2 ground plane via 4× thermal vias (0.3 mm drill, 0.8 mm pad)
2. **LDO heat sinking**: SOT-223 tab connected to large copper pour on L1 with 6× thermal vias to L2
3. **Copper fills**: Top and bottom signal layers have ground copper pours (0.3 mm clearance from signals)
4. **Air flow**: USB-A plug orientation ensures device hangs with both sides exposed
5. **Operating range**: −20°C to +60°C ambient — at 600 mW total dissipation, junction temp stays below limits with forced convection

## 8. Isolation Barriers

### 8.1 RF Isolation (Radio A ↔ Radio B)

The two nRF52832 radios operate simultaneously and must not interfere:

| Technique | Implementation |
|-----------|---------------|
| Physical separation | Minimum 20 mm between antenna keep-out zones |
| Ground fence | Row of ground vias (0.2 mm drill) on 1 mm pitch between the two radio sections |
| Power supply isolation | Separate decoupling banks; ferrite bead on each nRF VDD (optional for BOM savings, omitted) |
| Channel management | Firmware ensures Radio A and Radio B never TX on same channel simultaneously (orthogonal hop) |
| L4 ground pour | Bottom side continuous ground copper between antenna areas |

### 8.2 Digital/RF Isolation

| Technique | Implementation |
|-----------|---------------|
| Ground plane separation | SPI traces on L1/L4, RF on L1, with L2 continuous ground between |
| Analog power | Separate VDDA filter for STM32 (LC filter: 10 µH + 100 nF) |
| Crystal guard rings | Ground via ring around each crystal (Y2, Y3, Y4) |
| ESD placement | ESD diode at connector before any other component |

### 8.3 USB Isolation

| Technique | Implementation |
|-----------|---------------|
| ESD diode | PRTR5V0U2X at USB connector |
| Series resistors | 22 Ω on D+/D− at STM32 end |
| Ground stitching | Dense via stitching between USB ground and L2 ground plane |

## 9. Design for Manufacturing

### 9.1 DFM Checklist

- [x] All components ≥ 0402 (no 0201)
- [x] All QFNs have thermal pad with defined paste mask opening (50% coverage)
- [x] Minimum trace width 6 mil (0.15 mm)
- [x] Minimum via drill 0.3 mm
- [x] No buried or blind vias (all through-hole vias)
- [x] Board outline has 0.5 mm rounded corners (except USB plug area)
- [x] Solder mask defined pads for QFNs
- [x] Test points on all critical nets (SWD, UART, VBUS, 3V3)
- [x] Fiducials: 3× on top, 2× on bottom (1.5 mm dia, 0.5 mm opening)

### 9.2 Assembly Notes

- **Side 1 (Top)**: All ICs, crystals, antennas, most passives
- **Side 2 (Bottom)**: USB-A plug (through-hole), battery connector, test points, select decoupling
- **Reflow profile**: Standard lead-free SAC305 profile (peak 245°C, 60s above 217°C)
- **Stencil thickness**: 0.1 mm (4 mil) for 0402 paste release
- **QFN paste mask**: 50% coverage with 4 diagonal rectangular openings per thermal pad

## 10. Board Outline Drawing (ASCII)

```
    USB-A Plug Region (12mm × 40mm)
    ┌──────┐
    │ ║ ║ ║│  ← USB contacts (D+, D−, VBUS, GND)
    │ ║ ║ ║│
    └──────┘────────────────────────────────────────┐
                                                    │
    ┌───────────────────────────────────────────────┘
    │                                              │
    │  TP1 TP2 TP3 ───── [U6] ──── [R7 R8]        │
    │                                              │
    │  [Y2] ── [C20 C21]                          │
    │                                              │
    │  ┌────────┐   ┌─────────┐   ┌────────┐      │
    │  │ nRF A  │   │ STM32   │   │ nRF B  │      │
    │  │ U2     │   │ U1      │   │ U3     │      │
    │  │ QFN48  │   │ QFN48   │   │ QFN48  │      │
    │  └────────┘   └─────────┘   └────────┘      │
    │                                              │
    │  [ANT1]        [Y3] [Y4]           [ANT2]   │
    │                                              │
    │  [D1 D2 D3 D4]    [SW1]                    │
    │                                              │
    │  [U4 MCP]  [U5 LDO]                        │
    │                           [J2 Battery]       │
    │                                              │
    └──────────────────────────────────────────────┘
         80mm total × 40mm

    Test Points (bottom):
    TP1: 3V3    TP2: GND    TP3: SWDIO    TP4: SWCLK
    TP5: TX     TP6: RX     TP7: VBUS
```

## 11. Copper Pour Strategy

### L1 (Top) — Signal + Component Side
- Ground pour with 0.3 mm clearance from all signals
- RF traces excluded from ground pour (0.5 mm clearance)
- Antenna keep-out zones: no copper at all

### L2 (Inner 1) — Ground Plane
- Solid ground plane, no splits
- Via stitching every 3 mm along board edges
- No signal routing on this layer (except short bridge segments if absolutely necessary)

### L3 (Inner 2) — Power Plane
- Split plane: 3V3 (80%) and VBUS (20%)
- 3V3 section powers all ICs
- VBUS section for USB and charger
- Split boundary runs along board center line

### L4 (Bottom) — Signal + Component Side
- Ground pour with 0.3 mm clearance
- SPI1 traces (Radio B) routed here
- Test points and USB connector pads
- Via stitching along board edges (every 3 mm)