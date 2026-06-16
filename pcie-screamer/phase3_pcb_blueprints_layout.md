# Phase 3: PCB Blueprints & Layout — PCIe Screamer

## 1. Board Overview

### 1.1 Physical Specifications

| Parameter | Value |
|-----------|-------|
| Form Factor | M.2 2280 Interposer |
| Board Dimensions | 22.00 mm × 80.00 mm |
| Board Thickness | 1.00 mm (8-layer HDI) |
| M.2 Edge Connector | 67-position M-key gold fingers, 0.50 mm pitch |
| Device Socket Footprint | Amphenol 10132797-0011LF, top-mount, 3.2 mm height |
| Total Stack Height | < 4.0 mm (PCB + components both sides + socket) |
| Copper Weight | Outer layers: 0.5 oz (18 µm) + plating; Inner layers: 0.5 oz (18 µm) |
| Surface Finish | ENIG (Electroless Nickel Immersion Gold) on gold fingers; ENIG on all pads |
| Solder Mask | Green LPI, both sides |
| Silkscreen | White, top side only (bottom side has no space) |
| Min Trace/Space | 3.0 mil / 3.0 mil (76 µm / 76 µm) |
| Min Drill | 0.20 mm (8 mil) mechanical; 0.10 mm (4 mil) laser microvia |
| Max Aspect Ratio | 10:1 for through-holes; 1:1 for microvias |

### 1.2 Board Outline & Mechanical Drawing

```
┌──────────────────────────────────────────────────────────────────────┐
│  TOP VIEW (Component Side)                                           │
│                                                                      │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │  M.2 Gold Fingers (Host Edge)                                │   │
│  │  ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐  │   │
│  │  │1│ │3│ │5│ │7│ │9│ │11│ │13│ │15│ │17│ │19│ │21│ │23│ │25│  │   │
│  │  └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘  │   │
│  │  ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐  │   │
│  │  │2│ │4│ │6│ │8│ │10│ │12│ │14│ │16│ │18│ │20│ │22│ │24│ │26│  │   │
│  │  └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘  │   │
│  │  ...continuing to pin 75 (top) / 74 (bottom)...              │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                      │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │  ZONE A: PCIe Redrivers & FPGA (0-35 mm from edge)           │   │
│  │  ┌──────────┐  ┌──────────┐                                  │   │
│  │  │ U2       │  │ U3       │  PI3EQX12908 Redrivers           │   │
│  │  │ Lanes0-1 │  │ Lanes2-3 │  TQFN-42, 9×3.5 mm              │   │
│  │  └──────────┘  └──────────┘                                  │   │
│  │                                                               │   │
│  │  ┌─────────────────────────────────┐                          │   │
│  │  │                                 │                          │   │
│  │  │  U1: LFE5U-45F-6BG381C         │                          │   │
│  │  │  ECP5 FPGA, caBGA-381          │                          │   │
│  │  │  17×17 mm, 0.8 mm pitch        │                          │   │
│  │  │                                 │                          │   │
│  │  └─────────────────────────────────┘                          │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                      │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │  ZONE B: DDR3, Power, Clock (35-55 mm from edge)             │   │
│  │  ┌──────────────┐  ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐       │   │
│  │  │ U5: DDR3     │  │U8  │ │U9  │ │U10 │ │U11 │ │U7  │       │   │
│  │  │ MT41K256M16  │  │1.35│ │1.2V│ │1.1V│ │1.8V│ │Si  │       │   │
│  │  │ FBGA-96      │  │Buck│ │Buck│ │Buck│ │Buck│ │5332│       │   │
│  │  │ 9×14 mm      │  └────┘ └────┘ └────┘ └────┘ └────┘       │   │
│  │  └──────────────┘                                            │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                      │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │  ZONE C: USB Bridge, Flash, Connectors (55-80 mm from edge)  │   │
│  │  ┌──────────────┐  ┌────┐                                    │   │
│  │  │ U4: FT601Q   │  │U6  │  SPI Flash                        │   │
│  │  │ USB 3.0 FIFO │  │128 │  SOIC-8                            │   │
│  │  │ QFN-76       │  │Mb  │                                    │   │
│  │  │ 9×9 mm       │  └────┘                                    │   │
│  │  └──────────────┘                                            │   │
│  │                                                               │   │
│  │  ┌──────────────────────────────────────┐                     │   │
│  │  │  J2: M.2 Device Socket              │                     │   │
│  │  │  Amphenol 10132797-0011LF           │                     │   │
│  │  │  Top-mount, 3.2 mm height           │                     │   │
│  │  └──────────────────────────────────────┘                     │   │
│  │                                                               │   │
│  │  ┌─────────┐  D1 D2 D3 D4 (LEDs)                             │   │
│  │  │ J3:     │                                                 │   │
│  │  │ USB-C   │  (at board edge, right side)                    │   │
│  │  └─────────┘                                                 │   │
│  └──────────────────────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────────────────────────┘
```

### 1.3 Bottom Side Layout

```
┌──────────────────────────────────────────────────────────────────────┐
│  BOTTOM VIEW (Solder Side)                                           │
│                                                                      │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │  FPGA Decoupling Capacitors (under BGA footprint)            │   │
│  │  ┌─────────────────────────────────┐                          │   │
│  │  │ 100nF array (0402) under U1     │                          │   │
│  │  │ 10µF array (0805) under U1      │                          │   │
│  │  │ 100µF tantalum (1210) near U1   │                          │   │
│  │  └─────────────────────────────────┘                          │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                      │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │  DDR3 Decoupling (under U5 BGA)                               │   │
│  │  ┌──────────────┐                                            │   │
│  │  │ 100nF ×8     │                                            │   │
│  │  │ 10µF ×4      │                                            │   │
│  │  │ 100µF ×2     │                                            │   │
│  │  └──────────────┘                                            │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                      │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │  Power Inductors & Bulk Capacitors                            │   │
│  │  L1-L4 (1µH inductors for buck converters)                    │   │
│  │  Additional bulk tantalum capacitors                          │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                      │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │  Test Points & Debug Header                                   │   │
│  │  TP1 (UART TX), TP2 (UART RX), TP3 (GPIO15)                  │   │
│  │  TP4 (VCC3V3), TP5 (VCC1V1), TP6 (VCC1V35), TP7 (GND)       │   │
│  └──────────────────────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────────────────────────┘
```

## 2. PCB Layer Stackup

### 2.1 8-Layer HDI Stackup

```
Layer 1 (Top)          : Signal + Components  ───── 0.5 oz Cu + plating
    ▲ 0.10 mm Prepreg (1080, 65% resin)
Layer 2 (Inner 1)      : GND Plane              ───── 0.5 oz Cu
    ▲ 0.10 mm Core (0.10 mm, 2116)
Layer 3 (Inner 2)      : Signal (PCIe Routing)   ───── 0.5 oz Cu
    ▲ 0.20 mm Prepreg (2×1080)
Layer 4 (Inner 3)      : GND Plane              ───── 0.5 oz Cu
    ▲ 0.10 mm Core (0.10 mm, 2116)
Layer 5 (Inner 4)      : Power Planes (Split)   ───── 0.5 oz Cu
    ▲ 0.20 mm Prepreg (2×1080)
Layer 6 (Inner 5)      : Signal (DDR3, USB)     ───── 0.5 oz Cu
    ▲ 0.10 mm Core (0.10 mm, 2116)
Layer 7 (Inner 6)      : GND Plane              ───── 0.5 oz Cu
    ▲ 0.10 mm Prepreg (1080, 65% resin)
Layer 8 (Bottom)        : Signal + Decoupling    ───── 0.5 oz Cu + plating

Total Thickness: ~1.00 mm
```

### 2.2 Layer Material Properties

| Layer | Material | Thickness | Dk (@ 5 GHz) | Df (@ 5 GHz) |
|-------|----------|-----------|--------------|--------------|
| Prepreg 1080 | FR-4 (Isola 370HR) | 0.10 mm | 3.72 | 0.016 |
| Core 2116 | FR-4 (Isola 370HR) | 0.10 mm | 3.72 | 0.016 |
| Prepreg 2×1080 | FR-4 (Isola 370HR) | 0.20 mm | 3.72 | 0.016 |

### 2.3 Layer Assignment Rationale

| Layer | Purpose | Rationale |
|-------|---------|-----------|
| L1 (Top) | Components + short signal escapes | BGA breakouts, redriver connections, M.2 edge routing |
| L2 (GND) | Solid ground reference for L1 | Uninterrupted return path for top-layer PCIe signals |
| L3 (Signal) | PCIe differential pairs | Stripline between two GND planes (L2, L4) — best SI |
| L4 (GND) | Solid ground reference for L3 | Second reference plane for stripline, isolates PCIe from power |
| L5 (Power) | Split power planes | VCC3V3, VCC1V35, VCC1V2, VCC1V1, VCC1V8 zones |
| L6 (Signal) | DDR3, USB, general routing | Stripline between power (L5) and GND (L7) |
| L7 (GND) | Solid ground reference for L6 | Return path for bottom-side signals |
| L8 (Bottom) | Decoupling + test points | Capacitor placement under BGAs, test points |

## 3. High-Speed Routing Rules

### 3.1 PCIe Differential Pair Routing (5.0 GT/s, 2.5 GHz fundamental)

| Parameter | Value | Notes |
|-----------|-------|-------|
| Trace Width | 0.12 mm (4.7 mil) | For 85 Ω differential on L3 stripline |
| Trace Spacing (intra-pair) | 0.15 mm (5.9 mil) | Tight coupling for common-mode rejection |
| Trace Spacing (inter-pair) | ≥ 0.60 mm (23.6 mil) | 4× intra-pair spacing to minimize crosstalk |
| Max Intra-Pair Skew | < 0.15 mm (5.9 mil) | Length matching within each pair |
| Max Inter-Pair Skew | < 2.5 mm (98 mil) | Across all 4 lanes of a link |
| Max Via Count | 2 per net | Minimize via stub effects |
| Via Type | Back-drilled through-hole or blind microvia | Eliminate stub resonance |
| Reference Plane | Solid GND on L2 and L4 | No splits under PCIe routing |
| AC Coupling Caps | 100 nF, 0402, placed near transmitter | On L1, with minimal stub |

### 3.2 PCIe REFCLK Routing (100 MHz HCSL)

| Parameter | Value |
|-----------|-------|
| Trace Width | 0.10 mm (3.9 mil) |
| Trace Spacing (intra-pair) | 0.15 mm (5.9 mil) |
| Impedance | 100 Ω differential |
| AC Coupling | 100 nF at receiver (FPGA side) |
| Termination | 49.9 Ω to GND at driver (Si5332) for HCSL |

### 3.3 DDR3 Routing (800 MHz clock, 400 MHz data rate)

| Parameter | Value | Notes |
|-----------|-------|-------|
| Trace Width (DQ/DQS) | 0.10 mm (3.9 mil) | For 40 Ω single-ended on L6 stripline |
| Trace Width (Addr/Cmd/Ctrl) | 0.10 mm (3.9 mil) | 40 Ω single-ended |
| DQS Intra-Pair Spacing | 0.15 mm (5.9 mil) | 100 Ω differential |
| DQ-to-DQS Length Match | ±2 mm per byte lane | Critical for read/write timing |
| Addr/Cmd-to-CK Length Match | ±5 mm | Less critical, captured at CK edge |
| CK-to-CK# Length Match | ±0.15 mm | Differential pair matching |
| Max Trace Length | < 20 mm | Short as possible for minimal skew |
| VTT Resistor Placement | Near FPGA (driver side) | 240 Ω to VCC1V35/2 for SSTL135 |
| Reference Plane | Solid GND on L7 | Continuous return path |

### 3.4 USB 3.0 SuperSpeed Routing (5 Gbps)

| Parameter | Value |
|-----------|-------|
| Trace Width | 0.12 mm (4.7 mil) |
| Trace Spacing (intra-pair) | 0.15 mm (5.9 mil) |
| Impedance | 90 Ω differential |
| Max Trace Length | < 15 mm |
| AC Coupling | 100 nF at receiver (already in FT601) |
| ESD Protection | TPD4EUSB30 or similar at Type-C connector |

### 3.5 FT601 32-bit Data Bus Routing (100 MHz SDR)

| Parameter | Value |
|-----------|-------|
| Trace Width | 0.10 mm (3.9 mil) |
| Impedance | 50 Ω single-ended |
| Length Match (all 32 bits) | ±5 mm |
| Length Match (CLK to data) | CLK longer by 0-5 mm (setup margin) |
| Max Trace Length | < 40 mm |
| Series Termination | 22 Ω at FPGA driver (optional, FPGA has programmable drive) |

## 4. Via Strategy

### 4.1 Via Types

| Type | Structure | Use Case | Dimensions |
|------|-----------|----------|------------|
| Through-Hole (PTH) | L1-L8 | Power, GND, low-speed signals, connectors | Drill: 0.20 mm, Pad: 0.45 mm, Anti-pad: 0.65 mm |
| Blind Microvia (L1-L2) | L1→L2 | BGA escape from top to first GND | Drill: 0.10 mm, Pad: 0.25 mm (L1), 0.30 mm (L2) |
| Blind Microvia (L1-L3) | L1→L2→L3 (stacked) | BGA escape to stripline routing layer | Drill: 0.10 mm, Pad: 0.25 mm |
| Blind Microvia (L7-L8) | L7→L8 | Bottom-side BGA escape | Drill: 0.10 mm, Pad: 0.25 mm (L8), 0.30 mm (L7) |
| Buried Via (L3-L6) | L3→L4→L5→L6 | Inter-layer signal routing | Drill: 0.15 mm, Pad: 0.35 mm |

### 4.2 BGA Escape Strategy (FPGA, 381-ball, 0.8 mm pitch)

The ECP5-45F caBGA-381 has 0.8 mm ball pitch. Escape strategy:

```
Row 1 (outer): Direct escape on L1 (no via needed)
Row 2: Microvia to L2 (GND) then drop to L3 for routing
Row 3: Microvia to L3 (dog-bone fanout on L1)
Row 4+: Stacked microvia L1→L2→L3, route on L3

Inner rows (core power, GND): Direct microvia to L2 (GND) or L5 (power)
```

For the DDR3 FBGA-96 (0.8 mm pitch), similar strategy but fewer rows:
```
Row 1: Direct escape on L8 (bottom-mounted)
Row 2: Microvia L8→L7→L6 for routing
```

### 4.3 Via Count Budget

| Net Group | Max Vias/Net | Typical Vias/Net |
|-----------|-------------|------------------|
| PCIe Differential Pairs | 2 | 2 (L1→L3→L1) |
| PCIe REFCLK | 2 | 2 |
| DDR3 DQ/DQS | 2 | 1-2 |
| DDR3 Addr/Cmd/Ctrl | 2 | 1-2 |
| FT601 Data | 2 | 1-2 |
| FT601 Control | 2 | 1 |
| Power | 4+ | 2-4 (multiple for current capacity) |
| GND | Many | Many (thermal + electrical) |

### 4.4 Via Stub Mitigation

For through-hole vias carrying high-speed signals (>1 GHz):
- **Back-drilling:** Remove unused portion of via barrel below the last connected layer
- **Blind via preference:** Use blind microvias for all PCIe signals, eliminating stubs entirely
- **GND stitching vias:** No back-drill needed; stubs act as small shunt capacitance (benign)

## 5. Power Distribution Network (PDN)

### 5.1 Power Plane Splits (Layer 5)

```
┌──────────────────────────────────────────────────────────────────────┐
│  LAYER 5 — POWER PLANES                                              │
│                                                                      │
│  ┌────────────────────────────────────────────┐                      │
│  │  VCC3V3 (3.3V)                             │                      │
│  │  ~40% of board area                        │                      │
│  │  Covers: U2, U3, U4 IO, U6, U7, FPGA B0-3 │                      │
│  └────────────────────────────────────────────┘                      │
│                                                                      │
│  ┌──────────────────────┐ ┌──────────────────┐                      │
│  │  VCC1V35 (1.35V)     │ │  VCC1V2 (1.2V)   │                      │
│  │  ~15% area           │ │  ~15% area       │                      │
│  │  Covers: U5, FPGA B6 │ │  FPGA B4,5,7, U4 │                      │
│  └──────────────────────┘ └──────────────────┘                      │
│                                                                      │
│  ┌──────────────────────┐ ┌──────────────────┐                      │
│  │  VCC1V1 (1.1V)       │ │  VCC1V8 (1.8V)   │                      │
│  │  ~20% area           │ │  ~10% area       │                      │
│  │  Covers: FPGA Core   │ │  FPGA AUX, PLL   │                      │
│  └──────────────────────┘ └──────────────────┘                      │
└──────────────────────────────────────────────────────────────────────┘
```

### 5.2 Plane Spacing & Decoupling

| Rail | Plane Area | Target Impedance | Decoupling Strategy |
|------|-----------|-----------------|---------------------|
| VCC1V1 (FPGA Core) | ~800 mm² | < 50 mΩ to 100 MHz | 10×100nF + 4×10µF + 2×100µF |
| VCC1V35 (DDR3) | ~300 mm² | < 100 mΩ to 100 MHz | 8×100nF + 4×10µF + 2×100µF |
| VCC1V2 (FPGA I/O) | ~300 mm² | < 80 mΩ to 100 MHz | 12×100nF + 3×10µF |
| VCC3V3 (General) | ~800 mm² | < 60 mΩ to 100 MHz | 16×100nF + 4×10µF |
| VCC1V8 (AUX/PLL) | ~200 mm² | < 100 mΩ to 100 MHz | 4×100nF + 2×10µF |

### 5.3 GND Via Stitching

- GND vias around perimeter of every power plane shape (1 mm pitch)
- GND vias adjacent to every signal via transition (return path continuity)
- GND vias under every BGA component (thermal + electrical)
- GND vias along PCIe differential pair routes (every 5 mm)
- Total GND via count: ~400+

## 6. Thermal Management

### 6.1 Power Dissipation Estimates

| Component | Power (W) | Heat Source | Cooling Strategy |
|-----------|-----------|-------------|------------------|
| U1 (FPGA ECP5-45F) | 1.5-2.0W | Core logic, SERDES | GND vias under BGA + copper pour on L2/L4/L7 |
| U2, U3 (Redrivers) | 0.3W each | Analog signal conditioning | Exposed pad to GND plane + vias |
| U4 (FT601Q) | 0.8W | USB PHY, digital core | Exposed pad to GND plane + vias |
| U5 (DDR3) | 0.6W | Memory array, I/O | GND vias under BGA |
| U8-U11 (Bucks) | 0.15W each | Switching losses | Exposed pad + inductor DCR |
| **Total** | **~4.5W** | | |

### 6.2 Thermal Via Arrays

| Component | Thermal Via Count | Via Size | Pattern |
|-----------|-------------------|----------|---------|
| U1 (FPGA) | 81 (9×9 grid under die) | 0.20 mm PTH | Grid, 1.0 mm pitch |
| U2, U3 (Redrivers) | 9 each (3×3 under EP) | 0.20 mm PTH | Grid, 0.8 mm pitch |
| U4 (FT601Q) | 16 (4×4 under EP) | 0.20 mm PTH | Grid, 1.0 mm pitch |
| U5 (DDR3) | 25 (5×5 under die area) | 0.20 mm PTH | Grid, 1.0 mm pitch |

### 6.3 Copper Pour Strategy

- L2, L4, L7: Solid GND planes (also serve as heatsinks)
- L1: GND copper pour in unused areas, tied to L2 with vias
- L8: GND copper pour in unused areas, tied to L7 with vias
- All copper pours on signal layers are GND (not floating)

## 7. Isolation & Shielding

### 7.1 Analog/Digital Isolation

| Barrier | Purpose | Implementation |
|---------|---------|----------------|
| FPGA PLL Analog Supply | Isolate PLL from digital noise | Ferrite bead FB2 (600Ω @ 100 MHz) between VCC1V8 and VCCA_PLL |
| FT601 Analog Supply | Isolate USB PHY from digital | Ferrite bead FB1 (600Ω @ 100 MHz) between VCC3V3 and VCCA |
| PCIe SERDES Supply | Clean power for SERDES | Dedicated VCC1V2 plane section with extra decoupling |
| Redriver Analog | Clean 3.3V for signal conditioning | Ferrite bead FB3, FB4 (120Ω @ 100 MHz) per redriver |

### 7.2 Keep-Out Zones

| Zone | Area | Reason |
|------|------|--------|
| PCIe Differential Pairs | 0.6 mm clearance to any other copper | Crosstalk prevention |
| USB SuperSpeed Pairs | 0.6 mm clearance | Crosstalk prevention |
| DDR3 DQS Pairs | 0.4 mm clearance | Strobe integrity |
| Crystal/Oscillator | 2.0 mm clearance | Frequency pulling prevention |
| M.2 Gold Fingers | No components within 3 mm of edge | M.2 mechanical spec |
| Board Edge | 0.25 mm copper pullback | Manufacturing tolerance |

## 8. M.2 Edge Connector Design

### 8.1 Gold Finger Specifications

| Parameter | Value |
|-----------|-------|
| Finger Width | 0.50 mm (matches M.2 pad spec) |
| Finger Length | 3.50 mm (contact area) |
| Finger Pitch | 0.50 mm |
| Finger Count | 67 positions (top side odd 1-75, bottom side even 2-74) |
| Surface Finish | ENIG: 0.05-0.10 µm Au over 3-6 µm Ni |
| Bevel Angle | 20° ±5° (per M.2 spec) |
| Hard Gold Option | 0.75 µm hard gold over 3-6 µm Ni for durability (optional) |

### 8.2 M.2 Keying

M-key notch positions (per PCIe M.2 spec):
- Notch at pins 59-67 (top side, positions 59,61,63,65,67 removed)
- Notch at pins 6-28 (bottom side, positions 6,8,10,12,14,16,18,20,22,24,26,28 removed)

## 9. Component Placement Coordinates

### 9.1 Top Side Components (Origin: bottom-left of board, looking at top)

| Designator | X (mm) | Y (mm) | Rotation | Side |
|------------|--------|--------|----------|------|
| U1 (FPGA) | 12.0 | 11.0 | 0° | Top |
| U2 (Redriver 0-1) | 3.0 | 5.0 | 0° | Top |
| U3 (Redriver 2-3) | 3.0 | 17.0 | 0° | Top |
| U4 (FT601Q) | 55.0 | 11.0 | 0° | Top |
| U5 (DDR3) | 35.0 | 11.0 | 0° | Top |
| U6 (SPI Flash) | 68.0 | 5.0 | 0° | Top |
| U7 (Si5332) | 30.0 | 5.0 | 0° | Top |
| U8 (1.35V Buck) | 40.0 | 5.0 | 0° | Top |
| U9 (1.2V Buck) | 42.0 | 5.0 | 0° | Top |
| U10 (1.1V Buck) | 44.0 | 5.0 | 0° | Top |
| U11 (1.8V Buck) | 46.0 | 5.0 | 0° | Top |
| U12 (3.3V LDO) | 70.0 | 17.0 | 0° | Top |
| U13 (1.8V LDO) | 72.0 | 17.0 | 0° | Top |
| J2 (M.2 Socket) | 60.0 | 11.0 | 0° | Top |
| J3 (USB-C) | 78.0 | 11.0 | 90° | Top (edge-mounted) |
| D1 (Green LED) | 50.0 | 3.0 | 0° | Top |
| D2 (Blue LED) | 52.0 | 3.0 | 0° | Top |
| D3 (Amber LED) | 54.0 | 3.0 | 0° | Top |
| D4 (Red LED) | 56.0 | 3.0 | 0° | Top |

### 9.2 Bottom Side Components

| Designator | X (mm) | Y (mm) | Rotation | Side |
|------------|--------|--------|----------|------|
| C1-C10 (100nF, FPGA core) | 12.0 | 11.0 | various | Bottom (under U1) |
| C11-C14 (10µF, FPGA core) | 12.0 | 11.0 | various | Bottom (under U1) |
| C15-C16 (100µF, FPGA core) | 14.0 | 13.0 | 0° | Bottom |
| C20-C29 (100nF, FPGA IO) | 12.0 | 11.0 | various | Bottom (under U1) |
| C30-C33 (10µF, FPGA IO) | 12.0 | 11.0 | various | Bottom (under U1) |
| C40-C49 (100nF, DDR3) | 35.0 | 11.0 | various | Bottom (under U5) |
| C50-C53 (10µF, DDR3) | 35.0 | 11.0 | various | Bottom (under U5) |
| C54-C55 (100µF, DDR3) | 37.0 | 13.0 | 0° | Bottom |
| L1-L4 (1µH inductors) | 40-46 | 3.0 | 0° | Bottom |
| TP1-TP7 (Test points) | 75.0 | 3.0-19.0 | 0° | Bottom |

## 10. Design Rule Checks (DRC)

### 10.1 Clearance Matrix

| Object Pair | Minimum Clearance |
|-------------|-------------------|
| Trace-to-Trace (same net) | 3.0 mil |
| Trace-to-Trace (different net) | 3.0 mil |
| Trace-to-Pad | 3.0 mil |
| Pad-to-Pad | 4.0 mil |
| Trace-to-Board Edge | 10.0 mil (0.25 mm) |
| Copper-to-Board Edge | 10.0 mil |
| Via-to-Via (different net) | 5.0 mil |
| Via-to-Trace | 4.0 mil |
| Silkscreen-to-Pad | 4.0 mil |
| Solder Mask Opening-to-Pad | 2.0 mil expansion |

### 10.2 Annular Ring Requirements

| Via Type | Min Annular Ring |
|----------|-----------------|
| Through-Hole (0.20 mm drill) | 0.125 mm (5 mil) |
| Microvia (0.10 mm drill) | 0.075 mm (3 mil) |
| Component Pad | Per IPC-7351B (density level B) |

### 10.3 Manufacturing Tolerances

| Parameter | Tolerance |
|-----------|-----------|
| Board Outline | ±0.15 mm |
| Drill Position | ±0.075 mm |
| Layer-to-Layer Registration | ±0.075 mm |
| Solder Mask Registration | ±0.050 mm |
| Silkscreen Registration | ±0.100 mm |
| Gold Finger Bevel | ±5° |

## 11. Signal Integrity Simulation Notes

### 11.1 PCIe Channel Budget (Gen2, 5.0 GT/s)

| Parameter | Budget | Notes |
|-----------|--------|-------|
| Total Channel Loss @ 2.5 GHz | < 8 dB | Including redriver compensation |
| Redriver RX EQ | Up to 12 dB | PI3EQX12908 programmable |
| Redriver TX De-emphasis | -3.5 dB | Programmable |
| FPGA-to-Redriver Trace Loss | < 1.5 dB @ 2.5 GHz | Short traces (< 25 mm) |
| Redriver-to-Connector Trace Loss | < 2.0 dB @ 2.5 GHz | Medium traces |
| Return Loss | < -10 dB | Impedance matching |
| Insertion Loss Deviation | < 2 dB across 100 MHz-2.5 GHz | Smooth channel |

### 11.2 DDR3 Timing Budget (800 MHz clock)

| Parameter | Budget | Notes |
|-----------|--------|-------|
| tDS (Data Setup to DQS) | 30 ps min | At DDR3 balls |
| tDH (Data Hold from DQS) | 30 ps min | At DDR3 balls |
| tIS (Addr/Cmd Setup to CK) | 70 ps min | At DDR3 balls |
| tIH (Addr/Cmd Hold from CK) | 100 ps min | At DDR3 balls |
| Trace Skew (DQ within byte) | < 5 ps | < 1 mm length difference |
| Trace Skew (DQ to DQS) | < 10 ps | < 2 mm length difference |
| ISI Jitter | < 20 ps | Clean routing, minimal stubs |
| Crosstalk Jitter | < 10 ps | Adequate spacing |

## 12. Assembly Notes

### 12.1 SMT Process

- **Solder Paste:** Type 4 (20-38 µm particle size) for 0402 and 0.5 mm pitch QFN
- **Stencil Thickness:** 0.10 mm (4 mil) laser-cut stainless steel
- **Placement:** Automated pick-and-place, ±0.05 mm accuracy
- **Reflow Profile:** Lead-free SAC305, peak 245°C ±5°C, 60-90 sec above 217°C
- **BGA Inspection:** X-ray for U1 (FPGA) and U5 (DDR3)

### 12.2 Special Handling

- **M.2 Gold Fingers:** Masked during assembly, ENIG applied before assembly
- **M.2 Socket (J2):** Placed after reflow (through-hole tails soldered separately or using intrusive reflow)
- **USB-C Connector (J3):** Edge-mount, may require selective soldering
- **Moisture Sensitivity:** U1 (MSL 3), U5 (MSL 3) — bake before assembly if exposed

---

*End of Phase 3 — PCB Blueprints & Layout. This document defines the complete 8-layer HDI stackup, high-speed routing rules for PCIe/DDR3/USB3, via strategy, thermal management, power distribution, isolation barriers, and assembly specifications for the PCIe Screamer.*
