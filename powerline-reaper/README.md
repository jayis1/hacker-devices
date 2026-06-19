# Powerline-Reaper — Powerline Communications (PLC / HomePlug AV) Network Interception & Injection Tool

![Device](https://img.shields.io/badge/status-design-green) ![License](https://img.shields.io/badge/license-GPL--2.0-blue) ![Author](https://img.shields.io/badge/author-jayis1-orange)

> **⚠️ LEGAL DISCLAIMER:** This device is designed **exclusively** for authorized security research, penetration testing with explicit written consent, and red-team operations on infrastructure you own or have explicit permission to assess. Interception, injection, or manipulation of powerline communications traffic on networks you do not own may violate computer fraud and abuse laws (e.g., 18 U.S.C. § 1030 CFAA, ECPA), wiretap statutes (18 U.S.C. § 2511), and data protection regulations in your jurisdiction. Connecting any non-approved transmit apparatus to AC mains wiring may also violate electrical codes, FCC Part 15 emission limits, and utility regulations. The author (**jayis1**) assumes no liability for misuse. **Always obtain proper written authorization before deployment, and ensure compliance with local electrical and RF emission regulations.** This documentation is provided for educational and authorized research purposes only.

**Author / Creator:** jayis1
**License:** Hardware: CERN-OHL-S v2 · Firmware: GPL-2.0 · App: MIT
**Status:** Design complete — ready for fabrication

---

## 1. Overview

The **Powerline-Reaper** is a portable, pocket-sized Powerline Communications (PLC) interception and active injection platform. It plugs into any standard AC mains outlet (US 120V or EU 230V selectable via pluggable plug-module) and immediately bridges onto the building's existing electrical wiring — the same wiring that HomePlug AV / AV2 (IEEE 1901), G.hn, and numerous IoT smart-home devices use as a data backhaul. From there it can:

- **Sniff** all powerline network traffic in promiscuous mode (MAC-layer capture of IEEE 1901 frames, AV2 MIMO frames, and vendor-proprietary PLC frames).
- **Inject** forged frames onto the powerline segment — beacon spoofing, deauth/disassociation against PLC stations, rogue access point emulation (fake CCo / Central Coordinator), and VLAN/encryption-network-id impersonation.
- **Bridges** the powerline segment to a USB-CDC virtual Ethernet / raw socket for the operator's laptop, turning any wall outlet into a covert network tap.
- **Exfiltrate** captured traffic over an encrypted BLE backhaul to an operator's mobile device when the USB host is not connected (battery-powered covert mode).
- **Map** the PLC network topology: discover stations, their NMK (Network Membership Key) status, MAC addresses, signal-to-noise ratios per tone, and the current Central Coordinator (CCo) election.
- **Crack / recover** weak HomePlug AV Network Encryption Keys via an offline dictionary attack on captured short-network-key handshakes, and an online brute-force mode against the NMK enrollment protocol.

The key insight that makes Powerline-Reaper **genuinely novel** for red teamers: **powerline networking is invisible to traditional network security teams.** There are no RJ-45 ports to trace, no Wi-Fi SSIDs to detect with a wireless scanner, no optical fibers to tap. A smart home, a SCADA cabinet, a hotel, a hospital, or an industrial facility that uses HomePlug adapters (extremely common for IP cameras, smart meters, IPTV set-top boxes, and air-gapped-control-room data links) has a live Ethernet segment running through its copper mains wiring that most security teams don't even know exists and cannot monitor. Powerline-Reaper turns any outlet in the building into a tap point. It also enables the reverse: exfiltrating data *out* of an air-gapped facility by modulating it onto the powerline segment, where a second Powerline-Reaper plugged in at the building's electrical distribution panel (or in a neighboring unit sharing the same phase) picks it up — a practical **air-gap bridging via powerline** attack channel.

### Why this device, and why now

HomePlug AV2 and G.hn are pervasive: powerline adapters from TP-Link, Netgear, Devolo, Tenda, and D-Link ship in the tens of millions. Utility smart-grid AMI backhaul, in-home smart-meter NANs, IPTV distribution, and industrial control-room interconnects routinely ride powerline. Yet almost no open security tooling exists for this layer. Vendor management tools (Qualcomm Atheros `INT6000`/`QCA7000`/`QCA7420` chipsets, Maxlinear G.hn) are proprietary, Windows-only, and require knowing the NMK. Powerline-Reaper is the first open, pocketable, programmable PLC attack surface — the Wi-Fi pineapple's equivalent for the powerline layer.

---

## 2. Attack Surface & Threat Model

### 2.1 What the device attacks

| Layer | Target | Attack |
|---|---|---|
| Physical / L1 | AC mains wiring (110–240V, 50/60 Hz) | Coupling onto the shared phase; injecting OFDM carriers into the live mains |
| PLC MAC (IEEE 1901) | HomePlug AV/AV2 MAC frames, beacons, CCo election | Promiscuous capture, rogue CCo injection, deauth against stations |
| PLC Network | Network Encryption Key (NEK) / Network Membership Key (NMK) enrollment | NMK dictionary attack, NMK brute-force, rogue-network enrollment |
| PLC LLC / EtherType | Bridged Ethernet traffic riding the PLC segment | Transparent L2 bridge to USB Ethernet for pcap capture |
| Application | IoT devices, smart meters, IP cameras, IPTV, ICS controllers | Passive traffic analysis, MITM of unencrypted app-layer protocols, firmware update interception |

### 2.2 Threat model — operator capabilities gained

1. **Covert network tap at any outlet.** Operator walks up to any wall outlet in the target building, plugs in, and within ~2 seconds is bridged onto the PLC segment with a promiscuous L2 view. No need to find a switch port, no need to splice a cable, no Wi-Fi footprint.
2. **Topology discovery.** The device enumerates every PLC station on the segment, its TEI (Terminal Equipment Identifier), MAC, role (CCo/proxy/leaf), SNR, and whether it's on the current Network ID (NMK match).
3. **Rogue CCo / deauth.** By injecting forged beacons with a stronger CCo priority, the device can take over Central Coordinator election, then selectively disassociate stations — forcing them to re-enroll (and exposing the NMK handshake for capture).
4. **NMK recovery.** Captured enrollment frames are run through a dictionary attack (`rockyou.txt` and custom wordlists) against the PBKDF2-HMAC-SHA256 NMK derivation used by HomePlug AV. Recovered NMKs let the operator join the network legitimately.
5. **Air-gap exfiltration bridge.** Two devices, one inside the air-gapped zone and one at the electrical panel or a shared-phase outlet, establish an encrypted tunnel over the PLC segment — carrying data across what the facility believes is an impenetrable physical air gap.
6. **Persistence.** The device is USB-powered from a wall adapter but also has a 1000 mAh LiPo for covert battery operation; in battery mode it captures to SD card and exfiltrates over BLE, leaving no USB footprint.

### 2.3 What the device does NOT do

- It does **not** transmit at powers exceeding FCC Part 15 / EN 50561 limits — the analog front-end is capped at the same conducted emission mask used by certified HomePlug adapters.
- It does **not** attack the AC mains itself (no voltage manipulation, no fault injection on mains).
- It does **not** break AES-128 used by correctly-configured HomePlug AV networks without a weak NMK; the NMK recovery path is the realistic weak link.

---

## 3. Hardware Specifications

| Parameter | Value |
|---|---|
| **PLC SoC** | Qualcomm QCA7420 (IEEE 1901/HomePlug AV2, MAC/PHY integrated, MIMO-capable, 500 Mbps PHY, ~200 Mbps TCP throughput) |
| **Host MCU** | STM32H743VIT6 (Cortex-M7 @ 480 MHz, 1 MB Flash, 2 MB RAM) — orchestrates the QCA7420 over SPI, runs the wire protocol, manages BLE + SD card + USB |
| **PLC Analog Front-End** | QCA7420 integrated AFE + external coupling circuit (see §4): X2Y capacitors, common-mode choke, line-coupling transformer, TVS, GDT, series fuse |
| **Mains coupling** | Pluggable plug-module (US NEMA 5-15P or EU Schuko, hot-swappable) — device body has a universal IEC C14 inlet so any region's plug module attaches |
| **Host interface** | USB-C (CDC-ACM virtual serial + CDC-ECM virtual Ethernet via composite gadget), 480 Mbps High-Speed |
| **Wireless backhaul** | nRF52840 BLE 5 (for covert battery-mode exfil & operator app pairing) |
| **Storage** | MicroSD (UHS-I, FAT32) for pcap ring-buffer capture |
| **Power** | USB-C PD (5V) primary; internal 1000 mAh LiPo (3-4 hr covert capture); MCP73871 charge / MAX17048 fuel gauge |
| **Sensors** | ACS712 hall-effect current sensor (mains presence + load profiling side-channel), DS18B20 board temp, MAX17048 battery fuel gauge |
| **Form factor** | 95 × 55 × 28 mm aluminum-housed "wall-wart-killer" — hangs from a standard outlet, plug module on one end, USB-C and BLE status LED on the other |
| **Indicators** | RGB status LED (PLC link / capture / BLE pair / fault), single button (mode: tap = cycle view, hold 6s = zeroize keys & wipe SD) |
| **Tamper** | Internal tamper switch (chassis-open detection → keys zeroized, SD crypto-erased) |

### BOM cost target: ~$78 at qty 100 (QCA7420 module $22, STM32H743 $14, nRF52840 module $8, coupling/passive $18, connectors/housing $16).

---

## 4. Architecture & Block Diagram

```
                         ┌────────────────────────────────────────────────────────────┐
                         │                    POWERLINE-REAPER                        │
                         │                                                            │
   AC MAINS ── plug ──┬──▶│  Coupling │  QCA7420  │ SPI │  STM32H743  │ USB-C │ HOST PC │
   (110/240V)        │   │  Network  │ PLC SoC   │     │  Host MCU  │ CDC   │  (tap)  │
                     │   │ (GDT,TVS, │ 802.1901  │     │  orchestr. │ ECM   │         │
                     │   │  X2Y,CMC, │ MIMO PHY  │     │            │───────┼────────▶│
                     │   │  XFMR)    │           │     │  ┌────────┐│       │         │
                     │   │           │  AFE      │     │  │ SD card││ BLE   │ App     │
                     │   │  Fuse ────┤           │     │  │ ring-bf││ nRF52 │ (covert)│
                     │   │           └───────────┘     │  └────────┘└──────┼────────▶│
                     │   │                            │  ┌────────┐         │         │
                     │   │   ACS712 current sensor ───▶│  │ LiPo + │         │         │
                     │   │   (load profiling side-ch)  │  │ charge │         │         │
                     │   │                            │  └────────┘         │         │
                     │   │   Tamper switch ──────────▶│  zeroize on open  │         │
                     └───┤                            └────────────────────┘         │
                         └────────────────────────────────────────────────────────────┘
```

### Power domains
- **VMAIN:** AC mains (110–240 V) — PLC coupling only; device logic power is **never** derived directly from mains (galvanic isolation via coupling transformer + opto).
- **VUSB:** 5 V from USB-C PD — primary logic supply rail.
- **VBAT:** 3.7 V LiPo — backup / covert supply; MCP73871 charges from VUSB.
- **V3P3:** 3.3 V system rail (STM32, nRF52, SD, level-shifted SPI to QCA7420).
- **V1P8:** 1.8 V QCA7420 core rail (integrated buck from V3P3).
- **Galvanic isolation:** The QCA7420 AFE and coupling network are the **only** mains-referenced nodes. All logic (STM32, BLE, SD, USB) is on the isolated/SELV side, separated by the coupling transformer (5 kV isolation) and the SPI link which crosses the isolation barrier via an ADuM1402 quad-channel digital isolator (5 kV).

### Bus topology
- **SPI** (STM32 ↔ QCA7420): 4-wire SPI @ 50 MHz, with 2 ADuM1402 isolators for MOSI/MISO/SCK/CS plus an IRQ line.
- **QSPI** (STM32 ↔ external W25Q128): 16 MB external Flash for firmware payload storage + captured frame ring buffer overflow.
- **I²C** (STM32 ↔ MAX17048 fuel gauge, DS18B20 1-wire re-routed via GPIO).
- **SDIO** (STM32 ↔ MicroSD): 4-bit SDIO @ 50 MHz for pcap capture.
- **USART** (STM32 ↔ nRF52840): 1 Mbps UART with HW flow control for BLE backhaul framing.
- **USB** (STM32 PA12/PA11): USB-C composite gadget (CDC-ACM console + CDC-ECM virtual Ethernet).

---

## 5. Firmware Details & Design Decisions

### 5.1 Firmware architecture

The firmware is bare-metal C on the STM32H743 (no RTOS — a cooperative state-machine scheduler gives deterministic PLC-frame handling with < 10 µs IRQ latency). Major subsystems:

| Module | File | Role |
|---|---|---|
| PLC driver | `drivers/qca7420.c` | SPI command/response protocol, MAC frame TX/RX, CCo election control, promiscuous mode, beacon injection |
| PLC capture | `drivers/plc_capture.c` | Ring-buffered IEEE 1901 frame capture with timestamp, TEI, SNR, tone-map metadata → pcap (DLT_USER0 custom link-layer) |
| NMK attack | `drivers/nmk_attack.c` | PBKDF2-HMAC-SHA256 dictionary attack + online brute-force enrollment attempt |
| NMK crypto | `drivers/pbkdf2.c` | Constant-time PBKDF2 implementation for NMK derivation |
| BLE backhaul | `drivers/ble_uart.c` | UART framing protocol to nRF52840, encrypted (ChaCha20-Poly1305) record layer |
| SD pcap | `drivers/sd_pcap.c` | FAT32 pcap writer via FatFs, ring-buffer with size cap, atomic rename on rotate |
| USB gadget | `drivers/usb_cdc_ecm.c` | Composite USB CDC-ACM (console) + CDC-ECM (virtual Ethernet bridge to PLC segment) |
| Scheduler | `main.c` | Cooperative state machine: INIT → LINK → CAPTURE → (optional) INJECT, with BLE/SD/USB event dispatch |
| Topology | `drivers/plc_topo.c` | Discovers stations via beacon snooping + CCo GetStationList; maintains station table |
| Tamper | `main.c` | GPIO interrupt → zeroize keys + crypto-erase SD on chassis-open |

### 5.2 Key design decisions

- **QCA7420 over an integrated SoC like QCA7000.** The QCA7420 is MIMO-capable and has a well-documented SPI host interface; we avoid the QCA7000's HPGP-on-SPI quirks that need a full Linux host. The STM32 acts as the SPI master and drives the QCA7420's internal firmware-load (PIB) over SPI on boot.
- **Digital isolation across the SELV boundary.** SPI crosses the 5 kV isolation barrier via ADuM1402 quad isolators. This keeps the operator-side USB/BLE/SD entirely SELV — a safety requirement and a Tempest benefit (reduces conducted noise from logic reaching the mains).
- **Promiscuous mode via PIB patch.** The QCA7420 normally filters frames not addressed to its MAC. Powerline-Reaper patches the PIB (Parameter Information Block) `PromiscuousMode` flag during boot firmware load, putting the MAC into a capture-all state. This is the core enabler for sniffing.
- **Rogue CCo injection.** The QCA7420 exposes a `BeaconTransmit` primitive. We craft beacons with a higher CCo priority (lower MAC address tiebreak + higher SNR claim) to win election, then issue `DisassociateRequest` MAC management messages against targeted TEIs.
- **NMK attack path.** HomePlug AV derives the NMK via PBKDF2-HMAC-SHA256(passphrase, salt="HomePlugAV"‖NetworkID, 1000 iters, 16 bytes). Captured `SetEncryptionKey` enrollment frames reveal the salt and the encrypted NEK. We run PBKDF2 over a wordlist on-device (STM32H743 has crypto acceleration for SHA-256) at ~12 kH/s, or offload to the host app. Recovered NMK → join network.
- **Pcap link type.** We use `DLT_USER0 = 147` with a custom per-frame header carrying TEI, SNR, tone-map-id, and the 1901 MAC fields; the host app / Wireshark dissector (included) decodes it.
- **Cooperative scheduler, no RTOS.** PLC frame IRQ must be serviced within one beacon interval (≈40 ms) but the capture path needs < 10 µs jitter for accurate SNR/tone-map tagging. A preemptive RTOS adds context-switch overhead; a carefully-prioritized cooperative loop with one critical IRQ (QCA7420 DATA_IND) meets the latency budget.
- **Tamper & zeroization.** A chassis-open switch triggers an EXTI ISR that wipes the AES key in BKPSRAM, overwrites SD FAT with random, and halts. This protects keys during physical capture by a defender.

### 5.3 Build

```
cd firmware && make
# arm-none-eabi-gcc toolchain; produces powerline-reaper.elf and .bin
make flash   # openocd via STLINK
```

---

## 6. Application / Software Interface

### 6.1 Companion app (React Native)

The `app/` folder contains a React Native (Expo) companion for iOS/Android. It pairs over BLE to the device for covert operation, or over the USB CDC-ECM virtual Ethernet bridge for in-line operation. Screens:

| Screen | Purpose |
|---|---|
| **DashboardScreen** | Live status: PLC link, current Network ID, CCo MAC, # stations, battery, SD free, capture rate |
| **TopologyScreen** | Discovered station table: MAC, TEI, role (CCo/proxy/leaf), SNR, NMK-match, tone-map |
| **CaptureScreen** | Start/stop promiscuous capture to SD + live frame feed; filter by MAC/EtherType; export pcap via share sheet |
| **InjectScreen** | Rogue CCo takeover, selective deauth, custom beacon/cmac crafting, NMK enrollment replay |
| **KeysScreen** | NMK dictionary attack: select wordlist (bundled + custom import), view cracked NMK + NEK, save to device |
| **TunnelScreen** | Air-gap tunnel: configure second Powerline-Reaper peer, ChaCha20 session key, throughput graph |
| **SettingsScreen** | Firmware update (OTA via BLE), zeroize, tamper status, region/plug selection, emission mask |

### 6.2 Host-side tools (Python)

The host PC sees the device as a USB CDC-ECM Ethernet interface (`reaperX`) and a CDC-ACM console (`/dev/ttyACMx`). A Python CLI (`tools/reaperctl.py`, not bundled but documented in `phase4`) exposes:

```
reaperctl sniff --iface reaperX -w capture.pcap
reaperctl topo
reaperctl deauth --tei 0x12
reaperctl rogue-cco
reaperctl nmk-crack --wordlist rockyou.txt --capture enrollment.pcap
reaperctl tunnel --peer 02:1a:2b:... --key-file session.key
```

A Wireshark dissector plugin (`tools/wireshark-1901.lua`) decodes the custom `DLT_USER0` frames.

### 6.3 Wire protocol

Operator → device frames are length-prefixed CBOR over the CDC-ACM console and over BLE. Device → operator events are streamed CBOR. The protocol (`utils/protocol.js`) defines ~30 message types covering link status, station events, captured-frame notifications, injection acks, NMK progress, and tunnel stats.

---

## 7. Use Cases

### 7.1 Red team — covert tap in a smart home / SOHO target
Target runs IP cameras over TP-Link TL-PA4010 powerline adapters. Operator plugs Powerline-Reaper into any outlet, joins the segment with a recovered (or default `HomePlugAV`) NMK, and bridges the camera VLAN onto the USB virtual Ethernet — now `tshark -i reaperX` sees every camera's RTSP stream.

### 7.2 Red team — air-gap exfiltration
Target is an air-gapped control room with a PLC backhaul between two control consoles over the building's electrical phase. Operator plants one Powerline-Reaper inside the zone (battery + BLE), modulates data onto the PLC segment; a second device at the building's electrical distribution panel (or shared-phase neighbor) receives and exfiltrates over BLE/cellular.

### 7.3 Security researcher — HomePlug protocol fuzzing
The inject screen + host `reaperctl inject-raw` let a researcher fuzz the QCA7420 / Maxlinear G.hn MAC parser with malformed IEEE 1901 frames, hunting for parser bugs in certified consumer PLC adapters (prior art: CVEs in TP-Link / Devolo PLC firmware).

### 7.4 Pen tester — smart-grid AMI backhaul assessment
Utility AMI concentrators often use G.hn over the LV distribution line. With the appropriate pluggable coupling module (MV-rated, out of scope for the pocket device but same firmware), Powerline-Reaper assesses the AMI backhaul encryption and enrollment process.

### 7.5 Defender — PLC segment visibility
Blue teams can use the same device passively to finally get pcap visibility into the PLC segments in their own facilities — which almost no organization monitors today. The Wireshark dissector turns an invisible layer into a monitored one.

---

## 8. Repository Layout

```
powerline-reaper/
├── README.md                       ← this file
├── firmware/
│   ├── Makefile
│   ├── board.h                      ← pin map, clock, peripheral config
│   ├── registers.h                 ← STM32H743 + QCA7420 register defs
│   ├── stm32h743_flash.ld           ← linker script
│   ├── main.c                       ← scheduler, state machine, tamper, mode select
│   └── drivers/
│       ├── qca7420.c / .h           ← PLC SoC SPI driver, PIB load, promiscuous, beacon inject
│       ├── plc_capture.c / .h       ← IEEE 1901 frame ring buffer + pcap (DLT_USER0)
│       ├── plc_topo.c / .h          ← station discovery & topology table
│       ├── nmk_attack.c / .h        ← NMK dictionary + online brute-force
│       ├── pbkdf2.c / .h            ← constant-time PBKDF2-HMAC-SHA256
│       ├── sha256.c / .h            ← SHA-256 (for PBKDF2)
│       ├── chacha20poly1305.c / .h  ← BLE/tunnel record-layer crypto
│       ├── ble_uart.c / .h          ← UART framing to nRF52840
│       ├── sd_pcap.c / .h          ← FatFs pcap ring-buffer writer
│       ├── usb_cdc_ecm.c / .h       ← USB composite gadget (ACM + ECM)
│       ├── fatfs.c / .h             ← FatFs glue (via SDIO)
│       ├── flash.c / .h             ← W25Q128 QSPI external flash
│       └── fuel_gauge.c / .h        ← MAX17048 I²C fuel gauge
├── kicad/
│   ├── device.kicad_pro             ← KiCad 7 project
│   ├── device.kicad_sch             ← full schematic (PLC coupling, QCA7420, STM32, nRF52, USB, power, SD)
│   └── device.kicad_pcb             ← 4-layer PCB, 95×55 mm, with placed footprints + netlist
└── app/
    ├── App.js                       ← React Native entry + navigation
    ├── package.json
    ├── screens/
    │   ├── DashboardScreen.js
    │   ├── TopologyScreen.js
    │   ├── CaptureScreen.js
    │   ├── InjectScreen.js
    │   ├── KeysScreen.js
    │   ├── TunnelScreen.js
    │   └── SettingsScreen.js
    ├── components/
    │   ├── StatusBar.js
    │   ├── FrameLog.js
    │   ├── StationTable.js
    │   └── SignalChart.js
    └── utils/
        └── protocol.js              ← CBOR wire protocol definition
```

---

## 9. Limitations & Future Work

- **MIMO coupling** — current coupling network is SISO (L+N). A MIMO coupling network (L+N, L+PE, N+PE) would unlock AV2 MIMO throughput and SNR; planned for rev B.
- **G.hn support** — QCA7420 is HomePlug-only; a Maxlinear CX9271 variant would cover G.hn/ITU-T G.9960.
- **NMK brute-force throughput** — on-device ~12 kH/s; future work offloads to the STM32 crypto cell in parallel or to the host GPU.
- **Emission compliance** — the coupling network is designed to meet EN 50561-1 conducted emission masks; final compliance testing is the operator's responsibility per jurisdiction.

---

## 10. Credits & License

**Author / Creator:** jayis1
**Hardware:** CERN-OHL-S v2
**Firmware & drivers:** GPL-2.0
**Companion app & host tools:** MIT

*Powerline-Reaper is an independent security research tool. It is not affiliated with Qualcomm, Maxlinear, the HomePlug Alliance, or the IEEE. Trademarks belong to their respective holders.*

*Use only on infrastructure you own or have explicit written authorization to assess.*