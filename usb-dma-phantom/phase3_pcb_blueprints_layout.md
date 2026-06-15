# USB DMA Phantom — Phase 3: PCB Blueprints & Layout

## 1. PCB Stackup

4-layer stackup, designed for impedance control and Thunderbolt/PCIe signal integrity.

| Layer | Material | Thickness | Copper | Purpose |
|-------|----------|-----------|--------|---------|
| **L1 (Top)** | FR4 | — | 1 oz (35 µm) | Signal, components, USB-C pads, BLE antenna |
| **PP1** | FR4 prepreg | 0.10 mm | — | Dielectric between L1-L2 |
| **L2 (Inner 1)** | FR4 | — | 1 oz (35 µm) | Solid GND plane (unbroken) |
| **Core** | FR4 core | 0.50 mm | — | Rigid core |
| **L3 (Inner 2)** | FR4 | — | 1 oz (35 µm) | PCIe differential pairs (85 Ω), power splits |
| **PP2** | FR4 prepreg | 0.10 mm | — | Dielectric between L3-L4 |
| **L4 (Bottom)** | FR4 | — | 1 oz (35 µm) | Signal, bottom-side passives, SWD header |

**Total board thickness:** 0.80 mm (thin for USB stick form factor)

**Impedance calculations (FR4 εr = 4.2):**
- L1 microstrip 50 Ω: width = 0.30 mm (BLE antenna feed)
- L1-L2 edge-coupled diff 90 Ω: width = 0.18 mm, spacing = 0.12 mm
- L3-L2 stripline diff 85 Ω: width = 0.15 mm, spacing = 0.10 mm

## 2. Board Outline & Dimensions

```
    ┌─────────────────────────────────────────────────┐
    │                                                 │
    │  ┌─ USB-C ─┐                                    │ 12 mm
    │  │  J1     │                                    │
    │  └────┬────┘                                    │
    │       │                                         │
    │  ┌────┴──────────────────────────────────┐      │
    │  │                                       │      │
    │  │  [U4 HD3SS460]    [U3 XIO2001]       │      │
    │  │                                       │      │
    │  │  [U1 STM32F423]   [U2 nRF52832]      │      │
    │  │                                       │      │
    │  │  [U5 W25Q128]  [D1 WS2812B]          │      │
    │  │                                       │      │
    │  │  [U6 TPS62840]  [U7 DA1220]          │      │
    │  │                                       │      │
    │  │  [Y1] [Y2] [Y3]                      │      │
    │  │                                       │      │
    │  │              [J2 microSD]    [SW1]    │      │
    │  │                                       │      │
    │  │  ═══ BLE Trace Antenna ═══           │      │
    │  │                                       │      │
    │  └───────────────────────────────────────┘      │
    │                                                 │
    └─────────────────────────────────────────────────┘
                    65 mm
```

- **Overall dimensions:** 65 mm × 30 mm
- **Board thickness:** 0.80 mm
- **Connector overhang:** USB-C J1 extends 2 mm beyond board edge for plug-in ergonomics
- **Mounting:** No mounting holes (stick form factor; friction fit in USB-C port)
- **Edge:** USB-C connector edge has chamfered corners (0.5 mm) for insertion guidance

## 3. Component Placement Strategy

### 3.1 Zone Layout (Top Side, L1)

| Zone | X Range (mm) | Y Range (mm) | Contents |
|------|-------------|-------------|----------|
| **A: USB-C Interface** | 0–15 | 0–12 | J1, D2, D3, R3-R6, R13, C17-C18 |
| **B: High-Speed Mux** | 12–25 | 3–18 | U4 (HD3SS460), decoupling, Y3 |
| **C: PCIe Bridge** | 22–38 | 3–25 | U3 (XIO2001), decoupling, C17-C18 |
| **D: MCU Core** | 25–48 | 8–28 | U1 (STM32F423), Y1, C1-C10, R1, R2 |
| **E: BLE Radio** | 42–58 | 5–22 | U2 (nRF52832), Y2, C7-C9, antenna |
| **F: Flash & SD** | 38–55 | 18–28 | U5 (W25Q128), J2 (microSD), U7 (DA1220) |
| **G: Power** | 48–60 | 3–10 | U6 (TPS62840), L2-L3, C11-C12, U8 (TLV70033) |
| **H: Misc** | 55–63 | 15–28 | D1 (WS2812B), SW1, R14 |

### 3.2 Bottom Side (L4)

| Component | Purpose |
|-----------|---------|
| C1-C6 | STM32F423 decoupling (under U1) |
| C7-C9 | nRF52832 decoupling (under U2) |
| U3 decoupling | XIO2001 VDD/VDDIO bypass (under U3) |
| SWD header | 4-pin 1.27 mm pitch (PA13, PA14, VDD, GND) |
| Boot pads | Solder jumpers for BOOT0 / nRF DFU |

## 4. High-Speed Routing Rules

### 4.1 PCIe Differential Pairs (XIO2001 ↔ HD3SS460 ↔ USB-C)

| Rule | Value |
|------|-------|
| **Impedance** | 85 Ω ± 10% differential |
| **Layer** | L3 (stripline, between L2 GND and L4) |
| **Trace width** | 0.15 mm |
| **Pair spacing** | 0.10 mm |
| **Via transition** | Grounded via pair at each layer change (within 0.5 mm) |
| **Length matching** | ± 0.5 mm within pair, ± 5 mm between TX and RX |
| **Max via count** | 4 per differential pair |
| **AC coupling caps** | 0.1 µF 0402 on each TX pair, within 10 mm of source |
| **Return path** | Continuous L2 ground reference; no splits under PCIe |

### 4.2 USB 2.0 (STM32F423 ↔ USB-C via HD3SS460 pass-through)

| Rule | Value |
|------|-------|
| **Impedance** | 90 Ω ± 15% differential |
| **Layer** | L1 (microstrip over L2 GND) |
| **Trace width** | 0.18 mm |
| **Pair spacing** | 0.12 mm |
| **Guard trace** | GND pour with 0.2 mm clearance on each side |
| **ESD protection** | D2 (PRTR5V0U2X) within 5 mm of J1 |
| **Series termination** | R3, R4 (100 Ω) within 10 mm of U1 pins |

### 4.3 BLE Antenna (nRF52832)

| Rule | Value |
|------|-------|
| **Type** | Meandering inverted-F antenna (MIFA) |
| **Impedance** | 50 Ω single-ended |
| **Feed trace** | 0.30 mm coplanar waveguide on L1 |
| **Ground clearance** | ≥ 5 mm keepout around antenna element |
| **Matching network** | Π-network placeholder (0402, populate 0 Ω + DNI) |
| **Antenna dimensions** | ~26 mm total trace length for 2.44 GHz λ/4 |

## 5. Via Strategy

| Via Type | Drill | Pad | Usage |
|----------|-------|-----|-------|
| **Standard** | 0.2 mm | 0.4 mm | Signal vias, ≤ 1 per 0.5 mm route |
| **Power** | 0.3 mm | 0.6 mm | Power/GND transitions, decoupling |
| **Thermal** | 0.5 mm | 1.0 mm | Under XIO2001 thermal pad |
| **Stitching** | 0.2 mm | 0.3 mm | GND plane stitching every 2 mm on board perimeter |

**Via placement rules:**
- No vias in differential pair gaps (between + and − traces)
- Grounded return vias within 0.5 mm of every signal via
- Power vias clustered near IC power pins (minimum 2 per VDD pin)
- No via transitions on PCIe pairs between U3 and U4 (same-layer routing)
- Thermal vias under XIO2001: 5×5 grid, 0.8 mm pitch

## 6. Thermal Management

| Component | Power Dissipation | Mitigation |
|-----------|-------------------|------------|
| XIO2001 | ~1.2 W (active) | Thermal pad to L2 GND via 5×5 via array; 25 mm² copper pour on L1 |
| STM32F423 | ~0.3 W (active) | Thermal pad to L2 GND via 4×4 via array |
| nRF52832 | ~0.05 W (TX) | Minimal; copper pour suffices |
| HD3SS460 | ~0.15 W | Small copper pour under component |
| TPS62840 | ~0.1 W (buck loss) | Copper pour on L1 + L2 for heat spread |

**Thermal via patterns under U3 (XIO2001):**
```
  ●  ●  ●  ●  ●
  ●  ●  ●  ●  ●
  ●  ●  ●  ●  ●
  ●  ●  ●  ●  ●
  ●  ●  ●  ●  ●
  (0.5 mm drill, 0.8 mm pitch grid)
```

## 7. Isolation Barriers

### 7.1 High-Speed / Low-Speed Isolation

| Barrier | Purpose | Implementation |
|---------|---------|---------------|
| PCIe zone vs MCU zone | Prevent PCIe switching noise from coupling into MCU | L2 GND plane slot between zones; ferrite bead on 3V3 between zones |
| BLE antenna vs digital | Prevent MCU noise from desensitizing BLE RX | 5 mm keepout around antenna; L2 solid GND extends under antenna; L1 ground pour around antenna feed |
| USB power vs analog | Prevent buck converter ripple from coupling | Star-point GND; separate 3V3_LDO for nRF analog; ferrite bead between zones |

### 7.2 Ground Plane Strategy

```
L2 (GND plane):
┌────────────────────────────────────────────────┐
│                                                │
│  ┌─────────────┐     ┌────────────────────┐    │
│  │ PCIe/TBT    │     │ MCU / BLE / Flash   │    │
│  │ GND Zone    │slot │ GND Zone            │    │
│  │             │←──→ │                     │    │
│  └─────────────┘     └────────────────────┘    │
│                                                │
│  ┌──────┐                          ┌──────┐    │
│  │Power │                          │Antenna│    │
│  │GND   │                          │GND   │    │
│  └──────┘                          └──────┘    │
│                                                │
└────────────────────────────────────────────────┘

Slot: 0.3 mm gap in L2 GND plane between PCIe and MCU zones
Bridge: Single 0.5 mm connection at board center for DC return
Ferrite bead: L1 (NRV5015T4G) on 3V3 between zones
```

## 8. Design for Manufacturing

| Parameter | Value |
|-----------|-------|
| **Minimum trace width** | 0.10 mm (4 mil) |
| **Minimum trace spacing** | 0.10 mm (4 mil) |
| **Minimum via drill** | 0.20 mm (8 mil) |
| **Minimum annular ring** | 0.10 mm (4 mil) |
| **Solder mask** | Green LPI |
| **Silkscreen** | White, both sides |
| **Surface finish** | ENIG (Electroless Nickel Immersion Gold) |
| **Copper weight** | 1 oz (35 µm) all layers |
| **Board thickness** | 0.80 mm |
| **Impedance control** | ±10% on specified pairs |
| **UL rating** | 94V-0 |
| **Class** | IPC 6012 Class 2 |

## 9. Board Outline Drawing

```
Top View (dimensions in mm):

        0        10       20       30       40       50       60  65
        │        │        │        │        │        │        │   │
   0 ───┼────────┼────────┼────────┼────────┼────────┼────────┼───
        │                                                          │
        │  ╔══════╗                                                │
        │  ║ J1   ║← USB-C Plug                                   │
        │  ║USB-C ║                                                │
   12 ──╢  ╚══════╝                                                │
        │                                                          │
        │     [U4]      [U3]                                       │
        │                   [Y3]                                    │
        │        [U1]            [U2]                               │
   20 ──╢        [Y1]          [Y2]                                 │
        │                                                          │
        │              [U5]  [U7]  [U6]  [U8]                     │
        │                                [D1]                      │
        │                                                          │
   28 ──╢                                [J2]    [SW1]            │
        │                                                          │
        │              ═════ BLE MIFA Antenna ════                  │
        │                                                          │
   30 ───┼──────────────────────────────────────────────────────────┤
        │        │        │        │        │        │        │   │

Chamfer: 0.5 mm × 0.5 mm on J1 insertion end corners
Radius: 0.5 mm on all other corners
SWD pads: 4 × 1.27 mm on bottom edge (56-60 mm, Y=28-30 mm)
```

## 10. Signal Integrity Verification Checklist

- [ ] All differential pairs length-matched within ±0.5 mm intra-pair
- [ ] All differential pairs length-matched within ±5 mm inter-pair
- [ ] No via transitions on PCIe pairs between XIO2001 and HD3SS460
- [ ] L2 GND plane unbroken under all high-speed traces
- [ ] AC coupling caps placed within 10 mm of TX source
- [ ] ESD protection within 5 mm of USB-C connector
- [ ] 5 mm antenna keepout clear of copper on all layers
- [ ] Decoupling caps within 2 mm of IC VDD pins
- [ ] Power trace widths: 3V3 = 0.5 mm, 5V = 0.8 mm, GND = plane
- [ ] No 90° trace bends; all bends are 45° or curved
- [ ] All IC thermal pads connected to L2 GND via via arrays