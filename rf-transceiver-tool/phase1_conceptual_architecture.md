# Phase 1: Conceptual Architecture — RF Transceiver Tool

## 1. Purpose

The RF Transceiver Tool is a portable, USB-powered multi-protocol RF research device enabling legitimate security researchers to:

- **Capture and analyze** Sub-GHz (300–928 MHz) wireless protocols using the TI CC1101 transceiver
- **Capture and analyze** 2.4 GHz protocols (Enhanced ShockBurst, custom GFSK) using the nRF24L01+
- **Replay** captured RF packets for protocol testing and vulnerability analysis
- **Fuzz** protocol implementations by injecting malformed packets
- **Enumerate** active channels and transmitters via RSSI scanning
- **Log** all traffic to host via USB CDC virtual serial port for offline analysis

**Target use cases:** Garage door & keyfob security research, weather station protocol reverse engineering, IoT device RF security testing, wireless mouse/keyboard injection research, amateur radio digital mode experimentation.

## 2. Performance Targets

| Metric | Target | Basis |
|---|---|---|
| Sub-GHz RX sensitivity | ≤ −110 dBm @ 1.2 kbps | CC1101 datasheet typical |
| Sub-GHz TX power | +10 dBm (configurable to +12 dBm) | CC1101 programmable output |
| 2.4 GHz RX sensitivity | ≤ −82 dBm @ 2 Mbps | nRF24L01+ datasheet |
| 2.4 GHz TX power | 0 dBm | nRF24L01+ programmable (−18..0 dBm) |
| Channel switch time | < 200 µs (Sub-GHz), < 130 µs (2.4 GHz) | CC1101 / nRF24L01+ specs |
| Packet throughput (USB) | ≥ 500 kbps sustained | USB FS CDC bulk |
| Capture-to-host latency | < 2 ms | MCU processing + USB polling |
| OLED refresh | 30 fps for live RSSI bars | I2C at 400 kHz |
| Power consumption | < 350 mA total @ 5 V USB | All ICs active, both radios TX |
| Boot time | < 500 ms to operational | STM32 clock config + peripheral init |

## 3. Constraints

| Constraint | Value | Rationale |
|---|---|---|
| BOM cost | ≤ $28 single qty | Sub-$100 budget with margin for PCB + assembly |
| PCB size | 85 × 54 mm | Credit-card footprint, portable |
| PCB layers | 4 | Cost vs signal integrity |
| Power source | USB 5 V only (no battery) | Simplifies design, always-tethered use case |
| USB connector | USB-C 2.0 | Modern, reversible |
| MCU package | UFQFPN-48 | Small, available on STM32F401CCU6 |
| RF connectors | 2× SMA edge-launch | Standard, allows external antennas |
| Operating temp | −20 °C to +70 °C | Extended industrial |
| Regulatory | Intentional radiator — lab use only | Not FCC-certified for general transmission |

## 4. Block Diagram

```
                          ┌──────────┐
                          │ USB-C 2.0│
                          │  (J1)    │
                          └────┬─────┘
                               │  USB D+/D−
                    ┌──────────┴──────────┐
                    │                     │
               ┌────┴─────┐        ┌──────┴──────┐
               │ USB ESD  │        │  AMS1117-3.3 │
               │ (U6)     │        │    (U4)      │
               └────┬─────┘        └──────┬───────┘
                    │                     │ 3V3
          ┌─────────┴─────────┐    ┌──────┴──────────────────┐
          │                    │    │                         │
    ┌─────┴──────┐      ┌─────┴────┴──┐    ┌───────────────┐│
    │ STM32F401  │      │              │    │               ││
    │ CCU6 (U1)  │      │              │    │               ││
    │            │      │              │    │               ││
    │  SPI1 ────┼──────►│ CC1101 (U2)  │    │               ││
    │  SPI2 ────┼──┐    │              │    │               ││
    │            │  │   │              │    │               ││
    │  I2C1 ────┼──┼───┼──────────────┼────┤► SSD1306 (U5) ││
    │            │  │   │              │    │               ││
    │  GPIO ────┼──┼───┼──────────────┼────┤► LED1/LED2     ││
    │            │  │   │              │    │               ││
    │  GPIO ────┼──┼───┼──────────────┼────┤► BTN1/BTN2     ││
    │            │  │   │              │    └───────────────┘│
    │            │  │   └──────┬───────┘         │ 3V3      │
    │            │  │          │                  │          │
    │            │  │   ┌──────┴───────┐         │          │
    │            │  └──►│ nRF24L01+    │         │          │
    │            │      │    (U3)       │         │          │
    │            │      └──────┬───────┘         │          │
    │            │             │                  │          │
    └────────────┘      ┌─────┴─────┐    ┌──────┴──────┐   │
                         │  SMA (J2) │    │  SMA (J3)   │   │
                         │  Sub-GHz  │    │  2.4 GHz    │   │
                         └───────────┘    └─────────────┘   │
                                                            │
    Decoupling: C1–C12 (100nF) + C13–C14 (10µF) on 3V3 rail │
    Bulk: C15 (47µF) on 5V USB rail ─────────────────────────┘
```

## 5. Bus Topology

### 5.1 SPI Bus Assignment

| Bus | Peripheral | Pins (STM32) | Speed | Mode |
|---|---|---|---|---|
| SPI1 | CC1101 (U2) | PA5=CLK, PA6=MISO, PA7=MOSI, PA4=CSN | 10 MHz max | Mode 0 (CPOL=0, CPHA=0) |
| SPI2 | nRF24L01+ (U3) | PB13=CLK, PB14=MISO, PB15=MOSI, PB12=CSN | 10 MHz max | Mode 0 (CPOL=0, CPHA=0) |

### 5.2 I2C Bus

| Bus | Peripheral | Pins (STM32) | Speed | Address |
|---|---|---|---|---|
| I2C1 | SSD1306 (U5) | PB6=SCL, PB7=SDA | 400 kHz | 0x3C |

### 5.3 GPIO Assignments

| Pin | Direction | Function | Active |
|---|---|---|---|
| PA0 | Input | BTN1 (mode select) | Low (pulled high) |
| PA1 | Input | BTN2 (action/trigger) | Low (pulled high) |
| PB0 | Output | LED1 (Sub-GHz status) | High |
| PB1 | Output | LED2 (2.4 GHz status) | High |
| PA8 | Output | CC1101 GDO0 (TX/RX data) | — |
| PA9 | Output | CC1101 GDO2 (FIFO/status) | — |
| PB3 | Input | nRF24L01+ IRQ | Low |
| PA3 | Output | CC1101 RESETn | Low (active reset) |
| PA15 | Output | nRF24L01+ CE (chip enable) | High (TX/RX enable) |

### 5.4 USB

| Pin | Function |
|---|---|
| PA11 | USB DM (USB 2.0 FS negative) |
| PA12 | USB DP (USB 2.0 FS positive) |

## 6. Power Domains

```
USB 5V ─────────────────────────────────────────────────────┐
     │                                                       │
     ├──► VBATT (5V domain)                                  │
     │     ├── USB ESD protection (U6)                        │
     │     ├── Bulk cap C15 (47µF)                           │
     │     └── VBUS sense to STM32 PA9 (via divider)          │
     │                                                       │
     └──► AMS1117-3.3 (U4) ──► 3V3 (3.3V domain)            │
           │                                                  │
           ├── STM32 VDD (U1) × 4 pins                       │
           ├── CC1101 VDD (U2)                                │
           ├── nRF24L01+ VDD (U3)                             │
           ├── SSD1306 VDD (U5, via I2C 3V3)                  │
           ├── Decoupling: C1–C12 (100nF) per IC VDD pin     │
           ├── Bulk: C13 (10µF) + C14 (10µF)                  │
           └── LED current limiting via resistors               │
```

### Power Budget

| Component | Typical | Max | Notes |
|---|---|---|---|
| STM32F401 @ 84 MHz | 30 mA | 55 mA | All clocks on |
| CC1101 RX | 15 mA | 17 mA | @ 868 MHz |
| CC1101 TX (+10 dBm) | 30 mA | 34 mA | |
| nRF24L01+ RX | 13 mA | 14 mA | @ 0 dBm |
| nRF24L01+ TX | 11 mA | 12 mA | @ 0 dBm |
| SSD1306 OLED | 10 mA | 20 mA | Full white |
| AMS1117 quiescent | 5 mA | 10 mA | Ground current |
| **Total (both TX)** | **~100 mA** | **~150 mA** | Well within USB 500 mA |

## 7. Thermal Considerations

- **CC1101** dissipates up to 100 mW at +12 dBm output — thermal pad on QFN package tied to ground pour
- **STM32F401** up to 200 mW — thermal pad on UFQFPN tied to ground pour
- **nRF24L01+** negligible (30 mW TX)
- No heatsinks required; 4-layer PCB with solid ground planes provides adequate thermal spreading
- Thermal relief spokes on power plane connections for LDO and MCU