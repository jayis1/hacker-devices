# WFP-100 Firmware

Production firmware for the WFP-100 WiFi Pentesting Dongle (StarFive JH7110).

## Architecture

```
┌─────────────────────────────────────┐
│          Linux Kernel 6.6+         │
│  ax210_pcie.ko │ usb_gadget │ iwd  │
├─────────────────────────────────────┤
│         U-Boot SPL (this code)     │
│  Clock init → PMIC → DDR → Boot   │
├─────────────────────────────────────┤
│     Boot ROM (JH7110 on-chip)      │
└─────────────────────────────────────┘
```

## Building

### Prerequisites
- `riscv64-unknown-elf-gcc` cross-compiler toolchain
- GNU Make 4.3+
- U-Boot 2024.04 source (StarFive BSP branch)

### Build SPL and U-Boot
```bash
export CROSS_COMPILE=riscv64-unknown-elf-
export ARCH=riscv

# Build U-Boot SPL
make wfp100_defconfig
make -j$(nproc)

# Output files:
#   spl/u-boot-spl.bin     → Flash to QSPI @ 0x000000
#   u-boot.bin             → Flash to QSPI @ 0x040000
```

### Flash via JTAG
```bash
openocd -f board/wfp100.cfg \
  -c "init; flash write_image erase spl/u-boot-spl.bin 0x000000; \
      flash write_image erase u-boot.bin 0x040000; exit"
```

### Flash via U-Boot console
```bash
# From U-Boot serial console:
sf probe 0
tftp 0x40000000 u-boot-spl.bin
sf update 0x40000000 0x000000 ${filesize}
tftp 0x40000000 u-boot.bin
sf update 0x40000000 0x040000 ${filesize}
```

## Linux Kernel Module (AX210 Driver)

```bash
# Build the AX210 monitor mode driver
cd drivers/
make -C /path/to/linux M=$(pwd) modules

# Load on target:
insmod ax210_pcie.ko
```

## Directory Layout

```
firmware/
├── Makefile              # Top-level build
├── main.c                # Board initialization entry point
├── board.h               # GPIO pin definitions
├── registers.h           # MMIO register base addresses
├── usb_descriptors.h     # USB CDC-ECM + CDC-ACM descriptors
└── drivers/
    ├── ax210_pcie.h       # Intel AX210 driver header
    ├── ax210_pcie.c       # Intel AX210 PCIe monitor mode driver
    ├── usb_gadget.h       # USB composite gadget header
    ├── usb_gadget.c       # USB CDC-ECM + CDC-ACM gadget
    ├── axp2101.h          # AXP2101 PMIC driver header
    └── axp2101.c          # AXP2101 PMIC I2C driver
```

## License

- SPL/board code: GPL-2.0
- AX210 kernel driver: GPL-2.0 (derived from Linux kernel)
- USB gadget code: GPL-2.0