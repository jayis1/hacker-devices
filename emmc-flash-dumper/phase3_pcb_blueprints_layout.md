# Phase 3: PCB Blueprints & Layout — eMMC Flash Dumper

## 3.0 Board Overview

### 3.0.1 Physical Specifications

| Parameter | Value |
|-----------|-------|
| Board Outline | 85.0 mm × 54.0 mm (credit card form factor) |
| Board Thickness | 1.6 mm |
| Layer Count | 4 layers |
| Copper Weight | 1 oz (35 µm) outer, 0.5 oz (18 µm) inner |
| Min Trace/Space | 0.10 mm / 0.10 mm (4 mil / 4 mil) |
| Min Drill | 0.20 mm (8 mil) |
| Min Annular Ring | 0.10 mm (4 mil) |
| Surface Finish | ENIG (Electroless Nickel Immersion Gold) |
| Solder Mask | Green LPI, both sides |
| Silkscreen | White, top side only (component legends) |
| IPC Class | IPC-A-600 Class 2 |

### 3.0.2 Mechanical Constraints

| Feature | Position | Notes |
|---------|----------|-------|
| USB-C Connector | Top edge, centered | J1, vertical mount |
| microSD Slot | Right edge, top half | J2, push-push |
| ISP Header | Left edge, bottom half | J3, 20-pin 0.1" THT |
| eMMC Header | Bottom edge, centered | J4, 20-pin 0.05" SMT |
| OLED Display | Top face, center-right | U10, 0.96" module |
| Buttons | Right edge, vertical stack | SW1-SW4, 6mm tactile |
| Battery Connector | Bottom-left | J5, 2-pin JST-PH |
| SWD Header | Top-left | 5-pin 0.05" SMT (Tag-Connect) |
| Mounting Holes | 4× M2.5, corners | 3.0mm hole, 6.0mm pad |

## 3.1 Layer Stackup

```
┌─────────────────────────────────────────────────────────────┐
│ LAYER 1: TOP (Signal + Components)                           │
│   Copper: 1 oz (35 µm)                                      │
│   Purpose: Critical high-speed signals, component placement │
│   Impedance: 50Ω SE, 100Ω diff (edge-coupled)               │
├─────────────────────────────────────────────────────────────┤
│   Prepreg: 2× 1080 (0.15 mm)                                │
├─────────────────────────────────────────────────────────────┤
│ LAYER 2: GND (Solid Ground Plane)                           │
│   Copper: 0.5 oz (18 µm)                                    │
│   Purpose: Uninterrupted return path for all signals        │
│   No splits, no slots — single contiguous plane             │
├─────────────────────────────────────────────────────────────┤
│   Core: FR-4, 0.71 mm (28 mil)                              │
├─────────────────────────────────────────────────────────────┤
│ LAYER 3: PWR (Power Plane, segmented)                       │
│   Copper: 0.5 oz (18 µm)                                    │
│   Purpose: Power distribution with isolated rail segments   │
│   Segments: 3.3V, 1.35V, 1.8V, 1.2V, 1.0V                  │
├─────────────────────────────────────────────────────────────┤
│   Prepreg: 2× 1080 (0.15 mm)                                │
├─────────────────────────────────────────────────────────────┤
│ LAYER 4: BOTTOM (Signal + Components)                       │
│   Copper: 1 oz (35 µm)                                      │
│   Purpose: Lower-speed signals, decoupling caps, BGA escape │
│   Impedance: 50Ω SE (microstrip)                            │
└─────────────────────────────────────────────────────────────┘

Total thickness: ~1.6 mm (63 mil)
```

### 3.1.1 Impedance Calculations

| Layer | Type | Target Z0 | Trace Width | Trace Spacing (diff) | Dielectric Height |
|-------|------|-----------|-------------|---------------------|-------------------|
| L1 | Microstrip | 50Ω SE | 0.18 mm (7 mil) | - | 0.15 mm to L2 GND |
| L1 | Edge-coupled diff | 100Ω diff | 0.14 mm (5.5 mil) | 0.14 mm (5.5 mil) | 0.15 mm to L2 GND |
| L4 | Microstrip | 50Ω SE | 0.18 mm (7 mil) | - | 0.15 mm to L3 PWR |
| L4 | Edge-coupled diff | 100Ω diff | 0.14 mm (5.5 mil) | 0.14 mm (5.5 mil) | 0.15 mm to L3 PWR |

*Note: L4 impedance references L3 power plane. For critical signals, stitch L3 to L2 with vias near the signal path to ensure a solid GND return.*

## 3.2 Component Placement

### 3.2.1 Top Side (Layer 1)

```
┌──────────────────────────────────────────────────────────────┐
│ 85 mm                                                        │
│ ┌─────────────────────────────────────────────────────────┐  │
│ │ [SWD]  [OLED U10]                    [USB-C J1]          │  │
│ │                                                          │  │
│ │  ┌──────────────────────┐  ┌──────────────────┐         │  │
│ │  │   STM32H743VIT6      │  │  USB3320C U4     │         │  │
│ │  │   U1 (LQFP-100)      │  │  (QFN-32)        │         │  │
│ │  │                      │  └──────────────────┘         │  │
│ │  │   Center of board    │                                │  │
│ │  └──────────────────────┘                                │  │
│ │                                                          │  │
│ │  ┌──────────────┐  ┌──────────────────────┐             │  │
│ │  │ iCE40UP5K    │  │  MT41K256M16TW       │             │  │
│ │  │ U2 (QFN-48)  │  │  U3 (FBGA-96)        │             │  │
│ │  │              │  │  9×14mm              │             │  │
│ │  └──────────────┘  └──────────────────────┘             │  │
│ │                                                          │  │
│ │  ┌──────────────┐  ┌──────────────┐                     │  │
│ │  │TPS6521815    │  │ BQ25896      │                     │  │
│ │  │U5 (VQFN-48)  │  │ U6 (WQFN-24) │                     │  │
│ │  └──────────────┘  └──────────────┘                     │  │
│ │                                                          │  │
│ │  [J5 BAT]  [J4 eMMC Header]  [J3 ISP]  [J2 microSD]     │  │
│ └─────────────────────────────────────────────────────────┘  │
│                                                              │
│ 54 mm                                                       │
└──────────────────────────────────────────────────────────────┘
```

### 3.2.2 Bottom Side (Layer 4)

```
┌──────────────────────────────────────────────────────────────┐
│                                                              │
│  ┌──────────────────────────────────────────────────────┐    │
│  │  Decoupling capacitors (0402/0603/0805)              │    │
│  │  - Under STM32: 8× 0.1µF 0402, 2× 10µF 0805         │    │
│  │  - Under DDR3L: 16× 0.1µF 0402, 4× 22µF 0805        │    │
│  │  - Under FPGA: 8× 0.1µF 0402, 2× 10µF 0805          │    │
│  │  - Under PMIC: per datasheet reference               │    │
│  └──────────────────────────────────────────────────────┘    │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐    │
│  │  TXS0108E Level Shifters (U7, U8)                   │    │
│  │  - Near eMMC header J4                               │    │
│  └──────────────────────────────────────────────────────┘    │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐    │
│  │  Series termination resistors (0402/0603)            │    │
│  │  - DDR3L: 49.9Ω arrays near U3                       │    │
│  │  - eMMC: 33Ω/49.9Ω near U7/U8                        │    │
│  │  - SPI: 33Ω near FPGA                                 │    │
│  └──────────────────────────────────────────────────────┘    │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐    │
│  │  TPS74801 LDO (U11) for FPGA VCC                     │    │
│  └──────────────────────────────────────────────────────┘    │
│                                                              │
│  [SW1] [SW2] [SW3] [SW4]  (tactile buttons, right edge)     │
│  [LED1] [LED2]  (RGB LEDs, top edge near OLED)              │
│  [BZ1]  (Buzzer, bottom-right)                               │
└──────────────────────────────────────────────────────────────┘
```

## 3.3 High-Speed Routing Rules

### 3.3.1 DDR3L SDRAM Routing (Critical)

The DDR3L interface operates at 400 MHz (800 MT/s). This is the most critical routing on the board.

#### Topology: Fly-by for Address/Command/Control, Point-to-Point for Data

```
Address/Command/Control (Fly-by):
STM32 ────┬──── A0-A15, BA0-2 ────► DDR3L (terminated at SDRAM via ODT)
          │
          └──── CK/CK#, CKE, CS#, RAS#, CAS#, WE#, ODT ────► DDR3L

Data (Point-to-Point, byte-lane matched):
STM32 ──── DQ[0:7], LDQS, LDQS#, LDM ────► DDR3L (Byte Lane 0)
STM32 ──── DQ[8:15], UDQS, UDQS#, UDM ────► DDR3L (Byte Lane 1)
```

#### Length Matching Rules

| Group | Match To | Tolerance | Notes |
|-------|----------|-----------|-------|
| CK to CK# | Each other | ±1 mil | Differential pair |
| DQS to DQS# (per lane) | Each other | ±1 mil | Differential pair |
| DQ[0:7] to LDQS | LDQS | ±5 mil | Byte lane 0 |
| DQ[8:15] to UDQS | UDQS | ±5 mil | Byte lane 1 |
| All address/command | CK | ±10 mil | Fly-by group |
| CK to longest DQS | DQS | ±25 mil | Clock-to-strobe skew |

#### Routing Specifications

| Parameter | Value |
|-----------|-------|
| Trace impedance | 50Ω SE, 100Ω diff |
| Trace width | 0.18 mm (7 mil) SE, 0.14 mm (5.5 mil) diff |
| Via type | 0.20 mm drill, 0.45 mm pad, dog-bone for BGA |
| Max via count per net | 2 (STM32 pad → via → DDR3L pad) |
| Layer | L1 (top) preferred, L4 for escape |
| Reference plane | Solid GND (L2) |
| Spacing to other nets | ≥ 3× trace width (21 mil) |
| Stitching vias | Every 5 mm along differential pairs |

#### BGA Escape Strategy (DDR3L, 96-ball, 0.8 mm pitch)

```
Row A (outer): Route on L1 directly
Row B: Route on L1 between Row A pads
Row C: Via to L4, route on L4
Row D: Via to L4, route on L4
Row E: Via to L4, route on L4
Row F: Via to L4, route on L4
Row G: Via to L4, route on L4
Row H: Via to L4, route on L4
Row J: Via to L4, route on L4
Row K: Via to L4, route on L4
Row L: Via to L4, route on L4
Row M: Via to L4, route on L4
Row N: Via to L4, route on L4
Row P: Route on L1 directly (outer row)
Row R: Route on L1 between Row P pads
```

Dog-bone via pattern: 0.20 mm drill, 0.45 mm pad, 0.15 mm trace from pad to via. Via placed at 45° angle from pad center, 0.60 mm from pad center.

### 3.3.2 eMMC HS400 Routing

| Parameter | Value |
|-----------|-------|
| Max frequency | 200 MHz DDR |
| Trace impedance | 50Ω SE |
| Trace width | 0.18 mm (7 mil) |
| Length matching | CLK to longest DAT: ±10 mil; all DAT within ±5 mil |
| Series termination | 49.9Ω at STM32 side for CLK, 33Ω for CMD/DAT |
| Layer | L1 (top) |
| Reference plane | Solid GND (L2) |
| Spacing | ≥ 3× trace width between CLK and other signals |

### 3.3.3 USB 3.0 / ULPI Routing

| Parameter | Value |
|-----------|-------|
| ULPI clock | 60 MHz |
| ULPI data bus | 8-bit SDR |
| DP/DM impedance | 90Ω differential |
| DP/DM trace width | 0.22 mm (8.7 mil), spacing 0.14 mm (5.5 mil) |
| ULPI bus length matching | ±20 mil within data bus |
| Layer | L1 (top) |
| Reference plane | Solid GND (L2) |

### 3.3.4 OCTOSPI Routing

| Parameter | Value |
|-----------|-------|
| Max frequency | 100 MHz SDR |
| Trace impedance | 50Ω SE |
| Trace width | 0.18 mm (7 mil) |
| Length matching | CLK to longest IO: ±10 mil; all IO within ±5 mil |
| Series termination | 33Ω at STM32 side |
| Layer | L1 (top) |
| Reference plane | Solid GND (L2) |

## 3.4 Via Strategy

### 3.4.1 Via Types

| Type | Drill | Pad | Annular Ring | Usage |
|------|-------|-----|--------------|-------|
| Signal via | 0.20 mm (8 mil) | 0.45 mm (18 mil) | 0.125 mm (5 mil) | General signal routing |
| Power via | 0.30 mm (12 mil) | 0.60 mm (24 mil) | 0.15 mm (6 mil) | Power plane connections |
| Thermal via | 0.30 mm (12 mil) | 0.60 mm (24 mil) | 0.15 mm (6 mil) | Under QFN/BGA thermal pads |
| Stitching via | 0.20 mm (8 mil) | 0.45 mm (18 mil) | 0.125 mm (5 mil) | GND plane stitching |
| Mounting via | 3.00 mm (118 mil) | 6.00 mm (236 mil) | - | M2.5 mounting holes |

### 3.4.2 Via Placement Rules

- **Signal vias**: Max 2 per net. Avoid on high-speed nets; use L1-L4 layer transitions only at BGA escapes.
- **Stitching vias**: Grid of 5 mm spacing around board perimeter, 10 mm spacing interior. Tighter (2.5 mm) along differential pairs and high-speed buses.
- **Thermal vias**: 4×4 grid under QFN exposed pads (0.5 mm pitch), 6×6 grid under BGA (0.8 mm pitch). Connect L1→L2→L3→L4, filled and capped.
- **Anti-pad**: 0.15 mm clearance on L2 (GND) and L3 (PWR) for signal vias not connecting to those planes.

## 3.5 Power Distribution Network (PDN)

### 3.5.1 Power Plane Segmentation (Layer 3)

```
┌──────────────────────────────────────────────────────────────┐
│ LAYER 3 — Power Plane                                        │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐    │
│  │  3.3V VDDIO                                          │    │
│  │  ~60% of board area                                  │    │
│  │  Covers: STM32 VDD, FPGA VCCIO, USB3320 VDD33,      │    │
│  │          OLED, LEDs, level shifters, ISP header      │    │
│  └──────────────────────────────────────────────────────┘    │
│                                                              │
│  ┌─────────────────────────┐ ┌──────────────────────────┐    │
│  │  1.35V DDR3L            │ │  1.8V VDDIO_1V8          │    │
│  │  ~15% of board area     │ │  ~10% of board area      │    │
│  │  Under DDR3L + FMC area │ │  eMMC, USB3320 VDD18,    │    │
│  │                          │ │  FPGA VCCIO_1V8 option   │    │
│  └─────────────────────────┘ └──────────────────────────┘    │
│                                                              │
│  ┌──────────────────────┐ ┌─────────────────────────────┐    │
│  │  1.2V VCORE           │ │  1.0V VDDUSB               │    │
│  │  ~8% of board area    │ │  ~3% of board area          │    │
│  │  Under STM32 + FPGA   │ │  Near USB3320              │    │
│  └──────────────────────┘ └─────────────────────────────┘    │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐    │
│  │  VSYS (3.5-5V) — small island near PMIC/BQ25896      │    │
│  │  ~4% of board area                                   │    │
│  └──────────────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────────────┘
```

### 3.5.2 PDN Target Impedance

| Rail | DC Current | Target Z (DC-1MHz) | Target Z (1-100MHz) | Target Z (>100MHz) |
|------|-----------|-------------------|---------------------|--------------------|
| 3.3V | 500 mA | < 50 mΩ | < 200 mΩ | < 500 mΩ |
| 1.35V | 400 mA | < 40 mΩ | < 150 mΩ | < 400 mΩ |
| 1.2V | 500 mA | < 40 mΩ | < 150 mΩ | < 400 mΩ |
| 1.8V | 300 mA | < 60 mΩ | < 250 mΩ | < 600 mΩ |
| 1.0V | 100 mA | < 100 mΩ | < 400 mΩ | < 1 Ω |

### 3.5.3 Decoupling Placement Map

```
For each IC, decoupling capacitors placed in concentric rings:

Ring 1 (closest, < 2 mm from power pin):
  - 0402 0.1µF X7R (HF decoupling)
  - On bottom layer, directly under pin vias

Ring 2 (2-5 mm from power pin):
  - 0603 0.1µF X7R (mid-frequency)
  - On bottom layer

Ring 3 (5-15 mm from power pin):
  - 0805 10µF X5R (bulk)
  - On bottom layer

Ring 4 (near voltage regulator output):
  - 1210 100µF X5R or 0805 22µF X5R (bulk)
  - On top layer near regulator
```

## 3.6 Thermal Management

### 3.6.1 Power Dissipation Estimates

| Component | Max Power | Cooling Strategy |
|-----------|-----------|-----------------|
| STM32H743 @ 480 MHz | ~0.6W | 4-layer board conduction, exposed pad to GND plane |
| iCE40UP5K | ~0.1W | QFN exposed pad to GND plane |
| MT41K256M16 | ~0.5W | BGA to GND/PWR planes |
| USB3320C | ~0.15W | QFN exposed pad to GND plane |
| TPS6521815 | ~0.5W (total) | VQFN exposed pad to GND plane, multiple thermal vias |
| BQ25896 (charging) | ~1.0W | WQFN exposed pad, thermal vias, copper pour |
| TXS0108E ×2 | ~0.05W each | Negligible |
| TPS74801 | ~0.2W | VSON exposed pad |

**Total max dissipation: ~3.1W**

### 3.6.2 Thermal Via Arrays

| Component | Via Count | Via Pattern | Notes |
|-----------|-----------|-------------|-------|
| STM32H743 | 16 (4×4) | 0.5 mm pitch under exposed pad | Connect L1→L4, filled |
| iCE40UP5K | 9 (3×3) | 0.5 mm pitch | Connect L1→L4 |
| MT41K256M16 | 36 (6×6) | 0.8 mm pitch under BGA | Connect L1→L4 |
| USB3320C | 9 (3×3) | 0.5 mm pitch | Connect L1→L4 |
| TPS6521815 | 25 (5×5) | 0.5 mm pitch | Connect L1→L4 |
| BQ25896 | 16 (4×4) | 0.5 mm pitch | Connect L1→L4 |
| TPS74801 | 4 (2×2) | 0.5 mm pitch | Connect L1→L4 |

### 3.6.3 Copper Pour Strategy

- **L1 (Top)**: GND copper pour in all unused areas, tied to L2 GND with stitching vias every 5 mm
- **L2 (GND)**: Solid plane, no splits
- **L3 (PWR)**: Segmented power planes with 0.5 mm isolation gaps
- **L4 (Bottom)**: GND copper pour in all unused areas, tied to L2 GND with stitching vias

## 3.7 Isolation & Protection

### 3.7.1 ESD Protection

| Interface | Protection Device | Placement |
|-----------|------------------|-----------|
| USB-C VBUS, DP, DM | USBLC6-2SC6 (SOT-23-6) | Within 5 mm of J1 |
| ISP Header (all pins) | IP4220CZ6 (TVS array) | Within 10 mm of J3 |
| eMMC Header | ESD protection built into TXS0108E | At U7/U8 |
| microSD | ESD protection built into socket | At J2 |
| Battery | SMAJ5.0CA TVS | At J5 |

### 3.7.2 Overcurrent Protection

| Rail | Protection | Trip Point |
|------|-----------|------------|
| USB VBUS | PTC resettable fuse 1206L050, 500 mA | 1.0A hold, 2.0A trip |
| VSYS | BQ25896 internal current limit | 3.0A |
| Target VCC (ISP pin 1) | TPS74801 current limit | 1.5A |
| Battery charge | BQ25896 charge current limit | 2.0A (configurable) |

### 3.7.3 Reverse Polarity Protection

| Input | Protection |
|-------|-----------|
| USB VBUS | BQ25896 internal |
| Battery | P-ch MOSFET (Q1 DMG2305UX) + Schottky (D1 CDBU0520) |

### 3.7.4 Ground Isolation

The board uses a **single unified ground plane** (L2). There is no split between digital and analog grounds because:
- No sensitive analog circuitry (no ADC measurements)
- All high-speed signals reference the same ground
- Split planes create return path discontinuities that degrade signal integrity

The ISP header's target ground (pin 2) connects directly to the system ground plane. The operator must ensure the target device shares this ground reference (standard practice for ISP).

## 3.8 Board Outline & Mechanical Drawing

### 3.8.1 Board Outline (KiCad Edge.Cuts)

```
Points (origin at bottom-left, mm):
(0, 0) → (85, 0) → (85, 54) → (0, 54) → (0, 0)

Corner radius: 3.0 mm (all four corners)

Mounting holes (center coordinates):
H1: (3.5, 3.5) — M2.5, 3.0 mm hole, 6.0 mm pad
H2: (81.5, 3.5) — M2.5
H3: (3.5, 50.5) — M2.5
H4: (81.5, 50.5) — M2.5
```

### 3.8.2 Keepout Zones

| Zone | Location | Size | Reason |
|------|----------|------|--------|
| USB-C | Top edge, centered | 10×12 mm | Connector body + mating clearance |
| microSD | Right edge, y=30-45 mm | 15×18 mm | Card insertion path |
| ISP Header | Left edge, y=5-25 mm | 5×25 mm | Header + cable clearance |
| eMMC Header | Bottom edge, centered | 8×20 mm | Mating connector clearance |
| Battery | Bottom-left | 10×15 mm | JST connector + wire bend |
| OLED | Top face, x=30-55, y=35-50 | 28×16 mm | Display module |
| Buttons | Right edge, y=5-25 mm | 6×25 mm | Button bodies |
| Mounting holes | 4 corners | 6 mm diameter each | Screw head clearance |

### 3.8.3 Enclosure Clearance

The PCB fits in a 3D-printed enclosure with:
- Internal dimensions: 87 × 56 × 12 mm
- Wall thickness: 1.5 mm
- External dimensions: 90 × 59 × 15 mm
- Cutouts for: USB-C, microSD, ISP header, eMMC header, OLED window, buttons
- Snap-fit lid with 4× M2.5 brass inserts

## 3.9 Design Rule Check (DRC) Parameters

| Rule | Value |
|------|-------|
| Min track width | 0.10 mm (4 mil) |
| Min track spacing | 0.10 mm (4 mil) |
| Min via drill | 0.20 mm (8 mil) |
| Min via diameter | 0.45 mm (18 mil) |
| Min annular ring | 0.10 mm (4 mil) |
| Min silkscreen text | 0.8 mm height, 0.12 mm width |
| Min solder mask sliver | 0.10 mm (4 mil) |
| Copper to edge clearance | 0.40 mm (16 mil) |
| Hole to hole clearance | 0.25 mm (10 mil) |
| Solder mask expansion | 0.05 mm (2 mil) |
| Paste mask expansion | -0.05 mm (-2 mil) |

### 3.9.1 Net Classes

| Class | Track Width | Clearance | Via Drill | Via Diam |
|-------|------------|-----------|-----------|----------|
| Default | 0.15 mm (6 mil) | 0.15 mm (6 mil) | 0.20 mm | 0.45 mm |
| Power | 0.30 mm (12 mil) | 0.20 mm (8 mil) | 0.30 mm | 0.60 mm |
| High-Speed 50Ω | 0.18 mm (7 mil) | 0.21 mm (8 mil) | 0.20 mm | 0.45 mm |
| Diff 100Ω | 0.14 mm (5.5 mil) | 0.14 mm (5.5 mil) | 0.20 mm | 0.45 mm |
| USB 90Ω Diff | 0.22 mm (8.7 mil) | 0.14 mm (5.5 mil) | 0.20 mm | 0.45 mm |

## 3.10 Assembly Notes

### 3.10.1 SMT Assembly Order

1. **Bottom side first** (reflow):
   - 0402/0603/0805 passives (decoupling, termination)
   - TXS0108E level shifters (U7, U8)
   - TPS74801 LDO (U11)
   - WS2812B LEDs (LED1, LED2)
   - Tactile switches (SW1-SW4)
   - Buzzer (BZ1)

2. **Top side** (reflow):
   - STM32H743VIT6 (U1) — LQFP-100
   - iCE40UP5K (U2) — QFN-48
   - MT41K256M16TW (U3) — FBGA-96
   - USB3320C (U4) — QFN-32
   - TPS6521815 (U5) — VQFN-48
   - BQ25896 (U6) — WQFN-24
   - Crystals (Y1, Y2, Y3)
   - 0805/1210 passives

3. **Hand solder / selective**:
   - USB-C connector (J1)
   - microSD connector (J2)
   - ISP header (J3) — THT
   - eMMC header (J4) — SMT fine-pitch
   - Battery connector (J5) — THT
   - OLED module (U10) — SMT or socket

### 3.10.2 Critical Assembly Notes

- **BGA placement**: MT41K256M16 requires X-ray inspection after reflow
- **QFN center pads**: Ensure adequate solder paste coverage (70-80% pad area) for thermal pads
- **Crystal placement**: Y1, Y2, Y3 must be placed within 5 mm of their respective ICs with guard rings
- **USB-C**: Through-hole reinforcement tabs must be hand-soldered for mechanical strength
- **Conformal coating**: Optional acrylic coating for field-deployed units (avoid on connectors)

## 3.11 Test Points

| TP | Signal | Location | Type |
|----|--------|----------|------|
| TP1 | 3.3V | Near U5 | 1.0 mm pad |
| TP2 | 1.2V | Near U5 | 1.0 mm pad |
| TP3 | 1.35V | Near U5 | 1.0 mm pad |
| TP4 | 1.8V | Near U5 | 1.0 mm pad |
| TP5 | 1.0V | Near U5 | 1.0 mm pad |
| TP6 | GND | Multiple locations | 1.0 mm pad |
| TP7 | NRST | Near SWD header | 1.0 mm pad |
| TP8 | SWDIO | SWD header | Tag-Connect pad |
| TP9 | SWCLK | SWD header | Tag-Connect pad |
| TP10 | SWO | SWD header | Tag-Connect pad |
| TP11 | UART3_TX (debug) | Near STM32 | 1.0 mm pad |
| TP12 | UART3_RX (debug) | Near STM32 | 1.0 mm pad |

---

*End of Phase 3 — PCB Blueprints & Layout*
