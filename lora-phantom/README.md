# LoRaPhantom — LoRaWAN & LoRa Infiltration Device

![Device](https://img.shields.io/badge/status-design-green) ![License](https://img.shields.io/badge/license-GPL--2.0-blue) ![Author](https://img.shields.io/badge/author-jayis1-orange)

> **⚠️ LEGAL DISCLAIMER:** This device is designed **exclusively** for authorized security research, penetration testing with explicit written consent, and red-team operations on LoRaWAN networks, gateways, and end-devices you own or have explicit permission to assess. Unauthorized interception, replay, key extraction, or injection of LoRa/LoRaWAN traffic on networks you do not own may violate computer fraud and abuse laws (e.g., 18 U.S.C. § 1030 CFAA), wiretap statutes (18 U.S.C. § 2511), radio transmission regulations (FCC Part 15 / 47 CFR § 97 in the US, ETSI EN 300 220 in the EU), and data protection regulations in your jurisdiction. The author (**jayis1**) assumes no liability for misuse. **Always obtain proper written authorization before deployment, and ensure compliance with local radio emissions regulations.** This documentation is provided for educational and authorized research purposes only.

---

## Overview

**LoRaPhantom** is a pocket-sized, battery-operated hardware tool for **LoRa and LoRaWAN security research**. It combines a high-performance LoRa transceiver (Semtech SX1262, 150–960 MHz) with a dual-core applications processor (STM32H743), encrypted BLE + USB CDC backhaul, and on-device microSD capture — enabling red teams and IoT security researchers to **sniff, decode, replay, inject, fuzz, and emulate** LoRaWAN network traffic in the field without a tethered laptop.

LoRaWAN is the dominant low-power wide-area network (LPWAN) protocol for IoT deployments — smart cities, agriculture, industrial metering, asset tracking, and smart grids. Despite its widespread deployment, LoRaWAN's security model has well-documented weaknesses: predictable JoinEUI/DevEUI enumeration, weak default AppKeys, ABP devices with static sessions and no replay protection (when frame counters are reset), OTAA join-request capture enabling offline MIC brute-force, downlink injection from rogue gateways, and MAC command abuse. LoRaPhantom is purpose-built to expose and research these attack surfaces.

### What LoRaPhantom Does

| Capability | Description |
|------------|-------------|
| **Passive sniffing** | Capture all LoRa packets in a configurable region/frequency/spreading factor, decode LoRaWAN MAC payloads (join-request, join-accept, unconfirmed/confirmed up/down, MAC commands) on-device. |
| **OTAA join capture** | Sniff OTAA join-requests to extract DevEUI, JoinEUI, DevNonce; capture join-accept to derive session keys (if JoinKey is known or brute-forced). |
| **Key brute-force** | Offline MIC brute-force of join-requests against candidate AppKeys / NwkKeys, with on-device AES-128 hardware acceleration. |
| **Replay attack** | Replay captured uplinks (for ABP devices with counter-reset vulns) or replay captured join-requests. |
| **Rogue gateway emulation** | Answer join-requests as a fake gateway/network server, issue join-accepts, and establish rogue sessions with end-devices. |
| **Downlink injection** | Craft and inject downlink frames (MAC commands, application payloads, JoinAccept) toward end-devices. |
| **LoRa physical-layer fuzzing** | Transmit malformed LoRa PHY frames (bad header, wrong CR, invalid IQ, phantom headers) to fuzz end-device radio firmware. |
| **ABP device cloning** | Clone an ABP device's DevAddr + session keys + counters from a captured session and impersonate it on the network. |
| **Spectrum & activity scan** | Scan configured regional channels to map active LoRa deployments, DR distribution, gateway density. |
| **Encrypted backhaul** | Stream captured/decoded traffic to the operator app over AES-CTR encrypted BLE or USB CDC. |

### Why LoRaPhantom Is Novel

Existing LoRaWAN research tools fall into two camps: **SDR-based** (GNU Radio flowgraphs on a laptop tethered to a USRP/HackRF — expensive, power-hungry, no on-device decoding) and **gateway-based** (reprogrammed LoRa gateways — bulky, tied to a network server, cannot inject arbitrary PHY frames). LoRaPhantom is neither:

1. **On-device LoRaWAN protocol engine** — full PHY + MAC decoding runs on the STM32H7 using the SX1262's built-in LoRa modem, so the operator sees structured, decoded LoRaWAN frames in real time on a phone, not raw I/Q samples.
2. **Bidirectional attack capability** — it can both passively sniff and actively inject/emulate, something no stock gateway can do (gateways only relay).
3. **AES-128 hardware key search** — the STM32H7's hardware AES accelerator enables meaningful on-device MIC brute-force (millions of key trials/sec), making offline AppKey recovery practical for weak/default keys.
4. **Multi-region support** — EU868, US915, AS923, CN470, AU915, IN865, KR920, RU864 regional parameters are all selectable in firmware, with automatic channel hopping.
5. **Physical-layer fuzzing** — arbitrary LoRa header and payload construction lets researchers fuzz end-device radio stacks at the PHY level, a capability previously requiring custom FPGA radio work.

---

## Attack Surface and Threat Model

### LoRaWAN Security Model — Background

LoRaWAN (v1.0.4 and v1.1) uses AES-128 keys for authentication, integrity, and encryption:

- **v1.0.x (most deployed):** A single **AppKey** derives **NwkSKey** (network session, MIC) and **AppSKey** (application session, payload encryption). OTAA uses JoinEUI, DevEUI, DevNonce; ABP uses pre-provisioned DevAddr + NwkSKey + AppSKey with frame counters.
- **v1.1:** Separates **NwkKey** and **AppKey**, introduces JoinServer, RekeyInd, and improved replay protection — but v1.1 adoption is still low.

### Threats LoRaPhantom Exposes

1. **OTAA Join-Request Capture & Offline Brute-Force**
   The join-request is transmitted in the clear (DevEUI, JoinEUI, DevNonce) with a MIC computed as `cmac = AES-CMAC(NwkKey, MHDR | JoinEUI | DevEUI | DevNonce)`. An attacker who sniffs a join-request can:
   - Enumerate DevEUI/JoinEUI for device fingerprinting.
   - Offline brute-force the NwkKey/AppKey against the MIC — feasible if the key is a default, weak, or derived from DevEUI.
   - Once the key is recovered, derive session keys and decrypt all subsequent traffic and forge uplinks/downlinks.

2. **ABP Replay & Counter Reset**
   ABP devices use static session keys and frame counters. If a device resets its counters (e.g., on power loss without non-volatile counter storage — a common firmware bug), an attacker can replay previously captured uplinks that the network server will accept as "new." LoRaPhantom captures and replays these frames.

3. **Rogue Gateway / Network Server Emulation**
   LoRaWAN end-devices accept join-accepts from any gateway that produces a valid JoinAccept MIC (computed with the JoinKey). An attacker with a recovered JoinKey can emulate a network server: answer join-requests, issue join-accepts with attacker-chosen DevAddr and session keys, and establish a rogue session — enabling full control of the device's application layer. LoRaPhantom implements a full rogue join-accept generator.

4. **Downlink Injection**
   Many deployments use unencrypted or MAC-command-only downlinks. An attacker can inject downlink MAC commands (e.g., `LinkADRReq` to force a device to a weak DR, `DevStatusReq`, `NewChannelReq`) to manipulate device behavior, drain battery, or desynchronize from the real network.

5. **LoRa PHY Fuzzing**
   End-device radio firmware parses the LoRa physical header (PHDR) and payload. Malformed headers (invalid CR, bad header CRC, phantom/implicit header mode mismatches) can trigger parser bugs, crashes, or stack overflows in the device's LoRa modem driver. LoRaPhantom transmits arbitrary PHY frames for black-box fuzz testing.

6. **Deployment Mapping**
   Passive channel scanning reveals active LoRa deployments in an area — gateway density, device count, DR distribution, uplink intervals — useful for pre-engagement reconnaissance in authorized assessments.

### Threat Model Scope

LoRaPhantom operates at the **radio link layer** and **LoRaWAN MAC layer**. It does not attack the backhaul (gateway → network server IP link) or the application server. It requires **physical proximity** to the target devices/gateways (LoRa range: typically 100 m – 15 km depending on DR and environment). It assumes the operator has authorization to transmit on the relevant ISM bands under the applicable regulatory exemptions.

### Out of Scope

- Attacking network server software (TTN, ChirpStack, LoRIOT) over IP — use standard network pentest tools.
- Bypassing LoRaWAN 1.1's improved key separation if properly implemented with a hardware JoinServer.
- Regulatory compliance for transmissions — that is the operator's responsibility.

---

## Hardware Specifications

### Block Diagram

```
                        ┌─────────────────────────────────────────────┐
                        │              STM32H743VIT6                   │
                        │  Cortex-M7 @ 480 MHz, 2 MB Flash, 1 MB SRAM │
                        │  HW AES-128 / CMAC / SHA-256                │
                        │  FMC (SDRAM), SDMMC, USB OTG-HS, SPI, UART  │
                        └──┬──────┬──────┬──────┬──────┬──────┬────────┘
                           │ SPI6  │ SDIO │ USB  │ USART│ SPI3 │
                  ┌────────▼──┐  ┌─▼──┐ ┌─▼──┐ ┌─▼───┐ ┌─▼────┐
                  │  SX1262   │  │ µSD│ │USB │ │nRF52│ │ status│
                  │ LoRa      │  │card│ │-C  │ │840  │ │ LEDs  │
                  │ 150-960MHz│  │    │ │    │ │ BLE │ │       │
                  └────┬──────┘  └────┘ └────┘ │back-│ └───────┘
                       │ RFIO                     │haul│
                  ┌────▼────┐                     └────┘
                  │ RF FE   │  SKY66122-11 PA/LNA + BGA711L6
                  │ +TCXO   │  matching + 50Ω antenna
                  └────┬────┘
                       │ SMA (external antenna)
                       ▼
                  ISM bands (433 / 868 / 915 MHz region-dependent)

         Power: 3.7 V LiPo (1800 mAh) → TPS63020 buck-boost → 3.3 V rail
         Charging: MCP73831 LiPo charger via USB-C (5 V)
         Form factor: 85 × 55 × 14 mm (pocket-sized, 2-layer + 4-layer RF)
```

### BOM (Key Components)

| Ref | Part | Function | Package | Est. Cost |
|-----|------|----------|---------|-----------|
| U1 | STM32H743VIT6 | Applications MCU (Cortex-M7 480 MHz) | LQFP-100 | $12.50 |
| U2 | SX1262IMLTRT | LoRa transceiver (150–960 MHz, +22 dBm) | QFN-24 | $6.20 |
| U3 | nRF52840-qiaa | BLE 5.0 backhaul (peripheral mode) | QFN-73 | $5.10 |
| U4 | SKY66122-11 | RF PA + LNA front end (+22 dBm) | QFN-16 | $3.40 |
| U5 | ISL26102AVZ | 32.768 kHz TCXO (LoRa frequency reference) | SMD | $2.10 |
| U6 | TPS63020 | 3.3 V buck-boost regulator (2 A) | VQFN-14 | $3.80 |
| U7 | MCP73831T-2ACI | LiPo charger (4.2 V, 100 mA) | SOT-23 | $0.60 |
| U8 | IS42S16160G-6TLI | 16 MB SDRAM (capture buffer) | TSOP-54 | $2.30 |
| U9 | USBLC6-2SC6 | USB-C ESD protection | SOT-23-6 | $0.40 |
| — | microSD socket (Hirose DM3) | Capture log storage | SMD | $0.80 |
| — | 1800 mAh LiPo (403040) | Battery | — | $4.50 |
| — | USB-C receptacle (Amphenol 12401548E4#2A) | Power + USB CDC | SMD | $1.20 |
| — | SMA-F edge mount | RF antenna connector | — | $0.50 |
| — | PCB antenna / 868 MHz whip | LoRa antenna | — | $2.00 |
| Misc | passives, LEDs, buttons, crystals | — | — | $3.50 |
| **Total** | | | | **~$49** |

### Radio Specifications

- **Frequency range:** 150–960 MHz (covers all LoRaWAN regional bands)
- **Modulation:** LoRa (SF7–SF12, BW 125/250/500 kHz, CR 4/5–4/8) and FSK (for legacy)
- **Max TX power:** +22 dBm (SX1262 internal) / +27 dBm with SKY66122 PA
- **RX sensitivity:** −137 dBm (SF12, BW125)
- **Frequency source:** 32 MHz TCXO (±0.5 ppm) for accurate LoRa timing
- **Antenna:** SMA-connected external whip (region-tuned) or PCB trace antenna
- **Band hopping:** Configurable regional channel plans (EU868, US915, AS923, etc.)

### Connectivity & I/O

- **USB-C:** USB 2.0 Full-Speed CDC (control + data backhaul), LiPo charging (5 V)
- **BLE 5.0:** nRF52840 peripheral, AES-CTR encrypted GATT link to operator app
- **microSD:** Class 10, FAT32, full packet capture logging (PCAP-LoRa format)
- **SDRAM:** 16 MB capture ring buffer for high-rate sniffing
- **User I/O:** 2 pushbuttons (capture trigger, mode select), 4 status LEDs (RF activity, BLE, USB, error), 1 OLED connector (optional 0.96" SSD1306 over I²C)

### Power

- **Source:** 3.7 V 1800 mAh LiPo (internal, user-replaceable)
- **Regulator:** TPS63020 buck-boost → 3.3 V (2 A peak)
- **Charging:** MCP73831 via USB-C (100 mA default, firmware-configurable to 500 mA)
- **Battery life:** ~6 h active sniffing, ~12 h BLE-only standby, ~48 h idle
- **Power states:** ACTIVE (all on), SNIFF (MCU + radio, BLE off), SLEEP (radio off, BLE advertising)

### Form Factor

- **Dimensions:** 85 × 55 × 14 mm (credit-card footprint, pocketable)
- **PCB:** 4-layer (signal / GND / PWR / signal), 1.6 mm, ENIG finish
- **Enclosure:** 3D-printed PETG (STL provided in `/kicad/`), snap-fit, antenna side-exit
- **Weight:** ~85 g (with battery)

---

## Architecture and Design Decisions

### System Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                     APPLICATION LAYER                         │
│  Capture Manager │ Decoder │ Key Search │ Replay │ Injector │
│  Gateway Emulator │ Fuzzer │ Spectrum Scanner │ PCAP Logger  │
├──────────────────────────────────────────────────────────────┤
│                    LORAWAN PROTOCOL LAYER                     │
│  MHDR parse │ JoinReq/JoinAccept │ FRMPayload decrypt (AES)  │
│  MIC verify (AES-CMAC) │ MAC command decode │ FCtrl / FPort  │
│  Regional channel plans │ DR table │ Frame counter mgmt      │
├──────────────────────────────────────────────────────────────┤
│                    LORA PHY LAYER (SX1262)                    │
│  TX/RX config (SF, BW, CR, freq, power) │ IRQ handling       │
│  CAD (channel activity detection) │ Header / payload xfer    │
│  Arbitrary PHDR construction (for fuzzing)                    │
├──────────────────────────────────────────────────────────────┤
│                    HAL / DRIVERS                              │
│  SPI6 (SX1262) │ SDMMC (µSD) │ SDRAM (FMC) │ USB CDC │ UART  │
│  AES HW │ CRC HW │ Rng HW │ GPIO │ RTC │ DMA                  │
├──────────────────────────────────────────────────────────────┤
│                    MCU (STM32H743) + nRF52840                 │
└──────────────────────────────────────────────────────────────┘
```

### Design Decision: STM32H743 + SX1262 (not SDR, not gateway)

- **vs. SDR (HackRF/USRP):** SDRs provide raw I/Q and require a laptop for demodulation. LoRaPhantom uses the SX1262's hardware LoRa modem for on-device demod, enabling standalone phone-controlled operation and lower power. The SX1262 still allows **arbitrary PHDR construction** (via its packet TX engine and direct mode), preserving PHY fuzzing capability.
- **vs. reprogrammed gateway (RAK2287/SX1302):** Gateways are receive-only-relay devices — they cannot inject arbitrary downlinks or construct malformed PHY frames. LoRaPhantom is a full transceiver with attack-grade TX.
- **STM32H743 chosen for:** (1) hardware AES-128 (for MIC compute and key search at ~3 M AES ops/s with DMA), (2) 1 MB SRAM + external SDRAM for capture buffer, (3) SDMMC for µSD, (4) USB OTG-HS for high-rate CDC, (5) abundant SPI/UART for the radio + BLE module.

### Design Decision: Separate BLE module (nRF52840) instead of integrated

The STM32H743 has no integrated BLE. Using a separate nRF52840 over UART keeps the BLE stack off the application MCU (no SoftDevice timing conflicts), allows the BLE link to run independently during MCU-heavy operations (key search), and provides a secondary processor for OTA firmware updates of the main MCU via its USB bootloader.

### Design Decision: On-device AES key search

LoRaWAN MIC is AES-CMAC over a short fixed-length field. The STM32H743's AES-128 core with DMA chaining achieves ~3 million CMAC trials/sec on a 16-byte block. This makes on-device brute-force of weak/default AppKeys (common in commercial deployments: `00000000000000000000000000000000`, `DEVEUI-derived`, etc.) practical — a 10⁶-key dictionary completes in <1 s; a full 8-character hex-key space (~10¹⁶) is still infeasible on-device (offloaded to a GPU rig via the app), but the common-weak-key and dictionary cases are fully covered in the field.

### Design Decision: PCAP-LoRa capture format

Captures are logged to µSD in a PCAP-compatible format with a custom LoRa link-layer type (DLT_USER0 + custom pseudo-header carrying frequency, SF, BW, RSSI, SNR, timestamp). This allows offline analysis in Wireshark with the LoRaWAN dissector after a simple DLT plugin, and is directly compatible with the `lorawan-parser` toolchain.

---

## Firmware Details

### Source Layout

```
firmware/
├── main.c              — System init, mode loop, command dispatcher (RTOS-free super-loop)
├── board.h             — Pin map, peripheral assignments, board constants
├── registers.h         — STM32H7 RCC/AES/SDMMC/SPI register definitions
├── Makefile            — arm-none-eabi-gcc build, link script, flash targets
└── drivers/
    ├── board_init.c    — Clock tree, GPIO, SDRAM, µSD, AES HW init
    ├── sx1262.c        — SX1262 SPI driver: TX/RX config, IRQ, CAD, payload xfer, PHDR fuzz
    ├── lorawan.c       — LoRaWAN MAC layer: MHDR, JoinReq/JoinAccept, MIC, FRMPayload decrypt
    ├── aes_hw.c        — AES-128 + AES-CMAC using STM32H7 hardware accelerator
    ├── keysearch.c     — Dictionary + brute-force AppKey/NwkKey search against captured MIC
    ├── replay.c        — Capture buffer + replay engine (counter-rollback ABP replay)
    ├── injector.c      — Downlink injection + rogue gateway join-accept generator
    ├── fuzzer.c        — LoRa PHY fuzzer: malformed PHDR, bad CR, phantom header modes
    ├── spectrum.c      — Channel activity scan, RSSI map, DR distribution
    ├── ble_uart.c      — UART protocol to nRF52840 BLE module (framed, AES-CTR encrypted)
    ├── usb_cdc.c       — USB CDC virtual serial backhaul + command channel
    ├── sdcard.c        — FAT32 µSD logging, PCAP-LoRa writer
    └── protocol.c      — Wire protocol codec (shared with app): commands, frames, status
```

### Firmware Operation Modes

| Mode | Description |
|------|-------------|
| `MODE_SNIFF` | Passive capture on a channel/DR plan; decode + log + stream |
| `MODE_KEYSEARCH` | Offline MIC brute-force against a captured join-request |
| `MODE_REPLAY` | Replay captured uplink frames (ABP counter-reset attack) |
| `MODE_INJECT` | Craft and transmit downlink frames |
| `MODE_GATEWAY_EMU` | Rogue network server: answer join-requests with forged join-accepts |
| `MODE_FUZZ` | PHY-layer fuzzing: transmit malformed LoRa frames |
| `MODE_SCAN` | Spectrum/activity scan across regional channels |

### Key Firmware Routines

- `sx1262_rx_config(freq, sf, bw, cr)` — configure the radio for sniffing on a given channel.
- `lorawan_decode_frame(buf, len, ctx)` — parse MHDR, devaddr, fctrl, fport, frmpayload; decrypt with AppSKey; verify MIC with NwkSKey.
- `lorawan_parse_join_request(buf, len, out)` — extract JoinEUI, DevEUI, DevNonce, MIC.
- `aes_cmac_compute(key, msg, len, out)` — hardware-accelerated AES-CMAC (RFC 4493).
- `keysearch_bruteforce_mic(join_req, dict, dict_len)` — iterate candidate keys, compute CMAC, compare MIC; returns matching key if found.
- `injector_forge_join_accept(nwk_key, join_req, dev_addr, out)` — build a valid JoinAccept (encrypted, MIC'd) for rogue gateway emulation.
- `fuzzer_tx_phantom_header(sf, bw, payload)` — transmit a LoRa frame with an intentionally malformed PHDR (bad header CRC, invalid CR field) to fuzz end-device parsers.
- `pcap_write_packet(fp, ts, freq, sf, rssi, snr, payload, len)` — append a PCAP-LoRa record to the µSD capture file.

### Build & Flash

```bash
cd firmware
make              # builds lora-phantom.elf + .bin
make flash        # openocd via ST-Link (SWD)
make flash-usb    # dfu-util via USB-C (STM32 built-in USB DFU bootloader)
```

Target: `arm-none-eabi-gcc 12.2+`, `newlib`, `openocd`. Linker script targets 2 MB flash / 1 MB SRAM + 16 MB external SDRAM.

---

## Application / Software Interface

The companion app (React Native + Expo) provides operator control over BLE or USB CDC. Screens:

- **Connection** — BLE scan/pair, USB attach, link status, firmware version.
- **Dashboard** — Battery, RF state, current mode, capture rate, µSD free space.
- **Sniffer** — Live decoded LoRaWAN frame list (MHDR, DevAddr, FPort, payload hex, RSSI/SNR, direction). Region/DR/frequency filters. Start/stop capture. Export PCAP.
- **Key Search** — Load a captured join-request, run dictionary / custom key list / brute-force; display recovered keys + derived NwkSKey/AppSKey.
- **Replay Manager** — Browse capture buffer, select frames, replay with optional counter manipulation.
- **Injector** — Craft downlink: DevAddr, FPort, payload, confirmed/unconfirmed, MAC commands; transmit.
- **Gateway Emulator** — Configure rogue JoinEUI, JoinKey, DevAddr pool; answer incoming join-requests; show established rogue sessions.
- **Fuzzer** — PHY fuzz config (malformed header types, payload length sweep, random seeds); live transmit.
- **Spectrum Scan** — Channel activity heatmap (RSSI per channel), DR distribution, device count estimate.
- **Settings** — Region select, regulatory power limit, AES backhaul key, µSD format, firmware OTA.

### Wire Protocol

A compact binary framed protocol (see `utils/protocol.js` and `drivers/protocol.c`) over BLE GATT characteristic `0xDEADF00D-...` or USB CDC:

```
[SYNC 0x55 0xAA][LEN u16][CMD u8][PAYLOAD...][CRC16 u16]
```

Commands include `CMD_SNIFF_START`, `CMD_FRAME_RECV`, `CMD_KEYSEARCH_RUN`, `CMD_REPLAY_SEND`, `CMD_INJECT_SEND`, `CMD_GWEMU_START`, `CMD_FUZZ_START`, `CMD_SCAN_START`, `CMD_STATUS`, etc. The BLE link is encrypted with AES-CTR using a shared key provisioned at pairing.

---

## Use Cases

### For Red Teams

1. **Pre-engagement recon:** Walk/drive the target site in SCAN mode to map LoRa deployments — gateway count, device count, channels, DR — before the engagement begins.
2. **OTAA key recovery:** Sniff join-requests during device commissioning, run dictionary brute-force on-site to recover weak AppKeys, then decrypt all subsequent traffic and forge uplinks to spoof sensor readings.
3. **Rogue gateway hijack:** Emulate a network server to answer join-requests from field devices, establishing rogue sessions that let the red team inject application commands (e.g., "meter reading accepted", "valve open").
4. **ABP replay:** Capture uplinks from a device that resets frame counters, then replay them to inject stale/duplicate data into the network server's database.

### For Security Researchers

1. **LoRaWAN v1.0 vs v1.1 key separation research:** Capture both versions' traffic, compare MIC computation, test v1.1's improved replay protection.
2. **End-device radio firmware fuzzing:** Use the PHY fuzzer to find parser vulnerabilities in commercial LoRa end-device stacks (header handling, CRC, length fields).
3. **MAC command abuse testing:** Inject crafted MAC commands (`LinkADRReq`, `DutyCycleReq`, `RxParamSetupReq`) to test device compliance and resilience.
4. **Gateway protocol research:** Sniff gateway uplink traffic to analyze regional channel utilization and gateway-side processing.

### For Penetration Testers

1. **IoT deployment assessment:** Comprehensive LoRaWAN security audit — key strength, replay protection, downlink injection, MAC command handling — in a single pocket tool.
2. **Compliance verification:** Demonstrate to clients that their LoRaWAN deployment is vulnerable to OTAA capture + brute-force, ABP replay, or rogue gateway attacks, with captured PCAP evidence.
3. **Device cloning for access control testing:** Clone an ABP device's session to impersonate it on the network and test the application server's authorization model.

---

## Legal & Ethical Notes

- **Authorization required:** Only use on networks, gateways, and devices you own or have explicit written permission to assess.
- **Radio regulations:** LoRa operates in license-free ISM bands, but transmissions are still subject to regulatory limits (FCC Part 15 / ETSI EN 300 220). LoRaPhantom enforces configurable regional power limits and duty-cycle limits in firmware; the operator is responsible for compliance.
- **No jamming:** LoRaPhantom does not implement deliberate jamming. Its active modes (inject, fuzz, gateway-emu) transmit within regulatory duty-cycle limits and are intended for authorized testing only.
- **Responsible disclosure:** If vulnerabilities are found in commercial LoRaWAN end-devices or gateways, follow responsible disclosure with the vendor.

---

## License

| Component | License |
|-----------|---------|
| Hardware design (KiCad) | CERN-OHL-S v2 |
| C firmware & drivers | GPL-2.0 |
| React Native companion app | MIT |

## Author

**jayis1** — All hardware designs, firmware, applications, and documentation in this directory are authored by **jayis1**. See author metadata in every source file header and KiCad title block.

---

*LoRaPhantom — exposing the airwaves, one frame at a time.*