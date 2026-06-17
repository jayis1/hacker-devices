# Phase 3: PCB Blueprints & Layout — HDMI-Siphon

**Author: jayis1**  
**Version: 1.0.0**

---

## 1. PCB Stackup

4-layer FR-4, 1.6 mm total thickness, ENIG finish, 1 oz copper all layers:

| Layer | Type | Thickness | Notes |
|---|---|---|---|
| Top | Signal | 35 µm copper | Components, HDMI traces, pixel data bus |
| Inner 1 | GND | 35 µm copper | Continuous ground plane, no splits |
| Inner 2 | Power | 35 µm copper | 5V, 3.3V, 1.8V power planes |
| Bottom | Signal | 35 µm copper | SDRAM routing, I²C, SPI, SD card traces |

Dielectric: FR-4 (εr = 4.5), 0.2 mm between layers

## 2. Critical Routing Rules

| Signal Group | Impedance | Length Match | Max Length | Notes |
|---|---|---|---|---|
| Pixel data bus (24-bit) | 50Ω ±10% | ±10 mm within group | 80 mm | All 24 bits + syncs + clock |
| SDRAM bus | 50Ω ±10% | ±5 mm for address, ±2 mm for data | 40 mm | Match data group DQ0-DQ15 |
| HDMI TMDS pairs | 100Ω ±10% differential | ±0.5 mm within pair, ±5 mm between pairs | 30 mm | On top layer only |
| I²C bus | — | No matching | 100 mm | Route with 10k pull-ups |
| SPI bus | 50Ω ±10% | ±20 mm between signals | 100 mm | 20 MHz, keep short |

## 3. Trace Width & Spacing

| Signal | Width (mm) | Spacing (mm) | Notes |
|---|---|---|---|
| Pixel data (50Ω) | 0.254 | 0.254 | 10 mil trace, 10 mil gap |
| SDRAM data (50Ω) | 0.254 | 0.254 | Match within group |
| HDMI diff pair (100Ω) | 0.150 | 0.150 | 6 mil trace, 6 mil gap on top |
| SPI (50Ω) | 0.254 | 0.254 | Keep away from pixel bus |
| I²C | 0.254 | 0.300 | Minimal crosstalk concern |
| Power (3.3V, 5V) | 0.500 | 0.300 | 1.5A capable |
| Power (1.8V) | 0.400 | 0.300 | 0.5A capable |
| GND fill | 0.500 | — | Copper pour all layers |

## 4. Via Strategy

| Via Type | Diameter | Hole Size | Use |
|---|---|---|---|
| Signal via | 0.500 mm | 0.250 mm | General signal routing |
| Power via | 0.700 mm | 0.400 mm | Power distribution, 2+ per net |
| Thermal via | 0.500 mm | 0.250 mm | Under FPGA and SDRAM for thermal |
| HDMI via | AVOID | — | No vias in HDMI differential pairs |

## 5. Component Placement

```
┌───────────────────────────────────────────────────────┐
│                   65 mm                               │
│ ┌────────────┐                    ┌────────────┐     │
│ │  HDMI IN   │                    │ HDMI OUT   │     │
│ │  (J1)      │                    │  (J2)      │     │
│ └─────┬──────┘                    └─────┬──────┘     │
│       │                                 │            │
│ ┌─────▼──────┐                   ┌──────▼─────┐     │
│ │  TFP401    │───24-bit bus───▶│  TFP410     │     │
│ │  (U1)     │                   │  (U2)       │     │
│ └─────┬──────┘                   └──────┬─────┘     │
│       │    ┌───────────────────┐        │            │
│       └────│  FPGA iCE40UP5K  │────────┘            │
│            │  (U3)            │                      │
│            └────────┬─────────┘                      │
│                     │                                │
│            ┌────────▼─────────┐                      │
│            │  SDRAM 32MB      │                      │
│            │  IS42S16160 (U5) │                      │
│            └──────────────────┘                      │
│                                                      │
│  ┌────────────┐      ┌───────────────┐               │
│  │ ESP32-S3  │      │ microSD (J4)   │               │
│  │  (U4)     │◀────▶│               │               │
│  └─────┬──────┘      └───────────────┘               │
│        │                                              │
│  ┌─────▼──────┐      ┌───────────────┐               │
│  │  CH340C   │      │ USB-C (J3)    │               │
│  │  (U9)     │◀────▶│               │               │
│  └────────────┘      └───────────────┘               │
│                                                      │
│  ┌────────────┐      ┌───────────────┐               │
│  │  Battery   │      │  Li-Po (BT1)  │               │
│  │  Charger   │◀────▶│  300mAh       │               │
│  │  MCP73831  │      └───────────────┘               │
│  │  (U10)    │                                       │
│  └────────────┘                                       │
│                                                      │
│  ┌────────────┐  ┌────────────┐  ┌──────────────┐   │
│  │ LDO 3.3V  │  │ LDO 1.8V  │  │ EDID EEPROM │   │
│  │ (U11)     │  │ (U12)     │  │ 24LC02 (U6) │   │
│  └────────────┘  └────────────┘  └──────────────┘   │
└───────────────────────────────────────────────────────┘
             45 mm
```

## 6. Thermal Management

- **TFP401 (U1):** Small heatsink area on bottom layer GND via array (4×4 vias under IC)
- **TFP410 (U2):** Similar via array, lower power dissipation
- **FPGA (U3):** 6 thermal vias under center pad to inner GND plane
- **ESP32-S3 (U4):** Module includes internal heatsink; no additional required
- **LDOs (U11, U12):** Keep away from other heat sources; free airflow

## 7. DFM Notes

- Minimum trace width: 0.150 mm (6 mil)
- Minimum spacing: 0.150 mm (6 mil)
- Minimum via diameter: 0.500 mm (20 mil)
- Minimum hole size: 0.250 mm (10 mil)
- Solder mask clearance: 0.075 mm (3 mil)
- Silkscreen line width: 0.150 mm (6 mil)
- Edge clearance to copper: 0.300 mm (12 mil)
- Castellated edges: None

**Author: jayis1**
