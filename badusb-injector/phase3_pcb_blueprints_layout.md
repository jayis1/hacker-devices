# PHANTOM — Phase 3: PCB Blueprints & Layout

## 1. PCB Specifications

| Parameter | Value |
|-----------|-------|
| **Dimensions** | 50 mm × 25 mm |
| **Form factor** | USB-A stick (like standard flash drive) |
| **Layers** | 4 |
| **Stackup** | SIG-GND-PWR-SIG |
| **Board thickness** | 1.6 mm |
| **Copper weight** | 1 oz (35 µm) all layers |
| **Surface finish** | ENIG (Electroless Nickel Immersion Gold) |
| **Solder mask** | Green (or black for stealth) |
| **Silkscreen** | White |
| **Minimum trace width** | 0.15 mm (6 mil) |
| **Minimum trace spacing** | 0.15 mm (6 mil) |
| **Minimum via** | 0.4 mm drill, 0.6 mm pad |
| **Impedance control** | USB D+/D- 90Ω differential |
| **Design for** | JLCPCB 4-layer, ENIG |

## 2. PCB Stackup

```
┌─────────────────────────────────────────────────────┐
│ Layer 1 (Top)    │ F.Cu    │ Signal / Components    │
│                   │         │ RP2040, passives, etc. │
├───────────────────┼─────────┼───────────────────────┤
│                   │ Prepreg │ 0.1 mm, εr=4.4       │
├───────────────────┼─────────┼───────────────────────┤
│ Layer 2 (Inner1)  │ In1.Cu  │ GND Plane             │
│                   │         │ Solid ground pour      │
├───────────────────┼─────────┼───────────────────────┤
│                   │ Core    │ 0.5 mm, εr=4.4        │
├───────────────────┼─────────┼───────────────────────┤
│ Layer 3 (Inner2)  │ In2.Cu  │ Power Plane            │
│                   │         │ VDD_3V3, VDD_1V8 split │
├───────────────────┼─────────┼───────────────────────┤
│                   │ Prepreg │ 0.1 mm, εr=4.4        │
├───────────────────┼─────────┼───────────────────────┤
│ Layer 4 (Bottom)  │ B.Cu    │ Signal / Routing       │
│                   │         │ ESP32-C3 antenna area  │
└─────────────────────────────────────────────────────┘

Total thickness: 1.6 mm ± 0.1 mm
```

### 2.1 Layer Assignments

| Layer | Primary Use | Notes |
|-------|------------|-------|
| F.Cu (Top) | Component placement, short traces, USB differential pair | RP2040, flash, passives |
| In1.Cu (L2) | Solid GND plane | No signal traces, via stitching |
| In2.Cu (L3) | Power plane (VDD_3V3, VDD_1V8) | Split plane, bridge between rails |
| B.Cu (Bottom) | ESP32-C3 module, encoder, routing | Keep-out under ESP32 antenna |

### 2.2 Impedance Calculation

For USB 2.0 Full-Speed differential pair on top layer:

```
Target: Zdiff = 90Ω (±10%)
Stackup: 1 oz copper, 0.1 mm prepreg (εr = 4.4)

Calculation (microstrip edge-coupled):
  Trace width: 0.18 mm (7 mil)
  Trace gap: 0.12 mm (5 mil)
  Height above GND: 0.1 mm (prepreg)
  
Result: Zdiff ≈ 92Ω (within spec)

For 50Ω single-ended:
  Trace width: 0.28 mm (11 mil)
  Height above GND: 0.1 mm (prepreg)
  
Result: Z0 ≈ 52Ω (within USB spec)
```

## 3. Component Placement

### 3.1 Top Side Layout

```
┌─────────────────────────────────────────────────────────┐
│  USB-A Edge Connector                                    │
│  ┌────┐                                                  │
│  │ VBUS D- D+ GND                                       │
│  └────┘                                                  │
│                                                          │
│  ┌──────────────┐  ┌──────┐  ┌───────────────────────┐ │
│  │   RP2040      │  │ TS3  │  │  W25Q128 + W25Q16    │ │
│  │   (QFN-56)    │  │ USB2 │  │  (SOIC-8 × 2)        │ │
│  │               │  │ 221A │  │                       │ │
│  │               │  │      │  │                       │ │
│  └──────────────┘  └──────┘  └───────────────────────┘ │
│                                                          │
│  ┌─────┐ ┌──────┐ ┌──────┐ ┌───────┐ ┌──────────────┐ │
│  │Y1   │ │AP2112│ │AP2112│ │MCP7383│ │ EC11 Encoder │ │
│  │12MHz│ │ 3.3V │ │ 1.8V │ │  1    │ │   (side)     │ │
│  └─────┘ └──────┘ └──────┘ └───────┘ └──────────────┘ │
│                                                          │
│  ┌──┐  ┌──┐  ┌──┐                                       │
│  │SW1│  │D1│  │C │  (decoupling caps)                  │
│  │Kill│  │LED│  │  │                                      │
│  └──┘  └──┘  └──┘                                       │
│                                                          │
│  BAT1 (500mAh LiPo, bottom side or ZIF connector)      │
└─────────────────────────────────────────────────────────┘
```

### 3.2 Bottom Side Layout

```
┌─────────────────────────────────────────────────────────┐
│                                                          │
│  ┌──────────────────────────────────────────┐           │
│  │         ESP32-C3-MINI-1 Module           │           │
│  │         (WiFi 6 + BLE 5)                 │           │
│  │                                          │           │
│  │    ┌─────────────────────────────┐      │           │
│  │    │   PCB Trace Antenna         │      │           │
│  │    │   (2.4 GHz inverted-F)     │      │           │
│  │    │   ← KEEP COPPER CLEAR →    │      │           │
│  │    └─────────────────────────────┘      │           │
│  └──────────────────────────────────────────┘           │
│                                                          │
│  ┌──────────┐                                           │
│  │ SSD1306  │  (OLED display, FPC connector or COG)     │
│  │ 128×64   │                                           │
│  └──────────┘                                           │
│                                                          │
│  (Passive components, routing)                          │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

### 3.3 Placement Constraints

| Component | Constraint | Reason |
|-----------|-----------|--------|
| RP2040 | Center of top side, >3mm from USB edge | Thermal, routing |
| W25Q16 | Within 5mm of RP2040 QSPI pins | Short boot flash traces |
| W25Q128 | Within 10mm of RP2040 SPI0 pins | Short SPI traces |
| TS3USB221A | Within 10mm of USB-A connector | Short USB diff pair |
| ESP32-C3 | Bottom side, antenna at board edge | RF performance |
| Y1 (12MHz) | Within 3mm of RP2040 XIN/XOUT | Clock integrity |
| Decoupling caps | Within 2mm of IC VDD pins | Minimize inductance |
| SSD1306 | Bottom side, opposite end from USB | Display visibility |

## 4. Routing Rules

### 4.1 USB Differential Pair

```
Rule: USB D+/D- must be routed as 90Ω differential pair

Specifications:
  - Single-ended impedance: 45Ω ± 10%
  - Differential impedance: 90Ω ± 10%
  - Max skew: 0.15 mm (6 mil)
  - Max length: 50 mm (well within spec)
  - No vias on diff pair (route on top layer only)
  - No 90° bends (use 45° or curved)
  - Guard traces: GND pour on adjacent layers
  
Routing:
  RP2040 USB_DP/DM → TS3USB221A → USB-A connector
  Length: ~15 mm total
  No stubs, no test points on diff pair
```

### 4.2 QSPI Boot Flash (RP2040 → W25Q16)

```
Rule: QSPI traces must be length-matched ±0.5 mm

Specifications:
  - Max frequency: 133 MHz (overclock)
  - Target impedance: 50Ω single-ended
  - Length matching: SCLK, SS, SD0-SD3 within ±0.5 mm
  - Max trace length: 25 mm
  - Series termination: 33Ω if signal integrity issues
```

### 4.3 SPI0 Payload Flash (RP2040 → W25Q128)

```
Rule: SPI traces should be short and direct

Specifications:
  - Max frequency: 50 MHz (SPI Mode 0)
  - Target impedance: 50Ω single-ended
  - Max trace length: 20 mm
  - CS line: direct route, no stubs
```

### 4.4 I²C Bus (RP2040 → SSD1306)

```
Rule: I²C traces with pull-ups near master

Specifications:
  - Max frequency: 400 kHz (Fast Mode)
  - Pull-up resistors: 4.7 kΩ to VDD_3V3
  - Place pull-ups within 5mm of RP2040 pins
  - Target impedance: not critical at 400 kHz
```

### 4.5 UART (RP2040 → ESP32-C3)

```
Rule: Short, direct UART connection

Specifications:
  - Baud rate: 115200 (max 460800)
  - TX/RX traces: direct, no length matching needed
  - Cross-connected: RP2040 TX → ESP32-C3 RX
```

### 4.6 General Routing Rules

| Rule | Value | Exception |
|------|-------|-----------|
| Minimum trace width | 0.15 mm (6 mil) | Power: 0.4 mm, USB diff: 0.18 mm |
| Minimum trace spacing | 0.15 mm (6 mil) | USB diff gap: 0.12 mm |
| Minimum via drill | 0.2 mm | Power via: 0.3 mm |
| Minimum via pad | 0.4 mm | Power via pad: 0.6 mm |
| Keepout from board edge | 0.5 mm | USB connector |
| Keepout from ESP32 antenna | 5 mm copper clear | — |
| Decoupling cap distance | < 2 mm from IC pin | — |
| Ground via stitching | Every 2 mm around board edge | — |
| Thermal via under RP2040 | 4× 0.3mm drill, 0.6mm pad | Thermal pad on bottom |

### 4.7 Power Routing

| Net | Width | Via | Notes |
|-----|-------|-----|-------|
| VBUS_5V | 0.6 mm | 0.6 mm pad, 0.3 mm drill | USB power, high current |
| VBAT | 0.4 mm | 0.6 mm pad, 0.3 mm drill | Battery, moderate current |
| VDD_3V3 | 0.4 mm (plane) | Stitching vias | Main power plane on In2.Cu |
| VDD_1V8 | 0.3 mm (plane) | Stitching vias | Core voltage, split plane |
| GND | Plane | Stitching vias | Continuous on In1.Cu |

## 5. Thermal Management

### 5.1 RP2040 Thermal

| Parameter | Value |
|-----------|-------|
| Max junction temperature | 125°C |
| Thermal resistance (QFN-56) | θJA ≈ 45°C/W |
| Max power dissipation | ~200 mW (133 MHz, dual core active) |
| Expected temperature rise | ~9°C above ambient |
| Thermal pad | Exposed pad on bottom, 4 thermal vias to GND plane |
| No heatsink required | Within spec for commercial temperature range |

### 5.2 ESP32-C3 Thermal

| Parameter | Value |
|-----------|-------|
| Max power dissipation | ~600 mW (WiFi TX burst) |
| Expected temperature rise | ~27°C above ambient |
| Thermal mitigation | Power plane copper pour, via stitching |
| No heatsink required | Within spec for 0-60°C range |

### 5.3 Battery Thermal

| Parameter | Value |
|-----------|-------|
| Charge current | 470 mA |
| Max charge temperature | 45°C (MCP73831 thermal shutdown) |
| Battery location | Away from heat-generating components |
| Air gap | 1 mm minimum from RP2040 and ESP32-C3 |

## 6. Mechanical Design

### 6.1 Board Outline

```
    0,0 ─────────────────────────────────────────────── 50,0
     │                                                     │
     │  ┌──── USB-A Plug (PCB edge, 12mm wide) ────┐     │
     │  │  VBUS  D-  D+  GND                        │     │
     │  │  (gold fingers, 2.54mm pitch)              │     │
     │  └────────────────────────────────────────────┘     │
     │                                                     │
     │  Component Area (Top)                               │
     │  RP2040, Flash, Power, USB Switch                   │
     │                                                     │
     │  Component Area (Top)                               │
     │  Encoder, Crystal, Kill Switch, LED                 │
     │                                                     │
     │  ┌──── OLED Cutout ────┐                           │
     │  │  10mm × 7mm          │                           │
     │  └──────────────────────┘                           │
     │                                                     │
     │                                                     │
    0,25 ─────────────────────────────────────────────── 50,25
```

### 6.2 USB-A Connector

The USB-A connector is implemented as PCB edge fingers:

```
Pin assignment (looking at plug face):
  Pin 1: VBUS (+5V) — 2.0 mm wide
  Pin 2: D- (data)  — 2.0 mm wide  
  Pin 3: D+ (data)  — 2.0 mm wide
  Pin 4: GND        — 2.0 mm wide

Spacing: 2.54 mm between pin centers
Gold plating: 30 µin hard gold over 100 µin nickel
Beveled edge: 45° chamfer on insertion end
```

### 6.3 Mounting Holes

| Location | Size | Type |
|----------|------|------|
| None | — | USB-A stick form factor, no mounting holes |

*Note: The board relies on the USB-A plug for mechanical retention. Battery is held by adhesive pad.*

### 6.4 Height Constraints

| Side | Max Height | Component |
|------|-----------|-----------|
| Top | 4.0 mm | RP2040 QFN-56, tallest component |
| Bottom | 3.2 mm | ESP32-C3-MINI-1 module |
| Total | 12.0 mm | Including USB-A housing |

### 6.5 Keepout Zones

| Zone | Location | Reason |
|------|----------|--------|
| ESP32 antenna | Bottom, last 15mm of board edge | RF performance |
| USB fingers | Top, first 5mm from insertion edge | USB connector |
| Battery | Designated area, no traces on bottom | Adhesive mounting |

## 7. DFM Notes

### 7.1 Assembly

- **Process**: SMT only, no through-hole components
- **Solder paste**: SAC305, type 4, 0.1mm stencil
- **Reflow profile**: Standard lead-free (260°C peak)
- **Double-sided**: Yes — top side reflow first, bottom side second (using red adhesive for ESP32-C3 module)
- **Inspection**: AOI for component placement, X-ray for QFN voiding
- **Test**: ICT bed of nails + USB enumeration test

### 7.2 Panelization

- **Panel size**: 100 mm × 100 mm (standard small batch)
- **Boards per panel**: 8 (2 × 4 grid)
- **Breakaway tabs**: 5mm mouse bites on two edges
- **Fiducials**: 3 on panel, 2 per board
- **Tooling holes**: 2 per panel (3mm diameter)

### 7.3 Cost Optimization

| Item | Cost | Notes |
|------|------|-------|
| PCB (4-layer, ENIG) | $1.50 | 50×25mm, JLCPCB |
| SMT assembly | $3.00 | Per board at 1K volume |
| Components | $17.72 | BOM total |
| Battery | $3.00 | 500mAh LiPo |
| Testing | $1.00 | USB enumeration + flash |
| **Total** | **~$26.22** | Assembled, tested |

### 7.4 Yield Considerations

- RP2040 QFN-56: Thermal pad requires good solder coverage (>70%)
- ESP32-C3 module: Pre-certified, no RF tuning needed
- USB fingers: Hard gold plating (30µin) for durability
- Crystal: Verify startup margin (ESR < 100Ω)
- Battery connector: JST PH 2-pin, right-angle for low profile

## 8. Schematic Verification Checklist

- [ ] All RP2040 VDD pins have 100nF decoupling caps within 2mm
- [ ] USB D+/D- routed as 90Ω differential pair, no vias
- [ ] QSPI boot flash within 5mm of RP2040 QSPI pins
- [ ] I²C pull-ups (4.7kΩ) on SDA and SCL lines
- [ ] Kill switch properly disconnects USB data via TS3USB221A
- [ ] MCP73831 charge current set by R5 (27.4kΩ = 470mA)
- [ ] Crystal load capacitors calculated for 12.5pF load (C = 2×(Cload - Cstray))
- [ ] ESP32-C3 antenna keepout area clear on all layers
- [ ] Battery protection: MCP73831 includes undervoltage lockout
- [ ] All unused GPIO pins have pull-down resistors or configured as input with pull-up
- [ ] USB VBUS has 10µF bulk capacitor and ferrite bead
- [ ] RP2040 RUN pin has 10kΩ pull-up to VDD_3V3
- [ ] W25Q128 WP and HOLD pins pulled to VDD_3V3 (always write-enabled, always running)
- [ ] WS2812B data line has 100Ω series resistor near source
- [ ] Rotary encoder has 100nF debounce capacitors on A and B phases