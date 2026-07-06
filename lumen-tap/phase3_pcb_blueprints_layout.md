# Phase 3: PCB Blueprints & Layout — LumenTap

**Document Version:** 1.0
**Date:** 2026-07-06
**Author:** jayis1
**Status:** Final

---

## 1. Board Stackup

4-layer FR4, 1.6 mm total thickness, ENIG finish.

| Layer | Function | Copper |
|-------|----------|--------|
| L1 (F.Cu) | Signal + components | 1 oz (35 µm) |
| L2 (In1.Cu) | Solid GND plane | 1 oz |
| L3 (In2.Cu) | Power plane (split 5V / 3V3) | 1 oz |
| L4 (B.Cu) | Signal + QFN thermal pads | 1 oz |

Prepreg between L2–L3 is 1.065 mm to keep the power plane far from the
sensitive analog signals on L1; core L1–L2 is 0.21 mm for tight
impedance coupling to the ground plane.

---

## 2. Form Factor

- **Outline:** 110 × 60 mm rectangular with 4 × M2 mounting holes.
- **Enclosure:** milled aluminium, 35 mm tall, with a 25 mm optical
  aperture on the front face for the laser + photodiode pair, and a
  side cutout for USB-C and microSD.
- **Tripod mount:** 1/4"-20 brass insert on the bottom face for repeatable
  aiming.
- **Class label:** laser warning sticker on the front per IEC 60825-1.

---

## 3. Routing Rules

### 3.1 Analog signal (PD → TIA → FDA → ADC)

| Parameter | Value |
|-----------|-------|
| Track width | 0.3 mm (Analog class) |
| Clearance to digital | ≥ 1.0 mm |
| Via stitching | GND vias every 5 mm along the analog trace |
| Length match | PD→TIA and TIA→FDA within 0.5 mm (differential) |
| Ground return | solid L2 plane, no splits under analog |

### 3.2 USB D+/D-

| Parameter | Value |
|-----------|-------|
| Differential impedance | 90 Ω ± 10% |
| Track width | 0.2 mm, 0.18 mm gap |
| Length match | within 0.15 mm |
| ESD | TVS array (USBLC6-4SC6) at the connector |

### 3.3 SDMMC

| Parameter | Value |
|-----------|-------|
| Track width | 0.2 mm |
| Length match CMD/D0..D3/CK | within 5 mm |
| Series terminations | 22 Ω on CLK, CMD, DAT0..3 |

### 3.4 Power

| Rail | Min width | Via |
|------|-----------|-----|
| VBUS_5V | 0.8 mm | 0.8/0.4 mm |
| VCC_3V3 | 0.5 mm | 0.6/0.3 mm |
| VCC_1V8 | 0.3 mm | 0.6/0.3 mm |
| VCC_5V_CLEAN | 0.5 mm | 0.6/0.3 mm (analog class) |

---

## 4. Component Placement

```
  ┌──────────────── 110 mm ───────────────┐
  │ [USB-C]      STM32H743      [microSD] │
  │  buck  LT3045           SDMMC1 traces │
  │  TSL257                                 │
  │  ┌──── optical aperture ────┐          │
  │  │ PLT5 510R    S1223 PD    │          │
  │  │ iC-HK        OPA380      │          │
  │  │              OPA1632     │          │
  │  └──────────────────────────┘          │
  │  arm key SW1   status LED  fault LED  │
  │  1/4"-20 tripod insert                │
  └─────────────────────────────────────────┘
```

- Digital (MCU, SD, USB) on the left third; analog (PD, TIA, FDA) on
  the right third behind the optical aperture; power supplies in the
  middle as a shield.
- The laser driver and TIA share the clean-5V rail island; nothing
  digital draws from it.
- The OPA1632 output runs as a tightly-coupled differential pair
  directly into the MCU's PA0/PA1 ADC inputs across the mid-line.

---

## 5. Thermal

- STM32H743 dissipation ~ 250 mW at 480 MHz — no special heatsink
  needed; the inner GND plane acts as a spreader.
- LT3045 dissipates (3.3-5.0)·Iload — but Iload ≈ 20 mA so ~ 34 mW.
- MP1584 buck dissipates ~ 150 mW at full load; placed with a copper
  pour on L1 for heat spreading.

---

## 6. DFM Notes

- All passives 0402 to keep analog area compact; 0805 only for the
  10 µF bulk cap and the buck inductor.
- QFN parts (iC-HK, LT3045) use 0.45/0.5 mm pitch — within JLCPCP
  6/6 mil design rules.
- S1223 photodiode is through-hole TO-5 with a window — hand-soldered
  after reflow.
- microSD connector is SMT, placed near the board edge for card
  insertion.

— *jayis1*