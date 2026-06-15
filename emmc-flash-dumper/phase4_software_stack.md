# Phase 4: Software Stack — eMMC Flash Dumper

## 4.0 Software Architecture Overview

The eMMC Flash Dumper firmware is a bare-metal C application running on the STM32H743VI Cortex-M7 at 480 MHz. It uses a cooperative multitasking architecture with interrupt-driven I/O and DMA-based data movement. The firmware is built with `arm-none-eabi-gcc` and links against a minimal subset of the STM32CubeH7 HAL for peripheral initialization, with custom drivers for all performance-critical paths.

### 4.0.1 Firmware Module Map

```
┌─────────────────────────────────────────────────────────────┐
│                        main.c                                │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌────────────┐  │
│  │ System   │  │ Task     │  │ CLI      │  │ Error      │  │
│  │ Init     │  │ Dispatch │  │ Parser   │  │ Handler    │  │
│  └──────────┘  └──────────┘  └──────────┘  └────────────┘  │
├─────────────────────────────────────────────────────────────┤
│  board.h                  registers.h                        │
│  Pin definitions          MMIO register map                  │
├─────────────────────────────────────────────────────────────┤
│  drivers/                                                   │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌────────────┐  │
│  │ emmc     │  │ nand     │  │ spinor   │  │ fpga       │  │
│  │ .c/.h    │  │ .c/.h    │  │ .c/.h    │  │ .c/.h      │  │
│  ├──────────┤  ├──────────┤  ├──────────┤  ├────────────┤  │
│  │ sdram    │  │ usb_dev  │  │ sdcard   │  │ oled       │  │
│  │ .c/.h    │  │ .c/.h    │  │ .c/.h    │  │ .c/.h      │  │
│  ├──────────┤  ├──────────┤  ├──────────┤  ├────────────┤  │
│  │ hash     │  │ power    │  │ button   │  │ led        │  │
│  │ .c/.h    │  │ .c/.h    │  │ .c/.h    │  │ .c/.h      │  │
│  └──────────┘  └──────────┘  └──────────┘  └────────────┘  │
├─────────────────────────────────────────────────────────────┤
│  usb_descriptors.h                                           │
│  USB device, configuration, BOS, string descriptors          │
└─────────────────────────────────────────────────────────────┘
```

## 4.1 Boot Strategy

### 4.1.1 Boot Sequence

```
Power-On / NRST Release
        │
        ▼
┌───────────────────────────┐
│ 1. Hardware Init (Reset Handler)                           │
│    - Set MSP from vector table                             │
│    - Enable FPU (CPACR)                                    │
│    - Disable all interrupts                                │
│    - Configure system clocks (HSE→PLL1→480 MHz)            │
│    - Enable all power rails via PMIC I2C                   │
│    - Wait for all rails stable (PMIC status poll)          │
│    - Release FPGA reset, load bitstream via SPI            │
│    - Wait for FPGA CDONE                                   │
│    - Initialize DDR3L SDRAM (FMC init sequence)            │
│    - Run memory test on first 64 KB of SDRAM               │
│    - Copy .data from flash to SRAM                         │
│    - Zero .bss                                              │
│    - Initialize ITCM/DTCM (copy critical code)             │
└───────────┬───────────────┘
            │
            ▼
┌───────────────────────────┐
│ 2. Peripheral Init (system_init)                           │
│    - Enable all peripheral clocks (AHB/APB)                │
│    - Configure NVIC priority groups                        │
│    - Initialize GPIO (board_init)                          │
│    - Initialize I2C1 (OLED)                                │
│    - Initialize OCTOSPI1 (FPGA + SPI NOR)                  │
│    - Initialize SDMMC1 (microSD)                            │
│    - Initialize SDMMC2 (eMMC)                              │
│    - Initialize USB OTG_HS (ULPI)                          │
│    - Initialize HASH (SHA-256)                              │
│    - Initialize RTC (timestamping)                         │
│    - Initialize DMA streams (MDMA + BDMA)                  │
│    - Initialize TIM2 (button debounce)                      │
│    - Initialize TIM1 (buzzer PWM)                           │
│    - Initialize WS2812B LED driver (SPI-based)             │
└───────────┬───────────────┘
            │
            ▼
┌───────────────────────────┐
│ 3. Self-Test (built-in test)                               │
│    - SDRAM full test (walking 1s, address test)            │
│    - FPGA communication test (ID register read)            │
│    - OLED display test (show splash)                       │
│    - microSD detect + test read                            │
│    - USB PHY link test (ULPI clock detect)                 │
│    - Battery level read (PMIC ADC)                        │
│    - Button test (all released)                            │
│    - LED test (RGB cycle)                                  │
│    - Buzzer test (short beep)                              │
└───────────┬───────────────┘
            │
            ▼
┌───────────────────────────┐
│ 4. Runtime Init                                            │
│    - Mount microSD (FatFS)                                 │
│    - Initialize USB device stack                           │
│    - Show main menu on OLED                                │
│    - Enter main loop                                       │
└───────────────────────────┘
```

### 4.1.2 Memory Layout

```
┌─────────────────────────────────────────────────────────────┐
│ Flash (2 MB, 0x08000000)                                     │
│ ┌──────────────────────────────────────────────────────────┐ │
│ │ Sector 0 (128 KB): Bootloader + Vector Table             │ │
│ │ Sector 1 (128 KB): Firmware .text + .rodata              │ │
│ │ Sector 2 (128 KB): FPGA bitstream (golden image)         │ │
│ │ Sector 3 (128 KB): FPGA bitstream (backup)              │ │
│ │ Sectors 4-7 (512 KB): Reserved / future                  │ │
│ │ Sectors 8-15 (1 MB): File system (LittleFS)              │ │
│ └──────────────────────────────────────────────────────────┘ │
├─────────────────────────────────────────────────────────────┤
│ ITCM RAM (64 KB, 0x00000000)                                 │
│ ┌──────────────────────────────────────────────────────────┐ │
│ │ Critical interrupt handlers, hot code paths              │ │
│ └──────────────────────────────────────────────────────────┘ │
├─────────────────────────────────────────────────────────────┤
│ DTCM RAM (128 KB, 0x20000000)                                │
│ ┌──────────────────────────────────────────────────────────┐ │
│ │ System stack (32 KB), critical data structures           │ │
│ │ DMA descriptors, interrupt contexts                     │ │
│ └──────────────────────────────────────────────────────────┘ │
├─────────────────────────────────────────────────────────────┤
│ AXI SRAM (512 KB, 0x24000000)                                │
│ ┌──────────────────────────────────────────────────────────┐ │
│ │ USB endpoint buffers (64 KB)                             │ │
│ │ File system buffers (64 KB)                              │ │
│ │ General heap (384 KB)                                    │ │
│ └──────────────────────────────────────────────────────────┘ │
├─────────────────────────────────────────────────────────────┤
│ FMC SDRAM Bank 1 (256 MB, 0xC0000000)                        │
│ ┌──────────────────────────────────────────────────────────┐ │
│ │ Acquisition ring buffer (256 MB)                         │ │
│ │ Organized as 4096 × 64 KB blocks                        │ │
│ │ Write pointer: acquisition DMA                           │ │
│ │ Read pointer: USB / microSD DMA                          │ │
│ └──────────────────────────────────────────────────────────┘ │
├─────────────────────────────────────────────────────────────┤
│ FMC SDRAM Bank 2 (256 MB, 0xD0000000)                        │
│ ┌──────────────────────────────────────────────────────────┐ │
│ │ Secondary buffer / scratch space                         │ │
│ │ Hash computation workspace                               │ │
│ │ NAND page reconstruction buffer                          │ │
│ └──────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### 4.1.3 Linker Script Key Sections

```ld
MEMORY
{
    ITCM_RAM   (rwx) : ORIGIN = 0x00000000, LENGTH = 64K
    DTCM_RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
    AXI_SRAM   (rwx) : ORIGIN = 0x24000000, LENGTH = 512K
    SRAM1      (rwx) : ORIGIN = 0x30000000, LENGTH = 128K
    SRAM2      (rwx) : ORIGIN = 0x30020000, LENGTH = 128K
    SRAM3      (rwx) : ORIGIN = 0x30040000, LENGTH = 32K
    SRAM4      (rwx) : ORIGIN = 0x38000000, LENGTH = 64K
    SDRAM_BANK1(rwx) : ORIGIN = 0xC0000000, LENGTH = 256M
    SDRAM_BANK2(rwx) : ORIGIN = 0xD0000000, LENGTH = 256M
    FLASH      (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
}

SECTIONS
{
    .vectors : { KEEP(*(.vectors)) } > FLASH
    .itcm_text : { *(.itcm_text*) } > ITCM_RAM AT> FLASH
    .text : { *(.text*) } > FLASH
    .rodata : { *(.rodata*) } > FLASH
    .data : { *(.data*) } > DTCM_RAM AT> FLASH
    .bss : { *(.bss*) } > DTCM_RAM
    .axiram : { *(.axiram*) } > AXI_SRAM
    .sdram_buf : { *(.sdram_buf*) } > SDRAM_BANK1
}
```

## 4.2 MMIO Register Map

### 4.2.1 STM32H743 Peripheral Base Addresses

| Peripheral | Base Address | Bus |
|------------|-------------|-----|
| RCC | 0x52024400 | AHB3 |
| GPIOA | 0x52020800 | AHB4 |
| GPIOB | 0x52020400 | AHB4 |
| GPIOC | 0x52020800 | AHB4 |
| GPIOD | 0x52020C00 | AHB4 |
| GPIOE | 0x52021000 | AHB4 |
| GPIOF | 0x52021400 | AHB4 |
| GPIOG | 0x52021800 | AHB4 |
| GPIOH | 0x52021C00 | AHB4 |
| GPIOI | 0x52022000 | AHB4 |
| FMC | 0x52004000 | AHB3 |
| OCTOSPI1 | 0x52005000 | AHB3 |
| SDMMC1 | 0x52007000 | AHB3 |
| SDMMC2 | 0x52007400 | AHB3 |
| USB_OTG_HS | 0x50040000 | AHB1 |
| HASH | 0x50060400 | AHB2 |
| DMA1 | 0x50020000 | AHB1 |
| DMA2 | 0x50020400 | AHB1 |
| MDMA | 0x52020000 | AHB3 |
| BDMA | 0x52025400 | AHB3 |
| I2C1 | 0x50005400 | APB1 |
| SPI1 | 0x50013000 | APB2 |
| TIM1 | 0x50010000 | APB2 |
| TIM2 | 0x50000000 | APB1 |
| RTC | 0x52002800 | APB4 |
| NVIC | 0xE000E100 | System |
| SCB | 0xE000ED00 | System |
| FPU_CPACR | 0xE000ED88 | System |
| SYSTICK | 0xE000E010 | System |

### 4.2.2 RCC Register Map (Critical Registers)

| Register | Offset | Description |
|----------|--------|-------------|
| RCC_CR | 0x00 | Clock control: HSEON, HSERDY, PLLON, PLLRDY |
| RCC_CFGR | 0x10 | Clock config: SW, SWS, HPRE, PPRE |
| RCC_D1CFGR | 0x18 | Domain 1 config: D1CPRE, D1PPRE |
| RCC_D2CFGR | 0x1C | Domain 2 config: D2PPRE1, D2PPRE2 |
| RCC_D3CFGR | 0x20 | Domain 3 config: D3PPRE |
| RCC_PLLCKSELR | 0x28 | PLL source: PLLSRC, DIVM |
| RCC_PLL1DIVR | 0x30 | PLL1 dividers: DIVN, DIVP, DIVQ, DIVR |
| RCC_PLL2DIVR | 0x38 | PLL2 dividers |
| RCC_PLL3DIVR | 0x40 | PLL3 dividers |
| RCC_AHB3ENR | 0xD4 | AHB3 peripheral clock enable |
| RCC_AHB1ENR | 0xD8 | AHB1 peripheral clock enable |
| RCC_AHB2ENR | 0xDC | AHB2 peripheral clock enable |
| RCC_AHB4ENR | 0xE0 | AHB4 peripheral clock enable |
| RCC_APB1LENR | 0xE4 | APB1 low peripheral clock enable |
| RCC_APB1HENR | 0xE8 | APB1 high peripheral clock enable |
| RCC_APB2ENR | 0xEC | APB2 peripheral clock enable |
| RCC_APB3ENR | 0xF0 | APB3 peripheral clock enable |
| RCC_APB4ENR | 0xF4 | APB4 peripheral clock enable |

### 4.2.3 FMC Register Map (SDRAM + NAND)

| Register | Offset | Description |
|----------|--------|-------------|
| FMC_SDCR1 | 0x140 | SDRAM Control Register 1 (Bank 1) |
| FMC_SDCR2 | 0x144 | SDRAM Control Register 2 (Bank 2) |
| FMC_SDTR1 | 0x148 | SDRAM Timing Register 1 |
| FMC_SDTR2 | 0x14C | SDRAM Timing Register 2 |
| FMC_SDCMR | 0x150 | SDRAM Command Mode Register |
| FMC_SDRTR | 0x154 | SDRAM Refresh Timer Register |
| FMC_SDSR | 0x158 | SDRAM Status Register |
| FMC_PCR | 0x080 | NAND Flash Control Register (common space) |
| FMC_PMEM | 0x088 | NAND Flash Common Memory Timing |
| FMC_PATT | 0x08C | NAND Flash Attribute Memory Timing |
| FMC_ECCR | 0x094 | NAND Flash ECC Result Register |

### 4.2.4 SDMMC2 Register Map (eMMC)

| Register | Offset | Description |
|----------|--------|-------------|
| SDMMC_POWER | 0x00 | Power control |
| SDMMC_CLKCR | 0x04 | Clock control: CLKDIV, CLKEN, PWRSAV, WIDBUS, NEGEDGE, HWFC_EN |
| SDMMC_ARG | 0x08 | Command argument |
| SDMMC_CMD | 0x0C | Command: CMDINDEX, WAITRESP, WAITINT, CPSMEN |
| SDMMC_RESP1 | 0x14 | Response register 1 |
| SDMMC_RESP2 | 0x18 | Response register 2 |
| SDMMC_RESP3 | 0x1C | Response register 3 |
| SDMMC_RESP4 | 0x20 | Response register 4 |
| SDMMC_DTIMER | 0x24 | Data timeout |
| SDMMC_DLEN | 0x28 | Data length (bytes) |
| SDMMC_DCTRL | 0x2C | Data control: DTEN, DTDIR, DTMODE, DBLOCKSIZE |
| SDMMC_DCOUNT | 0x30 | Data counter (remaining) |
| SDMMC_STA | 0x34 | Status: CMDACT, TXACT, RXACT, DCRCFAIL, CTIMEOUT, etc. |
| SDMMC_ICR | 0x38 | Interrupt clear |
| SDMMC_MASK | 0x3C | Interrupt mask |
| SDMMC_FIFOCNT | 0x48 | FIFO counter |
| SDMMC_FIFO | 0x80 | FIFO data (32-bit × 32 words) |

### 4.2.5 OCTOSPI1 Register Map

| Register | Offset | Description |
|----------|--------|-------------|
| OCTOSPI_CR | 0x00 | Control: EN, ABORT, DMAEN, TCEN, FTHRES |
| OCTOSPI_DCR1 | 0x08 | Device config 1: MTYP, DEVSIZE, CSHT |
| OCTOSPI_DCR2 | 0x0C | Device config 2: WRAPSIZE, PRESCALER |
| OCTOSPI_DCR3 | 0x10 | Device config 3: CSBOUND, MAXTRAN |
| OCTOSPI_DLR | 0x20 | Data length |
| OCTOSPI_AR | 0x30 | Address |
| OCTOSPI_CCR | 0x40 | Communication config: IMODE, ADMODE, ABMODE, DMODE, FMODE |
| OCTOSPI_SR | 0x50 | Status: TCF, SMF, TOF, BUSY |
| OCTOSPI_FCR | 0x54 | Flag clear |
| OCTOSPI_DR | 0x60 | Data register (FIFO) |
| OCTOSPI_PSMKR | 0x80 | Polling status mask |
| OCTOSPI_PSMAR | 0x84 | Polling status match |
| OCTOSPI_PIR | 0x88 | Polling interval |
| OCTOSPI_WPCCR | 0x90 | Wrap communication config |
| OCTOSPI_WPSR | 0x94 | Wrap status |
| OCTOSPI_WPTCR | 0x98 | Wrap timeout counter |
| OCTOSPI_WPABR | 0x9C | Wrap alternate bytes |

### 4.2.6 USB OTG_HS Register Map

| Register | Offset | Description |
|----------|--------|-------------|
| OTG_HS_GOTGCTL | 0x000 | OTG control and status |
| OTG_HS_GOTGINT | 0x004 | OTG interrupt |
| OTG_HS_GAHBCFG | 0x008 | AHB config: GINT, DMAEN, TXFELVL, PTXFELVL |
| OTG_HS_GUSBCFG | 0x00C | USB config: TOCAL, PHYSEL, ULPI_UTMI_SEL, ULPI_CLK_SUSP |
| OTG_HS_GRSTCTL | 0x010 | Reset: CSRST, TXFFLSH, RXFFLSH |
| OTG_HS_GINTSTS | 0x014 | Interrupt status |
| OTG_HS_GINTMSK | 0x018 | Interrupt mask |
| OTG_HS_GRXSTSR | 0x01C | Receive status debug read |
| OTG_HS_GRXSTSP | 0x020 | Receive status debug pop |
| OTG_HS_GRXFSIZ | 0x024 | Receive FIFO size |
| OTG_HS_DIEPTXF0 | 0x028 | EP0 transmit FIFO size |
| OTG_HS_HNPTXSTS | 0x02C | Non-periodic Tx FIFO status |
| OTG_HS_GCCFG | 0x038 | General core config |
| OTG_HS_CID | 0x03C | Core ID |
| OTG_HS_DCFG | 0x800 | Device config: DSPD, NZLSOHSK, DAD |
| OTG_HS_DCTL | 0x804 | Device control: RWUSIG, SDIS, GINSTS, GONSTS |
| OTG_HS_DSTS | 0x808 | Device status: SUSPSTS, ENUMSPD, EERR, FNSOF |
| OTG_HS_DIEPMSK | 0x810 | Device IN EP common mask |
| OTG_HS_DOEPMSK | 0x814 | Device OUT EP common mask |
| OTG_HS_DAINT | 0x818 | Device all endpoints interrupt |
| OTG_HS_DAINTMSK | 0x81C | Device all endpoints interrupt mask |
| OTG_HS_DIEPCTL0 | 0x900 | Device IN EP0 control |
| OTG_HS_DIEPCTL1 | 0x920 | Device IN EP1 control (BULK IN) |
| OTG_HS_DOEPCTL0 | 0xB00 | Device OUT EP0 control |
| OTG_HS_DOEPCTL1 | 0xB20 | Device OUT EP1 control (BULK OUT) |
| OTG_HS_DIEPTSIZ1 | 0x924 | Device IN EP1 transfer size |
| OTG_HS_DOEPTSIZ1 | 0xB24 | Device OUT EP1 transfer size |
| OTG_HS_DIEPDMA1 | 0x928 | Device IN EP1 DMA address |
| OTG_HS_DOEPDMA1 | 0xB28 | Device OUT EP1 DMA address |

### 4.2.7 HASH Register Map

| Register | Offset | Description |
|----------|--------|-------------|
| HASH_CR | 0x00 | Control: INIT, MODE, ALGO, DATATYPE, DMAE |
| HASH_DIN | 0x04 | Data input (32-bit) |
| HASH_STR | 0x08 | Start |
| HASH_HR0-HASH_HR7 | 0x10-0x2C | Digest registers (256-bit SHA-256) |
| HASH_IMR | 0x30 | Interrupt mask |
| HASH_SR | 0x34 | Status: BUSY, DMAS, DCIS, DINIS |
| HASH_CSR0-HASH_CSR53 | 0xF8-0x1CC | Context swap registers |

### 4.2.8 MDMA Register Map

| Register | Offset | Description |
|----------|--------|-------------|
| MDMA_GISR0 | 0x000 | Global interrupt status |
| MDMA_C0CR | 0x040 | Channel 0 control: EN, TEIE, CTCIE, BRTIE, PRIO, BURST, DIR |
| MDMA_C0TCR | 0x044 | Channel 0 transfer config: SINC, DINC, SWAP, TRG |
| MDMA_C0BNDTR | 0x048 | Channel 0 block number/data: BNDT, BRC |
| MDMA_C0SAR | 0x04C | Channel 0 source address |
| MDMA_C0DAR | 0x050 | Channel 0 destination address |
| MDMA_C0BRUR | 0x054 | Channel 0 block repeat address update |
| MDMA_C0LAR | 0x058 | Channel 0 link address |
| MDMA_C0MDR | 0x070 | Channel 0 mask/data: MASK, MDATA |

## 4.3 Clock & GPIO Initialization

### 4.3.1 System Clock Configuration (480 MHz)

```c
/* PLL1: 25 MHz HSE / 5 * 192 / 2 = 480 MHz */
#define PLLM    5       /* DIVM: 25 MHz / 5 = 5 MHz VCO input */
#define PLLN    192     /* DIVN: 5 MHz * 192 = 960 MHz VCO */
#define PLLP    2       /* DIVP: 960 MHz / 2 = 480 MHz SYSCLK */
#define PLLQ    4       /* DIVQ: 960 MHz / 4 = 240 MHz (FMC, QSPI) */
#define PLLR    8       /* DIVR: 960 MHz / 8 = 120 MHz (unused) */

/* Bus prescalers */
#define AHB_PRE    1    /* HPRE: /1 = 240 MHz */
#define APB1_PRE   4    /* D2PPRE1: /4 = 60 MHz */
#define APB2_PRE   2    /* D2PPRE2: /2 = 120 MHz */
#define APB3_PRE   2    /* D1PPRE: /2 = 120 MHz */
#define APB4_PRE   2    /* D3PPRE: /2 = 120 MHz */

/* PLL2: 25 MHz / 5 * 160 / 2 = 400 MHz (SDMMC1, OCTOSPI) */
#define PLL2M   5
#define PLL2N   160
#define PLL2P   2

/* PLL3: 25 MHz / 5 * 120 / 10 = 60 MHz (USB ULPI) */
#define PLL3M   5
#define PLL3N   120
#define PLL3R   10
```

### 4.3.2 Clock Initialization Sequence

```c
void system_clock_init(void) {
    /* 1. Enable HSE */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    /* 2. Configure PLL1 for 480 MHz */
    RCC->PLLCKSELR = (PLLM << RCC_PLLCKSELR_DIVM1_Pos) |
                     RCC_PLLCKSELR_PLLSRC_HSE;
    RCC->PLL1DIVR = (PLLN << RCC_PLL1DIVR_DIVN1_Pos) |
                    ((PLLP-1) << RCC_PLL1DIVR_DIVP1_Pos) |
                    ((PLLQ-1) << RCC_PLL1DIVR_DIVQ1_Pos) |
                    ((PLLR-1) << RCC_PLL1DIVR_DIVR1_Pos);
    RCC->CR |= RCC_CR_PLL1ON;
    while (!(RCC->CR & RCC_CR_PLL1RDY));

    /* 3. Configure bus prescalers */
    RCC->D1CFGR = RCC_D1CFGR_D1CPRE_1 | RCC_D1CFGR_D1PPRE_1;  /* /2 */
    RCC->D2CFGR = RCC_D2CFGR_D2PPRE1_3 | RCC_D2CFGR_D2PPRE2_1; /* /4, /2 */
    RCC->D3CFGR = RCC_D3CFGR_D3PPRE_1;  /* /2 */

    /* 4. Configure PLL2 for 400 MHz */
    RCC->PLLCKSELR |= (PLL2M << RCC_PLLCKSELR_DIVM2_Pos);
    RCC->PLL2DIVR = (PLL2N << RCC_PLL2DIVR_DIVN2_Pos) |
                    ((PLL2P-1) << RCC_PLL2DIVR_DIVP2_Pos);
    RCC->CR |= RCC_CR_PLL2ON;
    while (!(RCC->CR & RCC_CR_PLL2RDY));

    /* 5. Configure PLL3 for 60 MHz */
    RCC->PLLCKSELR |= (PLL3M << RCC_PLLCKSELR_DIVM3_Pos);
    RCC->PLL3DIVR = (PLL3N << RCC_PLL3DIVR_DIVN3_Pos) |
                    ((PLL3R-1) << RCC_PLL3DIVR_DIVR3_Pos);
    RCC->CR |= RCC_CR_PLL3ON;
    while (!(RCC->CR & RCC_CR_PLL3RDY));

    /* 6. Select PLL1 as system clock */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL1;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL1);

    /* 7. Enable overdrive mode for 480 MHz */
    PWR->CR3 |= PWR_CR3_SCUEN;
    PWR->D3CR |= PWR_D3CR_VOS_1 | PWR_D3CR_VOS_0;  /* VOS1 */
    while (!(PWR->D3CR & PWR_D3CR_VOSRDY));
}
```

### 4.3.3 GPIO Initialization (board_init)

```c
void board_init(void) {
    /* Enable GPIO clocks */
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN |
                    RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN |
                    RCC_AHB4ENR_GPIOEEN | RCC_AHB4ENR_GPIOFEN |
                    RCC_AHB4ENR_GPIOGEN | RCC_AHB4ENR_GPIOHEN |
                    RCC_AHB4ENR_GPIOIEN;

    /* Configure buttons (PA0-PA3) as inputs with pull-up */
    GPIOA->MODER &= ~(0xFF << 0);  /* Input mode */
    GPIOA->PUPDR |= (0x55 << 0);   /* Pull-up */

    /* Configure FPGA control pins */
    GPIOB->MODER |= (1 << 24);  /* PB12: FPGA_CRESET, output */
    GPIOB->MODER |= (1 << 26);  /* PB13: FPGA_CDONE, input */
    GPIOG->MODER &= ~(3 << 12); /* PG6: FPGA_INTR, input */

    /* Configure LEDs (PB14, PB15) as outputs */
    GPIOB->MODER |= (1 << 28) | (1 << 30);

    /* Configure buzzer (PE2) as AF (TIM1) */
    GPIOE->MODER |= (2 << 4);
    GPIOE->AFR[0] |= (1 << 8);  /* AF1 = TIM1 */

    /* Configure I2C1 (PB0, PB1) as AF */
    GPIOB->MODER |= (2 << 0) | (2 << 2);
    GPIOB->AFR[0] |= (4 << 0) | (4 << 4);  /* AF4 = I2C1 */
    GPIOB->OTYPER |= (1 << 0) | (1 << 1);  /* Open-drain */
    GPIOB->PUPDR |= (1 << 0) | (1 << 1);   /* Pull-up */

    /* Configure OCTOSPI1 (PA8-PA12, PB2) as AF */
    GPIOA->MODER |= (2 << 16) | (2 << 18) | (2 << 20) | (2 << 22) | (2 << 24);
    GPIOA->AFR[1] |= (10 << 0) | (10 << 4) | (10 << 8) | (10 << 12) | (10 << 16);
    GPIOB->MODER |= (2 << 4);
    GPIOB->AFR[0] |= (10 << 8);

    /* Configure SPI1 (PA4-PA7) as AF for FPGA config */
    GPIOA->MODER |= (2 << 8) | (2 << 10) | (2 << 12) | (2 << 14);
    GPIOA->AFR[0] |= (5 << 16) | (5 << 20) | (5 << 24) | (5 << 28);

    /* Configure ULPI (PB5-PB15, PC0) as AF */
    GPIOB->MODER |= (2 << 10) | (2 << 12) | (2 << 14) | (2 << 16) |
                    (2 << 18) | (2 << 20) | (2 << 22) | (2 << 24) |
                    (2 << 26) | (2 << 28) | (2 << 30);
    GPIOB->AFR[1] |= (10 << 20) | (10 << 24) | (10 << 28);  /* PB13-PB15 */
    GPIOB->AFR[0] |= (10 << 20) | (10 << 24) | (10 << 28);  /* PB5-PB7 */
    /* PB8-PB12: AF10 for ULPI */
    GPIOC->MODER |= (2 << 0);
    GPIOC->AFR[0] |= (10 << 0);

    /* Configure SDMMC2 (eMMC) pins */
    GPIOC->MODER |= (2 << 12) | (2 << 14) | (2 << 16) | (2 << 18) |
                    (2 << 20) | (2 << 22) | (2 << 24);
    GPIOC->AFR[1] |= (12 << 16) | (12 << 20) | (12 << 24) | (12 << 28);
    GPIOC->AFR[0] |= (12 << 0) | (12 << 4) | (12 << 8);
    GPIOD->MODER |= (2 << 4) | (2 << 6) | (2 << 8);
    GPIOD->AFR[0] |= (12 << 16) | (12 << 20) | (12 << 24);

    /* Configure SDMMC1 (microSD) pins */
    GPIOD->MODER |= (2 << 16) | (2 << 18) | (2 << 20) | (2 << 22) | (2 << 24);
    GPIOD->AFR[1] |= (12 << 0) | (12 << 4) | (12 << 8) | (12 << 12) | (12 << 16);

    /* Configure FMC SDRAM pins — done in sdram_init() */
}
```

## 4.4 Device Drivers

### 4.4.1 eMMC Driver (emmc.c / emmc.h)

The eMMC driver implements the full JESD84-B51 eMMC 5.1 protocol over the SDMMC2 peripheral.

**Key Features:**
- eMMC initialization sequence (CMD0 → CMD1 → CMD2 → CMD3 → CMD9 → CMD7 → CMD8 → CMD6)
- HS400 mode negotiation (HS_TIMING = 0x03, BUS_WIDTH = 0x06 (8-bit DDR))
- DMA-based block read (CMD18: multiple block read)
- Reliable write for RPMB access (CMD23 + CMD25)
- EXT_CSD register parsing (512 bytes of device configuration)
- Boot partition access (boot0, boot1 via CMD6 SWITCH)
- RPMB authenticated read (if key provided)
- Automatic retry on CRC errors (up to 3 retries)
- Card detection and voltage switching (1.8V signaling)

**eMMC Initialization Sequence:**

```
1. CMD0 (GO_IDLE_STATE) — Reset eMMC to idle
2. CMD1 (SEND_OP_COND) — Initiate OCR polling, wait for !busy
3. CMD2 (ALL_SEND_CID) — Get 128-bit CID
4. CMD3 (SET_RELATIVE_ADDR) — Assign RCA (0x0001)
5. CMD9 (SEND_CSD) — Get CSD register
6. CMD7 (SELECT_CARD) — Select card by RCA
7. CMD8 (SEND_EXT_CSD) — Read 512-byte EXT_CSD
8. Parse EXT_CSD[196] CMD_SET_REV (must be ≥ 1.5 for HS400)
9. CMD6 (SWITCH) — Set HS_TIMING to 0x01 (High Speed)
10. Change SDMMC2 clock to 52 MHz
11. CMD6 (SWITCH) — Set BUS_WIDTH to 0x02 (8-bit SDR)
12. CMD6 (SWITCH) — Set HS_TIMING to 0x03 (HS400)
13. CMD6 (SWITCH) — Set BUS_WIDTH to 0x06 (8-bit DDR)
14. Change SDMMC2 clock to 200 MHz DDR
15. Enable hardware flow control (HWFC_EN)
16. eMMC ready for HS400 data transfer
```

**DMA Block Read (CMD18):**

```c
int emmc_read_blocks_dma(uint32_t start_block, uint32_t block_count,
                          uint8_t *buffer) {
    sdmmc2_wait_ready();
    sdmmc2_set_block_size(512);
    sdmmc2_set_block_count(block_count);

    /* Configure MDMA: SDMMC2 FIFO → SDRAM */
    MDMA_Channel0->CSAR = (uint32_t)&SDMMC2->FIFO;
    MDMA_Channel0->CDAR = (uint32_t)buffer;
    MDMA_Channel0->CBNDTR = block_count * 512;
    MDMA_Channel0->CCR = MDMA_CCR_EN | MDMA_CCR_TCIE | MDMA_CCR_BRTIE |
                          (3 << MDMA_CCR_PRIO_Pos) | MDMA_CCR_DIR_PER2MEM;

    /* Send CMD18 (READ_MULTIPLE_BLOCK) */
    sdmmc2_send_cmd(18, start_block, SDMMC_RESP_R1, true);

    /* Wait for DMA completion */
    while (!(MDMA->GISR0 & MDMA_GISR0_TCIF0));
    MDMA->GISR0 = MDMA_GISR0_TCIF0;  /* Clear flag */

    /* Send CMD12 (STOP_TRANSMISSION) */
    sdmmc2_send_cmd(12, 0, SDMMC_RESP_R1b, false);

    return 0;
}
```

### 4.4.2 NAND Flash Driver (nand.c / nand.h)

The NAND driver interfaces with the FPGA-based timing controller via the FMC peripheral in asynchronous mode.

**Key Features:**
- ONFI 4.0 parameter page detection and parsing
- Automatic timing configuration via FPGA registers
- Raw page read (data + OOB/spare area)
- Block erase, page program (for recovery operations)
- Bad block table management
- Multi-plane read support
- ECC bypass (raw data capture for forensic imaging)

**NAND Read Page Sequence:**

```
1. Configure FPGA timing registers from ONFI parameters
2. Write 0x00 (READ PAGE) command via FPGA NAND_CMD register
3. Write 5-byte column + row address via FPGA NAND_ADDR register
4. Write 0x30 (READ PAGE CONFIRM) command
5. Wait for R/B# to go high (FPGA monitors, signals via INTR)
6. Read data bytes from FPGA FIFO (FMC reads FPGA NAND_DATA_IN)
7. FPGA captures DQ[0:7] on each RE# pulse, fills internal FIFO
8. FMC reads FIFO via NAND_DATA_IN register, DMA to SDRAM
9. Page size: typically 4096+224 bytes (data + OOB) or 8192+448
```

**ONFI Parameter Page Parsing:**

```c
struct onfi_params {
    uint32_t signature;         /* "ONFI" at offset 0 */
    uint16_t revision;          /* 0x0400 = ONFI 4.0 */
    uint16_t features;
    uint16_t opt_commands;
    uint8_t  manufacturer[12];
    uint8_t  model[20];
    uint8_t  jedec_id;
    uint16_t date_code;
    uint8_t  pad1[13];
    uint32_t data_bytes_per_page;
    uint16_t spare_bytes_per_page;
    uint32_t pages_per_block;
    uint32_t blocks_per_lun;
    uint8_t  luns_per_target;
    uint8_t  addr_cycles;       /* 4 or 5 */
    uint8_t  bits_per_cell;     /* 1=SLC, 2=MLC, 3=TLC */
    uint16_t max_bad_blocks_per_lun;
    uint16_t block_endurance;
    uint8_t  guaranteed_valid_blocks;
    uint16_t block_endurance_guaranteed;
    uint8_t  programs_per_page;
    uint8_t  partial_programming;
    uint8_t  bits_ecc_correctability;
    uint8_t  interleaved_addr;
    uint8_t  interleaved_oper;
    uint8_t  pad2[13];
    uint8_t  io_pin_capacitance;
    uint16_t timing_mode;       /* 0-5 for ONFI 4.0 */
    uint16_t tPROG_max;
    uint16_t tBERS_max;
    uint16_t tR_max;            /* Max page read time (µs) */
    uint16_t tCCS_min;
    uint8_t  pad3[23];
    uint16_t vendor_revision;
    uint8_t  vendor_specific[88];
    uint16_t crc;               /* CRC-16 over bytes 0-253 */
};
```

### 4.4.3 SPI NOR Driver (spinor.c / spinor.h)

The SPI NOR driver uses the OCTOSPI1 peripheral in quad SPI mode for maximum throughput.

**Key Features:**
- JEDEC SFDP (Serial Flash Discoverable Parameters) parsing
- Quad I/O fast read (0xEB command) at 100 MHz
- Memory-mapped mode for direct read access
- Sector/block erase
- Write enable/disable management
- Status register polling

**Quad I/O Fast Read Sequence:**

```
1. Read JEDEC ID (0x9F): manufacturer + memory type + capacity
2. Parse SFDP table at address 0x000000
3. Configure OCTOSPI1 for quad mode:
   - IMODE = 0x01 (single line)
   - ADMODE = 0x03 (quad line)
   - ABMODE = 0x03 (quad line, 2 bytes = 8 dummy cycles)
   - DMODE = 0x03 (quad line)
   - FMODE = 0x00 (indirect write)
4. Send 0xEB (Quad I/O Read), 3-byte address, 8 dummy cycles
5. Read data via OCTOSPI FIFO, DMA to SDRAM
6. Or: enable memory-mapped mode for direct pointer access
```

### 4.4.4 FPGA Driver (fpga.c / fpga.h)

The FPGA driver manages the iCE40UP5K configuration and runtime communication.

**Key Features:**
- SPI bitstream loading (slave configuration mode)
- CRESET/CDONE handshake
- Register read/write via SPI
- NAND timing configuration
- Interrupt handling (R/B# transitions, FIFO thresholds)
- Version/ID verification

**FPGA Bitstream Loading:**

```c
int fpga_load_bitstream(const uint8_t *bitstream, uint32_t length) {
    /* 1. Assert CRESET (active low) */
    gpio_write(FPGA_CRESET_PORT, FPGA_CRESET_PIN, 0);
    delay_ms(1);

    /* 2. Release CRESET — FPGA enters config mode */
    gpio_write(FPGA_CRESET_PORT, FPGA_CRESET_PIN, 1);
    delay_us(200);  /* Wait for FPGA to be ready */

    /* 3. Send bitstream via SPI1 at 20 MHz */
    spi1_select_slave();  /* CS# low */
    spi1_transmit_dma(bitstream, length);
    spi1_deselect_slave();  /* CS# high */

    /* 4. Send 49+ dummy clocks for FPGA to finish config */
    spi1_send_clocks(100);

    /* 5. Wait for CDONE to go high */
    uint32_t timeout = 100000;
    while (!gpio_read(FPGA_CDONE_PORT, FPGA_CDONE_PIN) && timeout--);
    if (timeout == 0) return -1;

    /* 6. Verify FPGA ID */
    uint32_t id = fpga_read_reg(FPGA_REG_ID);
    if (id != FPGA_EXPECTED_ID) return -2;

    return 0;
}
```

### 4.4.5 SDRAM Driver (sdram.c / sdram.h)

The SDRAM driver initializes the DDR3L memory and provides a ring buffer management API.

**Key Features:**
- FMC SDRAM initialization sequence (precharge, auto-refresh, mode register load)
- DDR3L timing configuration (tRCD, tRP, tRAS, tRC, tWR, tRFC)
- Memory test (walking 1s, address uniqueness, data bus integrity)
- Ring buffer management (write pointer, read pointer, wrap detection)
- DMA buffer allocation

**DDR3L Initialization Sequence:**

```c
int sdram_init(void) {
    /* 1. Configure FMC GPIO pins (AF12 for all SDRAM pins) */
    /* ... GPIO config ... */

    /* 2. Configure FMC SDCR1 for MT41K256M16TW */
    FMC->SDCR1 = FMC_SDCR1_NC_1 |      /* 9 column bits */
                 FMC_SDCR1_NR_2 | FMC_SDCR1_NR_1 |  /* 13 row bits */
                 FMC_SDCR1_MWID_1 |     /* 16-bit data */
                 FMC_SDCR1_NB |         /* 1 bank */
                 FMC_SDCR1_CAS_2 |      /* CAS latency 3 */
                 FMC_SDCR1_SDCLK_1 | FMC_SDCR1_SDCLK_0 |  /* 2× HCLK */
                 FMC_SDCR1_RBURST |     /* Read burst */
                 FMC_SDCR1_RPIPE_1;     /* 2 HCLK pipe delay */

    /* 3. Configure FMC SDTR1 timing */
    FMC->SDTR1 = (2 << FMC_SDTR1_TMRD_Pos) |   /* tMRD = 4 cycles */
                 (6 << FMC_SDTR1_TXSR_Pos) |   /* tXSR = 6 cycles */
                 (2 << FMC_SDTR1_TRAS_Pos) |   /* tRAS = 2 cycles */
                 (2 << FMC_SDTR1_TRC_Pos) |    /* tRC = 2 cycles */
                 (2 << FMC_SDTR1_TWR_Pos) |    /* tWR = 2 cycles */
                 (2 << FMC_SDTR1_TRP_Pos) |    /* tRP = 2 cycles */
                 (2 << FMC_SDTR1_TRCD_Pos);    /* tRCD = 2 cycles */

    /* 4. Configure FMC SDTR2 timing */
    FMC->SDTR2 = (7 << FMC_SDTR2_TRAS_Pos) |   /* tRAS min = 7 */
                 (7 << FMC_SDTR2_TRC_Pos) |    /* tRC = 7 */
                 (4 << FMC_SDTR2_TWR_Pos) |    /* tWR = 4 */
                 (2 << FMC_SDTR2_TRP_Pos) |    /* tRP = 2 */
                 (2 << FMC_SDTR2_TRCD_Pos);    /* tRCD = 2 */

    /* 5. Configure refresh timer */
    FMC->SDRTR = (1386 << FMC_SDRTR_COUNT_Pos);  /* 7.8µs * 240MHz / 2 */

    /* 6. Issue SDRAM init sequence */
    FMC->SDCMR = FMC_SDCMR_CTB1 | FMC_SDCMR_MODE_001;  /* Precharge all */
    delay_us(100);

    FMC->SDCMR = FMC_SDCMR_CTB1 | FMC_SDCMR_MODE_010 | (2 << 5);  /* Auto-refresh ×2 */
    delay_us(100);

    FMC->SDCMR = FMC_SDCMR_CTB1 | FMC_SDCMR_MODE_011 |  /* Load mode register */
                 (0x0 << 9) |  /* MR0: CL=6, WR=5, DLL reset */
                 (0x0 << 17);  /* MR1: DIC=0, ODT=RZQ/4 */
    delay_us(100);

    /* 7. Enable auto-refresh */
    FMC->SDCR1 |= FMC_SDCR1_RA;

    return 0;
}
```

### 4.4.6 USB Device Driver (usb_device.c / usb_device.h)

The USB driver implements a USB 3.0 SuperSpeed bulk device with two endpoints for high-speed data transfer.

**Key Features:**
- USB 3.0 enumeration (BOS descriptor, SuperSpeed capability)
- ULPI PHY initialization (USB3320C)
- Bulk IN endpoint (EP1 IN, 512-byte max packet, burst up to 16)
- Bulk OUT endpoint (EP1 OUT, 512-byte max packet)
- DMA-based transfer from SDRAM to USB FIFO
- Stream protocol framing (see protocol.js)

### 4.4.7 microSD Driver (sdcard.c / sdcard.h)

The microSD driver provides FatFS-based file system access for standalone acquisition mode.

**Key Features:**
- SD card initialization (CMD0 → CMD8 → ACMD41 → CMD2 → CMD3 → CMD7)
- UHS-I SDR104 mode (208 MHz, 1.8V signaling)
- FatFS integration (f_mount, f_open, f_write, f_close)
- Multi-block write (CMD25) with DMA
- Card detect (mechanical switch in socket)
- Automatic file naming (IMG_YYYYMMDD_HHMMSS.bin)

### 4.4.8 OLED Driver (oled.c / oled.h)

The OLED driver controls the SSD1306 128×64 display via I2C.

**Key Features:**
- SSD1306 initialization sequence
- Framebuffer (128×64 pixels = 1024 bytes)
- 5×7 font rendering (print string, print hex, print decimal)
- Progress bar rendering
- Menu system (4-line display with cursor)
- Partial update for efficiency

### 4.4.9 HASH Driver (hash.c / hash.h)

The HASH driver uses the STM32H743 hardware SHA-256 accelerator.

**Key Features:**
- SHA-256 computation over arbitrary data length
- DMA-based data feeding from SDRAM
- Digest comparison (verify mode)
- HMAC-SHA256 for eMMC RPMB authentication
- Streaming mode for large images (process in 64-byte blocks)

## 4.5 DMA Architecture

### 4.5.1 DMA Stream Assignments

| Stream | Controller | Source | Destination | Priority | Burst | Usage |
|--------|-----------|--------|-------------|----------|-------|-------|
| MDMA Ch0 | MDMA | SDMMC2 FIFO | SDRAM | Very High | 16 beats | eMMC acquisition |
| MDMA Ch1 | MDMA | OCTOSPI1 FIFO | SDRAM | High | 8 beats | SPI NOR acquisition |
| MDMA Ch2 | MDMA | SDRAM | USB OTG_HS FIFO | High | 16 beats | USB transfer |
| MDMA Ch3 | MDMA | SDRAM | SDMMC1 FIFO | Medium | 8 beats | microSD write |
| MDMA Ch4 | MDMA | SDRAM | HASH DIN | Medium | 4 beats | SHA-256 hash |
| BDMA Ch0 | BDMA | FMC NAND | SDRAM | High | 4 beats | NAND acquisition |
| BDMA Ch1 | BDMA | SPI1 TX | Flash | Low | 1 beat | FPGA bitstream load |
| DMA1 Str0 | DMA1 | SPI1 RX | SRAM | Low | 1 beat | FPGA register read |
| DMA2 Str0 | DMA2 | I2C1 TX | SRAM | Low | 1 beat | OLED framebuffer |

### 4.5.2 MDMA Configuration for eMMC → SDRAM

```c
void mdma_emmc_to_sdram_config(uint8_t *sdram_addr, uint32_t size) {
    /* MDMA Channel 0: SDMMC2 FIFO → SDRAM */
    MDMA_Channel0->CSAR = (uint32_t)&SDMMC2->FIFO;  /* Source: SDMMC2 FIFO */
    MDMA_Channel0->CDAR = (uint32_t)sdram_addr;     /* Dest: SDRAM buffer */
    MDMA_Channel0->CBNDTR = size;                    /* Block size in bytes */
    MDMA_Channel0->CTCR = MDMA_CTCR_SINC_0 |        /* Source inc by 4 */
                          MDMA_CTCR_DINC_0 |         /* Dest inc by 4 */
                          MDMA_CTCR_SWAP_0 |         /* No swap */
                          (0x0C << MDMA_CTCR_TRG_Pos); /* Trigger: SDMMC2 */
    MDMA_Channel0->CBRUR = 0;                        /* No block repeat */
    MDMA_Channel0->CMDR = MDMA_CMDR_MASK_0 |        /* No masking */
                          MDMA_CMDR_MDATA_0;        /* No data */
    MDMA_Channel0->CCR = MDMA_CCR_EN |               /* Enable */
                         MDMA_CCR_TCIE |             /* Transfer complete IRQ */
                         MDMA_CCR_BRTIE |            /* Block repeat IRQ */
                         (3 << MDMA_CCR_PRIO_Pos) | /* Priority: Very High */
                         (3 << MDMA_CCR_BURST_Pos) | /* Burst: 16 beats */
                         MDMA_CCR_DIR_PER2MEM;       /* Direction: Periph→Mem */
}
```

## 4.6 Interrupt Architecture

### 4.6.1 NVIC Priority Assignment

| IRQ | Priority | Purpose |
|-----|----------|---------|
| MDMA Ch0 (eMMC) | 0 (highest) | eMMC DMA completion |
| MDMA Ch1 (SPI NOR) | 1 | SPI NOR DMA completion |
| MDMA Ch2 (USB) | 2 | USB transfer DMA completion |
| USB OTG_HS | 3 | USB enumeration/control |
| MDMA Ch3 (microSD) | 4 | microSD write DMA completion |
| BDMA Ch0 (NAND) | 5 | NAND DMA completion |
| FPGA_INTR (EXTI) | 6 | FPGA interrupt (R/B#, FIFO) |
| SDMMC2 | 7 | eMMC command/error |
| SDMMC1 | 8 | microSD command/error |
| MDMA Ch4 (HASH) | 9 | Hash DMA completion |
| TIM2 (buttons) | 10 | Button debounce (1ms tick) |
| SYSTICK | 11 | System tick (1ms) |
| RTC_WKUP | 12 | RTC wakeup (1s) |
| BDMA Ch1 (FPGA load) | 13 | FPGA bitstream load done |
| I2C1 | 14 | OLED I2C completion |
| DMA1 Str0 (SPI1 RX) | 15 (lowest) | FPGA register read |

### 4.6.2 Critical Interrupt Handlers

```c
/* MDMA Ch0 ISR: eMMC block read complete */
void MDMA_Ch0_IRQHandler(void) {
    if (MDMA->GISR0 & MDMA_GISR0_TCIF0) {
        MDMA->GISR0 = MDMA_GISR0_TCIF0;  /* Clear flag */
        acquisition_state.dma_complete = 1;
        /* Update ring buffer write pointer */
        ring_buffer_advance_write(acquisition_state.block_count * 512);
        /* Trigger next CMD18 if more blocks remain */
        if (acquisition_state.blocks_remaining > 0) {
            emmc_continue_read();
        }
    }
}

/* FPGA interrupt handler */
void EXTI9_5_IRQHandler(void) {
    if (EXTI->PR1 & EXTI_PR1_PIF6) {  /* PG6 = EXTI6 */
        EXTI->PR1 = EXTI_PR1_PIF6;
        uint32_t fpga_status = fpga_read_reg(FPGA_REG_INTR_STATUS);
        if (fpga_status & FPGA_INTR_RB_RISE) {
            /* R/B# went high — NAND page ready */
            nand_page_ready_callback();
        }
        if (fpga_status & FPGA_INTR_FIFO_THRESH) {
            /* FPGA FIFO threshold reached — drain to SDRAM */
            nand_drain_fifo();
        }
        fpga_write_reg(FPGA_REG_INTR_STATUS, fpga_status);  /* W1C */
    }
}
```

## 4.7 Main Loop & Task Dispatch

### 4.7.1 Main Loop Architecture

```c
typedef enum {
    TASK_IDLE,
    TASK_EMMC_ACQUIRE,
    TASK_NAND_ACQUIRE,
    TASK_SPINOR_ACQUIRE,
    TASK_USB_TRANSFER,
    TASK_SDCARD_WRITE,
    TASK_HASH_VERIFY,
    TASK_SELFTEST,
    TASK_FPGA_LOAD,
    TASK_MENU_NAVIGATE,
} task_id_t;

typedef struct {
    task_id_t current_task;
    task_id_t next_task;
    uint32_t task_state;
    uint32_t task_progress;
    uint32_t task_total;
    uint8_t  task_error;
    uint32_t task_start_tick;
} task_context_t;

void main_loop(void) {
    task_context_t ctx = {0};

    while (1) {
        /* Process button inputs */
        button_state_t btns = button_read_debounced();
        if (btns.changed) {
            ui_handle_input(&ctx, btns);
        }

        /* Process USB commands */
        if (usb_command_pending()) {
            usb_command_t cmd = usb_read_command();
            task_dispatch_usb(&ctx, &cmd);
        }

        /* Run current task state machine */
        switch (ctx.current_task) {
        case TASK_IDLE:
            ui_update_idle_screen(&ctx);
            break;
        case TASK_EMMC_ACQUIRE:
            task_emmc_acquire_run(&ctx);
            break;
        case TASK_NAND_ACQUIRE:
            task_nand_acquire_run(&ctx);
            break;
        case TASK_SPINOR_ACQUIRE:
            task_spinor_acquire_run(&ctx);
            break;
        case TASK_USB_TRANSFER:
            task_usb_transfer_run(&ctx);
            break;
        case TASK_SDCARD_WRITE:
            task_sdcard_write_run(&ctx);
            break;
        case TASK_HASH_VERIFY:
            task_hash_verify_run(&ctx);
            break;
        case TASK_SELFTEST:
            task_selftest_run(&ctx);
            break;
        }

        /* Check for task completion */
        if (ctx.task_state == TASK_STATE_DONE) {
            task_complete_handler(&ctx);
        }

        /* Update OLED display (throttled to 10 Hz) */
        if ((HAL_GetTick() - last_display_update) >= 100) {
            ui_render(&ctx);
            last_display_update = HAL_GetTick();
        }

        /* Power management: check battery */
        if ((HAL_GetTick() - last_battery_check) >= 5000) {
            power_check_battery();
            last_battery_check = HAL_GetTick();
        }
    }
}
```

### 4.7.2 eMMC Acquisition Task State Machine

```
States:
  IDLE → DETECT → INIT → EXT_CSD → CONFIG_HS400 → ACQUIRE → VERIFY → DONE
                                                       │
                                                       ▼
                                                    ERROR (on failure)

DETECT:   Send CMD0, CMD1, check OCR
INIT:     CMD2, CMD3, CMD7, CMD9
EXT_CSD:  CMD8, parse EXT_CSD, determine device size
CONFIG:   CMD6 SWITCH sequence for HS400
ACQUIRE:  CMD18 loop with DMA, ring buffer management
VERIFY:   SHA-256 over acquired data
DONE:     Display summary, signal completion
```

## 4.8 USB Wire Protocol

### 4.8.1 Command Frame Format

```
┌──────────┬──────────┬──────────┬──────────┬───────────┬──────────┐
│ SYNC (4) │ CMD (2)  │ LEN (4)  │ SEQ (2)  │ PAYLOAD   │ CRC16(2) │
│ 0xFD4D4644│          │          │          │ (LEN bytes)│          │
└──────────┴──────────┴──────────┴──────────┴───────────┴──────────┘

SYNC:   0xFD 0x4D 0x46 0x44 ("FDMD" — Flash Dumper Magic)
CMD:    Command ID (little-endian)
LEN:    Payload length in bytes (little-endian, max 65535)
SEQ:    Sequence number (little-endian, wraps)
PAYLOAD: Variable-length data
CRC16:  CRC-16/XMODEM over CMD+LEN+SEQ+PAYLOAD
```

### 4.8.2 Command IDs

| ID | Name | Direction | Description |
|----|------|-----------|-------------|
| 0x0001 | CMD_GET_INFO | H→D | Request device info |
| 0x0002 | CMD_INFO_RESP | D→H | Device info response |
| 0x0010 | CMD_EMMC_DETECT | H→D | Detect eMMC device |
| 0x0011 | CMD_EMMC_INFO | D→H | eMMC EXT_CSD data |
| 0x0012 | CMD_EMMC_ACQUIRE | H→D | Start eMMC acquisition |
| 0x0013 | CMD_EMMC_DATA | D→H | eMMC data block (stream) |
| 0x0014 | CMD_EMMC_PROGRESS | D→H | Acquisition progress |
| 0x0015 | CMD_EMMC_COMPLETE | D→H | Acquisition complete + hash |
| 0x0016 | CMD_EMMC_ABORT | H→D | Abort acquisition |
| 0x0020 | CMD_NAND_DETECT | H→D | Detect NAND device |
| 0x0021 | CMD_NAND_INFO | D→H | NAND ONFI params |
| 0x0022 | CMD_NAND_ACQUIRE | H→D | Start NAND acquisition |
| 0x0023 | CMD_NAND_DATA | D→H | NAND data block (stream) |
| 0x0024 | CMD_NAND_PROGRESS | D→H | Acquisition progress |
| 0x0025 | CMD_NAND_COMPLETE | D→H | Acquisition complete + hash |
| 0x0030 | CMD_SPINOR_DETECT | H→D | Detect SPI NOR device |
| 0x0031 | CMD_SPINOR_INFO | D→H | SPI NOR SFDP data |
| 0x0032 | CMD_SPINOR_ACQUIRE | H→D | Start SPI NOR acquisition |
| 0x0033 | CMD_SPINOR_DATA | D→H | SPI NOR data block (stream) |
| 0x0034 | CMD_SPINOR_COMPLETE | D→H | Acquisition complete + hash |
| 0x0040 | CMD_SDCARD_STATUS | H→D | Query microSD status |
| 0x0041 | CMD_SDCARD_START | H→D | Start writing to microSD |
| 0x0042 | CMD_SDCARD_PROGRESS | D→H | microSD write progress |
| 0x0050 | CMD_HASH_START | H→D | Start hash computation |
| 0x0051 | CMD_HASH_RESULT | D→H | Hash digest result |
| 0x0060 | CMD_SELFTEST | H→D | Run self-test |
| 0x0061 | CMD_SELFTEST_RESULT | D→H | Self-test results |
| 0x0070 | CMD_FPGA_LOAD | H→D | Load FPGA bitstream |
| 0x0071 | CMD_FPGA_STATUS | D→H | FPGA status |
| 0x00FF | CMD_ERROR | D→H | Error response |
| 0x0100 | CMD_ACK | Both | Acknowledge |
| 0x0101 | CMD_NACK | Both | Negative acknowledge |

### 4.8.3 Data Stream Frame

For high-throughput data transfer, a simplified streaming frame is used:

```
┌──────────┬──────────┬───────────┬──────────┐
│ SYNC (4) │ SEQ (4)  │ DATA (N)  │ CRC32(4) │
│ 0xFD444154│          │           │          │
└──────────┴──────────┴───────────┴──────────┘

SYNC:   0xFD 0x44 0x41 0x54 ("FDAT" — Flash Data)
SEQ:    32-bit block sequence number
DATA:   Raw flash data (typically 65536 bytes)
CRC32:  CRC-32/MPEG2 over SEQ+DATA
```

## 4.9 Build Instructions

### 4.9.1 Firmware Build

```bash
# Prerequisites
sudo apt install arm-none-eabi-gcc arm-none-eabi-binutils make

# Clone and build
cd firmware/
make clean
make -j$(nproc)

# Output: build/emmc-flash-dumper.elf, .bin, .hex

# Flash via ST-Link or SWD
st-flash write build/emmc-flash-dumper.bin 0x08000000
# Or via OpenOCD
openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
        -c "program build/emmc-flash-dumper.elf verify reset exit"
```

### 4.9.2 FPGA Bitstream Build

```bash
# Prerequisites
sudo apt install yosys nextpnr-ice40 icestorm

# Build
cd fpga/
make clean
make

# Output: build/nand_controller.bin

# Embed in firmware
cd ../firmware/
xxd -i ../fpga/build/nand_controller.bin > fpga_bitstream.h
make clean && make
```

### 4.9.3 Companion App Build

```bash
cd app/
npm install
npx react-native start
# For Android:
npx react-native run-android
# For USB communication, use react-native-usb library
```

### 4.9.4 KiCad PCB Build

```bash
# Open in KiCad 8.0
kicad kicad/device.kicad_pro

# Generate gerbers
File → Fabrication Outputs → Gerbers
# Use JLCPCB 4-layer stackup preset

# Generate BOM
File → Fabrication Outputs → BOM
```

## 4.10 Device Tree (Conceptual)

While bare-metal firmware doesn't use a traditional device tree, the hardware description is structured similarly for documentation:

```dts
/ {
    compatible = "nous,emmc-flash-dumper";
    model = "eMMC Flash Dumper v1.0";

    mcu: stm32h743@0 {
        compatible = "st,stm32h743";
        clock-frequency = <480000000>;

        fmc@52004000 {
            compatible = "st,stm32h7-fmc";
            reg = <0x52004000 0x400>;

            sdram@c0000000 {
                compatible = "micron,mt41k256m16tw";
                reg = <0xc0000000 0x10000000>;
                bank-width = <16>;
                row-bits = <13>;
                col-bits = <9>;
                cas-latency = <6>;
            };

            nand@80000000 {
                compatible = "onfi,nand-flash";
                reg = <0x80000000 0x10000000>;
                nand-on-flash-bbt;
                fpga-timing-controller;
            };
        };

        sdmmc2: mmc@52007400 {
            compatible = "st,stm32h7-sdmmc2";
            reg = <0x52007400 0x400>;
            bus-width = <8>;
            max-frequency = <200000000>;
            mmc-hs400-1_8v;
        };

        octospi1: spi@52005000 {
            compatible = "st,stm32h7-octospi";
            reg = <0x52005000 0x400>;
            spi-max-frequency = <100000000>;
        };

        usb_otg_hs: usb@50040000 {
            compatible = "st,stm32h7-otghs";
            reg = <0x50040000 0x40000>;
            ulpi-phy = <&usb3320>;
            dr_mode = "peripheral";
        };
    };

    fpga: ice40up5k@1 {
        compatible = "lattice,ice40up5k";
        spi-max-frequency = <20000000>;
        config-pin = <&gpio PB12>;
        done-pin = <&gpio PB13>;
        intr-pin = <&gpio PG6>;
    };

    usb3320: ulpi-phy@2 {
        compatible = "microchip,usb3320";
        clkout-frequency = <60000000>;
    };

    pmic: tps6521815@3 {
        compatible = "ti,tps6521815";
        reg = <0x24>;  /* I2C address */
        regulators {
            dcdc1: vddcore { regulator-voltage = <1200000>; };
            dcdc2: vddio_3v3 { regulator-voltage = <3300000>; };
            dcdc3: vdd_ddr { regulator-voltage = <1350000>; };
            dcdc4: vddio_1v8 { regulator-voltage = <1800000>; };
            ldo1: vdd_usb { regulator-voltage = <1000000>; };
            ldo2: vdd_fpga_io { regulator-voltage = <1800000>; };
        };
    };

    charger: bq25896@4 {
        compatible = "ti,bq25896";
        reg = <0x6A>;
        ti,charge-current = <2000000>;
        ti,charge-voltage = <4200000>;
    };

    display: ssd1306@5 {
        compatible = "solomon,ssd1306";
        reg = <0x3C>;
        width = <128>;
        height = <64>;
    };
};
```

## 4.11 Error Handling Strategy

### 4.11.1 Error Categories

| Category | Examples | Handling |
|----------|----------|----------|
| Fatal Hardware | SDRAM test fail, PMIC rail fail, FPGA load fail | Red LED, buzzer, halt |
| Recoverable HW | eMMC CRC error, NAND bit error, USB disconnect | Retry (3×), then degrade |
| Configuration | Invalid EXT_CSD, unknown ONFI params | Report to host, offer manual config |
| Resource | SDRAM buffer full, microSD full | Backpressure, pause acquisition |
| User | Invalid command, out-of-range params | NACK with error code |

### 4.11.2 Error Codes

```c
typedef enum {
    ERR_NONE                = 0x0000,
    ERR_SDRAM_INIT          = 0x1001,
    ERR_SDRAM_TEST          = 0x1002,
    ERR_FPGA_LOAD           = 0x2001,
    ERR_FPGA_ID             = 0x2002,
    ERR_FPGA_TIMEOUT        = 0x2003,
    ERR_EMMC_NO_CARD        = 0x3001,
    ERR_EMMC_INIT_FAIL      = 0x3002,
    ERR_EMMC_HS400_FAIL     = 0x3003,
    ERR_EMMC_CRC            = 0x3004,
    ERR_EMMC_TIMEOUT        = 0x3005,
    ERR_NAND_NO_CHIP        = 0x4001,
    ERR_NAND_ONFI_FAIL      = 0x4002,
    ERR_NAND_TIMING         = 0x4003,
    ERR_NAND_READ_FAIL      = 0x4004,
    ERR_SPINOR_NO_CHIP      = 0x5001,
    ERR_SPINOR_SFDP_FAIL    = 0x5002,
    ERR_SPINOR_READ_FAIL    = 0x5003,
    ERR_USB_ENUM_FAIL       = 0x6001,
    ERR_USB_DISCONNECT      = 0x6002,
    ERR_USB_FIFO_OVERFLOW   = 0x6003,
    ERR_SDCARD_NO_CARD      = 0x7001,
    ERR_SDCARD_FULL         = 0x7002,
    ERR_SDCARD_WRITE_FAIL   = 0x7003,
    ERR_HASH_FAIL           = 0x8001,
    ERR_POWER_LOW_BATTERY   = 0x9001,
    ERR_POWER_RAIL_FAIL     = 0x9002,
    ERR_CMD_INVALID         = 0xA001,
    ERR_CMD_PARAM           = 0xA002,
    ERR_CMD_SEQUENCE        = 0xA003,
} error_code_t;
```

## 4.12 Performance Optimizations

### 4.12.1 Critical Code in ITCM

Functions placed in ITCM (64 KB instruction TCM, zero-wait-state):

```c
__attribute__((section(".itcm_text")))
void sdmmc2_send_cmd_fast(uint8_t cmd, uint32_t arg, uint8_t resp_type);

__attribute__((section(".itcm_text")))
void mdma_trigger_transfer(uint8_t channel);

__attribute__((section(".itcm_text")))
void ring_buffer_advance_write(uint32_t bytes);

__attribute__((section(".itcm_text")))
uint32_t crc16_compute_fast(const uint8_t *data, uint32_t len);
```

### 4.12.2 Data in DTCM

Critical data structures in DTCM (128 KB data TCM, zero-wait-state):

```c
__attribute__((section(".dtcm")))
volatile task_context_t g_task_ctx;

__attribute__((section(".dtcm")))
volatile acquisition_state_t g_acq_state;

__attribute__((section(".dtcm")))
uint8_t g_usb_ep0_buf[512];

__attribute__((section(".dtcm")))
mdma_descriptor_t g_mdma_desc[8];
```

### 4.12.3 Cache Configuration

```c
/* Enable I-Cache and D-Cache */
SCB_EnableICache();
SCB_EnableDCache();

/* Configure MPU for SDRAM as non-cacheable (DMA coherence) */
MPU_Region_InitTypeDef MPU_InitStruct = {0};
MPU_InitStruct.Enable = MPU_REGION_ENABLE;
MPU_InitStruct.BaseAddress = 0xC0000000;
MPU_InitStruct.Size = MPU_REGION_SIZE_256MB;
MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
MPU_InitStruct.Number = 0;
MPU_InitStruct.SubRegionDisable = 0x00;
MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
HAL_MPU_ConfigRegion(&MPU_InitStruct);
MPU->CTRL = MPU_CTRL_ENABLE_Msk | MPU_CTRL_PRIVDEFENA_Msk;
```

---

*End of Phase 4 — Software Stack*
