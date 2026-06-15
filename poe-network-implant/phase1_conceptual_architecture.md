# PoE Network Implant — Phase 1: Conceptual Architecture

## 1. System Purpose

The **PoE Network Implant** ("PhantomBridge") is a stealthy, inline Ethernet tap and MITM platform disguised as a standard RJ45 coupler/barrel. It sits transparently between a target's wall Ethernet jack and their device (VoIP phone, IP camera, workstation), harvesting operating power from the PoE delivery on the line while performing:

- **Passive traffic mirroring** — full-duplex 10/100/1000BASE-T sniffing with zero packet loss
- **Active MITM injection** — ARP spoofing, DNS hijack, ICMP redirect, DHCP rogue server
- **Selective packet modification** — on-the-fly payload tampering via rule engine
- **Encrypted exfiltration tunnel** — GRE/IPsec or TLS WebSocket C2 channel to operator
- **PoE pass-through** — fully transparent PoE Class 0–3 delivery to the downstream victim device

The device is physically indistinguishable from a common RJ45 inline coupler, making it an ideal persistent implant for red team operations, internal pentest drop boxes, and physical security assessments.

## 2. Attack Surface & Target Environments

| Target Environment | Attack Vector | Value |
|---|---|---|
| Enterprise VoIP (PoE phones) | Inline tap on desk phone Ethernet | Credential capture, call metadata |
| IP Camera / IoT (PoE cameras) | Inline between switch & camera | Video stream interception, firmware injection |
| Workstation drops | Inline at wall jack under desk | Full network MITM, credential harvesting |
| Industrial (PoE switches) | Inline on uplink/truck | ICS protocol inspection |
| Conference room AV (PoE APs) | Inline on AP uplink | WiFi traffic at wire level |

## 3. Performance Targets

| Parameter | Target |
|---|---|
| Ethernet line rate | 10/100/1000 Mbps full-duplex |
| Packet capture loss rate | < 0.01% at line rate |
| Injection latency (modify-in-place) | < 10 µs per packet |
| PoE power budget (harvested) | Class 0 (12.95W) max, device uses ≤ 3W |
| PoE pass-through efficiency | ≥ 99% (≤ 130mW drop) |
| Form factor | 35mm × 20mm × 15mm (RJ45 coupler form) |
| BOM cost | < $75 |
| Operating temperature | 0°C – 70°C |
| Boot-to-operational | < 3 seconds |
| Onboard capture buffer | 128 Mb (16 MB) circular |
| C2 channel bandwidth | ≥ 5 Mbps (via separate management interface or in-band) |

## 4. Constraints

1. **No visible LEDs** — stealth operation; status via C2 only
2. **No audible noise** — switching PSU must be silent
3. **PoE signature compliance** — must present valid Class 0 signature to PSE while passing through
4. **Galvanic isolation** — 1500V isolation between line-side and device-side Ethernet (magnetics)
5. **No external power connector** — 100% PoE powered
6. **Firmware OTA** — must support in-field firmware updates via C2
7. **Configurable via companion app** — BLE 5.0 management interface for initial provisioning

## 5. High-Level Block Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    PoE Network Implant                          │
│                                                                 │
│  ┌──────┐    ┌─────────────┐    ┌──────────────┐    ┌──────┐   │
│  │ RJ45 │    │  Ethernet   │    │   ARM Cortex │    │ RJ45 │   │
│  │ IN   ├───►│  Switch IC  ├───►│   M7 MCU     ├───►│ OUT  │   │
│  │(PSE) │    │ (Tap/Mirror)│    │  (STM32H743) │    │(PD)  │   │
│  └──┬───┘    └──────┬──────┘    └──────┬───────┘    └──┬───┘   │
│     │               │                   │               │       │
│     │         ┌─────▼─────┐       ┌──────▼──────┐       │       │
│     │         │ RMII/RGMII│       │ 128Mb SDRAM │       │       │
│     │         │   MAC     │       │  (IS42S)    │       │       │
│     │         └───────────┘       └─────────────┘       │       │
│     │                                                 │       │
│     │    ┌────────────┐    ┌──────────────┐           │       │
│     ├───►│  PoE PD     ├───►│  DC-DC      │───────────┤       │
│     │    │  Controller │    │  3.3V/1.2V  │           │       │
│     │    │  (TPS2378)  │    │  (TPS62A)   │           │       │
│     │    └────────────┘    └──────────────┘           │       │
│     │                                              Pass-    │
│     │                                              Through  │
│     └────────────────────────────────────────────────┘       │
│                                                                 │
│         ┌───────────┐    ┌──────────┐                          │
│         │ BLE 5.0   │    │  SPI     │                          │
│         │ (nRF528)  │    │  Flash   │                          │
│         │  Config   │    │ (16MB)  │                          │
│         └───────────┘    └──────────┘                          │
└─────────────────────────────────────────────────────────────────┘
```

## 6. Data Flow

### 6.1 Normal Pass-Through (Stealth Mode)
```
PSE/Switch ──► RJ45-IN ──► [Magnetics + Tap] ──► Ethernet Switch (mirror port) ──► MCU Capture
                                   │
                                   └───► [PoE PD + Pass] ──► RJ45-OUT ──► Victim Device
```

1. Ethernet traffic enters RJ45-IN from the PSE (Power Sourcing Equipment / switch)
2. Magnetics isolate and couple the signal
3. The Ethernet Switch IC (KSZ9897) mirrors all traffic to the MCU's RGMII port
4. Traffic simultaneously passes through to RJ45-OUT toward the victim device
5. PoE power is extracted inline: the PD controller negotiates Class 0, draws 12.95W; most power is passed through to the downstream PD

### 6.2 Active MITM (Injection Mode)
```
PSE/Switch ──► RJ45-IN ──► Ethernet Switch ──► MCU (inspect/modify) ──► Ethernet Switch ──► RJ45-OUT
```

1. In injection mode, the switch is reconfigured: IN port traffic is redirected to the MCU
2. MCU inspects each packet against rule engine
3. Matching packets are modified (ARP, DNS, DHCP, etc.)
4. Modified packets are re-injected via the switch to the OUT port
5. Non-matching packets are forwarded unmodified with < 10µs added latency

### 6.3 C2 Exfiltration
```
MCU ──► [Encapsulate in GRE/IPsec] ──► Ethernet Switch ──► RJ45-IN ──► Network (to operator)
```
OR
```
MCU ──► BLE 5.0 ──► Operator's phone (companion app) ──► Cloud C2
```

## 7. Bus Topology

```
                    ┌───────────────────┐
                    │   STM32H743ZIT6   │
                    │   (Cortex-M7)     │
                    └──┬──┬──┬──┬──┬──┘
                       │  │  │  │  │
            ┌──────────┘  │  │  │  └──────────┐
            │             │  │  │             │
     ┌──────▼──────┐     │  │  │     ┌───────▼──────┐
     │  KSZ9897R   │     │  │  │     │  W25Q128JV   │
     │  Eth Switch │     │  │  │     │  SPI Flash   │
     │  (RGMII)    │     │  │  │     │  (SPI1)      │
     └──────┬──────┘     │  │  │     └──────────────┘
            │             │  │  │
     ┌──────▼──────┐     │  │  │     ┌──────────────┐
     │ IS42S32800G │     │  │  └────►│  nRF52832    │
     │  SDRAM      │     │  │        │  BLE Config  │
     │  (FMC)      │     │  │        │  (UART4)     │
     └─────────────┘     │  │        └──────────────┘
                          │  │
                    ┌─────▼──▼────────┐
                    │  TPS2378DDW     │
                    │  PoE PD Ctrl    │
                    │  (I2C1 + Power) │
                    └─────────────────┘
```

| Bus | Master | Slaves | Protocol | Speed |
|-----|--------|--------|----------|-------|
| RGMII | MCU MAC | KSZ9897 Switch | RGMII (4-bit DDR) | 125 MHz clock |
| FMC | MCU | IS42S32800G SDRAM | SDRAM (32-bit) | up to 200 MHz |
| SPI1 | MCU | W25Q128JV Flash | SPI Mode 0 | 80 MHz |
| I2C1 | MCU | TPS2378 PD Ctrl | I2C 7-bit | 400 kHz |
| UART4 | MCU | nRF52832 | 8N1 | 115200 baud |
| SPI2 | MCU | KSZ9897 MDC/MDIO | SMI (Clause 22) | 2.5 MHz |

## 8. Key Design Decisions

1. **STM32H743** over Linux SoC — deterministic <10µs latency, lower power (300mW), no OS overhead for packet modification, instant boot
2. **KSZ9897** managed switch — hardware port mirroring eliminates MCU load in passive mode, supports VLAN QinQ for covert channel
3. **PoE Class 0** — simplest signature, maximum available power (12.95W), passes ~10W to downstream PD
4. **BLE for provisioning only** — not for data exfil (too slow); BLE used for initial rule upload and status queries before wired C2 is established
5. **No Ethernet PHYs on MCU side** — the KSZ9897's RGMII MAC-to-MAC connection avoids redundant PHYs and saves power/space
6. **Circular SDRAM buffer** — 16 MB allows ~1.3 seconds of full-duplex gigabit capture at line rate; enough for burst analysis and protocol dissection