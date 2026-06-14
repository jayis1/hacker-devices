# ShadowTap — Phase 4: Software Stack

## 1. Boot Strategy

The i.MX RT1062 supports multiple boot modes controlled by BOOT_MODE[1:0] pins and eFUSE configuration.

### 1.1 Boot Configuration

| BOOT_MODE[1:0] | Mode | Our Setting |
|---|---|---|
| 00 | Internal boot (fuse-defined) | **Selected** — boot from SPI NOR flash |
| 01 | Serial Downloader | Fallback for recovery |
| 10 | Internal boot | Not used |
| 11 | Boot from Fuses | Not used |

- **Primary boot**: SPI NOR flash (U7 IS25LP016D) via FlexSPI at 60 MHz
- **Boot header**: FlexSPI Configuration Block (FCB) at offset 0x0 in flash
- **Image Vector Table (IVT)**: At offset 0x1000, points to Device Configuration Data (DCD)
- **DCD**: Configures SDRAM (if added) and initial clock settings
- **Application**: Starts at offset 0x2000 in flash

### 1.2 Boot Sequence

```
Power-on / Reset
    │
    ▼
POR_B released → SoC internal reset deasserted
    │
    ▼
Boot ROM reads BOOT_MODE pins → Internal Boot from FlexSPI
    │
    ▼
ROM reads FCB from SPI flash → Configures FlexSPI for fast read
    │
    ▼
ROM reads IVT → Validates HMAC/signature (if HAB enabled)
    │
    ▼
ROM loads DCD → Configures clocks, IOMUX, pad settings
    │
    ▼
ROM jumps to application entry point (0x60002000 in XIP mapping)
    │
    ▼
Reset_Handler → .data init, .bss clear → main()
```

### 1.3 Flash Memory Map

| Offset | Size | Content |
|---|---|---|
| 0x000000 | 4 KB | FlexSPI Configuration Block (FCB) |
| 0x001000 | 4 KB | Image Vector Table (IVT) + Boot Data |
| 0x001400 | 1 KB | Device Configuration Data (DCD) |
| 0x002000 | 60 KB | Bootloader / recovery image |
| 0x011000 | 4 KB | Configuration store (MITM rules, device ID) |
| 0x012000 | 988 KB | Main application (XIP) |
| 0x100000 | 15 MB | PCAP storage overflow / firmware backup |

## 2. MMIO Register Map

### 2.1 ENET1 Registers (Base: 0x402D_8000)

| Offset | Name | Access | Purpose |
|---|---|---|---|
| 0x000 | ENET_EIR | R/W1C | Interrupt Event |
| 0x004 | ENET_EIMR | R/W | Interrupt Mask |
| 0x008 | ENET_RDAR | R/W | Receive Descriptor Active |
| 0x010 | ENET_TDAR | R/W | Transmit Descriptor Active |
| 0x014 | ENET_ECR | R/W | Ethernet Control (enable, speed) |
| 0x018 | ENET_MSCR | R/W | MII Speed Control |
| 0x040 | ENET_RCR | R/W | Receive Control (promisc, loop) |
| 0x048 | ENET_TCR | R/W | Transmit Control (CRC, pad) |
| 0x04C | ENET_PALR | R/W | Physical Address Low |
| 0x050 | ENET_PAUR | R/W | Physical Address High |
| 0x05C | ENET_OPD | R/W | Opcode/Pause Duration |
| 0x084 | ENET_IAUR | R/W | Descriptor Upper (hash) |
| 0x088 | ENET_IALR | R/W | Descriptor Lower (hash) |
| 0x0C4 | ENET_TFWR | R/W | Transmit FIFO Watermark |
| 0x0E4 | ENET_RDSR | R/W | Receive Descriptor Ring Start |
| 0x0E8 | ENET_TDSR | R/W | Transmit Descriptor Ring Start |
| 0x0EC | ENET_MRBR | R/W | Maximum Receive Buffer Size |

### 2.2 ENET2 Registers (Base: 0x402D_8400)
Same layout as ENET1, offset by 0x400.

### 2.3 FlexSPI Registers (Base: 0x402A_8000)

| Offset | Name | Access | Purpose |
|---|---|---|---|
| 0x000 | FLEXSPI_MCR0 | R/W | Module Control 0 (timeout, enable) |
| 0x004 | FLEXSPI_MCR2 | R/W | Module Control 2 |
| 0x010 | FLEXSPI_STS0 | R | Status 0 (idle, seq done) |
| 0x014 | FLEXSPI_STS1 | R | Status 1 |
| 0x020 | FLEXSPI_LUTKEY | R/W | LUT Key (unlock LUT writes) |
| 0x024 | FLEXSPI_LUTCR | R/W | LUT Control (lock, unlock) |
| 0x080 | FLEXSPI_LUT0–63 | R/W | Lookup Tables (sequencer opcodes) |
| 0x100 | FLEXSPI_AHBCR | R/W | AHB Control (cache, prefetch) |
| 0x104 | FLEXSPI_AHBRX_BUF0CR0 | R/W | AHB RX Buffer 0 Config |

### 2.4 LPUART1 Registers (Base: 0x4018_5000)

| Offset | Name | Access | Purpose |
|---|---|---|---|
| 0x000 | VERID | R | Version ID |
| 0x004 | PARAM | R | Parameter |
| 0x008 | CTRL | R/W | Control (enable, parity, TX/RX) |
| 0x00C | GLOBAL | R/W | Global (reset, doze) |
| 0x010 | PINCFG | R/W | Pin Configuration |
| 0x014 | BAUD | R/W | Baud Rate (OSR, SBR) |
| 0x018 | STAT | R/W1C | Status flags |
| 0x01C | CTRLIDLE | R | — |
| 0x020 | DATA | R/W | Data register (read RX / write TX) |
| 0x024 | MATCH | R/W | Address match |
| 0x028 | MODIR | R/W | Modem IrDA |
| 0x02C | FIFO | R/W | FIFO control (txflush, rxflush) |
| 0x030 | WATER | R/W | Watermark (tx/rx counts) |

### 2.5 uSDHC1 Registers (Base: 0x402C_0000)

| Offset | Name | Access | Purpose |
|---|---|---|---|
| 0x000 | DS_ADDR | R/W | DMA System Address |
| 0x004 | BLK_ATT | R/W | Block Attributes (size, count) |
| 0x008 | CMD_ARG | R/W | Command Argument |
| 0x00C | CMD_XFR_TYP | R/W | Command Transfer Type |
| 0x010 | CMD_RSP0 | R | Command Response 0 |
| 0x014 | CMD_RSP1 | R | Command Response 1 |
| 0x020 | INT_STATUS | R/W1C | Interrupt Status |
| 0x024 | INT_STATUS_EN | R/W | Interrupt Status Enable |
| 0x028 | INT_SIGNAL_EN | R/W | Interrupt Signal Enable |
| 0x02C | AUTOCMD12_ERR | R | Auto CMD12 Error |
| 0x030 | HOST_CTRL_CAP | R/W | Host Controller Capability |
| 0x034 | VEND_SPEC | R/W | Vendor Specific (DMA, clock) |
| 0x038 | MMC_BOOT | R/W | MMC Boot |
| 0x03C | VEND_SPEC2 | R/W | Vendor Specific 2 |
| 0x040 | TUNING_CTRL | R/W | Tuning Control |

## 3. Clock & GPIO Initialization

### 3.1 Clock Tree

```
24 MHz XTAL (Y1) ──▶ PLL_ARM    (×50/2 = 600 MHz) ──▶ ARM Core M7
                  ──▶ PLL_SYS    (×22/2 = 264 MHz) ──▶ SYS bus, ENET
                  ──▶ PLL_ENET   (×25/2 = 312.5 MHz) ──▶ ENET 125 MHz ref
                  │                                          ──▶ ENET 50 MHz MDC
                  ──▶ PLL_AUDIO (×24/4 = 144 MHz) ──▶ Audio (unused)
                  ──▶ PLL_USB1   (×20/2 = 240 MHz) ──▶ USB (unused)
```

### 3.2 Clock Configuration Code

```c
// Target clocks:
// ARM: 600 MHz
// IPG: 150 MHz
// ENET: 125 MHz (RGMII TX CLK)
// ENET 50 MHz: for MDC

void board_clock_init(void) {
    // Set ARM PLL to 600 MHz: 24 MHz * (50/2) = 600 MHz
    CCM_ANALOG->PLL_ARM = CCM_ANALOG_PLL_ARM_ENABLE_MASK |
                          CCM_ANALOG_PLL_ARM_DIV_SELECT(50);

    // Set SYSTEM PLL to 264 MHz for IP bus
    CCM_ANALOG->PLL_SYS = CCM_ANALOG_PLL_SYS_ENABLE_MASK |
                          CCM_ANALOG_PLL_SYS_DIV_SELECT(22);

    // Set ENET PLL for 125 MHz TX reference and 50 MHz MDC
    CCM_ANALOG->PLL_ENET = CCM_ANALOG_PLL_ENET_ENABLE_MASK |
                           CCM_ANALOG_PLL_ENET_ENET_125M_REF_EN_MASK |
                           CCM_ANALOG_PLL_ENET_ENET_50M_REF_EN_MASK |
                           CCM_ANALOG_PLL_ENET_ENET_25M_REF_EN_MASK;

    // Clock selectors
    CCM->CBCMR = (CCM->CBCMR & ~CCM_CBCMR_PRE_PERIPH_CLK_SEL_MASK) |
                 CCM_CBCMR_PRE_PERIPH_CLK_SEL(0x3); // PLL_SYS = 528 MHz / 2

    // Enable ENET1 and ENET2 clocks
    CCM->CCGR1 |= CCM_CCGR1_CG10_MASK |  // ENET1 clock gate
                  CCM_CCGR1_CG11_MASK;     // ENET2 clock gate

    // Enable LPUART1 clock
    CCM->CCGR5 |= CCM_CCGR5_CG12_MASK;

    // Enable LPSPI1 clock
    CCM->CCGR1 |= CCM_CCGR1_CG0_MASK;

    // Enable uSDHC1 clock
    CCM->CCGR6 |= CCM_CCGR6_CG0_MASK;

    // Enable LPI2C1, LPI2C2 clocks
    CCM->CCGR2 |= CCM_CCGR2_CG3_MASK | CCM_CCGR2_CG4_MASK;
}
```

### 3.3 GPIO/IOMUX Configuration

```c
void board_iomux_init(void) {
    // ENET1 RGMII TX (with 49.9Ω drive strength)
    IOMUXC->SW_MUX_CTL_PAD[GPIO_AD_B0_11] = 0x09; // ENET1_TXD0
    IOMUXC->SW_MUX_CTL_PAD[GPIO_AD_B0_12] = 0x09; // ENET1_TXD1
    IOMUXC->SW_MUX_CTL_PAD[GPIO_AD_B0_15] = 0x09; // ENET1_TXEN
    IOMUXC->SW_MUX_CTL_PAD[GPIO_AD_B0_10] = 0x09; // ENET1_TX_CLK

    IOMUXC->SW_PAD_CTL_PAD[GPIO_AD_B0_11] = IOMUXC_PAD_DSE(6) | IOMUXC_PAD_SRE_MASK;
    IOMUXC->SW_PAD_CTL_PAD[GPIO_AD_B0_12] = IOMUXC_PAD_DSE(6) | IOMUXC_PAD_SRE_MASK;
    IOMUXC->SW_PAD_CTL_PAD[GPIO_AD_B0_15] = IOMUXC_PAD_DSE(6) | IOMUXC_PAD_SRE_MASK;
    IOMUXC->SW_PAD_CTL_PAD[GPIO_AD_B0_10] = IOMUXC_PAD_DSE(6) | IOMUXC_PAD_SRE_MASK;

    // ENET1 RGMII RX
    IOMUXC->SW_MUX_CTL_PAD[GPIO_AD_B0_04] = 0x09; // ENET1_RXD0
    IOMUXC->SW_MUX_CTL_PAD[GPIO_AD_B0_05] = 0x09; // ENET1_RXD1
    IOMUXC->SW_MUX_CTL_PAD[GPIO_AD_B0_09] = 0x09; // ENET1_RXDV
    IOMUXC->SW_MUX_CTL_PAD[GPIO_AD_B0_08] = 0x09; // ENET1_RX_CLK

    // ENET1 MDIO/MDC
    IOMUXC->SW_MUX_CTL_PAD[GPIO_AD_B0_02] = 0x09; // ENET1_MDIO
    IOMUXC->SW_MUX_CTL_PAD[GPIO_AD_B0_03] = 0x09; // ENET1_MDC

    // ENET2 — same pattern for GPIO_AD_B1_xx
    IOMUXC->SW_MUX_CTL_PAD[GPIO_AD_B1_07] = 0x09; // ENET2_TXD0
    IOMUXC->SW_MUX_CTL_PAD[GPIO_AD_B1_08] = 0x09; // ENET2_TXD1
    IOMUXC->SW_MUX_CTL_PAD[GPIO_AD_B1_09] = 0x09; // ENET2_TXEN
    IOMUXC->SW_MUX_CTL_PAD[GPIO_AD_B1_06] = 0x09; // ENET2_TX_CLK
    IOMUXC->SW_MUX_CTL_PAD[GPIO_AD_B1_02] = 0x09; // ENET2_RXD0
    IOMUXC->SW_MUX_CTL_PAD[GPIO_AD_B1_03] = 0x09; // ENET2_RXD1
    IOMUXC->SW_MUX_CTL_PAD[GPIO_AD_B1_05] = 0x09; // ENET2_RXDV
    IOMUXC->SW_MUX_CTL_PAD[GPIO_AD_B1_04] = 0x09; // ENET2_RX_CLK
    IOMUXC->SW_MUX_CTL_PAD[GPIO_AD_B1_00] = 0x09; // ENET2_MDIO
    IOMUXC->SW_MUX_CTL_PAD[GPIO_AD_B1_01] = 0x09; // ENET2_MDC

    // LPUART1 for BLE C2
    IOMUXC->SW_MUX_CTL_PAD[GPIO_B0_00] = 0x08; // LPUART1_TX
    IOMUXC->SW_MUX_CTL_PAD[GPIO_B0_01] = 0x08; // LPUART1_RX

    // LPSPI1 for NOR flash
    IOMUXC->SW_MUX_CTL_PAD[GPIO_B0_04] = 0x07; // LPSPI1_SCK
    IOMUXC->SW_MUX_CTL_PAD[GPIO_B0_05] = 0x07; // LPSPI1_SDO
    IOMUXC->SW_MUX_CTL_PAD[GPIO_B0_06] = 0x07; // LPSPI1_SDI
    IOMUXC->SW_MUX_CTL_PAD[GPIO_B0_07] = 0x07; // LPSPI1_PCS0

    // SDIO for microSD
    IOMUXC->SW_MUX_CTL_PAD[GPIO_SD_B0_00] = 0x04; // uSDHC1_CMD
    IOMUXC->SW_MUX_CTL_PAD[GPIO_SD_B0_01] = 0x04; // uSDHC1_CLK
    IOMUXC->SW_MUX_CTL_PAD[GPIO_SD_B0_02] = 0x04; // uSDHC1_D0
    IOMUXC->SW_MUX_CTL_PAD[GPIO_SD_B0_03] = 0x04; // uSDHC1_D1
    IOMUXC->SW_MUX_CTL_PAD[GPIO_SD_B0_04] = 0x04; // uSDHC1_D2
    IOMUXC->SW_MUX_CTL_PAD[GPIO_SD_B0_05] = 0x04; // uSDHC1_D3
    IOMUXC->SW_MUX_CTL_PAD[GPIO_SD_B0_06] = 0x04; // uSDHC1_CD

    // I²C for EEPROM and PoE
    IOMUXC->SW_MUX_CTL_PAD[GPIO_B0_11] = 0x06; // LPI2C1_SDA
    IOMUXC->SW_MUX_CTL_PAD[GPIO_B0_12] = 0x06; // LPI2C1_SCL
    IOMUXC->SW_MUX_CTL_PAD[GPIO_B0_13] = 0x06; // LPI2C2_SDA
    IOMUXC->SW_MUX_CTL_PAD[GPIO_B0_14] = 0x06; // LPI2C2_SCL

    // GPIO for LED and button
    IOMUXC->SW_MUX_CTL_PAD[GPIO_EMC_04] = 0x05; // GPIO1[4] LED data
    IOMUXC->SW_MUX_CTL_PAD[GPIO_EMC_05] = 0x05; // GPIO1[5] Button
}
```

## 4. Device Drivers

### 4.1 Ethernet Driver (enet_driver.h / enet_driver.c)

See firmware/drivers/ for full implementation. Key features:
- Zero-copy DMA ring buffers (128 RX descriptors, 64 TX descriptors per ENET)
- Cut-through forwarding: start TX on port B as soon as 64 bytes received on port A
- Promiscuous mode by default (RCR.PRO=1)
- No MAC learning (transparent bridge)
- Frame matching engine: TCAM-like 8-tuple match on {dst_mac, src_mac, ethertype, dst_ip, src_ip, dst_port, src_port, tcp_flags}
- MITM action engine: modify, drop, inject, redirect

### 4.2 BLE C2 Driver (ble_c2_driver.h / ble_c2_driver.c)

See firmware/drivers/ for full implementation. Key features:
- UART-based command protocol with SLIP framing
- AES-256-CTR encrypted channel (key exchange over BLE SMP)
- Command set: ADD_RULE, DEL_RULE, LIST_RULES, START_CAP, STOP_CAP, STREAM_PCAP, GET_STATUS
- Asynchronous notifications: rule match, PoE status, link change

### 4.3 PHY Management (88E1510) via MDIO

```c
// 88E1510 Register Map (Clause 22)
#define PHY_REG_BMCR       0x00  // Basic Mode Control
#define PHY_REG_BMSR       0x01  // Basic Mode Status
#define PHY_REG_PHYID1     0x02  // PHY ID High (0x0141 for Marvell)
#define PHY_REG_PHYID2     0x03  // PHY ID Low (0x0C82 for 88E1510)
#define PHY_REG_ANAR       0x04  // Auto-Negotiation Advertisement
#define PHY_REG_ANLPAR     0x05  // Auto-Negotiation Link Partner
#define PHY_REG_1000BT     0x09  // 1000Base-T Control
#define PHY_REG_SPEC_CTRL  0x10  // Specific Control
#define PHY_REG_SPEC_STAT  0x11  // Specific Status
#define PHY_REG_LED_FUNC   0x10  // LED Function (Page 3, Reg 16)

// Initialization sequence for RGMII mode
void phy_88e1510_init(uint8_t phy_addr) {
    // Page 0: Copper control
    mdio_write(phy_addr, 0x16, 0x0000); // Select Page 0

    // Software reset
    mdio_write(phy_addr, PHY_REG_BMCR, 0x8000);
    while (mdio_read(phy_addr, PHY_REG_BMCR) & 0x8000); // Wait for reset

    // Enable RGMII mode, configure timing
    mdio_write(phy_addr, 0x16, 0x0002); // Select Page 2 (MAC interface)
    mdio_write(phy_addr, 0x14, 0x0F72); // RGMII mode, TX/RX delay, 1000M
    mdio_write(phy_addr, 0x15, 0x0000); // No additional delay
    mdio_write(phy_addr, 0x16, 0x0000); // Back to Page 0

    // Enable auto-negotiation with 1000Base-T
    mdio_write(phy_addr, PHY_REG_1000BT, 0x0200); // Advertise 1000Base-T full
    mdio_write(phy_addr, PHY_REG_ANAR, 0x01E1);   // Advertise 100/10 full/half
    mdio_write(phy_addr, PHY_REG_BMCR, 0x1200);   // Enable AN, restart

    // Wait for link
    while (!(mdio_read(phy_addr, PHY_REG_SPEC_STAT) & 0x0800)); // Link up
}
```

## 5. Device Tree (Linux-style, for reference)

```dts
/dts-v1/;

/ {
    model = "ShadowTap PoE Network Tap";
    compatible = "shadowtap,imxrt1062";

    chosen {
        bootargs = "console=lpuart1,115200";
    };

    memory@20200000 {
        device_type = "memory";
        reg = <0x20200000 0x00060000>; /* 384 KB DTCM + OCRAM */
    };

    clocks {
        osc: osc {
            compatible = "fixed-clock";
            clock-frequency = <24000000>;
        };
    };

    enet1: ethernet@402d8000 {
        compatible = "fsl,imxrt-enet";
        reg = <0x402d8000 0x400>;
        interrupts = <114 0>, <115 0>;
        clocks = <&osc>;
        phy-mode = "rgmii-id";
        phy-handle = <&phy1>;
        mdio {
            #address-cells = <1>;
            #size-cells = <0>;
            phy1: ethernet-phy@0 { reg = <0>; };
        };
    };

    enet2: ethernet@402d8400 {
        compatible = "fsl,imxrt-enet";
        reg = <0x402d8400 0x400>;
        interrupts = <116 0>, <117 0>;
        clocks = <&osc>;
        phy-mode = "rgmii-id";
        phy-handle = <&phy2>;
        mdio {
            #address-cells = <1>;
            #size-cells = <0>;
            phy2: ethernet-phy@1 { reg = <1>; };
        };
    };

    lpuart1: serial@40185000 {
        compatible = "fsl,imxrt-lpuart";
        reg = <0x40185000 0x20>;
        clocks = <&osc>;
        status = "okay";
    };

    flexspi: spi@402a8000 {
        compatible = "fsl,imxrt-flexspi";
        reg = <0x402a8000 0x400>, <0x60000000 0x1000000>;
        reg-names = "regs", "memory";
        clocks = <&osc>;
        status = "okay";

        flash@0 {
            compatible = "issi,is25lp016d";
            reg = <0>;
            spi-max-frequency = <60000000>;
        };
    };

    usdhc1: mmc@402c0000 {
        compatible = "fsl,imxrt-usdhc";
        reg = <0x402c0000 0x400>;
        clocks = <&osc>;
        bus-width = <4>;
        status = "okay";
    };
};
```

## 6. Main Application Architecture

```
┌─────────────────────────────────────────────┐
│                  main()                      │
│                                              │
│  ┌──────────┐  ┌──────────┐  ┌───────────┐ │
│  │ ENET1    │  │ ENET2    │  │ BLE C2    │  │
│  │ RX Ring  │  │ TX Ring  │  │ UART RX   │  │
│  │ (DMA)    │  │ (DMA)    │  │ (DMA)     │  │
│  └────┬─────┘  └────┬─────┘  └─────┬─────┘ │
│       │              │              │         │
│  ┌────▼──────────────▼──────────────▼──────┐ │
│  │         Frame Processing Engine         │ │
│  │                                         │ │
│  │  ┌─────────────┐  ┌─────────────────┐   │ │
│  │  │ Cut-Through │  │ Rule Matcher    │   │ │
│  │  │ Forwarder   │  │ (TCAM)          │   │ │
│  │  │ (fast path) │  │ (slow path)     │   │ │
│  │  └─────────────┘  └────────┬────────┘   │ │
│  │                            │            │ │
│  │                   ┌────────▼────────┐    │ │
│  │                   │ MITM Engine     │    │ │
│  │                   │ (modify/inject) │    │ │
│  │                   └────────┬────────┘   │ │
│  └────────────────────────────┼────────────┘ │
│                                │              │
│                   ┌────────────▼────────────┐ │
│                   │ PCAP Writer             │ │
│                   │ (SDIO DMA → microSD)    │ │
│                   └─────────────────────────┘ │
└─────────────────────────────────────────────┘
```

### 6.1 Interrupt Priority

| Priority | ISR | Rationale |
|---|---|---|
| 0 (highest) | ENET1_RX / ENET2_RX | Cut-through latency |
| 1 | ENET1_TX / ENET2_TX | TX completion, ring refill |
| 2 | uSDHC1 | PCAP write completion |
| 3 | LPUART1 | BLE C2 commands |
| 4 | LPSPI1 | Flash operations |
| 5 | LPI2C1/2 | EEPROM / PoE polling |
| 6 (lowest) | GPIO | Button, LED |

## 7. Build Instructions

### 7.1 Prerequisites

```bash
# ARM GCC toolchain
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi

# OpenOCD for flashing
sudo apt-get install openocd

# NXP MCUXpresso SDK (download from nxp.com)
# Copy SDK_2.x_MIMXRT1062 to sdk/ directory
```

### 7.2 Build

```bash
cd firmware
make CROSS_COMPILE=arm-none-eabi- SDK_PATH=../sdk clean all
# Output: shadow_tap.elf, shadow_tap.bin, shadow_tap.hex
```

### 7.3 Flash via OpenOCD (SPI NOR via SWD)

```bash
openocd -f interface/cmsis-dap.cfg -f target/imxrt1062.cfg \
    -c "program shadow_tap.elf verify reset exit"
```

### 7.4 Flash via Serial Recovery (USB)

Hold SW1 during power-on → enters Serial Downloader mode:
```bash
# Using NXP blhost tool
blhost -u 0x15A2,0x0073 -- flash-image shadow_tap.sb
```

## 8. PCAP File Format

On-device capture writes standard PCAP format to microSD:

| Field | Offset | Size | Value |
|---|---|---|---|
| Magic | 0 | 4 | 0xA1B2C3D4 |
| Version Major | 4 | 2 | 2 |
| Version Minor | 6 | 2 | 4 |
| ThisZone | 8 | 4 | 0 |
| SigFigs | 12 | 4 | 0 |
| SnapLen | 16 | 4 | 65535 |
| Network | 20 | 4 | 1 (LINKTYPE_ETHERNET) |
| Per-packet: | | | |
| ts_sec | 0 | 4 | Second from ENET timer |
| ts_usec | 4 | 4 | Microsecond |
| incl_len | 8 | 4 | Captured length |
| orig_len | 12 | 4 | Original length |
| packet data | 16 | incl_len | Raw frame bytes |