# PoE Network Implant — Phase 3: PCB Blueprints & Layout

## 1. PCB Stackup

4-layer stackup, 1.6mm total thickness, ENIG finish.

| Layer | Material | Thickness | Copper Weight | Purpose |
|-------|----------|-----------|--------------|---------|
| L1 (Top) | FR-4 | 0.035mm | 1 oz (35µm) | Signal, component pads, Ethernet diff pairs |
| PP1 | Prepreg 2116 | 0.12mm | — | L1–L2 dielectric |
| L2 (GND) | FR-4 core | 0.035mm | 1 oz (35µm) | Solid ground plane |
| Core | FR-4 | 0.90mm | — | L2–L3 dielectric |
| L3 (PWR) | FR-4 core | 0.035mm | 1 oz (35µm) | Power plane splits: VPOE, VDD_3V3, VDD_1V2 |
| PP2 | Prepreg 2116 | 0.12mm | — | L3–L4 dielectric |
| L4 (Bottom) | FR-4 | 0.035mm | 1 oz (35µm) | Signal, SDRAM escape, BLE antenna |

### Impedance Calculations

**100Ω differential pairs (L1, microstrip over L2 GND):**
- Trace width: 5.0 mil (0.127mm)
- Trace spacing: 7.0 mil (0.178mm)
- Dielectric height (L1 to L2): 5.4 mil (0.137mm)
- Er = 4.2 (2116 prepreg)
- Zdiff ≈ 100.2Ω

**50Ω single-ended (L1, microstrip over L2 GND):**
- Trace width: 9.8 mil (0.249mm)
- Z0 ≈ 50.1Ω

**50Ω BLE feed (L4, microstrip over L3):**
- Trace width: 14.2 mil (0.361mm) [wider due to thinner dielectric]
- Z0 ≈ 50.0Ω

## 2. Board Outline

```
┌─────────────────────────────────────┐
│                                     │
│  ┌─J1─┐                    ┌─J2─┐   │
│  │RJ45│                    │RJ45│   │
│  │ IN │                    │OUT │   │
│  └────┘                    └────┘   │
│                                     │
│  35mm × 20mm PCB                    │
│  (fits inside RJ45 coupler shell)   │
│                                     │
│  SW1 SW2                            │
│                                     │
└─────────────────────────────────────┘
```

- Board dimensions: 35.0mm × 20.0mm × 1.6mm
- Corner radius: 1.0mm
- Mounting: 2× M1.6 (1.6mm) holes at (3.5, 2.0) and (31.5, 2.0)
- RJ45 J1 center-left: x=6.5mm
- RJ45 J2 center-right: x=28.5mm

## 3. Component Placement (Top View)

```
┌──────────────────────────────────────────┐
│                                          │
│ ┌──J1──┐   ┌─T1─┐         ┌─T2─┐  ┌──J2──┐│
│ │ RJ45 │   │Mag │         │Mag │  │RJ45  ││
│ │  IN  │   │ 1  │         │ 2  │  │ OUT  ││
│ └──────┘   └────┘         └────┘  └──────┘│
│                                          │
│   ┌──────────────┐   ┌───────┐           │
│   │  KSZ9897R    │   │STM32H7│   ┌────┐ │
│   │  (U2)        │   │43 (U1)│   │U11 │ │
│   └──────────────┘   └───────┘   │BLE │ │
│                                    └────┘ │
│   ┌───┐ ┌───┐   ┌──────────┐              │
│   │U3 │ │U4 │   │  U9 SDRAM│   ┌────┐    │
│   │PD │ │PD │   │          │   │U10 │    │
│   │in │ │out│   └──────────┘   │Flash│   │
│   └───┘ └───┘                   └────┘    │
│                                          │
│   ┌────┐┌────┐┌────┐┌────┐              │
│   │ U5 ││ U6 ││ U7 ││ U8 │  SW1 SW2    │
│   │DCDC││DCDC││DCDC││DCDC│              │
│   └────┘└────┘└────┘└────┘              │
│   D1 (TVS)                    Y1  Y2     │
└──────────────────────────────────────────┘
```

### Placement Rules

1. **RJ45 connectors** at extreme left/right edges for cable entry
2. **Magnetics T1, T2** within 10mm of respective RJ45 to minimize stub length
3. **KSZ9897** centered between magnetics, ≤15mm trace to each
4. **STM32H743** adjacent to KSZ9897 for shortest RGMII routing
5. **SDRAM** directly below MCU for FMC length matching
6. **BLE module** in corner, away from magnetics (EMI)
6. **DC-DC converters** along bottom edge (PoE input side)
7. **PoE PD controllers** near respective RJ45 for PoE routing

## 4. High-Speed Routing

### 4.1 Ethernet Differential Pairs (100Ω diff)

**Port 1 (IN):**
```
J1.P1 ────── T1.P1_TX_CT ────── U2.PORT1_TRXP
J1.P2 ────── T1.P1_TX_CT ────── U2.PORT1_TRXM
J1.P3 ────── T1.P1_RX_CT ────── U2.PORT1_RXP
J1.P6 ────── T1.P1_RX_CT ────── U2.PORT1_RXM
```
- All 4 pairs on L1 (top)
- Pair-to-pair spacing: ≥20 mil (5x width)
- Length matching within pair: ±5 mil
- Length matching between TX pairs: ±10 mil
- Length matching between RX pairs: ±10 mil
- No vias on differential pairs (stay on L1)

**Port 2 (OUT):** Same rules, mirrored to J2.

### 4.2 RGMII Signals

- All RGMII signals on L1 (top) for shortest path
- TX group (TXCLK, TXD[0:3], TXCTL): length match ±50 mil
- RX group (RXCLK, RXD[0:3], RXCTL): length match ±50 mil
- Add 2ns delay on TXCLK (serpentine) per KSZ9897 RGMII timing
- Series 22Ω termination resistors on all RGMII lines (at MCU source)
- 49.9Ω pull-down on RXCTL/RXD lines

### 4.3 SDRAM (FMC) Routing

- Data lines D[0:31] on L1 and L4 (split by byte lane)
- Address lines A[0:11], BA[0:1] on L4
- All data lines length-matched ±100 mil within byte lane
- All address lines length-matched ±200 mil
- Clock (SDCLK) routed first, all signals reference it
- Fly-by topology for address/command group
- Series 22Ω termination on address/command at MCU source

### 4.4 BLE Antenna Trace

- 50Ω microstrip on L4 (bottom)
- λ/4 meander antenna pattern on L4 edge
- Keep-out zone: 5mm clearance from all other copper
- No ground plane pour under antenna area (L3 cutout 15mm × 5mm)

## 5. Via Strategy

| Via Type | Drill | Pad | Use | Max Current |
|----------|-------|-----|-----|-------------|
| Signal via | 0.2mm (8mil) | 0.45mm (18mil) | All signal transitions | 0.5A |
| Power via | 0.3mm (12mil) | 0.60mm (24mil) | Power plane transitions | 1.0A |
| Thermal via | 0.3mm (12mil) | 0.60mm (24mil) | Under QFN pads | 1.0A |
| PoE via | 0.5mm (20mil) | 0.80mm (32mil) | 48V PoE routing | 2.0A |

### Via Placement Rules
1. No vias on Ethernet differential pairs
2. No vias between differential pair members
3. Ground vias placed every 500mil along differential pairs (stitching)
4. Minimum 3 power vias per IC power pin
5. Thermal vias under KSZ9897 QFN pad (3×3 grid, 9 vias, filled)
6. Thermal vias under STM32H743 QFN pad (4×4 grid, 16 vias, filled)
7. Return-path vias within 50mil of every signal via transition

## 6. Thermal Management

| Component | Power Dissipation | Thermal Solution |
|-----------|-------------------|-----------------|
| U1 (STM32H743) | ~0.5W @ 480MHz | 16 thermal vias under pad → L2 GND plane |
| U2 (KSZ9897) | ~0.8W | 9 thermal vias under pad → L2 GND plane |
| U5 (TPS62A02A) | ~0.3W | Thermal pad + 4 vias → L2 GND |
| U3/U4 (TPS2378) | ~0.15W each | Thermal pad + 2 vias → L2 GND |
| U9 (SDRAM) | ~0.2W | No special (low power) |
| **Total** | **~2.1W** | **Passive cooling sufficient (no fan)** |

### Thermal via Fill
- All thermal vias: 0.3mm drill, filled with epoxy + copper cap
- Connected to L2 (GND) and L3 (local copper pour) only
- Thermal resistance: < 15°C/W per via
- Junction-to-ambient for STM32H743: ~40°C/W (with 16 vias) → 84°C at 2.1W (well within 125°C max)

## 7. Isolation Barriers

### 7.1 PoE Isolation
- Magnetics T1, T2 provide 1500V galvanic isolation between cable side and board
- Center-tap PoE extraction is on the **cable side** (before isolation) as per IEEE 802.3
- No copper pour within 2mm of magnetics isolation barrier
- L2/L3 plane splits: no continuous copper under magnetics
- Creepage/clearance: ≥4mm between primary (cable) and secondary (board) circuits

### 7.2 Digital Isolation
- BLE module section has local ground island connected at single point (ferrite bead)
- PoE power section separated from digital section by 2mm no-copper zone
- SDRAM and MCU share ground (high-speed FMC needs solid reference)

### 7.3 ESD Protection
- TVS diode D1 (SMBJ48A) at PoE input: 48V standoff, clamps at 77.4V
- ESD protection D2 (PRTR5V0U2X) on UART lines to BLE module
- All connector pins have 500V/8kV ESD rating (RJ45 shield to chassis ground)
- Chassis ground connected to board ground via 1nF/2kV Y-capacitor and 1MΩ resistor

## 8. Power Plane Splits (L3)

```
┌──────────────────────────────────────────┐
│  VPOE (48V)  │  VDD_3V3  │  VDD_1V2    │
│  (left half)  │  (center) │  (right)    │
│               │           │             │
│  20mm wide    │  10mm     │  5mm wide   │
│               │           │             │
└──────────────────────────────────────────┘
```

- VPOE plane on left (near PoE input, DC-DC converters)
- VDD_3V3 plane in center (powers most ICs)
- VDD_1V2 plane on right (powers MCU core, KSZ9897 core)
- VDD_1V8 and VDD_2V5 are local fills on L1 (small areas, only need short traces)
- Plane-to-plane clearance: 20 mil (0.5mm)

## 9. Design Rules Summary

| Parameter | Value |
|-----------|-------|
| Minimum trace width | 4 mil (0.1mm) |
| Minimum trace spacing | 4 mil (0.1mm) |
| Minimum via drill | 8 mil (0.2mm) |
| Minimum via pad | 18 mil (0.45mm) |
| Differential pair width | 5 mil (0.127mm) |
| Differential pair spacing | 7 mil (0.178mm) |
| Differential pair-to-other spacing | 20 mil (0.5mm) |
| Minimum plane clearance | 10 mil (0.25mm) |
| Copper-to-edge clearance | 20 mil (0.5mm) |
| Solder mask expansion | 2 mil (0.05mm) |
| Paste mask reduction | 2 mil (0.05mm) |
| Silkscreen line width | 6 mil (0.15mm) |
| Board thickness | 62 mil (1.6mm) |
| Surface finish | ENIG (Au 1-2µin over Ni 120-240µin) |
| Solder mask color | Black (stealth) |
| Silkscreen color | White |
| Board material | FR-4 TG170 |
| IPC class | Class 2 |