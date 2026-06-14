# NFC Relay Phantom — Phase 1: Conceptual Architecture

## 1. System Purpose

**NFC Relay Phantom** is a pocket-sized, battery-powered multi-protocol NFC/RFID security research platform. It provides real-time sniffing of NFC-A/B/F/V (ISO 14443, ISO 15693, FeliCa) and legacy 125 kHz RFID (EM4100, HID Prox, T5577), card emulation/cloning, relay attack proxying, and Mifare Classic key recovery — all controlled via BLE or USB-C from a companion mobile app.

**Target use-cases:**
- Relay attack demonstration (card ↔ proxy reader ↔ proxy card ↔ real reader)
- Mifare Classic / Ultralight key brute-forcing and nested authentication
- ISO 14443 A/B frame-level sniffing and replay
- 125 kHz RFID cloning (EM4100, HID Prox II, T5577 write-back)
- NFC tag NDEF analysis and manipulation
- Payment terminal protocol fuzzing (EMV Contactless L1/L2)

> **⚠️ Legal Notice:** This device is designed for **authorized security research and penetration testing only**. Unauthorized interception or cloning of access credentials is illegal. Always obtain proper authorization before testing any system you do not own.

---

## 2. Attack Surface

| Interface | Protocol | Attack Mode | Direction |
|-----------|----------|-------------|-----------|
| 13.56 MHz antenna | NFC-A (ISO 14443A) | Sniff, Emulate, Relay | Bi-directional |
| 13.56 MHz antenna | NFC-B (ISO 14443B) | Sniff, Emulate | Bi-directional |
| 13.56 MHz antenna | NFC-F (FeliCa) | Sniff, Emulate | Bi-directional |
| 13.56 MHz antenna | NFC-V (ISO 15693) | Sniff, Emulate | Bi-directional |
| 125 kHz antenna | EM4100 / HID Prox | Sniff, Clone, Emulate | Bi-directional |
| BLE 5.0 | GAP/GATT | Command & control | Phone ↔ Device |
| USB-C 2.0 | CDC-ACM | Serial console, firmware update | Host ↔ Device |

---

## 3. Performance Targets

| Metric | Target |
|--------|--------|
| NFC sniffing frame capture rate | ≥ 100% at 106/212/424 kbps |
| Relay round-trip latency (proxy ↔ proxy) | < 50 ms end-to-end |
| 125 kHz read range | ≥ 5 cm with onboard antenna |
| 13.56 MHz read range | ≥ 4 cm with onboard antenna |
| Mifare Classic key brute-force rate | ≥ 500 auth attempts/sec |
| Battery life (active sniffing) | ≥ 4 hours continuous |
| Battery life (BLE standby) | ≥ 48 hours |
| Boot time (cold start to operational) | < 2 seconds |
| Firmware update via USB-C | OTA via DFU, < 30 sec |
| BOM cost (1K units) | < $85 |

---

## 4. Constraints

- **Form factor:** Credit-card sized (85 × 54 mm), < 8 mm thick
- **Power:** Single 3.7 V 1200 mAh LiPo, USB-C charging (5 V / 500 mA)
- **Regulatory:** Intentional radiator — lab/research use only, no FCC certification
- **Thermal:** Passive cooling only; all ICs rated -40°C to +85°C
- **Antenna:** Dual onboard PCB trace antennas (13.56 MHz + 125 kHz), no external antenna required
- **Security:** Encrypted BLE link (AES-CCM pairing), firmware read-protect via MCU Option Bytes
- **Interfaces:** No Ethernet, no WiFi — BLE and USB-C only
- **Budget:** Total BOM under $85 at 1000-unit quantities

---

## 5. Block Diagram

```
┌──────────────────────────────────────────────────────────────────┐
│                    NFC RELAY PHANTOM — TOP LEVEL                  │
│                                                                   │
│  ┌─────────────────────┐     ┌─────────────────────────────┐     │
│  │   STM32L4S5VIT6     │     │    125 kHz RFID Frontend     │     │
│  │   (Main MCU)        │     │                               │     │
│  │                     │     │  ┌───────────────┐           │     │
│  │  Cortex-M4 @120MHz │◄────►│  │ EM4095       │           │     │
│  │  2MB Flash / 640KB  │     │  │ 125kHz Reader │           │     │
│  │  SRAM              │     │  └──────┬────────┘           │     │
│  │                     │     │         │                    │     │
│  │  SPI1 — NFC IC     │     │  ┌──────▼────────┐           │     │
│  │  SPI2 — Flash      │     │  │ 125kHz Antenna │          │     │
│  │  UART1 — EM4095   │     │  │ (PCB trace)     │          │     │
│  │  UART2 — Debug     │     │  └────────────────┘           │     │
│  │  I2C1 — OLED/PWR  │     └─────────────────────────────┘     │
│  │  USB-C — CDC/DFU   │                                         │
│  │  GPIO — LEDs/SW    │     ┌─────────────────────────────┐     │
│  └────────┬────────────┘     │    13.56 MHz NFC Frontend   │     │
│           │                  │                               │     │
│  ┌────────▼────────────┐     │  ┌───────────────┐           │     │
│  │   NRF52832           │     │  │ PN5180        │           │     │
│  │   (BLE + Coprocessor)│     │  │ NFC Frontend  │           │     │
│  │                     │     │  │ Controller     │           │     │
│  │  Cortex-M4F @64MHz  │◄────►│  └──────┬────────┘           │     │
│  │  512KB Flash/64KB   │     │         │                    │     │
│  │  RAM               │     │  ┌──────▼────────┐           │     │
│  │                     │     │  │ 13.56MHz Antenna│          │     │
│  │  BLE 5.0 Radio      │     │  │ (PCB trace)     │          │     │
│  │  SPI — PN5180      │     │  └────────────────┘           │     │
│  │  UART — STM32L4    │     └─────────────────────────────┘     │
│  │  GPIO — Status     │                                         │
│  └─────────────────────┘     ┌─────────────────────────────┐     │
│                               │       Power Management       │     │
│  ┌─────────────────────┐     │                               │     │
│  │  W25Q128JVSIQ       │     │  ┌───────────────┐           │     │
│  │  16MB SPI Flash     │     │  │ BQ25896       │           │     │
│  │  (Capture storage) │     │  │ USB Charger    │           │     │
│  └─────────────────────┘     │  └───────┬───────┘           │     │
│                               │          │                    │     │
│  ┌─────────────────────┐     │  ┌───────▼───────┐           │     │
│  │  SSD1306 128×64     │     │  │ TPS62840      │           │     │
│  │  I2C OLED Display   │     │  │ 3.3V Buck     │           │     │
│  └─────────────────────┘     │  └───────┬───────┘           │     │
│                               │          │                    │     │
│                               │  ┌───────▼───────┐           │     │
│                               │  │ LiPo 3.7V     │           │     │
│                               │  │ 1200 mAh      │           │     │
│                               │  └───────────────┘           │     │
│                               └─────────────────────────────┘     │
└──────────────────────────────────────────────────────────────────┘
```

---

## 6. Data Flow

### 6.1 NFC Sniffing Mode

```
PN5180 RF ←→ NFC Tag/Reader
    │
    ├── SPI3 (STM32L4 master, 20 MHz)
    │   Raw IQ samples / framed data
    ▼
STM32L4 (Protocol decoder)
    │
    ├── SPI2 → W25Q128 (Capture to flash)
    ├── UART ↔ NRF52832 → BLE → Phone app
    └── I2C → SSD1306 (Status display)
```

### 6.2 Card Emulation Mode

```
Phone app → BLE → NRF52832 → UART → STM32L4 → SPI1 → PN5180 → RF
                                                        ↑
                                                    Emulated card
                                                    parameters
```

### 6.3 Relay Attack Mode (Two Devices)

```
Device A (Prox Card)          Device B (Prox Reader)
─────────────────             ──────────────────
PN5180 ←→ Real Reader        PN5180 ←→ Real Card
    │                              │
    └─► UART ─► NRF52 ──BLE──► NRF52 ──► UART ──► PN5180
         (Device A)      (relay)      (Device B)

Latency target: < 50 ms round-trip
```

### 6.4 125 kHz RFID Cloning

```
EM4095 ←→ 125 kHz Tag
    │
    ├── UART1 (9600 bps, Manchester decoded)
    ▼
STM32L4
    │
    ├── Decode EM4100/HID Prox ID
    ├── Write to T5577 blank tag via EM4095 TX
    └── BLE → Phone app (display, edit, write)
```

---

## 7. Bus Topology

| Bus | Master | Slaves | Speed | Pins |
|-----|--------|--------|-------|------|
| SPI1 | STM32L4 | PN5180 | 20 MHz | PA5/PA6/PA7/PA4 |
| SPI2 | STM32L4 | W25Q128 | 40 MHz | PB13/PB14/PB15/PB12 |
| SPI3 | NRF52832 | — (reserved) | — | P0.11/P0.12/P0.13 |
| I2C1 | STM32L4 | SSD1306, BQ25896 | 400 kHz | PB6/PB7 |
| UART1 | STM32L4 | EM4095 | 9600 bps | PA9/PA10 |
| UART4 | STM32L4 | NRF52832 | 115200 bps | PA0/PA1 |
| USB-C | STM32L4 | Host | FS 12 Mbps | PA11/PA12 |
| GPIO | STM32L4 | LEDs, buttons | — | Various |
| GPIO | NRF52832 | Status, IRQ | — | P0.x |

---

## 8. Dual-MCU Architecture Rationale

The **STM32L4S5** serves as the main application processor running the NFC/RFID protocol stack, capture engine, and card emulation logic. The **NRF52832** acts as a dedicated BLE radio coprocessor, handling:

1. **BLE GAP/GATT stack** — connection management, pairing, notifications
2. **Relay tunnel** — low-latency BLE tunnel between two Phantom devices during relay attacks
3. **Command parsing** — UART-based command protocol from STM32L4

This separation ensures:
- The STM32L4 can fully dedicate CPU to time-critical NFC frame processing
- BLE SoftDevice never starves the radio (no SPI/NFC interrupt conflicts)
- Each MCU can be independently updated via its own DFU path
- The NRF52832 can buffer and retransmit relay packets without adding latency to the STM32L4's NFC timing