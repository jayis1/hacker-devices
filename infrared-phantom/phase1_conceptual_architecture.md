# Infrared Phantom — Phase 1: Conceptual Architecture

## 1. System Purpose

**Infrared Phantom** is a portable, USB-powered infrared signal capture, analysis, modification, and replay toolkit for authorized security research. It targets IR-based access control systems (barrier gates, turnstiles, proximity sensors), vehicle keyless entry, smart home/consumer IR protocols, industrial IR data links, and IRDA communications.

The device enables security researchers to:

- **Capture** raw IR waveforms at up to 2 Msps with 32-bit timestamping
- **Decode** 15+ IR protocols in real-time (NEC, NECext, RC5, RC6, RC6A, Sony SIRC 12/15/20, Samsung, Sharp, Daikin, Mitsubishi, JVC, Panasonic, IRDA-SIR, raw Manchester)
- **Modify** decoded frames — change device addresses, command bytes, toggle bits
- **Replay** captured or crafted signals via high-power IR LED array (3W total, 30m+ range)
- **Fuzz** target devices by systematically mutating protocol fields
- **Learn** unknown protocols via pattern analysis and clustering
- **Bridge** IR to IP via BLE or USB for remote C2

### Attack Surface

| Vector | Target | Technique |
|--------|--------|-----------|
| Access control barriers | IR proximity sensors, beam-break systems | Signal replay, signal injection |
| Vehicle IR keyless | Toyota/Honda/Audi IR remotes | Rolling code capture & analysis |
| Smart home hubs | AC, TV, soundbar, smart lighting | Protocol fuzz, command injection |
| Industrial IRDA | Serial IR data links in SCADA | Packet injection, MITM |
| Consumer electronics | Any IR-controlled device | Universal brute-force scanning |

## 2. Performance Targets

| Parameter | Target | Notes |
|-----------|--------|-------|
| Capture bandwidth | DC–2 MHz | Covers all modulated IR up to 56 kHz carriers with full envelope |
| ADC resolution | 12-bit | 72 dB dynamic range for signal quality analysis |
| Sample buffer depth | 8 MB (2 Msps × 4 s) | Captures longest Daikin frame (~180 ms) with headroom |
| Protocol decode latency | < 10 ms | Real-time decode for interactive use |
| Replay carrier frequency | 30 kHz – 56 kHz programmable | Covers NEC (38), RC5 (36), Sony (40), IRDA (varies) |
| Replay duty cycle | 25% – 50% programmable | Standard IR modulation profiles |
| IR LED output power | 3 W peak (9 × 333 mW LEDs) | 30+ m range, 120° beam angle option |
| Receiver sensitivity | –40 dBm (TSMP58000) | Reliable 25 m capture range |
| USB throughput | 12 Mbps (USB 2.0 HS) | Continuous streaming to host |
| BLE range | 30 m (nRF52840) | Wireless control and monitoring |
| Boot time | < 1.5 s | From power-on to operational |
| Power consumption | < 500 mW total (USB bus power) | No external PSU needed |

## 3. Constraints

| Constraint | Value | Rationale |
|------------|-------|-----------|
| BOM cost | < $100 @ qty 100 | Affordable for researchers |
| Form factor | 85 × 34 × 12 mm (USB dongle) | Pocketable, direct USB-A plug |
| Power | USB 5 V, 500 mA max (bus-powered) | No battery, no external PSU |
| Operating temp | 0°C – 50°C | Indoor/outdoor security work |
| Interface | USB 2.0 HS + BLE 5.0 | Dual connectivity |
| Storage | 16 MB SPI NOR + microSD | Firmware + captures |
| Compliance | FCC Part 15 (intentional radiator exemption for IR) | IR LED devices are unlicensed |

## 4. Block Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        Infrared Phantom                                  │
│                                                                          │
│  ┌──────────┐    IR_RX     ┌──────────────────┐    IR_TX     ┌────────┐ │
│  │ IR       │──────────────▶│                  │─────────────▶│ IR LED │ │
│  │ Receiver │   (analog)   │                  │  (PWM)       │ Array  │ │
│  │ TSMP58000│              │   STM32H743VI    │              │ 9×    │ │
│  └──────────┘              │   Cortex-M7      │              │ TSAL...│ │
│                            │   @ 480 MHz      │              └────────┘ │
│  ┌──────────┐    SPI       │                  │                           │
│  │ 16 MB    │◀────────────▶│  ┌────────────┐  │              ┌────────┐ │
│  │ SPI NOR  │              │  │ IR Protocol│  │   UART       │ nRF    │ │
│  │ W25Q128  │              │  │ Engine     │  │─────────────▶│52840   │ │
│  └──────────┘              │  │ (NEC/RC5/  │  │   115200     │ BLE    │ │
│                            │  │  RC6/Sony/ │  │              │ C2     │ │
│  ┌──────────┐    SDIO      │  │  Daikin/   │  │              └────────┘ │
│  │ microSD  │◀────────────▶│  │  IRDA/     │  │                           │
│  │ Slot     │              │  │  Manchester│  │              ┌────────┐ │
│  └──────────┘              │  └────────────┘  │   USB       │ USB    │ │
│                            │                  │─────────────▶│ 2.0 HS │ │
│  ┌──────────┐    I²C       │                  │              │ PHY    │ │
│  │ OLED     │◀────────────▶│                  │              │ ULPI   │ │
│  │ SSD1306  │              │                  │              └────────┘ │
│  │ 128×32   │              │                  │                           │
│  └──────────┘              └──────────────────┘                           │
│                                                                          │
│  ┌──────────┐                                                            │
│  │ LDO      │ 5 V USB → 3.3 V (STM32/nRF) + 1.8 V (STM32 core)         │
│  │ TLV62569 │                                                             │
│  └──────────┘                                                            │
└─────────────────────────────────────────────────────────────────────────┘
```

## 5. Data Flow

### 5.1 Capture Pipeline

```
IR Photodiode (TSMP58000)
  │ Analog envelope (demodulated 38 kHz)
  ├─▶ ADC1_IN6 (12-bit, 2 Msps, DMA to SRAM)
  │    Circular buffer → DMA half/full IRQ
  │    ├─▶ Real-time decode engine (NEC/RC5/RC6/Sony/...)
  │    ├─▶ Waveform storage → SPI NOR / microSD
  │    └─▶ USB/BLE streaming to host
  │
  └─▶ TIM2_CH1 (digital edge-capture mode, 32-bit timestamp)
       Edge timestamps → protocol auto-detect
```

### 5.2 Replay Pipeline

```
Host (USB/BLE) → Protocol + Frame Data
  │
  ├─▶ Protocol encoder (builds modulated waveform)
  │    TIM3_CH1 (PWM carrier @ 38/36/40 kHz)
  │    TIM3_CH2 (gating: duty cycle pattern)
  │
  └─▶ DMA from waveform buffer → TIM3 CCR registers
       ▶ IR LED array (3 W peak via MOSFET driver)
```

### 5.3 Fuzzing Pipeline

```
Host → Fuzz target specification
  │
  ├─▶ Mutation engine (bit-flip, byte-swap, range-sweep, smart mutation)
  │    For each variant:
  │    ├─▶ Encode frame → TIM3 PWM DMA
  │    ├─▶ Transmit via IR LED array
  │    └─▶ (optional) Capture response via ADC/DIG
  │         → USB/BLE back to host
```

## 6. Bus Topology

```
                    ┌──────────────────┐
                    │  STM32H743VIT6    │
                    │  (LQFP-100)      │
                    │                   │
  SPI1 (50 MHz) ────│ SPI1_MOSI/MISO   │──── W25Q128JVSIQ (16 MB SPI NOR)
  SPI1_SCK/CS   ────│ SPI1_SCK/NSS     │
                    │                   │
  SPI2 (25 MHz) ────│ SPI2_MOSI/MISO   │──── SSD1306 OLED (128×32)
  SPI2_SCK/CS   ────│ SPI2_SCK/NSS     │
                    │                   │
  SDMMC1 (50 MHz)──│ SDMMC1_CK/CMD    │──── microSD slot
  SDMMC1_D0-D3  ────│ SDMMC1_D[0:3]    │
                    │                   │
  USART1 (115200)──│ USART1_TX/RX     │──── nRF52840 (BLE C2)
                    │                   │
  USB_OTG_HS ──────│ USB_OTG_HS       │──── USB3320 ULPI PHY → USB-A plug
                    │                   │
  ADC1 (2 Msps) ───│ ADC1_IN6          │──── TSMP58000 IR receiver (analog)
  ADC1 (DMA1)  ────│ ADC1_DMA          │
                    │                   │
  TIM2 (32-bit) ───│ TIM2_CH1          │──── TSMP58000 (digital edge mode)
                    │                   │
  TIM3 (PWM) ──────│ TIM3_CH1/CH2      │──── IR LED MOSFET driver
  TIM3_DMA ─────────│ TIM3_DMA1         │
                    │                   │
  I²C1 (400 kHz) ──│ I²C1_SDA/SCL     │──── (reserved for future sensors)
                    │                   │
  GPIO              │ PA8 [MCO1]        │──── nRF52840 RESET
                    │ PA0 [WKUP]        │──── User button
                    │ PB7 [GPIO]        │──── SSD1306 DC#
                    │ PD2 [GPIO]        │──── IR LED enable (MOSFET gate)
                    │                   │
  PWR               │ VDD = 3.3 V       │──── TLV62569 buck (5 V → 3.3 V)
                    │ VDDCORE = 1.8 V   │──── TLV62569 LDO mode (3.3 V → 1.8 V)
                    └──────────────────┘

Power tree:
  USB 5 V (VBUS)
    │
    ├─▶ TLV62569 (buck) → 3.3 V / 1.5 A (VDD domain)
    │    └─▶ TLV62569 (bypass/LDO) → 1.8 V / 0.5 A (VDDCORE domain)
    │
    └─▶ IR LED array (direct from VBUS via MOSFET, 3 W peak)
```

## 7. Memory Map & Resource Allocation

| Resource | Allocation | Purpose |
|----------|-----------|---------|
| ITCM (64 KB) | ISR vectors, capture IRQ handlers | Zero-wait-state interrupt response |
| DTCM (128 KB) | DMA ring buffers, decode scratch | Low-latency waveform processing |
| AXI SRAM (512 KB) | Protocol decode state, fuzz engine | Main working memory |
| SRAM1 (128 KB) | USB packet buffers | Bulk transfer FIFOs |
| SRAM2 (32 KB) | BLE UART ring buffer | nRF52840 communication |
| SRAM4 (64 KB) | Backup registers, persistent config | Settings survive reset |
| Flash (1 MB) | Firmware (512 KB) + filesystem (512 KB) | OTA update partition |
| SPI NOR (16 MB) | Captured waveforms, protocol library | Non-volatile capture storage |
| microSD | PCAP exports, extended captures | Unlimited offload |

## 8. IR Protocol Support Matrix

| Protocol | Carrier | Frame Length | Encoding | Address Bits | Command Bits |
|----------|---------|-------------|----------|-------------|-------------|
| NEC | 38 kHz | 32 bit | PPM | 8 + 8 (extended: 16) | 8 |
| RC5 | 36 kHz | 14 bit | Manchester | 5 | 6 |
| RC6 | 36 kHz | 21–36 bit | Manchester+ | 8 (mode 0) | 8 |
| RC6A | 36 kHz | 35+ bit | Manchester+ | 15+ | 8 |
| Sony SIRC 12 | 40 kHz | 12 bit | PPM | 5 | 7 |
| Sony SIRC 15 | 40 kHz | 15 bit | PPM | 8 | 7 |
| Sony SIRC 20 | 40 kHz | 20 bit | PPM | 5 + 8 extended | 7 |
| Samsung | 38 kHz | 32 bit | PPM (NEC-like) | 8 + 8 | 8 |
| Sharp | 38 kHz | 15 bit | PPM | 5 | 8 |
| Daikin | 38 kHz | 280 bit | PPM (multi-frame) | varies | varies |
| Mitsubishi | 33 kHz | 144 bit | PPM | varies | varies |
| JVC | 38 kHz | 16 bit | PPM (NEC-like) | 8 | 8 |
| Panasonic | 36.7 kHz | 48 bit | PPM | 16 + 8 | 8 |
| IRDA-SIR | 115.2 kbps | 8N1 UART | NRZ | N/A (serial) | N/A |
| Raw Manchester | varies | varies | Manchester | auto-detect | auto-detect |

## 9. Operating Modes

### 9.1 Capture Mode
- Continuous ADC sampling at 2 Msps, DMA to circular buffer
- Simultaneous digital edge-capture via TIM2 input capture
- Auto-detect protocol or record raw waveform
- Stream to USB/BLE in real-time

### 9.2 Replay Mode
- Select protocol → enter address/command → transmit
- Or replay captured waveform from buffer/SPI NOR/microSD
- Carrier frequency, duty cycle, and repeat count configurable
- High-power LED array for long-range (30+ m)

### 9.3 Fuzz Mode
- Target a specific protocol and field (address, command, toggle)
- Mutation strategies: bit-flip, byte-range, random, smart dictionary
- Configurable inter-transmit delay (50 ms – 5 s)
- Capture any response from target via simultaneous receive

### 9.4 Analyze Mode
- Record raw waveform, compute statistics
- Auto-detect carrier frequency via FFT
- Cluster similar waveforms for unknown protocol discovery
- Export as JSON/CSV for offline analysis

### 9.5 Bridge Mode
- IR → BLE: received IR frames forwarded over BLE to phone app
- BLE → IR: app commands translated to IR transmissions
- USB passthrough: transparent IR tunnel over USB serial

## 10. Physical Design

```
    ┌──────────────────────────────────────────────┐
    │  [USB-A Plug]                                │
    │                                              │
    │  ┌─────────┐  ┌──────────┐  ┌────────────┐  │
    │  │ nRF52840│  │STM32H743 │  │ USB3320    │  │
    │  └─────────┘  └──────────┘  └────────────┘  │
    │                                              │
    │  ┌─────┐  ┌────────┐  ┌──────────┐         │
    │  │SSD  │  │W25Q128 │  │ microSD  │         │
    │  │1306 │  │ SPI NOR│  │ slot     │         │
    │  └─────┘  └────────┘  └──────────┘         │
    │                                              │
    │  ○ [TSMP58000]    ◉◉◉ [IR LED Array]       │
    │  (RX side)        (TX side, 9 LEDs)         │
    │                                              │
    │  ┌─────┐                                     │
    │  │TLV  │  [User Button]                     │
    │  │62569│                                     │
    │  └─────┘                                     │
    └──────────────────────────────────────────────┘

Top view: 85 mm × 34 mm PCB
    
Front face (IR TX side):
    ┌───────────────────────────────────┐
    │  ◉ ◉ ◉                            │
    │  ◉ ◉ ◉    (IR LED Array 3×3)     │
    │  ◉ ◉ ◉                            │
    │  ┌──────┐                          │
    │  │OLED  │  (128×32 status display) │
    │  └──────┘                          │
    └───────────────────────────────────┘

Rear face (IR RX side):
    ┌───────────────────────────────────┐
    │  ○ TSMP58000 (IR receiver)        │
    │  ○ (secondary wideband photodiode)│
    └───────────────────────────────────┘
```