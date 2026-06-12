# WFP-100 — Portable WiFi Pentesting Dongle
## Phase 3: PCB Blueprints & Layout

---

## 3.1 PCB Stackup Definition

| Layer | Material | Thickness | Copper Weight | Function |
|---|---|---|---|---|
| L1 (Top) | Copper | 0.5 oz (17.5 µm) | 0.5 oz | Signal — USB SS, PCIe, GPIO |
| PP1 | Isola 370HR prepreg | 3.5 mil (89 µm) | — | Dielectric L1–L2 |
| L2 (GND) | Copper | 1 oz (35 µm) | 1 oz | Solid ground plane |
| Core 1 | Isola 370HR core | 8 mil (203 µm) | — | Dielectric L2–L3 |
| L3 (Signal) | Copper | 0.5 oz (17.5 µm) | 0.5 oz | Signal — DDR, eMMC, QSPI |
| PP2 | Isola 370HR prepreg | 8 mil (203 µm) | — | Dielectric L3–L4 |
| L4 (PWR) | Copper | 1 oz (35 µm) | 1 oz | Power plane — VDD_CORE, VDD_DDR |
| Core 2 | Isola 370HR core | 24 mil (610 µm) | — | Dielectric L4–L5 |
| L5 (Signal) | Copper | 0.5 oz (17.5 µm) | 0.5 oz | Signal — bottom routing |
| PP3 | Isola 370HR prepreg | 3.5 mil (89 µm) | — | Dielectric L5–L6 |
| L6 (Bottom) | Copper | 0.5 oz (17.5 µm) | 0.5 oz | Signal — DDR, bypass caps |

**Total PCB thickness:** 1.0 mm (0.039") — HDI, 0.3mm pitch BGA compatible

**Dielectric constant (Er):** 4.04 @ 1 GHz (Isola 370HR)
**Loss tangent:** 0.016 @ 1 GHz

### 3.1.1 Stackup Rationale

- **6-layer HDI** — Required for 0.3mm BGA (JH7110) breakout with adequate routing channels
- **1 oz ground/power planes** — Low impedance return paths for PCIe Gen3 and USB 3.2
- **0.5 oz signal layers** — Fine-line etching capability (3 mil traces, 3 mil spaces)
- **Thin prepreg L1–L2** — Tight coupling to ground plane for controlled impedance
- **1.0 mm total** — Flexible enough for USB-C stick form factor, but rigid for BGA reliability

---

## 3.2 Impedance Control Summary

| Signal Type | Layer | Target Z0 | Target Zdiff | Width | Spacing | Reference |
|---|---|---|---|---|---|---|
| PCIe TX/RX | L1 | 50Ω ±10% | 100Ω ±10% | 4.0 mil | 6.0 mil | L2 GND |
| PCIe REFCLK | L1 | 50Ω ±10% | 100Ω ±10% | 4.0 mil | 6.0 mil | L2 GND |
| USB SS TX/RX | L1 | 50Ω ±10% | 90Ω ±10% | 3.5 mil | 5.0 mil | L2 GND |
| DDR4 DQ (single) | L3 | 50Ω ±10% | — | 4.0 mil | — | L2 GND / L4 PWR |
| DDR4 DQS (diff) | L3 | 50Ω ±10% | 100Ω ±10% | 4.0 mil | 6.0 mil | L2 GND |
| DDR4 CK (diff) | L3 | 50Ω ±10% | 100Ω ±10% | 4.0 mil | 6.0 mil | L2 GND |
| DDR4 CA/CMD (single) | L3 | 50Ω ±10% | — | 4.0 mil | — | L2 GND |
| eMMC (single-ended) | L3 | 50Ω ±15% | — | 4.5 mil | — | L2 GND |
| QSPI (single-ended) | L1/L3 | 50Ω ±15% | — | 4.5 mil | — | L2 GND |

---

## 3.3 Routing Rules

### 3.3.1 DDR4 (LPDDR4X) Routing

**Data Group Assignment:**

| Byte Lane | DQ Signals | DQS Pair | DMI | Max Length Skew |
|---|---|---|---|---|
| Byte Lane 0 | DQ[0:7] | DQS0_t/DQS0_c | DMI0 | ±2 mil within lane |
| Byte Lane 1 | DQ[8:15] | DQS1_t/DQS1_c | DMI1 | ±2 mil within lane |
| Byte Lane 2 | DQ[16:23] | DQS2_t/DQS2_c | DMI2 | ±2 mil within lane |
| Byte Lane 3 | DQ[24:31] | DQS3_t/DQS3_c | DMI3 | ±2 mil within lane |

**Address/CMD Group:**

| Signal | Max Length | Length Match |
|---|---|---|
| CA[5:0] | <3000 mil | ±25 mil across all CA pins |
| CK_t/CK_c | <3000 mil | ±2 mil diff pair, ±25 mil to CA group |
| CS_n | <3000 mil | ±25 mil to CA group |
| CKE | <3000 mil | ±25 mil to CA group |

**Via Strategy for DDR:**
- Each DQ/DQS signal: **via-in-pad** (0.3mm BGA pitch requires micro-via)
- Via type: **Laser-drilled micro-via** (L1→L2, L1→L3) + **mechanical via** (L1→L6 through-hole)
- Micro-via diameter: 4 mil (100 µm), pad: 10 mil (250 µm)
- Through-via diameter: 8 mil (200 µm), pad: 20 mil (500 µm)
- Anti-pad clearance: 20 mil (500 µm) on power/ground planes

**DDR Routing Topology:**

```
JH7110 SoC (PoP bottom)                    MT53E256M32D2DS (PoP top)
┌─────────────────────┐                    ┌─────────────────────┐
│  DDR_PAD[31:0]       │◄─── Fly-by ──────►│  DQ[31:0]          │
│  DDR_DQS[3:0]       │◄─── Point-to-point►│  DQS[3:0]          │
│  DDR_CA[5:0]        │◄─── Fly-by ──────►│  CA[5:0]            │
│  DDR_CK_t/CK_c      │◄─── Point-to-point►│  CK_t/CK_c          │
│  DDR_CS_n            │◄─── Point-to-point►│  CS_n               │
│  DDR_CKE             │◄─── Point-to-point►│  CKE                │
└─────────────────────┘                    └─────────────────────┘
     │                                                │
     └─── PoP interconnect (embedded in substrate) ───┘
          Length: ~50 mil (very short, PoP)
          No external routing needed — PoP package routes internally
```

> **Note:** LPDDR4X is mounted as Package-on-Package (PoP) directly on top of the JH7110 BGA. All DDR routing occurs within the PoP interposer — no external PCB routing required. This dramatically simplifies layout.

### 3.3.2 PCIe Gen3 x1 Routing (AX210)

**Critical Requirements:**
- **Differential pairs:** TX+/- and RX+/- must be routed as 100Ω diff pairs on L1
- **Length matching:** ±5 mil within each diff pair, ±50 mil between TX and RX pairs
- **AC coupling capacitors:** 100nF 0201 caps on TX lines (near SoC source)
- **Via transitions:** Max 2 via transitions per diff pair
- **Ground stitching:** 1 ground via per 20 mil along diff pair route

```
JH7110 SoC                                    M.2 E-key Slot
┌──────────┐                                  ┌──────────────┐
│  TX+  ───┤── AC1 ──────────────────────────┤── PERp0 (24) │
│  TX-  ───┤── AC2 ──────────────────────────┤── PERn0 (26) │
│           │                                  │              │
│  RX+  ◄──┤──────────────────────────────────┤── PETp0 (21) │
│  RX-  ◄──┤──────────────────────────────────┤── PETn0 (23) │
│           │                                  │              │
│  CLK+ ───┤─────────────────────────────────┤── REFCLKp(38) │
│  CLK- ───┤─────────────────────────────────┤── REFCLKn(36) │
└──────────┘                                  └──────────────┘

AC1, AC2: 100nF 0201 AC coupling caps, placed within 200 mil of JH7110 TX balls
```

**PCIe Trace Length Budget:**

| Segment | Max Length | Notes |
|---|---|---|
| JH7110 → AC cap | 200 mil | Short, controlled |
| AC cap → M.2 slot | 4000 mil | Main route on L1 |
| Total | ≤4200 mil | Under PCIe Gen3 FR4 budget |

### 3.3.3 USB 3.2 Gen1 Routing

**Requirements:**
- **90Ω differential** pairs for SuperSpeed (TX and RX)
- **45Ω single-ended** for USB 2.0 D+/D-
- **Length match:** ±10 mil within diff pairs, ±500 mil between TX/RX
- **ESD protection:** TPD4E05U06 (TI) on USB-C receptacle side
- **Ground stitching:** Via every 15 mil along SS pairs

```
JH7110 SoC                                    USB-C Receptacle (J1)
┌──────────┐                                  ┌──────────────────┐
│  SSTX+ ──┤─────────────────────────────────┤── A2 / B11      │
│  SSTX- ──┤─────────────────────────────────┤── A3 / B10      │
│           │                                  │                  │
│  SSRX+ ◄──┤──────────────────────────────────┤── A10 / B3      │
│  SSRX- ◄──┤──────────────────────────────────┤── A11 / B2      │
│           │                                  │                  │
│  D+ ──────┤─────────────────────────────────┤── A6 / B6       │
│  D- ──────┤─────────────────────────────────┤── A7 / B7       │
└──────────┘                                  └──────────────────┘

ESD: TPD4E05U06 placed 200 mil from J1, on all SS and D+/D- lines
```

---

## 3.4 Via Strategy

### 3.4.1 Via Types

| Via Type | Drill | Pad | Anti-pad | Layers | Usage |
|---|---|---|---|---|---|
| Micro-via | 4 mil (100 µm) | 10 mil (250 µm) | 18 mil (450 µm) | L1→L2 or L1→L3 | BGA fanout (0.3mm pitch) |
| Blind via | 6 mil (150 µm) | 14 mil (350 µm) | 22 mil (550 µm) | L1→L4 | Power delivery under BGA |
| Buried via | 6 mil (150 µm) | 14 mil (350 µm) | 22 mil (550 µm) | L3→L6 | Inner-layer routing transitions |
| Through-hole | 8 mil (200 µm) | 24 mil (600 µm) | 30 mil (750 µm) | L1→L6 | General purpose, connectors |

### 3.4.2 Via Density Rules

- **BGA area (JH7110):** Via-in-pad for 0.3mm pitch BGA; micro-vias from L1 to L2 for ground, L1 to L3 for signal escape
- **DDR area (PoP):** No external vias needed — all connections within PoP interposer
- **PCIe/USB area:** Via transitions minimized to ≤2 per diff pair; ground return via within 20 mil of each signal via
- **Decoupling caps:** One ground via per cap pad, placed adjacent to BGA via

### 3.4.3 Ground Via Stitching

- **Perimeter stitch:** Ground via every 50 mil along PCB edges
- **Under AX210 M.2:** Ground via array (25 mil pitch) under M.2 slot for thermal and EMI
- **Under USB-C:** Ground via array (30 mil pitch) around connector for ESD and return path
- **Open areas:** Ground via fill on 40 mil grid in unused areas, connecting L2 and L6

---

## 3.5 Thermal Management

### 3.5.1 Thermal Budget

| Component | Power (typical) | Power (max) | Thermal Path |
|---|---|---|---|
| JH7110 SoC | 1.5W | 2.5W | BGA → PCB copper → enclosure |
| AX210 WiFi | 1.0W (RX) | 2.0W (TX) | M.2 thermal pad → EMI shield → enclosure |
| LPDDR4X | 0.3W | 0.5W | PoP → SoC thermal path |
| AXP2101 PMIC | 0.2W | 0.4W | Direct to PCB |
| eMMC | 0.1W | 0.3W | Direct to PCB |
| **Total** | **3.1W** | **5.7W** | **Passive cooling to enclosure** |

### 3.5.2 Thermal Via Arrays

| Location | Via Pattern | Via Count | Purpose |
|---|---|---|---|
| Under JH7110 BGA | 5×5 grid, 25 mil pitch | 25 | Conduct heat from BGA center to L6 |
| Under AX210 M.2 | 8×4 grid, 25 mil pitch | 32 | Conduct WiFi module heat to enclosure |
| PMIC area | 3×3 grid, 30 mil pitch | 9 | PMIC heat spreading |
| eMMC area | 4×4 grid, 30 mil pitch | 16 | eMMC heat spreading |

### 3.5.3 Thermal Interface

```
┌───────────────────────────────────────┐
│            Aluminum enclosure top      │
│            (heat spreader)             │
│                                       │
│  ┌─────────────┐  ┌────────────────┐  │
│  │ Thermal pad  │  │ Thermal pad    │  │
│  │ 1.5mm gap    │  │ 0.5mm gap      │  │
│  │ JH7110 → top │  │ AX210 → top    │  │
│  └─────────────┘  └────────────────┘  │
│                                       │
│  ┌────────────────────────────────┐   │
│  │ PCB (6-layer, 1.0mm)          │   │
│  │   JH7110 + LPDDR4X (PoP)     │   │
│  │   AX210 M.2                    │   │
│  └────────────────────────────────┘   │
│                                       │
│            Aluminum enclosure bottom   │
└───────────────────────────────────────┘

Thermal resistance: θ_JA ≈ 25°C/W (including enclosure)
Max junction temp: 105°C (JH7110), 95°C (AX210)
Max ambient: 60°C → Tj_max = 60 + 5.7 × 25 = 72.5°C ✓ (under 105°C limit)
```

---

## 3.6 Component Placement Plan

### 3.6.1 Top Layer (L1) Placement

```
┌─────────────────────────────────────────────────────────────────┐
│  WFP-100 PCB Top Layer (85mm × 35mm)                           │
│                                                                  │
│  ┌──────────┐         ┌────────────────────────────────┐        │
│  │ USB-C    │         │   M.2 2230 E-key Slot          │        │
│  │ J1       │         │   ┌──────────────────────────┐ │        │
│  │          │  ┌────┐ │   │  Intel AX210NGW          │ │  ○RP-SMA│
│  │ ┌────┐   │  │JH7 │ │   │  WiFi Module             │ │  J4    │
│  │ │ESD │   │  │7110│ │   │                          │ │        │
│  │ │TPD │   │  │+PoP│ │   └──────────────────────────┘ │  ○RP-SMA│
│  │ └────┘   │  │DDR │ │                                  │  J3    │
│  │          │  └────┘ │   ┌──────────┐ ┌──────────┐    │        │
│  └──────────┘         │   │ eMMC U5  │ │ QSPI U9  │    │        │
│                        │   │ 16GB     │ │ 16MB     │    │        │
│  ┌────────┐ ┌──────┐  │   └──────────┘ └──────────┘    │        │
│  │AXP2101 │ │BQ    │  │                                │        │
│  │PMIC U4 │ │25895 │  │   ┌──────┐ ┌──────┐ ┌──────┐  │        │
│  └────────┘ │U8    │  │   │LED   │ │LED   │ │LED   │  │        │
│             └──────┘  │   │PWR   │ │ACT   │ │MON   │  │        │
│  ┌──────┐ ┌────────┐  │   │D1    │ │D2    │ │D3    │  │        │
│  │PCF   │ │TPM U7  │  │   └──────┘ └──────┘ └──────┘  │        │
│  │8563  │ │        │  │                       [BTN]    │        │
│  │U6    │ │        │  │                        SW1     │        │
│  └──────┘ └────────┘  │                                │        │
│                        │   ┌──────────────────────┐    │        │
│  ┌──────────────────┐  │   │ microSD slot J2      │    │        │
│  │ Coin cell BT1    │  │   │                      │    │        │
│  └──────────────────┘  │   └──────────────────────┘    │        │
│                        └────────────────────────────────┘        │
│  [Battery connector — 1000mAh LiPo on bottom of PCB]           │
└─────────────────────────────────────────────────────────────────┘
```

### 3.6.2 Component Grouping

| Zone | Components | Rationale |
|---|---|---|
| **USB zone** | J1, F1, ESD (TPD4E05U06), R1-R3 | Minimize USB trace length to connector |
| **SoC zone** | U1 (JH7110), U2 (LPDDR4X PoP), decoupling caps | Central placement, shortest DDR paths |
| **WiFi zone** | M.2 slot, AX210, Y1 (27MHz), PCIe AC caps | PCIe trace length matching to SoC |
| **Power zone** | U4 (AXP2101), U8 (BQ25895), inductors, bulk caps | Near battery connector, separate from sensitive analog |
| **Storage zone** | U5 (eMMC), U9 (QSPI flash), J2 (microSD) | SDIO traces to SoC, grouped together |
| **LED/Button zone** | D1-D3, SW1, R4-R7 | Near edge for user visibility |

---

## 3.7 Design for Manufacturing (DFM) Notes

### 3.7.1 Minimum Feature Sizes

| Feature | Size | Notes |
|---|---|---|
| Minimum trace width | 3 mil (76 µm) | L1/L3 signal layers |
| Minimum trace spacing | 3 mil (76 µm) | L1/L3 signal layers |
| Minimum via drill | 4 mil (100 µm) | Micro-via (L1→L2, L1→L3) |
| Minimum via pad | 10 mil (250 µm) | Micro-via pad |
| Minimum via drill (TH) | 8 mil (200 µm) | Through-hole via |
| Minimum BGA pitch | 0.3 mm (12 mil) | JH7110 BGA-484 |
| Minimum solder mask dam | 3 mil (76 µm) | Between pads |
| Silkscreen line width | 6 mil (150 µm) | Component reference designators |

### 3.7.2 Assembly Notes

1. **JH7110 BGA-484 (0.3mm pitch):** Use Type 4 (0.2mm) SAC305 solder paste, 100µm stencil thickness. Requires X-ray inspection for BGA voiding (≤20% void area per IPC-7095).

2. **LPDDR4X PoP assembly:** Mount MT53E256M32D2DS on top of JH7110 using PoP interposer. Apply solder paste to JH7110 top-side pads first, then reflow LPDDR4X. Requires 2-pass reflow (bottom-side pass for JH7110, top-side pass for LPDDR4X).

3. **AX210 M.2 module:** Socketed (not soldered) — M.2 2230 E-key connector allows field replacement of WiFi module. Thermal pad (1.5 W/mK, 0.5mm) between module and EMI shield.

4. **USB-C connector:** Through-hole mounting tabs for mechanical strength, SMT signal pads. Apply additional solder fillet on mounting tabs.

5. **LiPo battery:** Connected via JST PH 2-pin connector (SMT). Battery sits in cavity between PCB and enclosure bottom. Adhesive foam pad secures battery.

6. **Conformal coating:** Apply HumiSeal 1B31 to all components except USB-C contacts, M.2 contacts, RP-SMA contacts, and microSD slot. Required for field durability.

### 3.7.3 Panelization

- **Panel size:** 24″ × 18″ (standard)
- **Boards per panel:** ~40 (85mm × 35mm with 5mm routing, 3mm edge rails)
- **Breakaway method:** V-score + mouse bites (hybrid)
- **Fiducials:** 3 on each board corner + 3 panel-level fiducials

### 3.7.4 Test Points

| Reference | Net | Location | Type |
|---|---|---|---|
| TP1 | VDD_CORE (0.9V) | Near JH7110 | 0.5mm via probe |
| TP2 | VDD_DDR (1.5V) | Near LPDDR4X | 0.5mm via probe |
| TP3 | VDD_3V3 | Near PMIC | 0.5mm via probe |
| TP4 | VBUS_5V | Near USB-C | 0.5mm via probe |
| TP5 | GND | PCB edge | Large via |
| TP6 | PCIE_REFCLK+ | Near M.2 slot | 0.5mm via probe |
| TP7 | USB_SS_TX+ | Near USB-C | 0.5mm via probe |
| TP8 | UART0_TX | Near SoC | 0.5mm via probe |
| TP9 | UART0_RX | Near SoC | 0.5mm via probe |
| TP10 | I2C0_SDA | Near PMIC | 0.5mm via probe |

### 3.7.5 Design Rule Checks (DRC)

| Check | Value | Action |
|---|---|---|
| Unrouted nets | 0 | Fail |
| Clearance violation | <3 mil | Fail |
| Differential pair length mismatch | >5 mil (PCIe), >10 mil (USB), >2 mil (DDR) | Warning |
| Differential pair impedance | Outside 50Ω ±10% | Warning |
| Via aspect ratio | >8:1 (through), >0.75:1 (micro) | Fail |
| Copper pour clearance | <8 mil from plane edge | Fail |
| Solder mask bridge | <3 mil | Fail |
| Silkscreen over pad | Any overlap | Fail |
| Thermal relief spoke width | <8 mil | Warning |
| Net tie length mismatch | >5 mil | Fail |