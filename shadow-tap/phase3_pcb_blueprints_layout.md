# ShadowTap — Phase 3: PCB Blueprints & Layout

## 1. PCB Stackup

4-layer stackup, 1.0 mm total thickness, fabricated with ENIG finish.

| Layer | Copper (oz) | Thickness (µm) | Material | Purpose |
|---|---|---|---|---|
| L1 (Top) | 1 | 35 | FR-4 | Signal — high-speed RGMII, diff pairs, SDIO |
| L2 (GND) | 1 | 35 | FR-4 | Solid ground plane — reference for L1 & L3 |
| L3 (PWR) | 1 | 35 | FR-4 | Power islands — VCC_3V3, VCC_1V2, 5V_SYS |
| L4 (Bot) | 1 | 35 | FR-4 | Signal — low-speed SPI, I²C, UART, LEDs, bottom-side parts |

Prepreg between L1–L2: 3.5 mil (89 µm) → Z0 ≈ 50Ω for 6 mil trace width
Core between L2–L3: 40 mil (1016 µm)
Prepreg between L3–L4: 3.5 mil (89 µm)

### Dielectric Targets
- Dk = 4.2 (FR-4 core), Dk = 3.9 (prepreg)
- Loss tangent ≤ 0.02 at 1 GHz
- 100Ω differential: 5 mil trace, 5 mil spacing (L1 to L2 GND ref)

## 2. Board Outline & Dimensions

```
┌─────────────────────────────────────────────────────────────┐
│  60 mm                                                        │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │                                                         │ │
│  │  J1 (RJ45)          ShadowTap            J2 (RJ45)      │ │
│  │  ┌────┐                               ┌────┐          │ │
│  │  │    │                               │    │          │ │
│  │  │    │   U2   U1      U7    U8       │    │          │ │ 40
│  │  │    │  PHY  SoC    Flash  EEPROM   │    │          │ mm
│  │  │    │        ▲                       │    │          │ │
│  │  └────┘     U3│    U4  U5   U6        └────┘          │ │
│  │             PHY│   5V  3.3V 1.2V                      │ │
│  │               │                                         │ │
│  │         U3 TPS2378     U9 (M.2)    J4 (uSD)            │ │
│  │         PoE PD         BLE         SD card             │ │
│  │                                                         │ │
│  │  ○ SW1    ○○○○ LEDs                                  │ │
│  └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

- Board: 60.0 mm × 40.0 mm, 4 corners 1.5 mm radius
- Mounting holes: 4× M2 (Ø2.2 mm) at corners, 3 mm from edges
- RJ45 MagJacks extend 3 mm beyond board edge (connector overhang)
- Total envelope: 60 mm × 46 mm × 15 mm (with MagJacks + M.2 module)

## 3. High-Speed Routing Rules

### 3.1 Ethernet Differential Pairs (1000Base-T)

| Parameter | Value |
|---|---|
| Impedance | 100Ω ± 10% differential |
| Single-ended Z0 | 50Ω ± 10% |
| Trace width | 5 mil (0.127 mm) |
| Diff pair spacing | 5 mil (0.127 mm) |
| Via stub length | ≤ 10 mil (use back-drill if needed) |
| Length matching | Within 10 mils intra-pair, 50 mils inter-pair |
| Max via count | 2 per diff pair (PHY to MagJack) |
| Guard trace | 20 mil clearance to other signals |

Each MagJack has 4 diff pairs (TX±, RX±, BI_DA±, BI_DB±). Route directly from MagJack pins to PHY pins on L1, transitioning through L2 GND vias at the PHY pad.

### 3.2 RGMII Signals (125 MHz clock domain)

| Parameter | Value |
|---|---|
| Impedance | 50Ω single-ended |
| Trace width | 6 mil (0.152 mm) on L1 |
| Max length | 4 inches (100 mm) |
| Length matching | All RGMII signals within 100 mils of each other per interface |
| Series termination | 49.9Ω ± 1% at source (U1 side) |
| Clock routing | Route TX_CLK and RX_CLK first, match data to clock |

### 3.3 SDIO (SDR50, 50 MHz)

| Parameter | Value |
|---|---|
| Impedance | 50Ω single-ended |
| Trace width | 6 mil on L1 |
| Length matching | Within 50 mils across all SD_D[0:3], CMD, CLK |
| Series resistor | 22Ω at source on CLK line only |
| Pull-ups | 10kΩ on CMD, D3 (at card slot) |

### 3.4 SPI (60 MHz max)

| Parameter | Value |
|---|---|
| Impedance | 50Ω single-ended |
| Trace width | 6 mil on L1 or L4 |
| Max length | 2 inches (50 mm) |
| Keep-away | ≥ 20 mil from Ethernet diff pairs |

## 4. Via Strategy

| Via Type | Drill (mil) | Pad (mil) | Usage |
|---|---|---|---|
| Standard via | 8 | 20 | General signal routing |
| Power via | 10 | 28 | Power planes, GND stitching |
| Micro via (if HDI) | 4 | 12 | BGA fanout only (not used — LQFP) |

### Via Placement Rules
1. **GND stitching**: Place GND vias every 200 mils along board edges and around high-speed signal vias (via fencing)
2. **Power transition**: Use minimum 2 power vias per power pin transition between L3 and components
3. **Diff pair vias**: Place return-path GND via within 50 mils of every signal via on diff pairs
4. **No vias on diff pairs** between PHY and MagJack except at component pads
5. **Via-in-pad**: Allowed only on QFN thermal pads (PHY, SoC)

## 5. Component Placement

### 5.1 Placement Zones

| Zone | Components | Priority |
|---|---|---|
| Zone A (left edge) | J1 MagJack, U2 PHY #1 | Shortest Ethernet diff pair path |
| Zone B (center-left) | U1 SoC, decoupling, RGMII terms | Central, close to both PHYs |
| Zone C (center) | U4, U5, U6 power, inductors | Between SoC and right side |
| Zone D (center-right) | U7 Flash, U8 EEPROM | Low-speed, flexible placement |
| Zone E (right edge) | J2 MagJack, U3 PHY #2 | Shortest Ethernet diff pair path |
| Zone F (bottom edge) | U9 M.2 socket, J4 microSD | Low-profile, edge-accessible |
| Zone G (bottom-left) | U3 TPS2378, PoE components | Near J1 CT pins |
| Zone H (bottom-right) | SW1, D1–D4 LEDs | User interface |

### 5.2 Placement Constraints

1. **U1 (SoC)**: Center of board, 100-LQFP oriented with ENET1 pins facing U2, ENET2 pins facing U3
2. **U2, U3 (PHYs)**: Within 1.5 inches of respective MagJack, diff pairs ≤ 2 inches total
3. **Decoupling caps**: Within 5 mm of IC power pins, connected via short wide traces (20 mil) to pin and via to GND
4. **Series termination resistors** (49.9Ω): Within 200 mils of U1 RGMII output pins
5. **Crystals**: Within 500 mils of respective IC, guard ring of GND vias
6. **Power inductors**: Not under SoC or PHY; maintain 2× inductor body spacing

## 6. Thermal Management

| Component | Power (max) | Thermal Method | Target Tj |
|---|---|---|---|
| U1 (i.MX RT1062) | 2.5 W | QFN thermal pad → L2 GND via array (9 vias) | ≤ 95°C |
| U2/U3 (88E1510) | 0.8 W each | QFN thermal pad → L2 GND via array (4 vias) | ≤ 85°C |
| U4 (TPS562201) | 0.3 W | SOT-563, copper pour on L1 | ≤ 100°C |
| U3 (TPS2378) | 0.5 W | SOIC-8, natural convection | ≤ 85°C |

### Thermal Via Arrays
- **SoC (U1)**: 3×3 array of 10 mil thermal vias under exposed pad, connected to L2 GND plane
- **PHYs (U2, U3)**: 2×2 array of 10 mil thermal vias under exposed pad
- All thermal vias filled with solder mask on top (prevent solder wicking) or use solder mask defined pads

### Airflow
- Device intended for passive cooling in free air
- No heatsink required at rated power
- Maximum ambient: 50°C (industrial)

## 7. Isolation Barriers

### 7.1 Ethernet Isolation
- MagJack integrated transformers provide 1500 Vrms isolation between line side and PHY side
- No additional isolation required; transformer is the barrier
- Keep ≥ 60 mil clearance between MagJack line-side pins (1–8) and any other circuitry

### 7.2 PoE Isolation
- TPS2378 internal bridge rectifier and detection circuitry are on the line side
- Maintain ≥ 60 mil clearance between PoE front-end and logic circuits
- Y-capacitor (1 nF, 2 kV) between PoE GND and system GND for EMI

### 7.3 Analog/Digital Isolation
- VCC_3V3A (PHY analog supply) separated from VCC_3V3 (digital) by ferrite bead
- VCC_3V3A plane island on L3, connected only at single point through ferrite
- Keep analog PHY pins (20, 21–28) away from digital RGMII routing

## 8. Ground Strategy

| Ground Domain | Scope | Connection |
|---|---|---|
| GND (system) | All digital ICs, L2 plane | Primary reference |
| GND_A (analog) | PHY VDDA decoupling | Single-point star to GND via ferrite |
| GND_POE | PoE front-end, TPS2378 | Y-cap to system GND, isolated otherwise |
| GND_CHASSIS | MagJack shield pins | Chassis ground, connect to GND at one point via 1 MΩ || 1 nF |

### Split Plane Rules
- L2 is a **single solid GND plane** with no splits under high-speed traces
- L3 power plane: split into islands for VCC_1V2, VCC_3V3, 5V_SYS, VCC_3V3A
- Island boundaries: 20 mil gap between power domains
- Each island has via connections to respective component power pins

## 9. Design Rules Summary

| Rule | Value |
|---|---|
| Minimum trace width | 4 mil (0.1 mm) |
| Minimum trace spacing | 4 mil (0.1 mm) |
| Minimum via drill | 8 mil (0.2 mm) |
| Minimum via pad | 20 mil (0.5 mm) |
| Minimum clearance (outer layer) | 4 mil |
| Minimum clearance (inner layer) | 5 mil |
| Solder mask clearance | 2 mil expansion |
| Silkscreen line width | 6 mil |
| Board thickness | 39 mil (1.0 mm) |
| Copper weight | 1 oz all layers |
| Surface finish | ENIG (Electroless Nickel Immersion Gold) |
| Solder mask | Green, LPI |
| Minimum annular ring | 4 mil |

## 10. DRC / DFM Checklist

- [ ] All differential pairs length-matched within spec
- [ ] No 90° trace angles (use 45° or curved)
- [ ] All power pins have decoupling caps within 5 mm
- [ ] No trace routing under crystals (keep-out zone)
- [ ] All test points accessible from top side
- [ ] MagJack shield pins tied to chassis ground
- [ ] PoE classification resistor correct value (127Ω for Class 3)
- [ ] Reset pull-up (10kΩ) on U1 POR_B
- [ ] Boot config resistors on U1 BOOT_MODE[1:0] pins (BOOT_MODE=00 for internal boot)
- [ ] All unused RGMII pins tied low (TXD2/3, RXD2/3)
- [ ] MicroSD card detect pin has pull-up
- [ ] M.2 key E pinout matches nRF52840-M.2 module