# Phase 3: PCB Blueprints & Layout — CAN Creeper

## 3.0 Board Overview

| Parameter | Value |
|---|---|
| Board name | CAN Creeper Rev A |
| Dimensions | 65.0 mm × 35.0 mm (credit-card format) |
| Thickness | 1.6 mm |
| Layers | 4-layer |
| Material | FR-4, Tg 150°C |
| Copper weight | 1 oz (35 µm) outer, 0.5 oz (17 µm) inner |
| Surface finish | ENIG (electroless nickel immersion gold) |
| Solder mask | Green LPI, both sides |
| Silkscreen | White, top side only |
| Min trace/space | 5 mil / 5 mil (0.127 mm) |
| Min via drill | 0.3 mm (12 mil) |
| Min via annular ring | 0.15 mm (6 mil) |
| Controlled impedance | Yes (USB D+/D- 90Ω diff, CAN 120Ω diff) |

## 3.1 Layer Stackup

```
Layer 1 (Top): Signal + Components
  ── 0.2 mm prepreg (2116) ──
Layer 2 (Inner 1): Solid GND Plane
  ── 0.5 mm core (FR-4) ──
Layer 3 (Inner 2): Power Plane (segmented: +3V3, +5V_CAN, +1V8)
  ── 0.2 mm prepreg (2116) ──
Layer 4 (Bottom): Signal + Components (low-speed, decoupling)
```

### Impedance Calculations (for 4-layer stackup above)

| Signal Type | Target Z | Trace Width | Differential Spacing | Layer |
|---|---|---|---|---|
| USB D+/D- (90Ω diff) | 90Ω ±10% | 0.30 mm | 0.20 mm gap | Top (L1), ref L2 GND |
| CAN-H/CAN-L (120Ω diff) | 120Ω ±5% | 0.25 mm | 0.30 mm gap | Top (L1), ref L2 GND |
| QSPI CLK (50Ω SE) | 50Ω ±15% | 0.35 mm | — | Top (L1), ref L2 GND |
| SPI signals (50Ω SE) | 50Ω ±15% | 0.35 mm | — | Top (L1), ref L2 GND |
| General digital | — | 0.20 mm (8 mil) | — | Any layer |

## 3.2 Board Outline and Mechanical

### Outline Drawing (ASCII, not to scale)

```
  0    5   10   15   20   25   30   35   40   45   50   55   60   65
  ┌────────────────────────────────────────────────────────────────┐ 0
  │  ╔══════════╗                                                  │
  │  ║  SSD1306 ║  ┌──────────┐  ┌──────────┐  ┌──────────┐     │ 5
  │  ║   OLED   ║  │ MCP2518  │  │ MCP2518  │  │ TJA1445  │     │
  │  ║  0.96"   ║  │  FD #0   │  │  FD #1   │  │   #0     │     │
  │  ╚══════════╝  └──────────┘  └──────────┘  └──────────┘     │10
  │                                                                │
  │  ┌──────────────────────────────────────┐  ┌──────────┐       │15
  │  │         nRF52840-QIAA                │  │ TJA1445  │       │
  │  │         7×7mm QFN-73                 │  │   #1     │       │
  │  │                                      │  └──────────┘       │20
  │  └──────────────────────────────────────┘                     │
  │                                                                │
  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐      │25
  │  │ APS6404L │  │ W25Q128  │  │ TPS63070 │  │ TPS7A4701│      │
  │  │  PSRAM   │  │  Flash   │  │ Buck-Boost│  │   LDO    │      │
  │  └──────────┘  └──────────┘  └──────────┘  └──────────┘      │30
  │                                                                │
  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐      │
  │  │ TP4056   │  │ DW01+FET │  │ AP2112K  │  │ USB-C    │      │35
  │  │ Charger  │  │ Protect  │  │ 5V LDO   │  │ Recept.  │      │
  │  └──────────┘  └──────────┘  └──────────┘  └──────────┘      │
  └────────────────────────────────────────────────────────────────┘
                                                                    35
```

### Mounting Holes
- 4× M2.5 mounting holes (3.0 mm drill, 5.0 mm pad) at corners: (3,3), (62,3), (3,32), (62,32)
- Keep-out zone: 4 mm radius around each hole (no components, no traces)

### Edge Connectors
- **USB-C**: Right edge, centered at y=17.5mm, x=65mm (board edge)
- **DB9 Female**: Left edge, centered at y=10mm, x=0mm (board edge, right-angle)
- **OBD-II Pigtail**: Exits through strain-relief slot at bottom edge, x=30-40mm
- **JST-PH Battery**: Bottom edge, x=50mm
- **Tag-Connect**: Top edge, x=55mm, y=0mm

## 3.3 Component Placement Strategy

### Zone Definitions

| Zone | Region (x,y) mm | Contents | Rationale |
|---|---|---|---|
| RF / BLE | (20-45, 0-10) | nRF52840, 32MHz crystal, antenna keep-out | Minimize digital noise near BLE antenna |
| CAN Interface 0 | (0-15, 5-15) | MCP2518FD #0, TJA1445 #0, 40MHz crystal, DB9 | Close to DB9 connector, short CAN traces |
| CAN Interface 1 | (50-65, 5-15) | MCP2518FD #1, TJA1445 #1, 40MHz crystal | Close to OBD-II exit |
| Memory | (20-45, 20-30) | APS6404L, W25Q128JV | Close to nRF52840 QSPI pins |
| Power | (45-65, 25-35) | TPS63070, TPS7A4701, AP2112K, TP4056, DW01, battery connector | Away from sensitive analog/RF |
| User Interface | (0-15, 0-5) | SSD1306 OLED, LEDs, button | Top-left for visibility |
| USB | (55-65, 15-20) | USB-C receptacle | Right edge |

### Critical Placement Rules

1. **nRF52840 32 MHz crystal**: Within 5mm of XL1/XL2 pins. Symmetric layout. No digital traces crossing crystal circuit. Guard ring connected to GND.
2. **nRF52840 antenna**: The QIAA package has no internal antenna — we use a PCB trace antenna in the top-left keep-out zone. Antenna matching network within 3mm of ANT pin.
3. **MCP2518FD crystals**: Within 5mm of XTAL1/XTAL2 pins. Keep away from CAN bus traces (noise coupling).
4. **TJA1445 transceivers**: As close as possible to CAN connectors. CAN-H/CAN-L traces: short (<15mm), wide (0.5mm), differential pair.
5. **QSPI memory devices**: Within 20mm of nRF52840 QSPI pins. Length-match CLK and IO[0:3] within 5mm.
6. **Decoupling capacitors**: On bottom layer directly under their IC power pins, connected with short vias.

## 3.4 High-Speed Routing Rules

### USB D+/D- (90Ω Differential Pair)
- Trace width: 0.30 mm, spacing: 0.20 mm
- Reference: Solid GND plane on Layer 2 (no splits under USB traces)
- Max length: 30 mm (nRF52840 to USB-C)
- No vias if possible; if unavoidable, both signals must transition together with symmetrical via placement
- Series resistors (22Ω): Place at nRF52840 end, within 5mm of D+/D- pins
- Keep away from: Switching regulators, CAN bus traces, crystal circuits

### QSPI Bus (80 MHz)
- CLK: 0.35 mm trace, 50Ω single-ended
- IO[0:3]: 0.20 mm traces, length-matched to CLK within ±5mm
- Max total length: 40 mm
- Daisy-chain topology: nRF52840 → APS6404L → W25Q128JV (shared CLK/IO, separate CS)
- Series termination: 33Ω at nRF52840 CLK driver
- No stubs on CLK line; IO lines can have short stubs to second device (<5mm)

### SPI Buses (8 MHz, less critical)
- Standard 0.20 mm traces, no length matching required
- Keep away from QSPI and USB to avoid crosstalk
- CS lines: 10kΩ pull-up to 3.3V near MCP2518FD

### CAN Bus Traces (Differential)
- CAN-H/CAN-L: 0.25 mm traces, 0.30 mm spacing (120Ω differential)
- Max length: 20 mm from TJA1445 to connector
- No vias; route on top layer only
- PESD1CAN-U ESD diodes: Place within 3mm of connector pins
- Common-mode choke (optional, provisioned footprint): DLW43SH101XK2 or similar, 0805 footprint between TJA1445 and connector

## 3.5 Via Strategy

| Via Type | Drill | Pad | Use |
|---|---|---|---|
| Standard signal via | 0.3 mm (12 mil) | 0.6 mm (24 mil) | General signal routing |
| Power via | 0.4 mm (16 mil) | 0.8 mm (32 mil) | Power net transitions, higher current |
| Thermal via | 0.3 mm (12 mil) | 0.6 mm (24 mil) | Under QFN exposed pads, arrays of 4-9 vias |
| Stitching via | 0.3 mm (12 mil) | 0.6 mm (24 mil) | GND plane stitching along board edges and between zones |
| Via-in-pad | Not used | — | Avoid for cost; use dog-bone fanout for QFN/BGA |

### Via Placement Rules
- Maximum 2 vias per high-speed signal (USB, QSPI CLK)
- GND stitching vias: Every 5-10mm along board perimeter, every 10mm between power/analog zones
- Thermal via arrays under:
  - nRF52840 center pad: 9 vias (3×3 grid)
  - TPS63070 exposed pad: 4 vias (2×2 grid)
  - TPS7A4701 exposed pad: 4 vias (2×2 grid)

## 3.6 Power Distribution Network (PDN)

### Power Plane Segmentation (Layer 3)

```
┌─────────────────────────────────────────────────────────────┐
│  +3V3 Rail (main)                                           │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Covers: nRF52840, MCP2518FDs, APS6404L, W25Q128JV, │   │
│  │  SSD1306, TJA1445 VIO, pull-ups                     │   │
│  │  Area: ~60% of board                                 │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                              │
│  +5V_CAN Rail                                                │
│  ┌──────────────────────────┐                               │
│  │  Covers: TJA1445 VCC pins│                               │
│  │  Area: ~10% of board     │                               │
│  └──────────────────────────┘                               │
│                                                              │
│  +1V8 Rail (provisioned)                                     │
│  ┌──────────┐                                               │
│  │  Small   │                                               │
│  │  island  │                                               │
│  └──────────┘                                               │
│                                                              │
│  VBAT Rail (unregulated)                                     │
│  ┌──────────────────────────────┐                           │
│  │  Covers: TPS63070 VIN,       │                           │
│  │  TP4056 BAT, battery conn    │                           │
│  └──────────────────────────────┘                           │
└─────────────────────────────────────────────────────────────┘
```

### PDN Target Impedance

| Rail | Max Current | Target Z (DC–1 MHz) | Target Z (1–100 MHz) |
|---|---|---|---|
| +3V3 | 150 mA | <50 mΩ | <200 mΩ |
| +5V_CAN | 20 mA | <100 mΩ | <500 mΩ |
| +1V8 | 30 mA | <100 mΩ | <500 mΩ |

Achieved through:
- Solid power planes (low DC resistance)
- Distributed decoupling capacitor network
- Multiple vias at each IC power pin (min 2 per pin for >50mA)

## 3.7 Thermal Management

### Heat Sources and Dissipation

| Component | Max Power Dissipation | Cooling Strategy |
|---|---|---|
| nRF52840 | ~30 mW (64 MHz + BLE TX) | QFN exposed pad → 9 thermal vias → GND plane |
| TPS63070 | ~200 mW (efficiency ~90% at 150mA) | VQFN exposed pad → 4 thermal vias → GND plane |
| TPS7A4701 | ~50 mW (3.3V→1.8V at 30mA) | VQFN exposed pad → 4 thermal vias → GND plane |
| TP4056 | ~600 mW (1A charge, 4.2V from 5V) | SOP-8 with copper pour on pins 4/5/6/7 (GND/BAT) |
| MCP2518FD | ~15 mW each | SOIC-14, no special cooling needed |
| TJA1445 | ~15 mW each | SOIC-8, no special cooling needed |

### Thermal Via Specifications
- Drill: 0.3 mm
- Pad: 0.6 mm (on all layers)
- Plating: 25 µm copper in barrel
- Spacing: 1.0 mm center-to-center
- Pattern: Grid aligned with IC exposed pad
- Connection: All thermal vias connect to solid GND plane on Layer 2

### Board-Level Thermal
- Max ambient: 85°C (automotive cabin temperature)
- Max junction temps: nRF52840=105°C, TPS63070=125°C, TP4056=120°C
- All components operate within spec at 85°C ambient with natural convection
- No heatsinks or forced air required

## 3.8 Isolation and Separation

### Analog/Digital Separation
- **Analog zone**: Battery voltage sense divider (P0.04), located near battery connector, away from digital
- **No dedicated analog ground** — single solid GND plane; analog return currents are low-frequency and low-magnitude
- **Guard ring**: Around 32 MHz and 32.768 kHz crystal circuits, connected to GND at single point

### CAN Bus Isolation
- TJA1445 provides galvanic isolation between CAN bus and logic
- CAN-H/CAN-L traces: Keep >2mm from any digital trace
- No digital traces crossing under or over CAN differential pair
- ESD protection diodes placed at connector entry point

### RF / BLE Isolation
- PCB antenna keep-out zone: 10mm × 10mm area in top-left corner
  - No copper on any layer in this zone
  - No components in this zone
  - No mounting holes in this zone
- nRF52840 ANT pin: Direct to antenna matching network, then to PCB antenna
- Digital traces: Keep >3mm from antenna matching network

## 3.9 PCB Antenna Design

### Meandered Inverted-F Antenna (MIFA)
- Type: Quarter-wave MIFA on top layer
- Dimensions: 8.5 mm × 5.5 mm (overall antenna area)
- Trace width: 0.5 mm
- Resonant length: ~28 mm (quarter-wave at 2.45 GHz on FR-4)
- Feed point: 50Ω microstrip from nRF52840 ANT pin
- Matching network: L-section (series 3.9nH inductor, shunt 1.2pF capacitor) — values tuned for this specific PCB
- Ground clearance: No GND plane on L2 under antenna area (10mm × 10mm)
- Antenna geometry:

```
Feed ──┐
       │  0.5mm wide trace
       ├── 2.0mm ──┐
       │           │ 1.5mm
       │           ├── 3.0mm ──┐
       │           │           │ 1.5mm
       │           │           ├── 4.0mm ──┐
       │           │           │           │ 1.5mm
       │           │           │           ├── 5.0mm ── open end
       │           │           │           │
       └───────────┴───────────┴───────────┘
       ←────────── 8.5mm total ──────────→
```

## 3.10 Design Rule Check (DRC) Parameters

| Rule | Value |
|---|---|
| Min track width | 0.127 mm (5 mil) |
| Min track spacing | 0.127 mm (5 mil) |
| Min via drill | 0.3 mm |
| Min via diameter | 0.6 mm |
| Min via spacing | 0.5 mm |
| Min annular ring | 0.15 mm |
| Min solder mask sliver | 0.1 mm |
| Min silkscreen text height | 0.8 mm |
| Min silkscreen text thickness | 0.15 mm |
| Copper-to-edge clearance | 0.5 mm |
| Hole-to-edge clearance | 1.0 mm |
| Courtyard overlap | 0.25 mm minimum |

## 3.11 Manufacturing Notes

### Panelization
- Single board per panel for prototype
- Production: 2×3 array (6 boards per panel), V-score separation
- Panel size: ~140mm × 110mm
- Tooling holes: 4× 3.0mm at panel corners
- Fiducials: 3× 1.0mm circular fiducials on top and bottom

### Solder Paste Stencil
- Thickness: 0.12 mm (5 mil) stainless steel
- Aperture reduction: 80% for QFN center pads (to prevent voiding)
- nRF52840 QFN-73: 0.25mm pitch, requires high-precision stencil

### Assembly Sequence
1. **Bottom side first**: Decoupling capacitors (0402/0603), passives
2. **Top side**: All ICs, connectors, crystals, LEDs
3. **Through-hole**: DB9 connector, JST-PH battery connector, OBD-II pigtail (hand-solder)
4. **Inspection**: AOI for SMT, visual for THT
5. **Programming**: SWD via Tag-Connect before battery connection

### Test Points
- +3V3 test pad (1.0mm diameter) near TPS63070
- +5V_CAN test pad near AP2112K
- VBAT test pad near battery connector
- GND test pads (×3, distributed)
- CAN0_H, CAN0_L test pads near DB9
- CAN1_H, CAN1_L test pads near OBD-II exit
- SWDIO, SWDCLK test pads (redundant with Tag-Connect)

## 3.12 Signal Integrity Considerations

### Critical Nets Requiring SI Analysis

| Net | Concern | Mitigation |
|---|---|---|
| QSPI CLK (80 MHz) | Reflections, crosstalk | 33Ω source series termination, length-matched IO, continuous GND reference |
| USB D+/D- (12 Mbps) | Impedance discontinuity | Controlled 90Ω diff pair, no vias, continuous GND reference |
| CAN-H/CAN-L | External noise pickup | Differential routing, ESD protection at connector, common-mode choke footprint |
| 32 MHz crystal | Frequency pulling | Short traces, symmetric layout, guard ring, no digital crossings |
| nRF52840 DC/DC (DCDCEN) | Switching noise | Inductor close to DEC4 pin, keep away from antenna and crystals |

### Crosstalk Budget
- Aggressor: QSPI CLK (80 MHz, 3.3V swing, ~1ns rise time)
- Victim: USB D+/D- (12 Mbps, 3.3V swing)
- Minimum separation: 3mm (achieved by routing QSPI on left side, USB on right side)
- Victim: CAN bus traces
- Minimum separation: 5mm (CAN traces on far left/right edges, QSPI in center)

## 3.13 Flex and Rigid-Flex Considerations
- Not applicable for Rev A — standard rigid FR-4
- Future Rev B may use rigid-flex for OBD-II dongle form factor (rigid section for PCB, flex tail for OBD-II connector)

## 3.14 Compliance Targets
- **FCC Part 15.247**: BLE intentional radiator, PCB antenna, <0dBm conducted power
- **CE RED**: EN 300 328 (BLE), EN 301 489 (EMC)
- **RoHS 3**: All components RoHS compliant
- **CAN bus**: ISO 11898-2:2016 physical layer compliance (via TJA1445)
- **USB-IF**: Not certified (CDC-ACM class, no logo required for research device)
