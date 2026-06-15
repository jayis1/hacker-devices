# PoE Network Implant — Phase 4: Software Stack

## 1. Boot Strategy

### 1.1 Boot Modes (BOOT0/BOOT1 Pins)

| BOOT1 | BOOT0 | Boot Source | Use Case |
|-------|-------|-------------|----------|
| 0 | 0 | Main Flash (0x08000000) | Normal operation |
| 0 | 1 | System Memory (bootloader) | DFU/UART firmware update |
| 1 | 0 | SRAM | Debug/testing |
| 1 | 1 | Reserved | — |

- **Production boot**: BOOT0 = 0 (SW2 open) → boots from internal flash
- **Recovery boot**: BOOT0 = 1 (SW2 closed) → enters STM32 built-in bootloader for UART/SWD reflash
- **BLE OTA**: nRF52832 can trigger BOOT0 high via GPIO, enabling remote recovery

### 1.2 Boot Sequence

```
Power On (PoE detected)
    │
    ├─► TPS2378 PG (Power Good) → MCU NRST released
    │
    ├─► STM32H743 boots from Flash @ 0x08000000
    │
    ├─► SystemInit(): Configure clocks, FMC, I/D-cache
    │   ├─ HSI 64MHz → PLL1: 480MHz (SYSCLK)
    │   ├─ PLL2: 200MHz (SDRAM FMC clock)
    │   ├─ PLL3: 50MHz (ETH MAC clock)
    │   └─ Enable I-Cache, D-Cache, ART accelerator
    │
    ├─► Early hardware init
    │   ├─ GPIO banks A-E clock enable
    │   ├─ RGMII pin mux configuration
    │   ├─ FMC SDRAM initialization (MR, EMR, refresh)
    │   └─ SPI1 flash probe (read JEDEC ID)
    │
    ├─► Peripheral init
    │   ├─ ETH MAC DMA configuration
    │   ├─ KSZ9897 switch configuration (SPI/SMI)
    │   ├─ UART4 to BLE (115200 8N1)
    │   ├─ I2C1 to PoE controllers
    │   └─ SysTick @ 1ms
    │
    ├─► Network stack init
    │   ├─ lwIP TCP/IP stack initialization
    │   ├─ ETH driver start (DMA descriptors in SDRAM)
    │   ├─ Configure switch port mirroring (Port1→CPU, Port2→CPU)
    │   └─ C2 tunnel setup (TLS WebSocket to operator)
    │
    ├─► Packet engine init
    │   ├─ Rule engine load from SPI flash
    │   ├─ ARP/DNS/DHCP spoof modules enabled per config
    │   └─ Capture ring buffer allocated in SDRAM (12MB)
    │
    └─► Main loop
        ├─ Process incoming packets (ETH DMA IRQ → inspect → modify → forward)
        ├─ Service C2 channel (command parser)
        ├─ Service BLE provisioning (UART4 IRQ)
        ├─ Update PoE telemetry (I2C1 periodic)
        └─ Housekeeping (watchdog, stats)
```

### 1.3 Startup Time Budget

| Stage | Time |
|-------|------|
| PoE PD negotiation | ~50ms |
| MCU POR + clock pll lock | ~10ms |
| SystemInit + cache | ~2ms |
| SDRAM init | ~5ms |
| Peripheral init | ~20ms |
| KSZ9897 config | ~50ms |
| lwIP + ETH DMA | ~30ms |
| Rule engine + C2 | ~100ms |
| **Total** | **~267ms** (well under 3s target) |

## 2. MMIO Registers

### 2.1 RCC Clock Registers (STM32H743)

| Register | Address | Reset | Function |
|----------|---------|-------|----------|
| RCC_CR | 0x58024400 | 0x00000063 | Clock control (HSI/HSE/PLL enable) |
| RCC_PLLCKSELR | 0x58024428 | 0x02020200 | PLL clock source selection |
| RCC_PLL1DIVR | 0x58024430 | 0x01010201 | PLL1 dividers (N, M, P, Q, R) |
| RCC_PLL2DIVR | 0x58024438 | 0x01010201 | PLL2 dividers |
| RCC_PLL3DIVR | 0x58024440 | 0x01010201 | PLL3 dividers |
| RCC_D1CCIPR | 0x5802448C | 0x00000000 | Domain 1 clock config (FMC, ETH) |
| RCC_AHB4ENR | 0x580244E0 | 0x00000100 | AHB4 periph enable (GPIOA-E) |
| RCC_APB1LENR | 0x5802448C | 0x00000000 | APB1L periph enable (UART4, I2C1) |
| RCC_AHB1ENR | 0x580244E0 | 0x00000100 | AHB1 enable (ETHMAC, FMC) |

### 2.2 GPIO Registers (STM32H743)

| Register | Address | Function |
|----------|---------|----------|
| GPIOA_MODER | 0x58020000 | Port A mode (0=input, 1=output, 2=alt, 3=analog) |
| GPIOA_OTYPER | 0x58020004 | Port A output type |
| GPIOA_OSPEEDR | 0x58020008 | Port A output speed (0=low, 3=very high) |
| GPIOA_PUPDR | 0x5802000C | Port A pull-up/pull-down |
| GPIOA_AFRL | 0x58020020 | Port A alternate function low (AF0-AF15) |
| GPIOA_AFRH | 0x58020024 | Port A alternate function high |
| GPIOB_MODER | 0x58020400 | Port B mode |
| GPIOC_MODER | 0x58020800 | Port C mode |
| GPIOD_MODER | 0x58020C00 | Port D mode |
| GPIOE_MODER | 0x58021000 | Port E mode |

### 2.3 ETH MAC Registers (STM32H743)

| Register | Address | Function |
|----------|---------|----------|
| ETH_MACCR | 0x40028000 | MAC control (RE, TE, Speed, DM) |
| ETH_MFFCR | 0x40028100 | MAC frame filter control |
| ETH_MACA0HR | 0x40028400 | MAC address 0 high |
| ETH_MACA0LR | 0x40028404 | MAC address 0 low |
| ETH_MMCCR | 0x40028500 | MMC control |
| ETH_DMAISR | 0x40028700 | DMA interrupt status |
| ETH_DMAIER | 0x58024410 | DMA interrupt enable |
| ETH_DMABMR | 0x400281000 | DMA bus mode (8DW, fixed burst) |
| ETH_DMARDLAR | 0x400280C8 | DMA RX descriptor list address |
| ETH_DMATDLAR | 0x400280D0 | DMA TX descriptor list address |
| ETH_DMAOMR | 0x40028038 | DMA operation mode (SR, ST) |

### 2.4 FMC SDRAM Registers (STM32H743)

| Register | Address | Function |
|----------|---------|----------|
| FMC_BCR5 | 0x58021440 | SDRAM bank 5 control |
| FMC_BTR5 | 0x58021444 | SDRAM bank 5 timing |
| FMC_SDCR1 | 0x580214C0 | SDRAM control register 1 |
| FMC_SDCR2 | 0x580214C4 | SDRAM control register 2 |
| FMC_SDTR1 | 0x580214E0 | SDRAM timing register 1 |
| FMC_SDTR2 | 0x580214E4 | SDRAM timing register 2 |
| FMC_SDCMR | 0x58021500 | SDRAM command mode |
| FMC_SDRTR | 0x58021504 | SDRAM refresh timer |

### 2.5 KSZ9897 SPI-Accessible Registers (via SMI/MDIO)

| Register | Address | Function |
|----------|---------|----------|
| CHIP_ID0 | 0x0000 | Chip ID low byte |
| CHIP_ID1 | 0x0001 | Chip ID high byte |
| P1_CTRL | 0x0100 | Port 1 control (speed, duplex) |
| P2_CTRL | 0x0200 | Port 2 control |
| P6_CTRL | 0x0600 | Port 6 (RGMII CPU) control |
| GLOBAL_CTRL | 0x0300 | Global switch control |
| MIRROR_CTRL | 0x0380 | Port mirroring control |
| VLAN_CTRL | 0x0400 | VLAN table control |
| P1_MIB_BASE | 0x1100 | Port 1 MIB counters |
| P2_MIB_BASE | 0x1200 | Port 2 MIB counters |

## 3. Clock & GPIO Initialization

### 3.1 PLL Configuration for 480MHz

```
HSI = 64 MHz (internal RC)
PLL1: 64MHz / M(2) × N(120) / P(2) = 1920MHz VCO / 4 = 480 MHz SYSCLK
PLL2: 64MHz / M(2) × N(100) / P(2) = 1600MHz VCO / 4 = 200 MHz (FMC)
PLL3: 64MHz / M(2) × N(50)  / P(2) = 800MHz VCO  / 4 = 50 MHz (ETH REF)
```

### 3.2 GPIO Mux Table (Alternate Functions)

| Pin | AF# | Peripheral | Signal |
|-----|-----|-----------|--------|
| PA0 | AF11 | ETH | MDC |
| PA1 | AF11 | ETH | MDIO |
| PA7 | AF11 | ETH | RGMII_CRSDV |
| PA8 | AF11 | ETH | RGMII_TXCLK |
| PA9 | AF11 | ETH | RGMII_TXD0 |
| PA10 | AF11 | ETH | RGMII_TXD1 |
| PA11 | AF11 | ETH | RGMII_TXD2 |
| PA12 | AF11 | ETH | RGMII_TXD3 |
| PA13 | AF11 | ETH | RGMII_TXCTL |
| PB0 | AF11 | ETH | RGMII_RXD0 |
| PB1 | AF11 | ETH | RGMII_RXD1 |
| PB2 | AF11 | ETH | RGMII_RXD2 |
| PB3 | AF11 | ETH | RGMII_RXD3 |
| PB4 | AF11 | ETH | RGMII_RXCTL |
| PB5 | AF11 | ETH | RGMII_RXCLK |
| PB7 | AF12 | FMC | NL |
| PB14 | AF5 | SPI2 | SCK |
| PB15 | AF5 | SPI2 | MOSI |
| PC1 | AF8 | UART4 | TX |
| PC2 | AF8 | UART4 | RX |
| PC3 | AF4 | I2C1 | SCL |
| PC4 | AF4 | I2C1 | SDA |
| PD0-PD15 | AF12 | FMC | D0-D15 |
| PE0-PE15 | AF12 | FMC | D16-D31, A0-A11, BA0-BA1 |

## 4. Device Drivers with DMA

### 4.1 Ethernet DMA Driver

The ETH MAC uses DMA descriptors located in SDRAM for zero-copy packet processing.

**RX Descriptor Ring (256 entries in SDRAM @ 0xC0000000):**
```
struct eth_dma_rx_desc {
    volatile uint32_t stat;     /* Own bit, First/Last, Length, Error */
    volatile uint32_t ctrl;     /* Buffer 1 size, Buffer 2 size */
    uint8_t *buf1;              /* Buffer 1 pointer (in SDRAM) */
    uint8_t *buf2;              /* Buffer 2 pointer (NULL for single buf) */
    struct eth_dma_rx_desc *next; /* Next descriptor (ring) */
    uint32_t reserved;          /* Reserved */
    uint32_t reserved2;
    uint32_t ts_low;            /* RX timestamp low */
    uint32_t ts_high;           /* RX timestamp high */
};
```

**TX Descriptor Ring (128 entries in SDRAM @ 0xC0100000):**
```
struct eth_dma_tx_desc {
    volatile uint32_t stat;     /* Own bit, First/Last, CRC, Checksum */
    volatile uint32_t ctrl;     /* Buffer 1 size, Buffer 2 size */
    uint8_t *buf1;              /* Buffer 1 pointer */
    uint8_t *buf2;              /* Buffer 2 pointer (NULL) */
    struct eth_dma_tx_desc *next;
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t ts_low;
    uint32_t ts_high;
};
```

**DMA Flow:**
1. ETH DMA receives packet → writes to RX buffer in SDRAM → sets OWN=0 → triggers IRQ
2. ISR reads descriptor → extracts packet pointer → passes to packet engine
3. Packet engine inspects/modifies → if injection needed, modifies in place or copies to TX buffer
4. For MITM: MCU TX descriptor set OWN=1 → DMA transmits modified packet to switch → switch forwards to OUT port
5. For passive: original packet untouched, copy stored in capture ring buffer

### 4.2 SPI Flash Driver (W25Q128JVSIQ)

- Uses SPI1 in DMA mode for bulk read/write
- JEDEC ID read: cmd 0x9F → response C2 28 17 (Macronix MX25L12835F-compatible)
- Read: cmd 0x0B (fast read, 133 MHz quad SPI) with 8 dummy clocks
- Page program: cmd 0x02, 256-byte pages
- Sector erase: cmd 0x20, 4KB sectors
- Status register polling via SPI (non-DMA) for write-in-progress check

### 4.3 KSZ9897 Switch Management Driver

- Clause 22 MDIO (MDC/MDIO) for PHY register access
- SPI (if using SPI interface) for switch register access
- Key operations:
  - Port mirroring: Set MIRROR_CTRL reg 0x0380 to mirror Port1+Port2 to CPU port
  - VLAN config: Set VLAN table entries for QinQ covert channel
  - Port disable/enable: For selective traffic blocking
  - MIB counters: Periodic read for traffic statistics

## 5. Device Tree (Conceptual)

```
/dts-v1/;

/ {
    model = "PhantomBridge PoE Network Implant";
    compatible = "phantom-bridge,poe-implant";

    cpus {
        cpu@0 {
            compatible = "arm,cortex-m7";
            clock-frequency = <480000000>;
        };
    };

    memory@C0000000 {
        device_type = "memory";
        reg = <0xC0000000 0x01000000>; /* 16MB SDRAM */
    };

    chosen {
        stdout-path = &uart4;
    };

    soc {
        #address-cells = <1>;
        #size-cells = <1>;

        eth@40028000 {
            compatible = "st,stm32h7-eth";
            reg = <0x40028000 0x1000>;
            interrupts = <61 2>; /* ETH global IRQ */
            clocks = <&rcc ETH_MAC_CLK>;
            phy-mode = "rgmii";
            phy-handle = <&ksz9897>;
            dma-descriptors = <0xC0000000>; /* in SDRAM */
        };

        fmc@52004000 {
            compatible = "st,stm32h7-fmc";
            reg = <0x52004000 0x1000>;
            clocks = <&rcc FMC_CLK>;
            sdram@0 {
                compatible = "issi,is42s32800g";
                reg = <0xC0000000 0x01000000>;
                bus-width = <32>;
                clock-freq = <200000000>;
                cas-latency = <3>;
                refresh-rate = <64000>; /* 64ms */
            };
        };

        spi1@40013000 {
            compatible = "st,stm32h7-spi";
            reg = <0x40013000 0x400>;
            clocks = <&rcc SPI1_CLK>;
            flash@0 {
                compatible = "winbond,w25q128";
                reg = <0>;
                spi-max-frequency = <80000000>;
            };
        };

        i2c1@40005400 {
            compatible = "st,stm32h7-i2c";
            reg = <0x40005400 0x400>;
            clocks = <&rcc I2C1_CLK>;
            poe-pd@20 {
                compatible = "ti,tps2378";
                reg = <0x20>;
            };
        };

        uart4@40004C00 {
            compatible = "st,stm32h7-uart";
            reg = <0x40004C00 0x400>;
            clocks = <&rcc UART4_CLK>;
            current-speed = <115200>;
            ble {
                compatible = "nordic,nrf52832";
            };
        };
    };
};
```

## 6. Packet Engine Architecture

### 6.1 Rule Engine

Rules are stored as a linked list in SDRAM, loaded from SPI flash at boot.

```c
typedef struct __attribute__((packed)) packet_rule {
    uint16_t rule_id;
    uint8_t  action;       /* 0=pass, 1=modify, 2=drop, 3=capture, 4=inject */
    uint8_t  protocol;     /* 0=any, 1=ARP, 2=IP, 3=ICMP, 4=TCP, 5=UDP, 6=DNS, 7=DHCP */
    uint32_t src_ip;       /* 0=any */
    uint32_t dst_ip;       /* 0=any */
    uint16_t src_port;     /* 0=any */
    uint16_t dst_port;     /* 0=any */
    uint8_t  match_offset;  /* Offset in payload to match */
    uint8_t  match_len;    /* Length of match pattern */
    uint8_t  match_data[32]; /* Pattern to match */
    uint8_t  replace_len;   /* Length of replacement */
    uint8_t  replace_data[32]; /* Replacement data */
    struct packet_rule *next;
} packet_rule_t;
```

### 6.2 ARP Spoofing Module
- On RX: detect ARP request/reply from gateway
- Modify: change sender MAC to implant's MAC (between victim and GW)
- Inject: send ARP reply to victim with implant MAC as gateway MAC
- Store: original gateway MAC for packet forwarding

### 6.3 DNS Hijack Module
- On RX: detect DNS response to victim
- Modify: change answer section IP to attacker-controlled resolver
- Or: inject DNS response before real one arrives (race condition)

### 6.4 DHCP Rogue Module
- On RX: detect DHCP Discover/Request from victim
- Inject: send DHCP Offer/ACK with implant as gateway and DNS
- Requires: switch port configured to allow DHCP server traffic

## 7. Build Instructions

### 7.1 Toolchain

```bash
# Install ARM GCC toolchain
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi

# Install OpenOCD for flashing
sudo apt-get install openocd

# Install STM32CubeProgrammer (optional, for DFU)
# Download from https://www.st.com/en/development-tools/stm32cubeprog.html
```

### 7.2 Firmware Build

```bash
cd firmware
make clean
make -j4

# Output: build/phantombridge.bin (binary image)
#          build/phantombridge.hex (Intel HEX)
#          build/phantombridge.elf (ELF with debug symbols)
```

### 7.3 Flash via SWD

```bash
openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
    -c "program build/phantombridge.bin verify reset exit 0x08000000"
```

### 7.4 Flash via UART (Recovery Mode)

```bash
# Set BOOT0 = HIGH (SW2 closed)
# Connect USB-UART adapter to PA9 (TX) / PA10 (RX)
stm32flash -w build/phantombridge.bin -v -g 0x08000000 /dev/ttyUSB0
```

### 7.5 BLE Module Firmware

```bash
# nRF52832 programmed separately via SWD
# Uses nRF5 SDK + SoftDevice S132
cd ble-firmware
make flash_softdevice  # Flash SoftDevice first
make flash             # Flash application
```

## 8. Memory Map

| Region | Start | Size | Use |
|--------|-------|------|-----|
| ITCM Flash | 0x00200000 | 2 MB | Execute from flash (cached) |
| AXI Flash | 0x08000000 | 2 MB | Flash storage |
| DTCM SRAM | 0x20000000 | 1 MB | Data, stack, heap |
| AXI SRAM | 0x24000000 | 512 KB | DMA buffers, ETH descriptors |
| SRAM1 | 0x30000000 | 128 KB | Backup |
| SRAM2 | 0x30020000 | 128 KB | Backup |
| SDRAM | 0xC0000000 | 16 MB | Capture buffer, packet engine |
| SPI Flash | (via SPI) | 16 MB | Rules, config, logs |
| Peripherals | 0x40000000 | — | MMIO registers |
| FMC Bank 5 | 0xC0000000 | 256 MB | SDRAM mapped region |

## 9. Interrupt Priority Table

| Priority | IRQ | Source | Function |
|----------|-----|--------|----------|
| 0 (highest) | ETH | ETH MAC global | Packet RX/TX DMA complete |
| 1 | FMC | SDRAM refresh | Error handling |
| 2 | SPI1 | SPI flash DMA | Flash R/W complete |
| 3 | UART4 | BLE UART | BLE data received |
| 4 | I2C1 | PoE I2C | I2C event/error |
| 5 | TIM6 | SysTick | 1ms housekeeping |
| 6 | DMA1 | DMA stream | General DMA complete |
| 7 (lowest) | RTC | RTC wakeup | Periodic telemetry |