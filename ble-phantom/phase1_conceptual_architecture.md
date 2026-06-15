# BLE Phantom — Phase 1: Conceptual Architecture

## 1. System Purpose

BLE Phantom is a dual-radio Bluetooth Low Energy security research platform that enables comprehensive offensive and defensive analysis of BLE 5.x ecosystems. It provides simultaneous advertising, scanning, connection establishment, and active MITM capabilities across two independent BLE radios — allowing security researchers to intercept, modify, inject, and replay BLE traffic in real time.

### Primary Use Cases
- **BLE sniffing**: Passive capture of advertising, scan request/response, and data channel traffic
- **Connection hijacking**: Active MITM between two BLE devices by establishing parallel connections
- **GATT fuzzing**: Automated mutation of GATT read/write operations to discover vulnerabilities
- **Address spoofing**: Rapid cycling of random/identity addresses for privacy analysis
- **Key extraction**: Capture and offline analysis of pairing exchanges (LE Secure Connections, Legacy)
- **Replay attacks**: Record and replay advertising payloads and data channel PDUs
- **Jamming detection**: RSSI monitoring and channel occupancy analysis
- **OTA firmware analysis**: Intercept DFU/OTA updates in transit

## 2. Attack Surface

| Target | Technique | Radio Mode |
|--------|-----------|------------|
| BLE Peripherals | Advertisement spoofing, scan response injection | Advertising |
| BLE Central devices | Rogue peripheral emulation, connection flood | Advertising + Data |
| Established connections | Connection hijack via LL spoofing | Data channel |
| Pairing process | Passive MITM on pairing exchange | Data channel (both radios) |
| GATT services | Read/write fuzzing, descriptor manipulation | Data channel |
| Privacy mechanisms | Address rotation tracking, IRK extraction | Advertising |
| OTA updates | Firmware interception, payload modification | Data channel |

## 3. Performance Targets

| Parameter | Target |
|-----------|--------|
| Dual-radio simultaneous operation | Both radios active concurrently |
| Advertising interval (TX) | 20 ms – 10.24 s configurable |
| Scan window | 2.5 ms – 10.24 s configurable |
| Packet capture throughput | ≥ 2 Mbit/s aggregate |
| Connection interval (MITM) | 7.5 ms – 4.0 s |
| Latency (relay MITM) | ≤ 2 ms added round-trip |
| Capture buffer | 256 KB ring buffer per radio |
| Battery life (active sniff) | ≥ 8 hours |
| Battery life (standby) | ≥ 72 hours |
| USB data transfer | Full-Speed (12 Mbit/s) |

## 4. Constraints

- **BOM cost**: < $100 at quantity 100
- **Form factor**: 80 mm × 40 mm USB dongle (USB-A plug)
- **Power**: USB bus-powered (5 V, ≤ 500 mA), optional 3.7 V LiPo (600 mAh)
- **Thermal**: Passive cooling only, industrial temp range −20°C to +60°C
- **Regulatory**: Intentional radiator — lab/research use only, not FCC-certified
- **Firmware**: ARM Cortex-M4F, bare-metal with RTOS, no Linux overhead
- **Software**: React Native companion app via USB-CDC serial bridge

## 5. Block Diagram

```
┌──────────────────────────────────────────────────────────────────┐
│                        BLE Phantom                               │
│                                                                  │
│  ┌──────────────┐         ┌──────────────────────┐              │
│  │  USB-A Plug  │◄───────►│  STM32F401CCU6       │              │
│  │  (VBUS/D+/D─│  USB    │  Cortex-M4F 84 MHz   │              │
│  │     /GND)    │  FS     │  256 KB Flash        │              │
│  └──────────────┘         │  64 KB SRAM          │              │
│                           │                      │              │
│  ┌──────────────┐         │  ┌─────────────┐     │              │
│  │  nRF52832    │  SPI0  ◄├─►│ SPI Master  │     │              │
│  │  Radio A     │◄──────►│  │  (Radio A)  │     │              │
│  │  BLE 5.x    │  + IRQ  │  └─────────────┘     │              │
│  │  2.4 GHz    │         │                      │              │
│  └──┬──────────┘         │  ┌─────────────┐     │              │
│     │ Ant A              │  │ SPI Master  │     │              │
│  ┌──┴──────────┐  SPI1  ◄├─►│  (Radio B)  │     │              │
│  │  nRF52832   │◄──────►│  └─────────────┘     │              │
│  │  Radio B    │  + IRQ  │                      │              │
│  │  BLE 5.x    │         │  ┌─────────────┐     │              │
│  │  2.4 GHz    │         │  │  USB CDC    │     │              │
│  └──┬──────────┘         │  │  + DFU      │     │              │
│     │ Ant B              │  └─────────────┘     │              │
│                           │                      │              │
│  ┌──────────────┐         │  ┌─────────────┐     │              │
│  │  LiPo Charger│◄───────┤──┤ Power Mgmt  │     │              │
│  │  MCP73831    │  I2C   │  │  + Battery  │     │              │
│  │  + 600mAh   │         │  │  Gauge      │     │              │
│  └──────────────┘         │  └─────────────┘     │              │
│                           └──────────────────────┘              │
│                                                                  │
│  ┌──────────────┐                                                │
│  │  Status LEDs │◄── GPIO (4: Radio A, Radio B, USB, Power)    │
│  └──────────────┘                                                │
│                                                                  │
│  ┌──────────────┐                                                │
│  │  User Button │─── GPIO (mode switch, trigger capture)         │
│  └──────────────┘                                                │
└──────────────────────────────────────────────────────────────────┘
```

## 6. Data Flow

### 6.1 Capture Flow (Sniff Mode)

```
Antenna A ──► nRF52832 Radio A ──► SPI0 ──► STM32F401 Ring Buffer ──► USB CDC ──► Host/App
Antenna B ──► nRF52832 Radio B ──► SPI1 ──► STM32F401 Ring Buffer ──► USB CDC ──► Host/App
```

### 6.2 MITM Relay Flow

```
Victim Peripheral ◄──► Radio A (connected) ◄──► STM32F401 (relay logic) ◄──► Radio B (connected) ◄──► Victim Central
```

Packets from Radio A are:
1. Received via SPI + interrupt
2. Parsed (LL header, L2CAP, ATT)
3. Optionally modified (GATT rewrite, PDU mutation)
4. Forwarded to Radio B for transmission
5. Vice versa for the return path

### 6.3 Advertising Flood Flow

```
STM32F401 ──► SPI0 ──► nRF52832 Radio A ──► Antenna A (broadcast spoofed ADV)
STM32F401 ──► SPI1 ──► nRF52832 Radio B ──► Antenna B (listen for connections)
```

## 7. Bus Topology

| Bus | Master | Slaves | Protocol | Speed | Purpose |
|-----|--------|--------|----------|-------|---------|
| SPI0 | STM32F401 | nRF52832 Radio A | SPI Mode 0, 8-bit | 8 MHz | Radio A command/data |
| SPI1 | STM32F401 | nRF52832 Radio B | SPI Mode 0, 8-bit | 8 MHz | Radio B command/data |
| USB | STM32F401 | Host PC | USB 2.0 FS | 12 Mbit/s | Data transfer, control |
| I2C1 | STM32F401 | MCP73831 (via I2C gauge) | I2C 100 kHz | 100 kHz | Battery charge status |
| GPIO | STM32F401 | nRF52832 x2 (IRQ), LEDs, Button | — | — | Interrupts, status, input |

### 7.1 SPI Protocol (STM32 ↔ nRF52832)

The nRF52832 runs a custom SPI slave firmware. Protocol frame format:

```
┌──────────┬──────────┬──────────┬──────────┬──────────────┬──────────┐
│ SYNC     │ CMD      │ LEN      │ PAYLOAD  │ CRC16       │ TRAILER  │
│ 0xAA55   │ 1 byte   │ 2 bytes  │ N bytes  │ 2 bytes     │ 0x55AA  │
└──────────┴──────────┴──────────┴──────────┴──────────────┴──────────┘
```

Commands: `0x01=RADIO_INIT`, `0x02=RADIO_START_ADV`, `0x03=RADIO_START_SCAN`, `0x04=RADIO_CONNECT`, `0x05=RADIO_TX_DATA`, `0x06=RADIO_RX_DATA`, `0x07=RADIO_SET_CHAN`, `0x08=RADIO_SET_TX_PWR`, `0x10=RADIO_GET_STATUS`, `0xFF=RESET`

### 7.2 IRQ Lines

Each nRF52832 has one active-low IRQ line to the STM32F401:
- **Radio A IRQ** → STM32 PA0 (EXTI0)
- **Radio B IRQ** → STM32 PA1 (EXTI1)

IRQ asserts when: RX buffer has data, TX complete, connection event, error.

## 8. Memory Map

| Region | Address Range | Size | Purpose |
|--------|--------------|------|---------|
| Flash | 0x0800 0000 – 0x0803 FFFF | 256 KB | Application firmware |
| SRAM | 0x2000 0000 – 0x2000 FFFF | 64 KB | Heap, stack, buffers |
| CCMSRAM | 0x1000 0000 – 0x1000 FFFF | 16 KB | Radio A capture buffer |
| Peripheral | 0x4000 0000 – 0x4002 3FFF | — | APB1/APB2 peripherals |
| SPI0 RX buf | 0x2000 4000 – 0x2000 BFFF | 32 KB | Radio A DMA ring |
| SPI1 RX buf | 0x2000 C000 – 0x2001 3FFF | 32 KB | Radio B DMA ring |

## 9. Power Architecture

```
VBUS (5V) ──┬──► MCP73831 ──► LiPo (3.7V 600mAh) ──┐
             │                                          │
             └──► LDO (3.3V) ◄─────────────────────────┘
                        │
                        ├──► STM32F401 (VDD 3.3V, ~50 mA)
                        ├──► nRF52832 Radio A (VDD 3.3V, ~15 mA)
                        ├──► nRF52832 Radio B (VDD 3.3V, ~15 mA)
                        └──► LEDs, logic (VDD 3.3V, ~20 mA)

Peak current: ~120 mA (both radios TX + MCU active)
Battery backup: ~5 hours standalone operation
```

## 10. State Machine

```
┌─────────┐     ┌──────────┐     ┌───────────┐     ┌──────────┐
│  IDLE   │────►│ SNIFFING │────►│ CONNECTED │────►│  MITM    │
│         │     │          │     │           │     │ ACTIVE   │
└─────────┘     └──────────┘     └───────────┘     └──────────┘
     │               │                  │                  │
     │               │                  │                  │
     ▼               ▼                  ▼                  ▼
┌─────────┐     ┌──────────┐     ┌───────────┐     ┌──────────┐
│  DFU    │     │ SCANNING │     │  PAIRING  │     │ REPLAY   │
│  MODE   │     │          │     │ CAPTURE   │     │ MODE     │
└─────────┘     └──────────┘     └───────────┘     └──────────┘
```

Mode transitions triggered by USB commands or user button presses.