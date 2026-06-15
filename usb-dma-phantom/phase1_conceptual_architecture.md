# USB DMA Phantom — Phase 1: Conceptual Architecture

## 1. System Purpose

**USB DMA Phantom** is a pocket-sized USB/Thunderbolt Direct Memory Access (DMA) attack platform for authorized security researchers and red team operators. It emulates legitimate Thunderbolt/USB4 peripherals to gain PCIe DMA access to a target host, enabling memory read/write, credential extraction, and kernel exploitation — all from a portable, self-contained device controlled via a companion mobile app over encrypted BLE.

Unlike generic BadUSB tools that operate at the HID layer, USB DMA Phantom exploits the fundamental trust model of Thunderbolt and USB4: when a peripheral connects over PCIe, the host grants it DMA access to physical memory. This device weaponizes that trust by presenting as a legitimate network adapter or storage controller, then executing DMA read/write operations to:

- **Read arbitrary physical memory** — extract credentials, encryption keys, and process data
- **Write to physical memory** — inject shellcode, patch kernel structures, disable security
- **Bypass IOMMU** — leverage known Thunderbolt/USB4 IOMMU misconfigurations on unpatched hosts
- **Enumerate PCIe devices** — discover and interact with host PCIe topology
- **Deploy known exploits** — integrate with PCILeech-style attack payloads

### Target Attack Surfaces

| Target | Attack Vector | Impact |
|--------|--------------|--------|
| Thunderbolt 3/4 hosts | PCIe DMA over TBT | Full physical memory R/W |
| USB4 hosts | PCIe tunnel over USB4 | Full physical memory R/W |
| Unpatched Windows | IOMMU bypass (CVE-2019-0090 etc.) | Kernel code execution |
| Linux with `iommu=off` | Unrestricted DMA | Credential extraction |
| macOS FileVault 2 | DMA before login screen | Key extraction |
| BitLocker with TPM | DMA before OS IOMMU init | Volume key extraction |

## 2. Performance Targets

| Parameter | Target |
|-----------|--------|
| **DMA read speed** | ≥ 500 MB/s (PCIe Gen2 x1) |
| **DMA write speed** | ≥ 400 MB/s |
| **Memory scan** | 4 GB in < 10 seconds |
| **Boot time** | < 3 s from USB power to operational |
| **Payload storage** | 256 KB on-chip, 16 MB external flash |
| **BLE C2 latency** | < 50 ms command → action |
| **Power** | Bus-powered via USB-C (5 V, ≤ 900 mA) |
| **Form factor** | 65 × 30 × 12 mm (stick form) |
| **BOM cost** | < $85 at qty 100 |

## 3. Design Constraints

1. **Bus-powered only** — no external power; must operate within USB-C 5V/900mA budget
2. **No onboard storage requirement** — microSD for captured data, SPI flash for firmware/payloads
3. **Must enumerate as legitimate PCIe device** — VID/PID must match known legitimate peripherals
4. **Dual-mode operation** — Thunderbolt/USB4 DMA mode and USB 2.0 control mode simultaneously
5. **Encrypted C2** — BLE link must be AES-256-CTR encrypted with key rotation
6. **Stealth enumeration** — configurable VID/PID, serial number, device class for social engineering
7. **Firmware updatable** — OTA via BLE or USB DFU class

## 4. Block Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                      USB DMA Phantom                        │
│                                                             │
│  ┌──────────────┐    ┌─────────────────────────────────┐    │
│  │  USB-C       │    │      STM32F423CHU6              │    │
│  │  Thunderbolt │◄──►│  Cortex-M4F @ 150 MHz          │    │
│  │  / USB4      │    │  USB 2.0 FS OTG                │    │
│  │  Port        │    │  SPI / UART / I2C / GPIO       │    │
│  └──────────────┘    └──────┬──────────┬──────────────┘    │
│         │                   │          │                    │
│         │            ┌──────┘    ┌─────┘                    │
│         │            │           │                          │
│         │     ┌──────▼──────┐ ┌──▼───────────┐             │
│         │     │ W25Q128JVS   │ │ nRF52832     │             │
│         │     │ 16 MB SPI   │ │ BLE 5.0 C2   │             │
│         │     │ NOR Flash   │ │ Radio         │             │
│         │     └─────────────┘ └──────┬────────┘             │
│         │                            │                       │
│  ┌──────▼──────────┐    ┌────────────▼───────┐              │
│  │ XIO2001         │    │ microSD Slot       │              │
│  │ PCIe-to-PCI     │    │ (Captured data)    │              │
│  │ Bridge          │    └────────────────────┘              │
│  │ (DMA Engine)    │                                       │
│  └──────┬──────────┘                                       │
│         │                                                   │
│  ┌──────▼──────────────────────────┐                       │
│  │ Thunderbolt Alternate Mode      │                       │
│  │ Retimer / Mux: HD3SS460         │                       │
│  └─────────────────────────────────┘                       │
│                                                             │
│  ┌────────────────┐  ┌──────────────┐  ┌─────────────────┐ │
│  │ Status LEDs    │  │ Tactile Btn  │  │ Power Mgmt      │ │
│  │ (RGB)         │  │ (Mode Sel)   │  │ TPS62840 3.3V   │ │
│  └────────────────┘  └──────────────┘  └─────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

## 5. Data Flow

### 5.1 DMA Attack Flow

```
[Host Thunderbolt Controller]
        │
        │ PCIe TBT Tunnel (Gen2 x1)
        ▼
[HD3SS460 USB-C Mux] ──► [XIO2001 PCIe Bridge]
                                │
                                │ PCI Configuration Space
                                │ + DMA Engine (Bus Master)
                                │
                                ▼
                        [STM32F423 Command Interface]
                                │
                          SPI Command Queue
                                │
                                ▼
                        [DMA Payload Engine]
                        ├── Read: addr → data
                        ├── Write: addr + data → memory
                        ├── Scan: pattern match
                        └── Inject: shellcode → executable region
```

### 5.2 C2 Communication Flow

```
[Companion App]
        │
        │ BLE 5.0 (AES-256-CTR encrypted)
        ▼
[nRF52832]
        │
        │ UART @ 1 Mbps
        ▼
[STM32F423]
        │
        │ SPI @ 50 MHz
        ▼
[XIO2001 / W25Q128 / microSD]
```

### 5.3 USB Control Mode Flow

```
[Host USB 2.0]
        │
        │ CDC ACM (Virtual COM Port)
        ▼
[STM32F423 USB OTG]
        │
        ├── Firmware update (DFU)
        ├── Command interface (CDC)
        └── Configuration (VID/PID/payloads)
```

## 6. Bus Topology

```
                    ┌─────────────────┐
                    │   Host System   │
                    │ (Thunderbolt/   │
                    │  USB4 Target)   │
                    └────────┬────────┘
                             │
                     USB-C TBT/USB4
                             │
          ┌──────────────────┼──────────────────┐
          │                  │                  │
    USB 2.0 D+/D-      SBU1/SBU2         TX/RX Pairs
          │            (Sideband)        (PCIe over TBT)
          │                  │                  │
          ▼                  │                  ▼
   ┌──────────────┐          │          ┌───────────────┐
   │ STM32F423    │          │          │ HD3SS460      │
   │ USB OTG      │          │          │ USB-C Mux     │
   │              │          │          └───────┬───────┘
   │              │          │                  │
   │              │          │          ┌───────▼───────┐
   │              │          │          │ XIO2001       │
   │              │          │          │ PCIe Bridge   │
   │              │          │          │ (DMA Engine)  │
   │              │          │          └───────────────┘
   └──────┬───────┘          │
          │                  │
    ┌─────┤           ┌──────┘
    │     │           │
    ▼     ▼           ▼
  SPI4  UART4      I2C1
    │     │           │
    ▼     ▼           ▼
  W25Q  nRF52832  microSD
  Flash  BLE C2   (Card Detect)
```

### 6.1 Bus Details

| Bus | Master | Speed | Devices | Protocol |
|-----|--------|-------|---------|----------|
| **SPI4** | STM32F423 | 50 MHz | W25Q128JVS (flash), XIO2001 (cmd) | SPI Mode 0 |
| **UART4** | STM32F423 | 1 Mbps | nRF52832 | 8N1, HW flow control |
| **I2C1** | STM32F423 | 400 kHz | microSD (via DA1220 level shifter), HD3SS460 (ctrl) | I2C |
| **USB 2.0 FS** | STM32F423 | 12 Mbps | Host USB-C port | CDC ACM + DFU |
| **PCIe Gen2 x1** | XIO2001 | 5 GT/s | Host TBT controller | PCI/PCIe config + DMA |
| **BLE 5.0** | nRF52832 | 2 Mbps | Companion app | AES-256-CTR over GATT |

## 7. Operating Modes

| Mode | Description | Activation |
|------|-------------|-----------|
| **Stealth DMA** | Enumerates as legitimate PCIe device, executes preloaded DMA payloads | Auto on TBT connect |
| **Interactive DMA** | Real-time memory R/W via BLE C2 or USB CDC | Button press + BLE connected |
| **Config** | USB CDC mode for VID/PID, payload, firmware updates | Boot with button held |
| **Sniffer** | Passive PCIe TLP sniffing (read-only, no DMA) | Software select |

## 8. Threat Model (Device Perspective)

| Threat | Mitigation |
|--------|-----------|
| Unauthorized BLE connection | AES-256-CTR with device-specific key; pairing requires physical button press |
| Firmware extraction | Lock bits set; SWD disabled in production; readout protection Level 2 |
| Payload tampering | HMAC-SHA256 over stored payloads; verification before execution |
| Bus snooping | Encrypted UART between STM32 and nRF; SPI commands obfuscated |
| Physical tamper | Secure boot (STM32F423 OTFDEC); tamper-evident epoxy over flash |