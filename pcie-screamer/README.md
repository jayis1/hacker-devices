# PCIe Screamer

**Codename:** SCREAMER-M2  
**Classification:** PCI Express Gen2/Gen3 x4 NVMe Interposer & Traffic Analyzer  

A transparent PCIe bus-level man-in-the-middle platform for security research. The PCIe Screamer sits between a host motherboard and an NVMe M.2 SSD, passively capturing all PCIe Transaction Layer Packets (TLPs) at line rate, with optional active DMA injection capability for red-team operations and vulnerability assessment.

## Key Capabilities

- **Passive TLP Capture** — Full-duplex capture of all PCIe traffic (NVMe commands, completions, config space, MMIO, DMA) at Gen2 x4 line rate (~1.6 GB/s per direction)
- **Active DMA Injection** — Craft and inject Memory Read/Write TLPs targeting arbitrary host physical addresses for DMA attack simulation
- **Zero Host Footprint** — Completely transparent to the target; no software installation required. All control and data egress via USB 3.0 side-channel
- **512 MB DDR3 Buffer** — ~260 ms of full-rate Gen2 x4 capture buffering
- **USB 3.0 Streaming** — FT601 SuperSpeed FIFO bridge for real-time TLP egress to analyst machine

## Hardware Specifications

| Parameter | Value |
|-----------|-------|
| Form Factor | M.2 2280 interposer |
| FPGA | Lattice ECP5-45F (LFE5U-45F-6BG381C) |
| DDR3 | MT41K256M16TW-107 (512 MB, 800 MT/s) |
| USB Bridge | FT601Q-B-T (USB 3.0 SuperSpeed) |
| PCIe | Gen2 x4 native (5.0 GT/s/lane); Gen3 x4 with retimer |
| Host Connector | M.2 M-key edge connector (card edge) |
| Device Connector | M.2 M-key socket (Amphenol 10132797-0011LF) |
| PCIe Redrivers | PI3EQX12908 (host + device sides) |
| Power | 3.3V from M.2 slot + supplemental USB |
| Power Budget | < 6W total |
| Operating Temp | 0°C to 70°C |

## Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                      PCIe SCREAMER INTERPOSER                       │
│                                                                     │
│  ┌──────────┐   ┌──────────────┐   ┌────────────┐   ┌──────────┐  │
│  │  HOST    │◄─►│  PCIe Gen2/3  │◄─►│    FPGA     │◄─►│  NVMe    │  │
│  │  M.2     │   │  Redriver    │   │  Lattice    │   │  SSD     │  │
│  │  SLOT    │   │  PI3EQX12908 │   │  ECP5-45F   │   │  M.2     │  │
│  └──────────┘   └──────────────┘   └──────┬─────┘   └──────────┘  │
│                                            │                        │
│                              ┌─────────────┼─────────────┐          │
│                              │             │             │          │
│                        ┌─────┴─────┐ ┌─────┴─────┐ ┌────┴────┐   │
│                        │   DDR3     │ │  TLP      │ │  FT601  │   │
│                        │  512 MB    │ │  Sniffer  │ │  USB    │   │
│                        │  Buffer    │ │  + Inject │ │  3.0    │──┼──► Analyst
│                        └───────────┘ └───────────┘ └─────────┘   │  Machine
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

The Lattice ECP5-45F FPGA runs a RISC-V (VexRiscv) soft CPU that handles initialization, USB command processing, and DDR3 buffer management. The TLP sniffer and injector pipelines run in parallel hardware, capturing and injecting TLPs at wire speed without CPU involvement.

## Threat Model

### Passive Capture Surface
- NVMe Admin/I/O Submission & Completion Queues
- PCIe Configuration Space (BAR programming, MSI/MSI-X, AER)
- Memory-Mapped I/O (doorbell writes, controller registers)
- DMA transactions (device-to-host memory reads/writes)
- PCIe Message TLPs (INTx, PME, error, vendor-defined)

### Active Injection Surface
- **DMA Read:** Inject Memory Read TLPs (MRd) to read arbitrary host physical memory
- **DMA Write:** Inject Memory Write TLPs (MWr) to corrupt kernel memory, manipulate page tables
- **Config Space:** Inject Configuration Read/Write TLPs targeting downstream devices
- **Message TLPs:** Trigger interrupts, PM events, or error conditions on the host

## Directory Structure

```
pcie-screamer/
├── README.md
├── firmware/
│   ├── main.c              # RISC-V firmware: init, USB command loop, TLP streaming
│   ├── board.h              # Pin definitions, clock config, GPIO mapping
│   ├── registers.h          # MMIO register map (0x80000000 base)
│   ├── Makefile             # RISC-V cross-compilation build
│   └── drivers/
│       ├── pcie.c/h         # PCIe Hard IP init, link training, LTSSM control
│       ├── ddr3.c/h         # DDR3 init, calibration, DMA read/write
│       ├── usb_ft601.c/h    # FT601 FIFO bridge init, TX/RX packet framing
│       ├── tlp_sniffer.c/h  # TLP capture pipeline, filter, timestamp
│       ├── tlp_injector.c/h # TLP injection engine, DMA attack sequencing
│       └── crc32c.c/h       # CRC-32C for USB frame integrity
├── kicad/
│   ├── device.kicad_sch     # Schematic (Lattice ECP5-45F, DDR3, FT601, redrivers)
│   ├── device.kicad_pcb     # PCB layout (M.2 2280, 4-layer)
│   └── device.kicad_pro     # KiCad project file
├── app/
│   ├── App.js               # React Native app entry point
│   ├── package.json         # Dependencies
│   ├── components/          # UI components (TLP viewer, hex editor, filter panel)
│   ├── screens/             # Capture, Analysis, Injection, Settings screens
│   └── utils/               # USB protocol, TLP decode, NVMe command parser
└── phase1_conceptual_architecture.md   # Detailed architecture document
    phase2_component_selection_schematics.md
    phase3_pcb_blueprints_layout.md
    phase4_software_stack.md
```

## Firmware

The firmware runs on a VexRiscv RISC-V soft CPU inside the ECP5-45F FPGA. Initialization sequence:

1. **PLL Lock** — 125 MHz system clock from 100 MHz PCIe REFCLK; 200 MHz DDR3 user clock
2. **DDR3 Init** — 512 MB MT41K256M16TW calibration and training
3. **PCIe Init** — Hard IP link training, Gen2 x4 negotiation
4. **USB Init** — FT601 enumeration, FIFO channel configuration
5. **Active Loop** — Watchdog kick, health monitoring, USB command dispatch, TLP streaming

### USB Command Protocol

All commands use a framed protocol with CRC-32C integrity:

| CMD  | Code | Description |
|------|------|-------------|
| Capture Start | 0x01 | Start TLP capture with filter/mask configuration |
| Capture Stop | 0x02 | Stop capture, flush DDR3 buffer |
| Inject TLP | 0x10 | Inject a crafted TLP (MRd/MWr/CFGRD/CFGWR) |
| Query Status | 0x20 | Read system status, link state, buffer pointers |
| Read Register | 0x22 | Direct MMIO register read |
| Write Register | 0x24 | Direct MMIO register write |
| FW Update Init | 0x30 | Begin firmware update sequence |
| FW Update Data | 0x31 | Transfer firmware block |
| FW Update Commit | 0x33 | Verify CRC and reconfigure FPGA |
| Self Test | 0x40 | Run built-in self-test |
| Reset | 0xFF | Full system reset |

## Companion App

React Native cross-platform app with four main screens:

- **Capture** — Real-time TLP stream viewer, filter controls, DDR3 buffer status
- **Analysis** — TLP decode, NVMe command extraction, hex search, DMA pattern detection
- **Injection** — TLP builder with field-level editing, DMA attack templates, safety interlocks
- **Settings** — Device configuration, firmware update, capture export

## Research Use Cases

- **Red Team** — Map NVMe driver behavior, demonstrate DMA attack feasibility
- **Vulnerability Research** — Reverse-engineer NVMe firmware, identify IOMMU bypass opportunities
- **Forensic Analysis** — Record all storage I/O including data that never reaches the filesystem
- **Hardware Audit** — Verify IOMMU/DMA remapping, ACS enforcement, ATS validation

## Legal Notice

This device is designed for authorized security research and penetration testing only. Unauthorized interception of PCIe traffic or injection of TLPs on systems you do not own or have explicit authorization to test is illegal in most jurisdictions. Always obtain proper authorization before use.

## License

All designs, firmware, and software in this repository are provided for security research purposes.