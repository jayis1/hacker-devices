# Phase 3: PCB Blueprints & Layout — VoltGlitcher Pro

## 4-Layer PCB Design for High-Precision Glitch Injection

**Document Version:** 1.0  
**Date:** 2026-01-15  
**Author:** Hacker Devices  
**Status:** Final

---

## 1. PCB Stack-up

### 1.1 Layer Assignment

| Layer | Name | Type | Thickness | Material | Usage |
|-------|------|------|-----------|----------|-------|
| 1 | F.Cu | Signal | 35 µm (1oz) | Copper | Component side, critical routing |
| 2 | In1.Cu | Signal | 35 µm (1oz) | Copper | Inner signal, high-speed traces |
| 3 | In2.Cu | Plane | 35 µm (1oz) | Copper | GND plane (solid) |
| 4 | B.Cu | Signal | 35 µm (1oz) | Copper | Bottom side, power routing |
| — | Core | — | 1.2 mm | FR4 | Core dielectric |
| — | Prepreg | — | 0.2 mm | FR4 | Outer prepreg |

**Total board thickness:** 1.6 mm  
**Board dimensions:** 120 mm × 80 mm  
**Surface finish:** ENIG (Electroless Nickel Immersion Gold)  
**Solder mask:** Green (both sides)  
**Silkscreen:** White (both sides)  
**Copper weight:** 1 oz (35 µm) all layers

### 1.2 Design Rules

| Parameter | Value | Notes |
|-----------|-------|-------|
| Minimum trace width | 0.10 mm (4 mil) | Standard signals |
| Minimum trace spacing | 0.10 mm (4 mil) | |
| Minimum via diameter | 0.40 mm | Laser-drilled micro-vias not used |
| Minimum via drill | 0.20 mm | |
| Minimum pad size | 0.60 mm | For 0.5mm pitch ICs |
| Minimum solder mask web | 0.05 mm | Between pads |
| Impedance control | 50 Ω ±10% | For USB D+/D- only |
| Glitch trace width | 1.0 mm | High-current glitch outputs |
| Power trace width | 0.4 mm | Supply rails |

---

## 2. Floor Planning

### 2.1 Functional Zones

```
┌──────────────────────────────────────────────────────────┐
│                    VoltGlitcher Pro PCB                    │
│                    120mm × 80mm                            │
│                                                            │
│  ┌──────────┐  ┌──────────────────┐  ┌────────────────┐   │
│  │  USB &   │  │    MCU Zone      │  │   FPGA Zone    │   │
│  │  Power   │  │  STM32F407VGT6   │  │ iCE40HX1K-TQ144│   │
│  │  Zone    │  │  (LQFP-100)     │  │  (TQFP-144)    │   │
│  │          │  │                  │  │                │   │
│  │  J1 USB  │  │  Y1 8MHz        │  │  Y3 12MHz      │   │
│  │  U2 1.2V │  │  Y2 32kHz       │  │  U9 EEPROM     │   │
│  │  U3 3.3V │  │  U10 INA219     │  │                │   │
│  │  U4 1.8V │  │                  │  │                │   │
│  │  U5 2.5V │  │                  │  │                │   │
│  └──────────┘  └──────────────────┘  └────────────────┘   │
│                                                            │
│  ┌──────────────────────────────────────────────────────┐  │
│  │              Glitch Output Zone                       │  │
│  │                                                       │  │
│  │  U6 Gate Driver ─── Q1 SiS426DN ─── J3 GLITCH_OUT    │  │
│  │  U7 INA210 ──── Q2 IRLML6344 ──── J3 EM_COIL        │  │
│  │                   Q3 BSS84P ─────── J3 CLK_GLITCH    │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                            │
│  ┌─────────────┐  ┌─────────────┐  ┌──────────────────┐    │
│  │ J2 SWD     │  │ J4 TRIG_IN │  │ J5 TARGET_UART   │    │
│  │ Debug      │  │ U8 Opto    │  │                   │    │
│  └─────────────┘  └─────────────┘  └──────────────────┘    │
│                                                            │
│  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────────────┐  │
│  │D1    │  │D2    │  │D3    │  │D4    │  │ J6 microSD   │  │
│  │Power │  │Glitch│  │Trig  │  │Error │  │              │  │
│  └──────┘  └──────┘  └──────┘  └──────┘  └──────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### 2.2 Zone Rationale

**USB & Power Zone (left edge):**
- USB connector at board edge for cable strain relief
- Voltage regulators close to USB to minimize 5V trace length
- Bulk capacitors near regulator outputs
- Keep switching noise away from sensitive analog circuits

**MCU Zone (center-left):**
- STM32F407 centered for balanced routing
- Crystals close to MCU (minimize trace length for oscillator stability)
- Decoupling caps on opposite side directly under IC

**FPGA Zone (center-right):**
- iCE40HX1K adjacent to MCU for short SPI traces
- Configuration flash accessible but not on main board (uses SD card or USB upload)
- EEPROM near I2C bus between MCU and FPGA

**Glitch Output Zone (bottom):**
- MOSFETs and gate driver close to output connectors
- Minimize inductance on glitch output paths
- Heavy copper pours on F.Cu and B.Cu for thermal dissipation
- Gate driver U6 between MCU outputs and Q1 gate

---

## 3. Critical Routing

### 3.1 Glitch Output Traces

**Design requirements:**
- Must carry 10A peak current (Q1 shunt path)
- Must have minimal inductance (<5 nH) for fast rise times
- Must be thermally robust for repeated pulsing

**Implementation:**
- Trace width: 1.0 mm on F.Cu
- Parallel copper pour on B.Cu (via-stitched every 2mm)
- 4× via array at MOSFET source/drain pads
- No sharp corners (45° or curved bends only)
- Keep-away from signal traces: 0.5 mm minimum

**Via stitching pattern:**
```
  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○   (F.Cu trace + B.Cu pour)
  │ │ │ │ │ │ │ │ │ │ │   (vias every 2mm)
  ○ ○ ○ ○ ○ ○ ○ ○ ○ ○ ○   (B.Cu copper pour)
```

### 3.2 USB D+/D- Differential Pair

**Design requirements:**
- 90 Ω differential impedance (USB 2.0 FS spec)
- Matched length ±0.5 mm
- No vias (route entirely on F.Cu)
- ESD protection close to connector

**Implementation:**
- Trace width: 0.25 mm
- Trace spacing: 0.15 mm
- Calculated Zdiff ≈ 90 Ω for 4-layer FR4 stackup
- Length: ~30 mm (connector to MCU)
- ESD diode (PRTR5V0U2X) within 5 mm of connector

### 3.3 FPGA SPI Interface (SPI1)

**Design requirements:**
- 20 MHz SPI clock → 50 ns period
- Clean signal integrity for reliable register access
- CS must be glitch-free

**Implementation:**
- SCK, MISO, MOSI: 0.15 mm traces, In1.Cu (inner layer)
- Route as star from MCU to FPGA (point-to-point)
- CS: F.Cu, 0.15 mm, directly from PA4 to FPGA pin
- Series termination resistors: 22 Ω on SCK and MOSI (near source)

### 3.4 ADC Input Traces

**Design requirements:**
- Low noise (ADC has 12-bit resolution = 0.8 mV LSB)
- Guard traces around sensitive inputs
- RC filter close to ADC pins

**Implementation:**
- PA0 (Target VCC): Route on In1.Cu with GND guard on both sides
- PA6 (Shunt current): Same treatment
- RC filter: 1 kΩ + 100 pF at ADC pin → fc ≈ 1.6 MHz (anti-alias)
- Analog supply (VDDA): LC filter (10 µH + 4.7 µF) from digital 3.3V

---

## 4. Power Distribution Network

### 4.1 Ground Plane (In2.Cu)

- Solid copper pour on In2.Cu (no splits)
- All GND pins connect via thermal reliefs to ground plane
- Via stitching: every 3 mm around board perimeter
- Via fencing: around glitch output zone to contain return currents

### 4.2 Power Rails

| Rail | Routing Layer | Width | Decoupling |
|------|-------------|-------|------------|
| VCC_5V | B.Cu | 0.8 mm | 100 µF Al + 10 µF Cer per reg |
| VCC_3V3 | B.Cu | 0.6 mm | 10 µF per 2 ICs + 100 nF per pin |
| VCC_1V2 | F.Cu (local) | 0.4 mm | 10 µF near FPGA + 100 nF per VCC pin |
| VCC_1V8 | F.Cu (local) | 0.3 mm | 4.7 µF near FPGA |
| VCC_2V5 | F.Cu (local) | 0.3 mm | 4.7 µF near FPGA |

### 4.3 Decoupling Strategy

**MCU (STM32F407):**
- 7 VDD/VSS pairs, each with 100 nF 0402 cap
- 2 bulk caps: 4.7 µF 0805 near VDD pins
- VDDA: 10 µH + 4.7 µF LC filter + 100 nF

**FPGA (iCE40HX1K):**
- 12 VCC_CORE pins: each with 100 nF 0402 + shared 10 µF 0805
- 8 VCC_IO pins: each with 100 nF 0402 + shared 4.7 µF 0805
- 2 VCC_PLL pins: 100 nF + 10 µF (low-ESR critical for PLL stability)
- 2 VCC_AUX pins: 100 nF + 4.7 µF

**Total decoupling caps:** ~120 (0402) + ~15 (0805) + 2 (tantalum)

---

## 5. Thermal Management

### 5.1 Heat Sources

| Component | Power Dissipation | Cooling Method |
|-----------|------------------|----------------|
| STM32F407 (168 MHz) | ~0.4W | PCB copper |
| iCE40HX1K (active) | ~0.2W | PCB copper + fan |
| Q1 SiS426DN (glitching) | ~3.3W peak (100ns) | Thermal via array |
| U2 TLV62569 (3A) | ~0.3W | PCB copper |
| U3 TLV62569 (3A) | ~0.3W | PCB copper |

### 5.2 Thermal Via Arrays

For Q1 (SiS426DN) MOSFET:
- 4×4 array of 0.6mm vias under drain pad
- Connected to B.Cu copper pour (50 mm × 30 mm)
- Thermal resistance: ~15°C/W (junction to ambient)

### 5.3 Cooling Fan

- 25 mm × 25 mm × 10 mm 5V brushless fan
- PWM speed control via FPGA (25 kHz carrier)
- Position: above FPGA zone, blowing across board
- Auto-regulated based on FPGA temperature reading

---

## 6. Connector Pinout

### 6.1 J3: GLITCH_OUT (1×6, 2.54mm pitch)

| Pin | Signal | Notes |
|-----|--------|-------|
| 1 | VCC_TARGET | Direct connection to target VCC (for shunt) |
| 2 | GND | Ground reference (connect to target GND) |
| 3 | EM_COIL+ | EM coil positive terminal |
| 4 | EM_COIL- | EM coil negative / return |
| 5 | TARGET_CLK | Clock signal pass-through (for clock glitch) |
| 6 | CLK_GND | Clock ground reference |

### 6.2 J4: TRIG_IN (1×4, 2.54mm pitch)

| Pin | Signal | Notes |
|-----|--------|-------|
| 1 | TRIG0_GPIO | Opto-isolated GPIO trigger input |
| 2 | TRIG1_UART_RX | Target UART receive (for pattern match) |
| 3 | TRIG2_JTAG | JTAG pass-through (TCK/TMS/TDI/TDO via separate header) |
| 4 | GND_ISO | Isolated ground for trigger inputs |

### 6.3 J5: TARGET_UART (1×4, 2.54mm pitch)

| Pin | Signal | Notes |
|-----|--------|-------|
| 1 | TX | MCU USART1_TX → Target RX |
| 2 | RX | Target TX → MCU USART1_RX |
| 3 | GND | Shared ground |
| 4 | VCC_SENSE | Target VCC sense input |

---

## 7. Manufacturing Notes

### 7.1 PCB Fabrication

- 4-layer FR4, 1.6 mm total thickness
- ENIG surface finish (good for fine-pitch + wire bonding not needed)
- Green solder mask, white silkscreen
- Electrical test (100% netlist comparison)
- IPC Class 2 (commercial)

### 7.2 Assembly

- All components SMD (no through-hole except pin headers)
- Reflow soldering (SAC305 lead-free paste)
- Manual placement for fine-pitch ICs (0.5mm pitch)
- Pin headers: wave solder or manual
- Inspection: AOI + X-ray for BGA/QFN if used

### 7.3 Test Points

- TP1: VCC_5V (test pad near USB)
- TP2: VCC_3V3 (test pad near MCU)
- TP3: VCC_1V2 (test pad near FPGA)
- TP4: GND (test pad, board center)
- TP5: GLITCH_SHUNT (test pad near Q1)
- TP6: ADC_IN0 (test pad near PA0)
- TP7: SPI1_SCK (test pad for protocol debug)
- TP8: FPGA_CDONE (test pad for config verification)

---

## 8. Design Verification Checklist

- [x] All signal traces meet minimum width/spacing
- [x] Differential pairs impedance-controlled (USB D+/D-)
- [x] Glitch output traces oversized for current capacity
- [x] Ground plane unbroken under critical components
- [x] Power sequencing circuit verified (EN pins, PG pins)
- [x] Decoupling caps placed within 2mm of power pins
- [x] Crystal load capacitors calculated and placed
- [x] ESD protection on all external connectors
- [x] Thermal vias under high-power components
- [x] Keep-out zones around analog circuits
- [x] No high-speed signals crossing analog inputs
- [x] Fan mounting holes provided
- [x] Board outline includes mounting holes (M2.5 × 4)