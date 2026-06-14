# Sub-GHz IoT Gateway Implant — Phase 3: PCB Blueprints & Layout

## 1. PCB Stackup

6-layer stackup optimized for RF performance and high-speed memory signals.

| Layer | Material | Thickness | Copper | Purpose |
|-------|----------|-----------|--------|---------|
| L1 (Top) | FR4 | 0.2 mm | 1 oz (35 µm) | Components, RF traces, high-speed signals |
| PP1 | Prepreg 1080 | 0.1 mm | — | L1-L2 dielectric |
| L2 (GND1) | FR4 | 0.2 mm | 1 oz (35 µm) | Solid ground plane (Sub-GHz RF reference) |
| Core1 | FR4 | 0.5 mm | — | L2-L3 core |
| L3 (PWR) | FR4 | 0.2 mm | 1 oz (35 µm) | Power planes (3.3V, 1.2V, VDD_PA) |
| PP2 | Prepreg 2116 | 0.12 mm | — | L3-L4 dielectric |
| L4 (GND2) | FR4 | 0.2 mm | 1 oz (35 µm) | Ground plane (BLE RF reference, EMI shield) |
| Core2 | FR4 | 0.5 mm | — | L4-L5 core |
| L5 (Signal2) | FR4 | 0.2 mm | 1 oz (35 µm) | SDRAM data bus, secondary routing |
| PP3 | Prepreg 1080 | 0.1 mm | — | L5-L6 dielectric |
| L6 (Bottom) | FR4 | 0.2 mm | 1 oz (35 µm) | Components (bottom side), slow signals, ground pour |

**Total board thickness**: 1.6 mm (standard)

### Impedance Calculations (using Saturn PCB toolkit, FR4 εr=4.4)

| Trace Type | Layer | Width | Spacing | Target Z | Calculated Z |
|------------|-------|-------|---------|----------|-------------|
| 50 Ω single-ended (RF) | L1 over L2 | 0.30 mm (coplanar) | — | 50 Ω | 49.8 Ω |
| 50 Ω single-ended (SDRAM) | L1 over L2 | 0.15 mm | — | 50 Ω | 51.2 Ω |
| 90 Ω diff pair (USB) | L1 over L2 | 0.18 mm | 0.12 mm | 90 Ω diff | 89.6 Ω |
| 50 Ω single-ended (L5) | L5 over L4 | 0.15 mm | — | 50 Ω | 50.4 Ω |

### Layer Assignment Rationale

- **L2 (GND1)** is the primary reference plane for all RF and high-speed signals on L1. The thin 0.1mm prepreg ensures tight coupling.
- **L3 (PWR)** splits into 3.3V (most area), 1.2V (CC1352P core), and VDD_PA (PA supply island). Each has a corresponding return path on L2 or L4.
- **L4 (GND2)** provides EMI shielding between the power/signal layers and acts as the BLE antenna reference plane.
- **L5** routes the SDRAM data bus to free L1 space for RF and critical signals.
- **L6** holds bottom-side passives and expansion connector, with ground pour.

## 2. Board Outline

```
   ┌─────────────────────────────────────────────────────────┐
   │  ○ SMA                                                │ 85mm
   │  (Sub-GHz)                                             │
   │                                                        │
   │    ┌──────────┐           ┌──────────────┐            │
   │    │  CC1352P │           │   U2 SDRAM   │            │
   │    │  U1 SoC  │           │   IS66WVS256  │            │
   │    └──────────┘           └──────────────┘            │
   │                                                        │
   │    ┌──────────┐   ┌──────────┐  ┌──────────────┐    │
   │    │ U3 Flash │   │U4 ATECC  │  │ U2 SDRAM #2   │    │
   │    │ MX25LW   │   │ 608A     │  │ IS66WVS256    │    │
   │    └──────────┘   └──────────┘  └──────────────┘    │
   │                                                        │
   │  ┌──────┐    ┌─────────┐    ┌────────────────┐      │
   │  │USB-C │    │Power Mgmt│    │  MicroSD Slot  │      │
   │  │ J1   │    │U5,U6,U7  │    │  J3            │      │
   │  └──────┘    └─────────┘    └────────────────┘      │
   │                                                        │
   │  ○ BTN_MODE  ○ BTN_ACTION  ○ RGB LED               │
   │                                [BLE Antenna ────]    │
   └─────────────────────────────────────────────────────────┘
                      54mm
```

### Board Dimensions & Key Features

| Feature | Location | Notes |
|---------|----------|-------|
| Board outline | 85.0 × 54.0 mm | Credit card footprint, rounded corners (1.5mm radius) |
| Mounting holes | 4 corners, M2.5 | 3.5mm from each edge |
| SMA connector | Top-left edge | Sub-GHz antenna, edge-launch |
| USB-C connector | Left edge, center | Bottom-mount, data + charge |
| MicroSD slot | Right edge, center | Bottom-mount, push-push |
| BLE antenna | Bottom edge, right | Inverted-F, clear keepout zone 40×15mm |
| SWD header | Top edge, center | 2×5 pin, 0.1" pitch (programming) |
| Battery connector | Center-bottom | JST PH 2-pin (3.7V LiPo) |

### Keepout Zones

1. **BLE Antenna Keepout**: 40mm × 15mm at bottom-right, no copper on any layer, no components
2. **Sub-GHz RF Keepout**: 5mm radius around SMA pad and match network, no ground pour on L1 (ground on L2 only)
3. **Crystal Keepout**: 3mm radius around Y1 and Y2, no high-speed signals, ground pour on L1 permitted but no vias within 1mm
4. **CC1352P Thermal**: Center ground pad (4mm × 4mm) requires 9 thermal vias (0.3mm drill, 0.6mm pad) to L2 ground plane

## 3. Component Placement Strategy

### 3.1 Placement Hierarchy (Critical → Less Critical)

1. **CC1352P (U1)** — Center of board, orientation aligned with RF paths pointing toward antenna connectors
2. **Sub-GHz RF match network** (L4, C27, C28) — Adjacent to U1 PA pins, minimal trace length to SMA
3. **BLE matching + antenna** — Adjacent to U1 RF2 pin, short path to inverted-F trace
4. **SDRAM (U2, U3)** — Close to U1 SDRAM interface pins, equal trace lengths
5. **SPI Flash (U4)** — Near U1 SPI pins
6. **Crystals (Y1, Y2)** — Adjacent to U1 XTAL pins, guard traces
7. **ATECC608A (U4 crypto)** — Near U1 I²C pins
8. **Power management** (U5, U6, U7, U8) — Clustered near USB-C connector
9. **MicroSD (J3)** — Edge-mounted, with TXB0108 level shifter between U1 and J3
10. **Decoupling caps** — Within 2mm of each VDD pin
11. **Buttons, LED** — Edges per board outline
12. **ESD protection** — At connector entries

### 3.2 Placement Coordinates (mm from bottom-left origin)

| Reference | X (mm) | Y (mm) | Side | Rotation |
|-----------|--------|--------|------|----------|
| U1 (CC1352P) | 35.0 | 42.0 | Top | 0° |
| U2 (SDRAM #1) | 58.0 | 42.0 | Top | 0° |
| U3 (SDRAM #2) | 58.0 | 30.0 | Top | 180° |
| U4 (Flash) | 15.0 | 30.0 | Top | 90° |
| U4c (ATECC608A) | 25.0 | 25.0 | Top | 0° |
| U5 (Buck 3.3V) | 10.0 | 15.0 | Top | 0° |
| U6 (Buck 1.2V) | 10.0 | 10.0 | Top | 0° |
| U7 (Buck-Boost) | 22.0 | 10.0 | Bottom | 0° |
| U8 (Charger) | 10.0 | 20.0 | Bottom | 0° |
| U9 (TXB0108) | 65.0 | 20.0 | Top | 0° |
| U10 (ESD) | 5.0 | 30.0 | Bottom | 0° |
| U11 (LDO) | 18.0 | 15.0 | Top | 0° |
| J1 (USB-C) | 0.0 | 35.0 | Edge | 180° |
| J2 (SMA) | 0.0 | 50.0 | Edge | 180° |
| J3 (MicroSD) | 85.0 | 25.0 | Edge | 90° |
| Y1 (24 MHz) | 30.0 | 50.0 | Top | 0° |
| Y2 (32.768 kHz) | 40.0 | 50.0 | Top | 0° |
| D1 (WS2812B) | 75.0 | 10.0 | Top | 0° |
| SW1 (Mode) | 70.0 | 5.0 | Top | 0° |
| SW2 (Action) | 80.0 | 5.0 | Top | 0° |
| Battery conn | 42.5 | 2.0 | Bottom | 0° |

## 4. High-Speed Routing

### 4.1 SDRAM Interface (up to 104 MHz)

The SDRAM interface is the fastest bus on the board. Both SDRAM chips share clock, CS#, and DQS signals, with separate data lanes.

**Routing rules:**
- **Signal group**: SDRAM_CLK, SDRAM_CS#, SDRAM_DQS, SDRAM_DQ[0:15]
- **Length matching**: All signals within ±200 mil (5 mm) of SDRAM_CLK
- **SDRAM_CLK**: Routed on L1 with grounded guard traces, series termination (22 Ω) near source
- **Data bus**: SDRAM_DQ[0:7] on L1 (to U2), SDRAM_DQ[8:15] on L5 (to U3, via transition under U1)
- **Via transition**: Maximum 2 vias per signal, via stitching at transition points
- **Max trace length**: 50 mm from U1 to SDRAM

**SDRAM_CLK series termination:**
```
U1 DIO_21 → R_series(22Ω) → SDRAM_CLK net → U2 CLK, U3 CLK
```

### 4.2 USB 2.0 Full-Speed (12 Mbps)

**Routing rules:**
- **Differential pair**: USB_DP, USB_DM
- **90 Ω ±10% differential impedance**
- **Max trace length**: 150 mm (well within budget)
- **No via transitions**: Routed entirely on L1
- **ESD protection**: U10 placed at connector entry, before any branch
- **Pull-downs**: 5.1 kΩ on CC1/CC2 within 5mm of J1

### 4.3 SPI Flash (up to 80 MHz)

**Routing rules:**
- **Signal group**: FLASH_CLK, FLASH_MOSI, FLASH_MISO, FLASH_CS#
- **Length matching**: Within ±500 mil of FLASH_CLK
- **Max trace length**: 30 mm
- **Single-ended 50 Ω** on L1
- **Series termination**: 33 Ω on FLASH_CLK near U1

### 4.4 Sub-GHz RF (868/915 MHz)

This is the most critical routing on the board. The PA output and LNA input are on adjacent pins (PA1, SUB1) and must be routed with extreme care.

**PA output path (TX):**
```
U1 PA1 (Pin 35) → L4(10nH) → C27(10pF to GND) → C28(0.5pF to GND) → 50Ω coplanar waveguide → J2 SMA
```
- **Trace width**: 0.30 mm (coplanar waveguide, gap=0.20 mm to ground pour)
- **Reference plane**: L2 solid ground (no splits under this trace)
- **Via fencing**: Ground vias every 1mm along the RF path on both sides
- **No right angles**: 45° bends only, no 90° corners
- **Component placement**: L4, C27, C28 within 3mm of PA1 pin

**LNA input path (RX):**
```
J2 SMA → 50Ω coplanar waveguide → C_coupling(100pF) → U1 SUB1 (Pin 34)
```
- Short, direct path from SMA to LNA input
- ESD diode (R05S series) at SMA connector entry

**Transceiver switch**: The CC1352P has an integrated RF switch that alternates between PA and LNA paths internally. No external switch needed.

### 4.5 BLE 2.4 GHz Antenna Trace

```
U1 RF2 (Pin 36) → C_match(1.5pF) → L_match(2.7nH) → Inverted-F PCB antenna
```
- **Antenna type**: Inverted-F, λ/4 at 2.45 GHz
- **Antenna trace length**: ~28 mm on FR4 (calculated for εr=4.4)
- **Antenna location**: Bottom-right of board, with no copper on any layer beneath
- **Ground plane cutout**: 40mm × 15mm clear area on L2 beneath antenna
- **Feed point**: 50Ω microstrip from matching network

### 4.6 I²C (ATECC608A, 1 MHz max)

- **SDA, SCL**: 0.15 mm traces on L1, 4.7 kΩ pull-ups at bus end (near ATECC608A)
- **Max trace length**: 30 mm
- **Series resistor**: 100 Ω near U1 for EMI reduction

## 5. Via Strategy

### 5.1 Via Types

| Via Type | Drill | Pad | Antipad | Use |
|----------|-------|-----|---------|-----|
| Standard | 0.2 mm | 0.4 mm | 0.5 mm | General signal routing |
| Power | 0.3 mm | 0.6 mm | 0.75 mm | Power/ground connections |
| Thermal | 0.3 mm | 0.6 mm | 0.75 mm | CC1352P center pad (9 vias) |
| RF Stitching | 0.2 mm | 0.4 mm | 0.5 mm | Ground via fencing along RF paths |

### 5.2 Via Placement Rules

1. **No vias on differential pairs** (USB_DP/DM routed entirely on L1)
2. **SDRAM signals**: Max 2 vias per net, transition at component pads
3. **RF paths**: No vias on RF traces (L1 only). Ground stitching vias every 1mm along RF paths.
4. **Power vias**: Multiple parallel vias for VDD_MAIN and GND connections (min 2 per power pin)
5. **CC1352P thermal pad**: 3×3 grid of thermal vias (0.3mm drill) connecting center pad to L2 ground

### 5.3 Via Fanout Density

- **CC1352P**: 48 signal pins + 4 power pins + center pad → approximately 60 vias in 10mm × 10mm area
- **SDRAM (each)**: 24 pins → approximately 20 vias each
- **Total estimated via count**: ~400 vias

## 6. Thermal Management

### 6.1 Thermal Sources

| Component | Max Power | Avg Power | Thermal Path |
|-----------|----------|-----------|-------------|
| CC1352P (TX at +20 dBm) | 1.5 W | 0.5 W | Center pad → L2 ground → board |
| TLV62569 (3.3V buck) | 0.5 W | 0.2 W | SOT-23 package → copper pour |
| TLV62569 (1.2V buck) | 0.2 W | 0.05 W | SOT-23 package → copper pour |
| TPS63020 (buck-boost) | 0.4 W | 0.15 W | VQFN pad → copper pour |
| CC1352P (RX) | 0.05 W | 0.03 W | Center pad → L2 ground |

### 6.2 Thermal Design

1. **CC1352P center pad**: 4mm × 4mm exposed thermal pad, connected to L2 ground plane with 9 thermal vias (0.3mm drill). This provides both electrical ground and thermal dissipation.
2. **Power ICs**: Local copper pours on L3 (power plane) with thermal spokes connecting to L2 ground via thermal vias.
3. **Board-level**: Continuous L2 ground plane acts as a heat spreader. With 85mm × 54mm board area, natural convection provides sufficient cooling for < 2W total dissipation.
4. **No heatsink required**: Maximum board temperature rise < 15°C above ambient at 25°C, well within CC1352P 85°C junction limit.

### 6.3 Thermal Via Calculations

- CC1352P junction-to-ambient (via PCB): θ_JA ≈ 40°C/W (with thermal vias)
- Max junction temp at +20 dBm TX: T_J = 25°C + 1.5W × 40°C/W = 85°C (at limit)
- **Mitigation**: Duty-cycle TX at 50% max in firmware → avg dissipation 0.75W → T_J = 55°C ✅

## 7. Isolation Barriers

### 7.1 RF-to-Digital Isolation

- **Sub-GHz RF section**: Isolated from digital section by L2 ground plane (solid underneath RF)
- **BLE antenna**: Ground plane cutout on L2 (40mm × 15mm) and ground pour on L1 with via stitching fence
- **RF ground stitching**: Via fence (0.2mm vias, 1mm pitch) along the boundary between RF and digital sections
- **Component separation**: Sub-GHz match network and BLE antenna ≥ 15mm from digital ICs

### 7.2 Power Domain Isolation

- **VDD_PA** (PA supply): Isolated from VDD_MAIN by ferrite bead FB1 (600Ω @ 100 MHz)
- **VDD_CORE** (1.2V): Separate buck converter, isolated ground return
- **Analog ground**: CC1352P AVSS pins connect to L2 ground at a single star point
- **Digital ground**: All other ground connections on L2 (solid plane)

### 7.3 ESD Protection Zones

| Zone | Component | Protected Lines |
|------|-----------|----------------|
| USB entry | TPD4E05U06 (U10) | USB_DP, USB_DM, VBUS, GND |
| SD card slot | TVS diode array | SD_CLK, SD_MOSI, SD_MISO, SD_CS |
| Sub-GHz SMA | R05S ESD diode | RF input/output |
| SWD header | TVS on reset line | RESET, SWD_CLK |

## 8. Design Rules Summary

| Parameter | Value |
|-----------|-------|
| Minimum trace width (signal) | 0.10 mm |
| Minimum trace width (power) | 0.25 mm |
| Minimum via drill | 0.2 mm |
| Minimum via pad | 0.4 mm |
| Minimum via spacing | 0.5 mm |
| Minimum component spacing | 0.2 mm |
| Solder mask clearance | 0.05 mm |
| Silkscreen line width | 0.12 mm |
| Minimum text height | 0.8 mm |
| Copper-to-edge clearance | 0.5 mm |
| Differential pair impedance | 90 Ω ±10% |
| Single-ended impedance | 50 Ω ±10% |
| Length matching tolerance | SDRAM: ±5mm, SPI: ±12mm |
| RF trace width (Sub-GHz) | 0.30 mm (coplanar) |
| RF trace width (BLE) | 0.25 mm |
| Board thickness | 1.6 mm |
| Surface finish | ENIG (Au over Ni) |
| Solder mask | Green (both sides) |
| Silkscreen | White (both sides) |
| Copper weight | 1 oz (35 µm) all layers |
| Minimum annular ring | 0.1 mm |
| Board edge clearance | 0.5 mm to nearest copper |

## 9. DFM Notes

1. **Component sizes**: All passives 0402 minimum (no 0201), all ICs ≤ 7mm × 7mm QFN/BGA
2. **Panel**: V-score panelization, 5mm rails, 3mm mouse bites
3. **Test points**: 1mm test pads on VDD_MAIN, VDD_CORE, VDD_RF, GND (4 total) for verification
4. **Fiducials**: 3 fiducials on top and bottom (1mm copper circle, 2mm solder mask opening)
5. **Stencils**: 0.1mm stencil thickness, 1:1 aperture ratio for 0402, reduced for QFN center pads
6. **No BGA rework required**: All components are QFN/SOIC/0402 — standard rework equipment sufficient