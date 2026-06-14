# NFC Relay Phantom — Phase 3: PCB Blueprints & Layout

## 1. PCB Stackup

4-layer stackup, 0.8 mm total thickness (credit-card profile):

| Layer | Material | Thickness | Copper | Function |
|-------|----------|-----------|--------|----------|
| Top | FR-4 | 0.035 mm | 1 oz (35 µm) | Signal + Component placement |
| L2 | FR-4 core | 0.2 mm | 1 oz (35 µm) | Ground plane (unbroken) |
| L3 | FR-4 core | 0.5 mm | 1 oz (35 µm) | Power planes (3V3, VBAT split) |
| Bottom | FR-4 | 0.035 mm | 1 oz (35 µm) | Signal + 125 kHz antenna + 13.56 MHz antenna |
| Prepreg | 1080 | 0.065 mm | — | Bonding L1-L2 |
| Core | 7628 | 0.2 mm | — | L2-L3 core |
| Prepreg | 1080 | 0.065 mm | — | Bonding L3-L4 |

**Total thickness:** ~0.8 mm (suitable for credit-card form factor)

**Surface finish:** ENIG (Electroless Nickel Immersion Gold) — excellent for wire bonding and solderability on antenna traces.

---

## 2. Board Outline

```
┌─────────────────────────────────────────────────────────────┐
│                                                             │
│  ┌──────────┐                          ┌──────────────────┐│
│  │ USB-C    │   ┌──────────────────┐   │   125 kHz       ││
│  │ J1       │   │                  │   │   Antenna        ││
│  └──────────┘   │   STM32L4        │   │   (8 turns)     ││
│                 │   U1              │   │                  ││
│  ┌──────────┐   │                  │   └──────────────────┘│
│  │ SW1 SW2  │   └──────────────────┘                        │
│                 ┌──────────────────┐   ┌──────────────────┐ │
│  ┌──────────┐   │   NRF52832       │   │  13.56 MHz       │ │
│  │ Battery  │   │   U2             │   │  Antenna         │ │
│  │ J3       │   └──────────────────┘   │  (4 turns)      │ │
│  └──────────┘                           │  40×30mm        │ │
│                 ┌──────────────────┐     └──────────────────┘ │
│  ┌──────────┐   │   PN5180         │                         │
│  │ OLED     │   │   U3             │   ┌──────────────────┐ │
│  │ SSD1306  │   └──────────────────┘   │   EM4095         │ │
│  └──────────┘                           │   U4             │ │
│                                         └──────────────────┘ │
│  [LED1] [LED2] [LED3]          [J2 Debug]                    │
└─────────────────────────────────────────────────────────────┘
     85.0 mm width × 54.0 mm height × 0.8 mm thickness
```

**Dimensions:** 85.0 × 54.0 mm (ISO 7810 ID-1 credit card dimensions)
**Corner radius:** 3.18 mm (matching ISO 7810)
**Mounting:** No mounting holes — device is pocket-sized, not panel-mount

---

## 3. High-Speed Routing Rules

### 3.1 USB 2.0 (12 Mbps Full Speed)

| Parameter | Value |
|-----------|-------|
| Differential impedance | 90 Ω ± 10% |
| Single-ended impedance | 45 Ω |
| Max trace length | 25 mm (J1 → U1) |
| Via count | 0 (route on same layer as connector) |
| Pair spacing | 0.15 mm |
| Trace width | 0.2 mm (calculated for stackup) |
| Guard | Ground pour on L2 directly beneath, no splits |
| ESD | USBLC6-2 placed < 3mm from J1 |
| Series resistors | 33 Ω ± 1%, placed < 5mm from U1 |

### 3.2 SPI1 (20 MHz, PN5180)

| Parameter | Value |
|-----------|-------|
| Max trace length | < 10 mm |
| Max skew (SCK-MOSI-MISO) | < 100 ps (≈15mm) |
| Trace width | 0.15 mm |
| Spacing | 0.2 mm (3W rule from aggressor) |
| Ground | Continuous L2 reference |
| Termination | No series term needed at 20 MHz / <10mm |

### 3.3 SPI2 (40 MHz, W25Q128)

| Parameter | Value |
|-----------|-------|
| Max trace length | < 15 mm |
| Max skew | < 500 ps |
| Trace width | 0.15 mm |
| Ground | Continuous L2 reference |

---

## 4. Via Strategy

| Via Type | Diameter | Drill | Pad | Usage |
|----------|----------|-------|-----|-------|
| Standard | 0.6 mm | 0.3 mm | 0.5 mm | Signal routing, GPIO |
| Power | 0.8 mm | 0.4 mm | 0.7 mm | Power plane stitching |
| Thermal | 0.8 mm | 0.4 mm | 0.7 mm | Thermal vias under ICs |
| Antenna | 0.5 mm | 0.25 mm | 0.4 mm | RF feed transitions |

**Via stitching:**
- Ground via stitching every 2 mm along board edges
- Ground via array (2 mm grid) under PN5180 thermal pad
- Power via stitching at each VDD/VSS pair on STM32L4
- Antenna feed via at 125 kHz and 13.56 MHz feed points

**Anti-via zones:**
- No vias within 0.5 mm of crystal oscillators
- No vias within 1 mm of 13.56 MHz antenna feed
- No vias within USB differential pair region

---

## 5. Thermal Management

| Component | Power Dissipation | Cooling Method | Notes |
|-----------|------------------|----------------|-------|
| STM32L4S5 | ~120 mW (120 MHz) | PCB copper pour | Thermal vias to L2 |
| NRF52832 | ~50 mW (TX) | PCB copper pour | QFN pad is primary heatsink |
| PN5180 | ~800 mW (TX active) | Thermal vias + pour | 4×4 thermal vias under pad |
| EM4095 | ~500 mW (active) | PCB copper pour | SOIC has good thermal |
| BQ25896 | ~400 mW (charging) | PCB copper pour | WQFN thermal pad |
| TPS62840 | ~30 mW | PCB copper pour | Minimal |

**Thermal via placement:**
- PN5180: 3×3 grid of 0.8mm thermal vias under exposed pad, connecting L1→L2 ground
- NRF52832: 2×2 grid under center pad
- BQ25896: 2×2 grid under center pad
- STM32L4: Perimeter thermal vias around LQFP package

**Thermal relief:**
- All power plane connections use thermal spokes (4 spoke, 0.25mm spoke width)
- 3.3V pour on L3 has 1mm clearance from all non-power features

---

## 6. Isolation Barriers

### 6.1 RF Isolation

| Barrier | Separation | Method |
|---------|-----------|--------|
| 125 kHz antenna ↔ 13.56 MHz antenna | ≥ 10 mm | Ground stitching fence on L2 |
| 13.56 MHz antenna ↔ digital traces | ≥ 5 mm | Ground pour guard, L2 unbroken |
| EM4095 ↔ PN5180 | ≥ 8 mm | Ground isolation moat on L1/L2 |
| NRF52 antenna (internal) ↔ PN5180 | ≥ 15 mm | Physical separation + ground |

### 6.2 Power Isolation

| Domain | Source | Isolation |
|--------|--------|-----------|
| VBUS_5V | USB-C | Ferrite bead (FB1) isolated from 3V3 |
| VREG_3V3 | TPS62840 | Main digital + analog rail |
| VREG_3V3_NRF | TLV70033 LDO | Separate LDO for NRF52 to prevent BLE TX noise injection |
| VBAT | LiPo | Direct to BQ25896, filtered with 10µF |
| 5V_ANT | VBUS via P-MOSFET Q1 | Gated, only during 125 kHz TX active |

### 6.3 Ground Strategy

- **Single ground plane (L2)** — no splits
- All ground connections via thermal vias or direct L2 connection
- Ground stitching every 2 mm around board perimeter
- Ground pour on L1 (top) and L4 (bottom) tied to L2 via stitching
- Star ground point near BQ25896 for analog ground references

---

## 7. Antenna Design

### 7.1 13.56 MHz NFC Antenna

**Type:** Rectangular multi-turn PCB trace antenna
**Dimensions:** 40 × 30 mm
**Turns:** 4
**Trace width:** 1.0 mm
**Trace spacing:** 0.5 mm
**Estimated inductance:** ~1.2 µH

```
┌────────────────────────────┐
│ ┌────────────────────────┐ │
│ │ ┌────────────────────┐ │ │
│ │ │ ┌────────────────┐ │ │ │
│ │ │ │                │ │ │ │
│ │ │ │                │ │ │ │
│ │ │ └────────────────┘ │ │ │
│ │ └────────────────────┘ │ │
│ └────────────────────────┘ │
└────────────────────────────┘
   Feed: TX1 ─┘         └─ TX2
```

**Matching network:** L2 (22 nH) + C17 (1 nF) on TX1, L3 (22 nH) + C18 (1 nF) on TX2, C19/C20 (27 pF) antenna resonant caps.

**Tuning:** Target resonant frequency 13.56 MHz ± 100 kHz. Adjust C19/C20 values during prototype bring-up.

### 7.2 125 kHz RFID Antenna

**Type:** Rectangular multi-turn PCB trace antenna
**Dimensions:** 50 × 30 mm (bottom layer, bottom half of board)
**Turns:** 8
**Trace width:** 0.5 mm
**Trace spacing:** 0.3 mm
**Estimated inductance:** ~345 µH

```
┌──────────────────────────────────────┐
│ ┌──────────────────────────────────┐ │
│ │ ┌──────────────────────────────┐ │ │
│ │ │ ┌──────────────────────────┐ │ │ │
│ │ │ │ ┌──────────────────────┐ │ │ │ │
│ │ │ │ │ ┌──────────────────┐ │ │ │ │ │
│ │ │ │ │ │ ┌──────────────┐ │ │ │ │ │ │
│ │ │ │ │ │ │              │ │ │ │ │ │ │
│ │ │ │ │ │ └──────────────┘ │ │ │ │ │ │
│ │ │ │ │ └──────────────────┘ │ │ │ │ │
│ │ │ │ └────────────────────┘ │ │ │ │
│ │ │ └──────────────────────┘ │ │ │ │
│ │ └────────────────────────┘ │ │ │
│ └────────────────────────────┘ │ │
└────────────────────────────────┘ │
└──────────────────────────────────┘
   Feed: ANT1 ─┘              └─ ANT2
```

**Tuning:** L1 (10 µH) + C21/C22 (100 nF) charge pump. Target Q ≈ 25 at 125 kHz.

---

## 8. Component Placement Zones

```
┌─────────────────────────────────────────────────────────────┐
│ Zone A: Power/USB        │ Zone B: Digital Processing      │
│ ┌───────────┐             │  ┌─────────────┐  ┌──────────┐  │
│ │ USB-C J1  │  BQ25896   │  │  STM32L4S5  │  │ W25Q128  │  │
│ │           │  U6         │  │  U1         │  │ U5       │  │
│ └───────────┘             │  └─────────────┘  └──────────┘  │
│  USBLC6    TPS62840       │                                 │
│  D1         U7             │  ┌─────────────┐  ┌──────────┐  │
│                            │  │  NRF52832   │  │ OLED     │  │
│ Zone C: Control           │  │  U2         │  │ U8       │  │
│ ┌──────┐ ┌──────┐         │  └─────────────┘  └──────────┘  │
│ │ SW1  │ │ SW2  │         │                                 │
│ └──────┘ └──────┘         │                                 │
│  LEDs                     │  Zone D: RF Frontend           │
│                            │  ┌─────────────┐  ┌──────────┐ │
│ Zone E: Battery           │  │  PN5180     │  │ EM4095   │ │
│ ┌───────────┐             │  │  U3         │  │ U4       │ │
│ │ LiPo J3  │             │  └─────────────┘  └──────────┘ │
│ └───────────┘             │                                 │
│                            │  Zone E: Antennas (bottom)     │
│                            │  ┌─────────────┐  ┌──────────┐ │
│                            │  │ 13.56 MHz   │  │125 kHz   │ │
│                            │  │ Antenna     │  │Antenna   │ │
│                            │  └─────────────┘  └──────────┘ │
└─────────────────────────────────────────────────────────────┘
```

---

## 9. Design Rules Summary

| Rule | Value |
|------|-------|
| Minimum trace width | 0.1 mm (signal), 0.5 mm (power) |
| Minimum trace spacing | 0.1 mm |
| Minimum via drill | 0.25 mm |
| Minimum via pad | 0.4 mm |
| Minimum annular ring | 0.075 mm |
| Minimum clearance (RF) | 0.2 mm |
| Minimum solder mask clearance | 0.05 mm |
| Copper pour clearance | 0.2 mm |
| Differential pair tolerance | ±10% impedance |
| Length matching tolerance | ±0.5 mm |
| Board edge clearance | 0.5 mm |
| Antenna clearance (keep-out) | 2 mm no copper |

---

## 10. Design for Manufacturing (DFM) Notes

1. **Solder paste:** Type 4 (T4) for 0402 and QFN components
2. **Reflow:** Standard SAC305 profile, peak 245°C
3. **Stencil thickness:** 0.1 mm (100 µm) for fine-pitch
4. **Inspection:** AOI required for QFN components, X-ray for BGA (none in this design)
5. **Testing:** ICT bed-of-nails on 4 test points:
   - TP1: VREG_3V3 (top, near TPS62840)
   - TP2: VBUS_5V (top, near USB connector)
   - TP3: GND (top, any ground via)
   - TP4: BOOT0 (top, for forcing boot mode)
6. **OLED assembly:** SSD1306 COG flex is pre-mounted on a carrier PCB, soldered as SMD
7. **Battery connector:** JST-PH 4-pin through-hole, soldered last in secondary operation
8. **Antenna tuning:** 0402 placeholders for C19/C20 (27 pF default) with provision for 0602 pads for easier hand-tuning