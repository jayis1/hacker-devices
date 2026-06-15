# Infrared Phantom — Phase 3: PCB Blueprints & Layout

## 1. PCB Stackup

| Layer | Material | Thickness | Copper | Purpose |
|-------|----------|-----------|--------|---------|
| Top (L1) | — | — | 1 oz (35 µm) | Signal routing, component pads |
| PP1 | FR-4 1705 | 0.10 mm | — | Prepreg |
| L2 | FR-4 core | 0.20 mm | 1 oz (35 µm) | Ground plane (unsplit) |
| PP2 | FR-4 1705 | 0.10 mm | — | Prepreg |
| L3 | FR-4 core | 0.20 mm | 1 oz (35 µm) | Power plane (3V3, VBUS, 1V8) |
| PP3 | FR-4 1705 | 0.10 mm | — | Prepreg |
| Bottom (L4) | — | — | 1 oz (35 µm) | Signal routing, IR LED/reflector pads |

**Total board thickness:** 1.0 mm (suitable for USB-A plug insertion)

**Dielectric constant (Er):** 4.5 @ 1 GHz  
**Loss tangent:** 0.02 @ 1 GHz  
**Controlled impedance:** 50 Ω single-ended, 90 Ω differential (USB)

### Stackup Cross-Section

```
L1 (Top)   ━━━━━━━━━━━━━━━━━━━━━━  35 µm Cu  (Signals, components)
            ┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄  100 µm PP (FR-4 1705)
L2         ═══════════════════════  35 µm Cu  (GND plane)
            ┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄  200 µm core (FR-4)
L3         ═══════════════════════  35 µm Cu  (Power plane)
            ┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄  100 µm PP (FR-4 1705)
L4 (Bot)   ━━━━━━━━━━━━━━━━━━━━━━  35 µm Cu  (Bottom signals, IR pads)
            ┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄  Solder mask
```

## 2. Board Outline

```
    ┌────────────────────────────────────────────────────────┐
    │ ←──────────── 85.0 mm ──────────────→                  │
    │                                                        │
    │  ┌──────────────────────────────────────────────────┐  │
    │  │                                                    │  │
    │  │  Component Side (Top)                              │  │
    │  │                                                    │ │34│
    │  │  ┌────┐  ┌─────┐   ┌─────┐   ┌──────┐           │  │m│
    │  │  │nRF │  │STM32│   │USB33│   │W25Q  │           │  │m│
    │  │  │5284│  │H743 │   │20   │   │128   │           │  │ │
    │  │  └────┘  └─────┘   └─────┘   └──────┘           │  │ │
    │  │                                                    │  │ │
    │  │  ┌────┐  ┌─────┐   ┌─────┐   ┌──────┐           │  │ │
    │  │  │TLV │  │SSD  │   │micro│   │SDcard│           │  │ │
    │  │  │6256│  │1306 │   │SD   │   │detect│           │  │ │
    │  │  └────┘  └─────┘   └─────┘   └──────┘           │  │ │
    │  │                                                    │  │ │
    │  │  ○ TSMP58000          ○ ○ 27 MHz XTAL             │  │ │
    │  │                                                    │  │ │
    │  └──────────────────────────────────────────────────┘  │
    │                                                        │
    │  ┌──┐                                                  │
    │  │US│── USB-A plug (edge, 12 mm)                       │
    │  │B │                                                  │
    │  └──┘                                                  │
    └────────────────────────────────────────────────────────┘

Bottom side (L4):
    ┌──────────────────────────────────────────────────────┐
    │                                                       │
    │   ◉ ◉ ◉    (IR LED Array, 3×3 grid, 10mm pitch)    │
    │   ◉ ◉ ◉                                              │
    │   ◉ ◉ ◉                                              │
    │                                                       │
    │   ○ ○ 25 MHz XTAL   ○ ○ 32.768 kHz XTAL            │
    │                                                       │
    │   [User Button]   [Status LED]   [Error LED]        │
    │                                                       │
    └───────────────────────────────────────────────────────┘
```

**Board dimensions:** 85.0 mm × 34.0 mm × 1.0 mm  
**USB-A plug:** 12.0 mm extension from board edge, gold-plated fingers  
**Corners:** 1.0 mm radius (all corners)  
**Mounting holes:** None (USB-A insertion provides mechanical retention)

## 3. High-Speed Routing Rules

### 3.1 USB 2.0 High-Speed (480 Mbps)

| Parameter | Value | Source |
|-----------|-------|--------|
| Differential impedance | 90 Ω ±10% | USB 2.0 spec |
| Single-ended impedance | 45 Ω ±10% | USB 2.0 spec |
| Max trace length | 150 mm | Practical limit |
| Length matching | Within 0.5 mm | Differential pair |
| Pair spacing | 0.15 mm (6 mil) | Coupled microstrip |
| Trace width | 0.20 mm (8 mil) | Calculated for 1.0 mm stackup |
| Via count | Max 2 per net | Minimize discontinuity |
| ESD clamp | PRTR5V0U2X, 0.5 mm from connector | Signal integrity |

**USB routing path:**
```
USB-A plug fingers → PRTR5V0U2X (ESD) → R4/R5 (27.4 Ω series) → USB3320 DP/DM
```

### 3.2 ULPI Interface (60 MHz, 8-bit parallel)

| Parameter | Value |
|-----------|-------|
| Trace width | 0.20 mm (8 mil) |
| Impedance | 50 Ω single-ended ±10% |
| Length matching | Within 50 mil (1.27 mm) across all 12 signals |
| Max skew | 200 ps |
| Series termination | Not required (ULPI is source-terminated) |

**ULPI signal group:** CLK, STP, DIR, NXT, D[7:0] — all length-matched.

### 3.3 SPI Buses

| Bus | Max Clock | Trace Width | Max Length | Notes |
|-----|-----------|-------------|------------|-------|
| SPI1 (NOR Flash) | 50 MHz | 0.20 mm | 50 mm | Clock, MOSI length-matched within 5 mil |
| SPI2 (OLED) | 10 MHz | 0.20 mm | 30 mm | Non-critical, short route |
| SDMMC1 (microSD) | 50 MHz | 0.20 mm | 50 mm | All 6 signals length-matched within 10 mil |

### 3.4 ADC Input (IR Analog)

| Parameter | Value |
|-----------|-------|
| Trace width | 0.25 mm (10 mil) |
| Guard ring | 0.5 mm copper pour tied to GND |
| Length | < 10 mm from TSMP58000 to STM32 pin |
| Decoupling | 100 nF within 2 mm of TSMP58000 VCC pin |
| Analog supply | Separate VDDA pin, filtered from digital 3V3 |

## 4. Via Strategy

| Via Type | Drill | Pad | Usage |
|----------|-------|-----|-------|
| Standard via | 0.2 mm | 0.4 mm | General signal routing |
| Thermal via | 0.3 mm | 0.6 mm | Power regulator pads (TLV62569, GND) |
| USB via | 0.2 mm | 0.4 mm | USB diff pair (max 2 per net) |

**Via placement rules:**
- No vias under BGA/QFN center pads
- Via-in-pad allowed for QFN thermal pads (plugged and plated over)
- Minimum 0.5 mm via spacing
- Maximum 4 vias in series for any signal net
- GND vias placed every 3 mm along board edges for stitching

**Via stitching plan:**
- Ground plane (L2): via fence around board perimeter, 3 mm spacing
- Power plane (L3): bypass vias at every decoupling capacitor
- USB return path: continuous GND via row along USB diff pair route

## 5. Thermal Management

### 5.1 Thermal Sources

| Component | Power Dissipation | Thermal Strategy |
|-----------|-------------------|-----------------|
| STM32H743 | 600 mW (480 MHz) | Exposed pad → thermal vias → L2 GND pour |
| USB3320 | 150 mW | QFN pad → thermal vias → L2 GND pour |
| TLV62569 | 100 mW (3V3 @ 400 mA) | SOT-23-6, copper pour pour on L1 |
| nRF52840 | 80 mW (BLE TX) | M.2 module, PCB copper heatsink |
| IR LED array | 3 W peak (10% duty = 300 mW avg) | Large copper pour on L4, 9× thermal pads |

### 5.2 Thermal Via Arrays

**STM32H743VIT6 (LQFP-100):**
- No exposed center pad (LQFP), but bottom-side copper pour under IC
- 4× thermal vias from top pour to L2 GND at each corner of IC footprint
- Top copper pour: 15 mm × 15 mm, connected to GND

**IR LED Array (Bottom side):**
- Each TSAL6100 has a large cathode pad (thermal)
- Copper pour on L4 under LED array: 25 mm × 25 mm
- 6× thermal vias from LED pads to L2 GND for heat spreading
- Peak thermal resistance: < 30 K/W per LED

### 5.3 Temperature Budget

| Component | Tambient | Tjunction_max | ΔT margin |
|-----------|----------|--------------|-----------|
| STM32H743 | 50°C | 105°C | 55°C |
| USB3320 | 50°C | 85°C | 35°C |
| TLV62569 | 50°C | 125°C | 75°C |
| IR LEDs | 50°C | 100°C | 50°C (at 10% duty) |

## 6. Isolation Barriers

### 6.1 Analog/Digital Isolation

```
┌─────────────────────────────────────────────┐
│ ANALOG DOMAIN                    │ DIGITAL DOMAIN │
│                                  │                │
│  TSMP58000 ─── ADC1_IN7 ────────│── DMA → SRAM  │
│  (VDDA filtered)                │                │
│                                  │                │
│  Guard ring around analog       │                │
│  traces (GND pour)              │                │
│                                  │                │
│  Ferrite bead (FB1) between     │                │
│  VDD and VDDA                   │                │
└─────────────────────────────────────────────┘
```

**VDDA filtering:**
- FB1 (BLM21PG221SN1, 220 Ω @ 100 MHz) between 3V3 and VDDA
- C31 (10 µF) + C32 (100 nF) on VDDA side of FB1
- Guard ring: 0.5 mm GND pour around all analog traces

### 6.2 Power Domain Isolation

| Domain | Source | Isolation | Purpose |
|--------|--------|-----------|---------|
| VDD (3.3 V) | TLV62569 buck | L1 C110 µF + C8 10 µF | MCU + digital logic |
| VDDA (3.3 V) | VDD via FB1 | Ferrite + 10 µF + 100 nF | ADC reference, analog input |
| VDDCORE (1.8 V) | LP3985 LDO | 10 µF output cap | STM32 core logic |
| VBUS_LED (5 V) | USB VBUS direct | MOSFET switched | IR LED array (high current) |

### 6.3 IR LED High-Current Isolation

- IR LED array powered directly from VBUS (5 V), not from 3.3 V rail
- MOSFET gate driven from STM32 GPIO via BC847C NPN
- High-current loop: VBUS → MOSFET → LEDs → GND (short, wide traces on L4)
- Gate drive loop: STM32 GPIO → BC847C → MOSFET gate (L1, isolated from LED loop)
- GND return for LEDs: dedicated trace to USB connector GND, avoids MCU ground bounce

## 7. Design for Manufacturing (DFM) Notes

### 7.1 Minimum Feature Sizes

| Feature | Size | Notes |
|---------|------|-------|
| Trace width (signal) | 0.20 mm (8 mil) | Standard for 50 Ω impedance |
| Trace width (power) | 0.40 mm (16 mil) | VBUS, 3V3, GND |
| Trace width (LED) | 1.00 mm (40 mil) | High-current LED traces |
| Via drill | 0.20 mm | Standard via |
| Via pad | 0.40 mm | Standard pad |
| Pad-to-pad spacing | 0.20 mm | Fine-pitch QFN |
| Solder mask opening | 0.05 mm over pad | Standard |
| Silkscreen text | 0.80 mm height min | Component reference designators |

### 7.2 Assembly Notes

- All components on top side except IR LEDs (bottom side, THT leads bent 90°)
- USB-A plug: edge-finger plating, 2 µm hard gold over 5 µm nickel
- Reflow: standard lead-free SAC305 profile (260°C peak)
- THT IR LEDs: wave-soldered on bottom side after reflow
- OLED module: FPC connector, post-reflow hand-placed
- microSD connector: through-hole mounting tabs for mechanical strength
- QFN thermal pads: via-in-pad, plugged and plated over

### 7.3 Panelization

- Panel size: 100 mm × 80 mm (fits 2×2 array)
- Breakaway tabs: 2 mm width, 0.5 mm mouse bites (5 holes per tab)
- Fiducials: 3× on panel edge, 1 mm copper circle, 2 mm solder mask opening
- Tooling holes: 4× 3.2 mm on panel corners

## 8. Component Placement Plan

### 8.1 Top Side Placement (L1)

```
    USB-A Plug (left edge)
    ╔════════════╗
    ║            ║
    ║ ┌──────┐   ║
    ║ │USB3320│  ║   ← Near USB connector, short DP/DM traces
    ║ └──────┘   ║
    ║             ║
    ║ ┌────────────────┐
    ║ │  STM32H743VIT6  │   ← Center of board, short routes to all peripherals
    ║ └────────────────┘
    ║             ║
    ║ ┌─────┐ ┌──────┐ ┌──────┐ ┌─────────┐
    ║ │nRF  │ │W25Q  │ │TLV   │ │microSD  │
    ║ │52840│ │128   │ │62569 │ │slot     │
    ║ └─────┘ └──────┘ └──────┘ └─────────┘
    ║                                    ║
    ║ ┌─────┐  ┌──────┐                  ║
    ║ │SSD  │  │TSMP  │                  ║
    ║ │1306 │  │58000 │  ← Near board edge, IR RX side
    ║ └─────┘  └──────┘                  ║
    ║             ║
    ║ [27MHz XTAL] ← Near USB3320       ║
    ║ [25MHz XTAL] ← Near STM32         ║
    ╚═════════════════════════════════════╝
```

### 8.2 Bottom Side Placement (L4)

```
    ╔══════════════════════════════════════════╗
    ║                                          ║
    ║   ◉ ◉ ◉    ◉ ◉ ◉    ◉ ◉ ◉              ║   ← IR LED array (TSAL6100 ×9)
    ║                                          ║   ← 3×3 grid, 10mm pitch
    ║   ┌──────┐    ┌──────┐                  ║
    ║   │25MHz │    │32kHz │                  ║   ← Crystals
    ║   │XTAL  │    │XTAL  │                  ║
    ║   └──────┘    └──────┘                  ║
    ║                                          ║
    ║   [SW1]   ●GRN   ●RED                  ║   ← User button + status LEDs
    ║   Button  LED     LED                   ║
    ║                                          ║
    ╚══════════════════════════════════════════╝
```

## 9. Routing Priority Order

1. **USB diff pair (D+/D–)** — Shortest path, impedance-controlled, length-matched
2. **ULPI bus** — Length-matched across all 12 signals, near USB3320
3. **ADC input (IR_ANALOG)** — Guarded analog trace, < 10 mm, isolated from digital
4. **IR PWM output** — Short gate drive, minimal loop area
5. **SPI1 (NOR flash)** — Length-matched clock/data
6. **SDMMC1 (microSD)** — Length-matched clock/data
7. **SPI2 (OLED)** — Non-critical, short route
8. **USART1 (nRF52840)** — Non-critical, 115200 baud
9. **Power traces** — Wide traces, adequate current capacity
10. **GPIO** — Non-critical, fill remaining space

## 10. Ground Plane Strategy

### 10.1 L2 Ground Plane

- Continuous unsplit ground plane covering entire board area
- No traces on L2 except ground connections
- Via stitching every 3 mm along board perimeter
- Anti-pad clearances around signal vias (0.3 mm annular ring)

### 10.2 L3 Power Plane

- Split plane: 3V3 (70%), VBUS (25%), 1V8 (5%)
- 3V3 pour under MCU and most ICs
- VBUS pour under USB connector and MOSFET/LED area
- 1V8 pour under STM32 VDDCORE pins
- 2 mm clearance between power domains

### 10.3 Ground Return Paths

- USB return current: continuous GND plane under diff pair
- IR LED return: dedicated wide GND trace (1.0 mm) on L4 from LED cathodes to USB GND
- ADC return: analog guard ring connected to GND plane at single point near ADC pin

## 11. Design Rules Summary

| Rule | Value | Layer |
|------|-------|-------|
| Minimum trace width | 0.20 mm | All signal layers |
| Minimum trace spacing | 0.20 mm | All layers |
| Minimum via drill | 0.20 mm | All |
| Minimum via pad | 0.40 mm | All |
| Minimum pad-to-pad | 0.20 mm | All |
| Differential pair gap | 0.15 mm | Top (USB) |
| Differential pair width | 0.20 mm | Top (USB) |
| Impedance: single-ended | 50 Ω ±10% | Top/L4 |
| Impedance: differential | 90 Ω ±10% | Top (USB) |
| Power trace width (3V3) | 0.40 mm | L1, L3 |
| Power trace width (VBUS) | 1.00 mm | L1, L3, L4 |
| Power trace width (GND) | 1.00 mm | L1, L4 |
| Thermal relief spoke | 0.25 mm | L2, L3 |
| Solder mask clearance | 0.05 mm over pad | All |
| Board outline clearance | 0.30 mm | All layers |