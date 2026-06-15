# Infrared Phantom — Phase 4: Software Stack

## 1. Boot Strategy

### 1.1 Boot Sequence

```
Power-on / USB plug-in
  │
  ├─ STM32H743 BOOT0 = LOW (boot from Flash)
  │   Option Bytes: BOOT_ADD0 = 0x08000000 (Flash base)
  │
  ├─ System initialization (SystemInit)
  │   ├─ Enable POWER clock (RCC_APB4ENR)
  │   ├─ Configure PWR supply: LDO mode (PWR_CR3)
  │   ├─ Configure voltage scale: SCALE1 (VOS = 1)
  │   ├─ Wait for VOS ready (PWR_CSR1, VOSRDY)
  │   ├─ Enable OVERDRIVE (PWR_CR5) for 480 MHz
  │   └─ Enable SYSCFG clock (RCC_APB4ENR)
  │
  ├─ Clock tree initialization (SystemClock_Config)
  │   ├─ Enable HSE (25 MHz external crystal)
  │   ├─ Wait for HSE ready (RCC_CR, HSERDY)
  │   ├─ Configure PLL1: M=5, N=192, P=2 → SYSCLK=480 MHz
  │   │   VCO = 25/5 × 192 = 960 MHz
  │   │   SYSCLK = 960/2 = 480 MHz
  │   │   AHB = 480 MHz (HPRE = 0)
  │   │   APB1 = 120 MHz (D1PPRE = 0b101 /4)
  │   │   APB2 = 240 MHz (D1PPRE = 0b100 /2)
  │   │   APB4 = 120 MHz (D3PPRE = 0b101 /4)
  │   ├─ Configure PLL2: M=5, N=80, P=2 → 200 MHz
  │   │   For SDMMC1 kernel clock
  │   ├─ Enable PLL1, wait for lock (RCC_CR, PLL1RDY)
  │   ├─ Enable PLL2, wait for lock (RCC_CR, PLL2RDY)
  │   ├─ Switch SYSCLK source to PLL1 (RCC_CFGR, SW=0b11)
  │   └─ Wait for switch confirmed (RCC_CFGR, SWS=0b11)
  │
  ├─ Memory initialization
  │   ├─ Enable DTCM (128 KB), AXI SRAM (512 KB)
  │   ├─ Enable SRAM1 (128 KB), SRAM2 (32 KB)
  │   ├─ Enable ART accelerator, enable instruction/data cache
  │   └─ Configure MPU for DMA-accessible regions
  │
  ├─ Peripheral initialization (board_init)
  │   ├─ GPIO: Configure all pins per pinout table
  │   ├─ SPI1: W25Q128 flash (50 MHz, mode 0)
  │   ├─ SPI2: SSD1306 OLED (10 MHz, mode 0)
  │   ├─ SDMMC1: microSD (50 MHz, 4-bit)
  │   ├─ USART1: nRF52840 BLE (115200, 8N1, flow control)
  │   ├─ ADC1: IR analog capture (12-bit, 2 Msps, DMA)
  │   ├─ TIM2: Digital edge capture (32-bit, input capture)
  │   ├─ TIM3: IR PWM generation (carrier + gating, DMA)
  │   ├─ USB_OTG_HS: Device mode, ULPI PHY
  │   └─ DMA streams allocation
  │
  ├─ Self-test
  │   ├─ Flash CRC verification
  │   ├─ SPI NOR JEDEC ID check (W25Q128 = 0xEF4018)
  │   ├─ OLED communication test
  │   ├─ SD card detection
  │   ├─ nRF52840 BLE ping
  │   └─ IR receiver self-test (noise floor)
  │
  └─ Main loop (main)
      ├─ USB command processor
      ├─ BLE command processor
      ├─ IR capture engine (background DMA)
      ├─ OLED status display
      └─ Watchdog refresh
```

### 1.2 Boot Time Budget

| Stage | Time | Cumulative |
|-------|------|-----------|
| Power-on reset | 5 ms | 5 ms |
| Clock init (PLL lock) | 50 ms | 55 ms |
| Memory init | 5 ms | 60 ms |
| GPIO/peripheral init | 20 ms | 80 ms |
| Flash JEDEC check | 5 ms | 85 ms |
| OLED init | 15 ms | 100 ms |
| SD card detect | 50 ms | 150 ms |
| nRF52840 BLE ping | 200 ms | 350 ms |
| USB enumeration | 500 ms | 850 ms |
| Ready | — | **< 1 s** |

## 2. MMIO Registers — Critical Definitions

### 2.1 STM32H743 Register Map (selected)

```c
// Base addresses
#define RCC_BASE            0x40040400
#define PWR_BASE            0x40007000
#define GPIOA_BASE          0x58020000
#define GPIOB_BASE          0x58020400
#define GPIOC_BASE          0x58020800
#define GPIOD_BASE          0x58020C00
#define GPIOE_BASE          0x58021000
#define DMA1_BASE           0x40020000
#define DMA2_BASE           0x40020400
#define DMAMUX1_BASE        0x40020800
#define SPI1_BASE           0x40013000
#define SPI2_BASE           0x40003800
#define USART1_BASE         0x40011000
#define SDMMC1_BASE         0x40016000
#define TIM2_BASE           0x40000000
#define TIM3_BASE           0x40000400
#define ADC1_BASE           0x40022000
#define ADC12_COMMON_BASE   0x40022300
#define USB_OTG_HS_BASE     0x40040000
#define SYSCFG_BASE         0x58000400

// RCC registers
#define RCC_CR              (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_PLLCFGR         (*(volatile uint32_t *)(RCC_BASE + 0x08))
#define RCC_CFGR            (*(volatile uint32_t *)(RCC_BASE + 0x10))
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0xD8))
#define RCC_AHB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0xDC))
#define RCC_AHB4ENR         (*(volatile uint32_t *)(RCC_BASE + 0xE0))
#define RCC_APB1LENR        (*(volatile uint32_t *)(RCC_BASE + 0xE8))
#define RCC_APB1HENR        (*(volatile uint32_t *)(RCC_BASE + 0xEC))
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0xF0))
#define RCC_APB4ENR         (*(volatile uint32_t *)(RCC_BASE + 0xF4))

// GPIO registers (per port)
#define GPIO_MODER(base)    (*(volatile uint32_t *)((base) + 0x00))
#define GPIO_OTYPER(base)   (*(volatile uint32_t *)((base) + 0x04))
#define GPIO_OSPEEDR(base)  (*(volatile uint32_t *)((base) + 0x08))
#define GPIO_PUPDR(base)    (*(volatile uint32_t *)((base) + 0x0C))
#define GPIO_IDR(base)      (*(volatile uint32_t *)((base) + 0x10))
#define GPIO_ODR(base)      (*(volatile uint32_t *)((base) + 0x14))
#define GPIO_BSRR(base)     (*(volatile uint32_t *)((base) + 0x18))
#define GPIO_AFRL(base)     (*(volatile uint32_t *)((base) + 0x20))
#define GPIO_AFRH(base)     (*(volatile uint32_t *)((base) + 0x24))

// TIM registers
#define TIM_CR1(base)       (*(volatile uint32_t *)((base) + 0x00))
#define TIM_CR2(base)       (*(volatile uint32_t *)((base) + 0x04))
#define TIM_DIER(base)      (*(volatile uint32_t *)((base) + 0x0C))
#define TIM_SR(base)        (*(volatile uint32_t *)((base) + 0x10))
#define TIM_CCMR1(base)     (*(volatile uint32_t *)((base) + 0x18))
#define TIM_CCMR2(base)     (*(volatile uint32_t *)((base) + 0x1C))
#define TIM_CCER(base)      (*(volatile uint32_t *)((base) + 0x20))
#define TIM_CNT(base)       (*(volatile uint32_t *)((base) + 0x24))
#define TIM_PSC(base)       (*(volatile uint32_t *)((base) + 0x28))
#define TIM_ARR(base)       (*(volatile uint32_t *)((base) + 0x2C))
#define TIM_CCR1(base)      (*(volatile uint32_t *)((base) + 0x34))
#define TIM_CCR2(base)      (*(volatile uint32_t *)((base) + 0x38))
#define TIM_CCR3(base)      (*(volatile uint32_t *)((base) + 0x3C))
#define TIM_CCR4(base)      (*(volatile uint32_t *)((base) + 0x40)))

// DMA registers
#define DMA_CR(base, stream)   (*(volatile uint32_t *)((base) + 0x10 + (stream) * 0x18))
#define DMA_NDTR(base, stream)  (*(volatile uint32_t *)((base) + 0x14 + (stream) * 0x18))
#define DMA_PAR(base, stream)   (*(volatile uint32_t *)((base) + 0x18 + (stream) * 0x18))
#define DMA_M0AR(base, stream)  (*(volatile uint32_t *)((base) + 0x1C + (stream) * 0x18))
#define DMA_M1AR(base, stream)  (*(volatile uint32_t *)((base) + 0x20 + (stream) * 0x18))
#define DMA_FCR(base, stream)   (*(volatile uint32_t *)((base) + 0x24 + (stream) * 0x18))

// ADC registers
#define ADC_ISR             (*(volatile uint32_t *)(ADC1_BASE + 0x00))
#define ADC_IER             (*(volatile uint32_t *)(ADC1_BASE + 0x04))
#define ADC_CR              (*(volatile uint32_t *)(ADC1_BASE + 0x08))
#define ADC_CFGR            (*(volatile uint32_t *)(ADC1_BASE + 0x0C))
#define ADC_CFGR2           (*(volatile uint32_t *)(ADC1_BASE + 0x10))
#define ADC_SMPR1           (*(volatile uint32_t *)(ADC1_BASE + 0x14))
#define ADC_SMPR2           (*(volatile uint32_t *)(ADC1_BASE + 0x18))
#define ADC_PCSEL           (*(volatile uint32_t *)(ADC1_BASE + 0x44))
#define ADC_DR              (*(volatile uint32_t *)(ADC1_BASE + 0x40))
```

## 3. Clock & GPIO Initialization

### 3.1 Clock Tree Configuration

```c
// Target frequencies:
//   SYSCLK = 480 MHz (PLL1)
//   AHB    = 480 MHz (HPRE = /1)
//   APB1   = 120 MHz (D1PPRE = /4)
//   APB2   = 240 MHz (D1PPRE = /2)
//   APB4   = 120 MHz (D3PPRE = /4)
//   PLL2   = 200 MHz (SDMMC kernel)
//   HSE    = 25 MHz

void SystemClock_Config(void) {
    // 1. Enable PWR and SYSCFG clocks
    RCC->APB4ENR |= RCC_APB4ENR_PWREN | RCC_APB4ENR_SYSCFGEN;
    RCC->APB4ENR;  // Read-back delay
    
    // 2. Configure voltage scale 1 (for 480 MHz)
    PWR->CR3 &= ~PWR_CR3_VOS;
    PWR->CR3 |= PWR_CR3_VOS_0;  // VOS = 1 (SCALE1)
    while (!(PWR->CSR1 & PWR_CSR1_VOSRDY));  // Wait for VOS ready
    
    // 3. Enable HSE (25 MHz)
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));  // Wait for HSE ready
    
    // 4. Configure PLL1: 25 MHz → 480 MHz
    //    VCO_IN = HSE / M = 25 / 5 = 5 MHz
    //    VCO_OUT = VCO_IN × N = 5 × 192 = 960 MHz
    //    SYSCLK = VCO_OUT / P = 960 / 2 = 480 MHz
    RCC->PLLCFGR = (5 << RCC_PLLCFGR_PLL1M_Pos) |    // M = 5
                   (192 << RCC_PLLCFGR_PLL1N_Pos) |    // N = 192
                   (0 << RCC_PLLCFGR_PLL1P_Pos) |      // P = 2 (0b00)
                   (0 << RCC_PLLCFGR_PLL1Q_Pos) |      // Q = 2 (not used)
                   (4 << RCC_PLLCFGR_PLL1R_Pos) |      // R = 8 (not used)
                   (RCC_PLLCFGR_PLL1SRC_HSE) |          // PLL1 source = HSE
                   RCC_PLLCFGR_PLL1REN;                  // Enable PLL1 R output
    
    // 5. Enable PLL1
    RCC->CR |= RCC_CR_PLL1ON;
    while (!(RCC->CR & RCC_CR_PLL1RDY));
    
    // 6. Configure PLL2: 25 MHz → 200 MHz (SDMMC)
    //    VCO_IN = HSE / M = 25 / 5 = 5 MHz
    //    VCO_OUT = 5 × 80 = 400 MHz
    //    PLL2_P = 400 / 2 = 200 MHz
    RCC->PLLCKSELR |= (5 << RCC_PLLCKSELR_DIVM2_Pos);
    RCC->PLLCFGR |= (80 << 16) | (0 << 6);  // PLL2N=80, PLL2P=2
    
    // 7. Set flash wait states (7 WS for 480 MHz at 1.8 V)
    FLASH->ACR = FLASH_ACR_LATENCY_7WS | FLASH_ACR_PRFTEN |
                 FLASH_ACR_ICEN | FLASH_ACR_DCEN;
    
    // 8. Switch SYSCLK to PLL1
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL1;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL1);
    
    // 9. Configure bus prescalers
    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_D1PPRE | RCC_CFGR_D3PPRE);
    RCC->CFGR |= (0 << RCC_CFGR_HPRE_Pos) |       // AHB = /1 = 480 MHz
                 (4 << RCC_CFGR_D1PPRE_Pos) |      // APB2 = /2 = 240 MHz
                 (5 << RCC_CFGR_D3PPRE_Pos);        // APB4 = /4 = 120 MHz
    
    // 10. Update SystemCoreClock
    SystemCoreClock = 480000000;
}
```

### 3.2 GPIO Initialization

```c
void board_gpio_init(void) {
    // Enable GPIO clocks
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN |
                    RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN |
                    RCC_AHB4ENR_GPIOEEN;
    RCC->AHB4ENR;  // Read-back delay
    
    // === PA0: nRF_INT (input, pull-down) ===
    GPIO_MODER(GPIOA_BASE) &= ~(3 << (0 * 2));    // Input
    GPIO_PUPDR(GPIOA_BASE) &= ~(3 << (0 * 2));
    GPIO_PUPDR(GPIOA_BASE) |= (2 << (0 * 2));     // Pull-down
    
    // === PA1: USER_BTN (input, pull-up) ===
    GPIO_MODER(GPIOA_BASE) &= ~(3 << (1 * 2));    // Input
    GPIO_PUPDR(GPIOA_BASE) &= ~(3 << (1 * 2));
    GPIO_PUPDR(GPIOA_BASE) |= (1 << (1 * 2));     // Pull-up
    
    // === PA2: SPI1_MOSI (AF5) ===
    GPIO_MODER(GPIOA_BASE) &= ~(3 << (2 * 2));
    GPIO_MODER(GPIOA_BASE) |= (2 << (2 * 2));     // AF mode
    GPIO_OSPEEDR(GPIOA_BASE) |= (3 << (2 * 2));   // Very high speed
    GPIO_AFRL(GPIOA_BASE) &= ~(0xF << (2 * 4));
    GPIO_AFRL(GPIOA_BASE) |= (5 << (2 * 4));       // AF5 = SPI1
    
    // === PA3: SPI1_MISO (AF5) ===
    GPIO_MODER(GPIOA_BASE) &= ~(3 << (3 * 2));
    GPIO_MODER(GPIOA_BASE) |= (2 << (3 * 2));
    GPIO_OSPEEDR(GPIOA_BASE) |= (3 << (3 * 2));
    GPIO_AFRL(GPIOA_BASE) &= ~(0xF << (3 * 4));
    GPIO_AFRL(GPIOA_BASE) |= (5 << (3 * 4));       // AF5 = SPI1
    
    // === PA4: SPI1_NSS (output, manual CS) ===
    GPIO_MODER(GPIOA_BASE) &= ~(3 << (4 * 2));
    GPIO_MODER(GPIOA_BASE) |= (1 << (4 * 2));     // Output
    GPIO_ODR(GPIOA_BASE) |= (1 << 4);             // CS high (inactive)
    GPIO_OSPEEDR(GPIOA_BASE) |= (3 << (4 * 2));   // Very high speed
    
    // === PA5: SPI1_SCK (AF5) ===
    GPIO_MODER(GPIOA_BASE) &= ~(3 << (5 * 2));
    GPIO_MODER(GPIOA_BASE) |= (2 << (5 * 2));
    GPIO_OSPEEDR(GPIOA_BASE) |= (3 << (5 * 2));
    GPIO_AFRL(GPIOA_BASE) &= ~(0xF << (5 * 4));
    GPIO_AFRL(GPIOA_BASE) |= (5 << (5 * 4));       // AF5 = SPI1
    
    // === PA6: IR_DIGITAL (TIM3_CH1 input capture, AF2) ===
    GPIO_MODER(GPIOA_BASE) &= ~(3 << (6 * 2));
    GPIO_MODER(GPIOA_BASE) |= (2 << (6 * 2));
    GPIO_AFRL(GPIOA_BASE) &= ~(0xF << (6 * 4));
    GPIO_AFRL(GPIOA_BASE) |= (2 << (6 * 4));       // AF2 = TIM3
    
    // === PA7: IR_ANALOG (ADC1_IN7, analog) ===
    GPIO_MODER(GPIOA_BASE) &= ~(3 << (7 * 2));
    GPIO_MODER(GPIOA_BASE) |= (3 << (7 * 2));     // Analog mode
    
    // === PA9: USART1_TX (AF7) ===
    GPIO_MODER(GPIOA_BASE) &= ~(3 << (9 * 2));
    GPIO_MODER(GPIOA_BASE) |= (2 << (9 * 2));
    GPIO_OSPEEDR(GPIOA_BASE) |= (3 << (9 * 2));
    GPIO_AFRH(GPIOA_BASE) &= ~(0xF << ((9-8) * 4));
    GPIO_AFRH(GPIOA_BASE) |= (7 << ((9-8) * 4));   // AF7 = USART1
    
    // === PA10: USART1_RX (AF7) ===
    GPIO_MODER(GPIOA_BASE) &= ~(3 << (10 * 2));
    GPIO_MODER(GPIOA_BASE) |= (2 << (10 * 2));
    GPIO_AFRH(GPIOA_BASE) &= ~(0xF << ((10-8) * 4));
    GPIO_AFRH(GPIOA_BASE) |= (7 << ((10-8) * 4));  // AF7 = USART1
    
    // === PD0: nRF_RESET (output, active-low) ===
    GPIO_MODER(GPIOD_BASE) &= ~(3 << (0 * 2));
    GPIO_MODER(GPIOD_BASE) |= (1 << (0 * 2));     // Output
    GPIO_ODR(GPIOD_BASE) |= (1 << 0);             // High (nRF not in reset)
    
    // === PD1: nRF_RTS (output) ===
    GPIO_MODER(GPIOD_BASE) &= ~(3 << (1 * 2));
    GPIO_MODER(GPIOD_BASE) |= (1 << (1 * 2));
    
    // === PD2: IR_PWM_EN (TIM3_CH1 PWM output, AF2) ===
    GPIO_MODER(GPIOD_BASE) &= ~(3 << (2 * 2));
    GPIO_MODER(GPIOD_BASE) |= (2 << (2 * 2));
    GPIO_OSPEEDR(GPIOD_BASE) |= (3 << (2 * 2));
    GPIO_AFRL(GPIOD_BASE) &= ~(0xF << (2 * 4));
    GPIO_AFRL(GPIOD_BASE) |= (2 << (2 * 4));       // AF2 = TIM3
    
    // === PE1: IR_LED_EN (output, MOSFET gate) ===
    GPIO_MODER(GPIOE_BASE) &= ~(3 << (1 * 2));
    GPIO_MODER(GPIOE_BASE) |= (1 << (1 * 2));
    GPIO_ODR(GPIOE_BASE) &= ~(1 << 1);            // Low (LEDs off)
    
    // === PE4: IR_PWM_GATE (TIM3_CH2, AF2) ===
    GPIO_MODER(GPIOE_BASE) &= ~(3 << (4 * 2));
    GPIO_MODER(GPIOE_BASE) |= (2 << (4 * 2));
    GPIO_OSPEEDR(GPIOE_BASE) |= (3 << (4 * 2));
    GPIO_AFRL(GPIOE_BASE) &= ~(0xF << (4 * 4));
    GPIO_AFRL(GPIOE_BASE) |= (2 << (4 * 4));       // AF2 = TIM3
    
    // === PE5: OLED_CS (output, active-low) ===
    GPIO_MODER(GPIOE_BASE) &= ~(3 << (5 * 2));
    GPIO_MODER(GPIOE_BASE) |= (1 << (5 * 2));
    GPIO_ODR(GPIOE_BASE) |= (1 << 5);             // CS high (inactive)
    
    // === PB6: OLED_RST (output, active-low) ===
    GPIO_MODER(GPIOB_BASE) &= ~(3 << (6 * 2));
    GPIO_MODER(GPIOB_BASE) |= (1 << (6 * 2));
    GPIO_ODR(GPIOB_BASE) |= (1 << 6);             // Reset high (not in reset)
    
    // === PB7: OLED_DC (output) ===
    GPIO_MODER(GPIOB_BASE) &= ~(3 << (7 * 2));
    GPIO_MODER(GPIOB_BASE) |= (1 << (7 * 2));
    
    // === PE2: LED_STATUS (output) ===
    GPIO_MODER(GPIOE_BASE) &= ~(3 << (2 * 2));
    GPIO_MODER(GPIOE_BASE) |= (1 << (2 * 2));
    GPIO_ODR(GPIOE_BASE) &= ~(1 << 2);            // Off
    
    // === PE3: LED_ERROR (output) ===
    GPIO_MODER(GPIOE_BASE) &= ~(3 << (3 * 2));
    GPIO_MODER(GPIOE_BASE) |= (1 << (3 * 2));
    GPIO_ODR(GPIOE_BASE) &= ~(1 << 3);            // Off
}
```

## 4. Device Drivers

### 4.1 IR Receiver Driver (ir_receiver.h / ir_receiver.c)

```c
// See firmware/drivers/ir_receiver.c for full implementation
// Key register operations:
//
// ADC1 configuration for 2 Msps continuous capture:
//   ADC_CR |= ADC_CR_ADVEN           // Enable ADC voltage regulator
//   ADC_CR |= ADC_CR_ADCALIN          // Linear calibration
//   ADC_CFGR = CONT | DMAEN | OVRDIS  // Continuous, DMA, no overrun
//   ADC_CFGR2 = OSRSEL(0) | ROVSE     // No oversampling
//   ADC_SMPR1 = SMP7(0b000)           // Channel 7: 1.5 cycles
//   ADC_PCSEL = PCSEL_7               // Pre-select channel 7
//   ADC_CR |= ADSTART                  // Start conversion
//
// DMA1 Stream 0 configuration:
//   DMA_CR(DMA1, 0) = CHSEL(0) | MINC | CIRC | DIR(0b11) | HTIE | TCIE
//   Channel 0 = ADC1
//   DMA_NDTR = 8192                    // Buffer size
//   DMA_M0AR = &adc_buffer[0]          // Primary buffer (DTCM)
//   DMA_M1AR = &adc_buffer[8192]       // Secondary buffer (DTCM)
//
// TIM2 input capture (32-bit timestamps):
//   TIM_CR1 = CEN                       // Enable counter
//   TIM_PSC = 0                         // No prescaler (120 MHz = 8.33 ns ticks)
//   TIM_ARR = 0xFFFFFFFF                // Full 32-bit range
//   TIM_CCMR1 = CC1S(0b01) | IC1F(0b0011) // CH1 input, CK_DIV8 filter
//   TIM_CCER = CC1E | CC1P              // Enable capture, both edges
//   TIM_DIER = CC1IE | DMAEN            // Capture IRQ + DMA
```

### 4.2 IR Transmitter Driver (ir_transmitter.h / ir_transmitter.c)

```c
// See firmware/drivers/ir_transmitter.c for full implementation
// Key register operations:
//
// TIM3 configuration for PWM carrier generation:
//   TIM_CR1 = ARPE | CEN
//   TIM_PSC = 0                         // 120 MHz APB1 × 2 = 240 MHz
//   TIM_ARR = CARRIER_PERIOD - 1        // e.g., 240000/38000 = 6316 for 38 kHz
//   TIM_CCMR1 = OC1M(0b110) | OC1PE    // PWM mode 1, preload enable
//   TIM_CCR1 = CARRIER_DUTY             // e.g., 6316/3 = 2105 for 33% duty
//   TIM_CCER = CC1E                     // Enable CH1 output
//
// DMA for waveform gating (TIM3_CH2):
//   DMA_CR(DMA1, 5) = CHSEL(1) | MINC | DIR(0b01) | HTIE | TCIE
//   Channel 1 = TIM3_CH2
//   Pattern buffer: sequence of CCR2 values (0=off, non-zero=on)
```

### 4.3 SPI NOR Flash Driver (w25q128.h / w25q128.c)

```c
// See firmware/drivers/w25q128.c for full implementation
// Commands:
//   W25Q128_JEDEC_ID    = 0x9F  → Expected: 0xEF4018 (Winbond W25Q128)
//   W25Q128_READ         = 0x03  → Read data (up to 50 MHz)
//   W25Q128_FAST_READ    = 0x0B  → Fast read (up to 104 MHz)
//   W25Q128_PAGE_PROGRAM = 0x02  → Program 1–256 bytes
//   W25Q128_SECTOR_ERASE = 0x20  → Erase 4 KB sector
//   W25Q128_BLOCK_ERASE  = 0xD8  → Erase 64 KB block
//   W25Q128_CHIP_ERASE   = 0xC7  → Erase entire chip
//   W25Q128_WRITE_ENABLE = 0x06  → Enable writes
//   W25Q128_READ_SR1     = 0x05  → Read status register 1
//   W25Q128_READ_SR2     = 0x35  → Read status register 2
```

### 4.4 SSD1306 OLED Driver (ssd1306.h / ssd1306.c)

```c
// See firmware/drivers/ssd1306.c for full implementation
// Commands:
//   SSD1306_DISPLAY_OFF        = 0xAE
//   SSD1306_DISPLAY_ON         = 0xAF
//   SSD1306_SET_MUX_RATIO      = 0xA8 (32 lines)
//   SSD1306_SET_DISPLAY_OFFSET = 0xD3
//   SSD1306_SET_START_LINE     = 0x40
//   SSD1306_SET_SEGMENT_REMAP  = 0xA1 (flip horizontal)
//   SSD1306_SET_COM_SCAN_DIR   = 0xC8 (flip vertical)
//   SSD1306_SET_COM_PINS       = 0xDA (sequential)
//   SSD1306_SET_CONTRAST       = 0x81 (0x7F default)
//   SSD1306_SET_PRECHARGE      = 0xD9
//   SSD1306_SET_VCOM_DETECT    = 0xDB
//   SSD1306_SET_CLOCK_DIV      = 0xD5 (0x80 = 100 fps)
//   SSD1306_CHARGE_PUMP        = 0x8D (enable)
//   SSD1306_SET_MEMORY_MODE    = 0x20 (horizontal addressing)
```

## 5. USB Device Descriptors

### 5.1 Device Descriptor

```c
// VID: 0x1209 (pid.codes)  PID: 0xIR01 (Infrared Phantom)
#define USBD_VID                    0x1209
#define USBD_PID                    0x1R01
#define USBD_LANGID                 0x0409    // English (US)
#define USBD_MANUFACTURER           "Infrared Phantom"
#define USBD_PRODUCT                "IR Security Research Toolkit"
#define USBD_SERIAL                 "IRPH-00000001"
#define USBD_CONFIG_MAX_POWER       250       // 500 mA (in 2 mA units)

// Interfaces:
//   IF0: CDC (virtual COM port) — command/control
//   IF1: Bulk data — waveform streaming
//   IF2: HID — button/mode reports

static const uint8_t usbd_device_descriptor[] = {
    0x12,       // bLength
    0x01,       // bDescriptorType (DEVICE)
    0x00, 0x02, // bcdUSB (2.00) — High Speed
    0xEF,       // bDeviceClass (Misc)
    0x02,       // bDeviceSubClass
    0x01,       // bDeviceProtocol (IAD)
    0x40,       // bMaxPacketSize0 (64)
    0x09, 0x12, // idVendor (0x1209)
    0x01, 0x1R, // idProduct (0x1R01)
    0x01, 0x01, // bcdDevice (1.01)
    0x01,       // iManufacturer
    0x02,       // iProduct
    0x03,       // iSerialNumber
    0x01        // bNumConfigurations
};
```

### 5.2 Configuration Descriptor (Composite: CDC + Bulk + HID)

```c
// Total configuration: 9+8+9+5+4+5+7+9+7+9 = 72 bytes
// Interface Association Descriptor (IAD) for CDC
//   IF0 + IF1 = CDC (control + data)
//   IF2 = Bulk data (waveform streaming)
//   IF3 = HID (button/mode reports)
```

## 6. Device Tree (for reference / Linux integration)

```dts
/dts-v1/;
/ {
    model = "Infrared Phantom IR Security Research Toolkit";
    compatible = "irphantom,ir-phantom", "st,stm32h743";

    chosen {
        stdout-path = &usart1;
    };

    cpus {
        cpu0: cpu@0 {
            compatible = "arm,cortex-m7";
            clock-frequency = <480000000>;
        };
    };

    soc {
        sram0: memory@20000000 {
            compatible = "mmio-sram";
            reg = <0x20000000 0x20000>;  // 128 KB DTCM
        };

        sram1: memory@24000000 {
            compatible = "mmio-sram";
            reg = <0x24000000 0x80000>;  // 512 KB AXI
        };

        flash0: flash@8000000 {
            compatible = "st,stm32h7-flash";
            reg = <0x08000000 0x100000>;  // 1 MB Flash
        };

        spi1: spi@40013000 {
            compatible = "st,stm32h7-spi";
            reg = <0x40013000 0x400>;
            clocks = <&rcc 0>;
            status = "okay";

            w25q128: flash@0 {
                compatible = "winbond,w25q128";
                reg = <0>;
                spi-max-frequency = <50000000>;
                size = <16777216>;  // 16 MB
            };
        };

        spi2: spi@40003800 {
            compatible = "st,stm32h7-spi";
            reg = <0x40003800 0x400>;
            clocks = <&rcc 1>;
            status = "okay";

            ssd1306: oled@0 {
                compatible = "solomon,ssd1306fb";
                reg = <0>;
                spi-max-frequency = <10000000>;
                solomon,width = <128>;
                solomon,height = <32>;
                solomon,page-offset = <0>;
            };
        };

        usart1: serial@40011000 {
            compatible = "st,stm32h7-usart";
            reg = <0x40011000 0x400>;
            clocks = <&rcc 2>;
            status = "okay";
            current-speed = <115200>;
        };

        adc1: adc@40022000 {
            compatible = "st,stm32h7-adc";
            reg = <0x40022000 0x400>;
            clocks = <&rcc 3>;
            status = "okay";
            #address-cells = <1>;
            #size-cells = <0>;

            channel@7 {
                reg = <7>;
                st,min-sample-time-ns = <60>;  // 1.5 cycles @ 25 MHz
            };
        };

        tim2: timer@40000000 {
            compatible = "st,stm32h7-tim32";
            reg = <0x40000000 0x400>;
            clocks = <&rcc 4>;
            status = "okay";
        };

        tim3: timer@40000400 {
            compatible = "st,stm32h7-tim16";
            reg = <0x40000400 0x400>;
            clocks = <&rcc 4>;
            status = "okay";
        };

        ir-receiver {
            compatible = "irphantom,tsmp58000";
            analog-gpios = <&gpioa 7 GPIO_ANALOG>;
            digital-gpios = <&gpioa 6 (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>;
        };

        ir-transmitter {
            compatible = "irphantom,ir-led-array";
            pwm-gpios = <&gpiod 2 GPIO_ACTIVE_HIGH>;
            gate-gpios = <&gpioe 4 GPIO_ACTIVE_HIGH>;
            enable-gpios = <&gpioe 1 GPIO_ACTIVE_HIGH>;
            carrier-frequency = <38000>;
            duty-cycle = <33>;
        };

        nrf52840 {
            compatible = "nordic,nrf52840-ble";
            uart-port = <&usart1>;
            reset-gpios = <&gpiod 0 GPIO_ACTIVE_LOW>;
            int-gpios = <&gpioa 0 GPIO_ACTIVE_HIGH>;
        };
    };
};
```

## 7. Wire Protocol — USB CDC Commands

### 7.1 Command Format

```
┌──────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
│ SYNC     │ CMD      │ LENGTH   │ PAYLOAD  │ CHECKSUM │ END      │
│ 0xAA55   │ 1 byte   │ 2 bytes  │ N bytes  │ 1 byte   │ 0x0D     │
└──────────┴──────────┴──────────┴──────────┴──────────┴──────────┘
```

### 7.2 Command Set

| CMD | Name | Payload | Response |
|-----|------|---------|----------|
| 0x01 | PING | none | PONG (0x01) |
| 0x02 | GET_VERSION | none | version string (4 bytes: major.minor.patch.dev) |
| 0x10 | CAP_START | {mode: u8, rate: u32, duration_ms: u16} | ACK + streaming data |
| 0x11 | CAP_STOP | none | frame count (u32) |
| 0x12 | CAP_STATUS | none | {running: u8, frames: u32, buffer_used: u32} |
| 0x20 | TX_PROTOCOL | {protocol: u8, addr_hi: u8, addr_lo: u8, cmd: u8, repeat: u8} | ACK |
| 0x21 | TX_RAW | {carrier_hz: u16, duty_pct: u8, length: u16, data[]: u16[] (us ticks)} | ACK |
| 0x22 | TX_STOP | none | ACK |
| 0x30 | FUZZ_START | {protocol: u8, field: u8, strategy: u8, count: u16, delay_ms: u16} | ACK + streaming results |
| 0x31 | FUZZ_STOP | none | results summary |
| 0x40 | ANALYZE_START | {duration_ms: u16} | FFT result (frequency bins) |
| 0x41 | ANALYZE_CLUSTER | {threshold: u8} | cluster IDs |
| 0x50 | FLASH_READ | {addr: u32, len: u16} | data[] |
| 0x51 | FLASH_WRITE | {addr: u32, data[]: u8} | ACK |
| 0x52 | FLASH_ERASE | {sector: u16} | ACK |
| 0x60 | SD_MOUNT | none | {status: u8, capacity_mb: u32} |
| 0x61 | SD_OPEN | {filename: char[64], mode: u8} | {fd: u8} |
| 0x62 | SD_READ | {fd: u8, len: u16} | data[] |
| 0x63 | SD_WRITE | {fd: u8, data[]: u8} | {written: u16} |
| 0x70 | BLE_SCAN | none | device list |
| 0x71 | BLE_CONNECT | {addr: u8[6]} | {status: u8} |
| 0x72 | BLE_SEND | {data[]: u8} | ACK |
| 0x80 | OLED_TEXT | {x: u8, y: u8, text: char[32]} | ACK |
| 0x81 | OLED_CLEAR | none | ACK |
| 0xFF | RESET | none | BOOT message |

### 7.3 IR Protocol IDs

| ID | Protocol | Encoding |
|----|----------|----------|
| 0x01 | NEC | PPM, 38 kHz |
| 0x02 | NEC Extended | PPM, 38 kHz |
| 0x03 | RC5 | Manchester, 36 kHz |
| 0x04 | RC6 | Manchester+, 36 kHz |
| 0x05 | RC6A | Manchester+, 36 kHz |
| 0x06 | Sony SIRC 12 | PPM, 40 kHz |
| 0x07 | Sony SIRC 15 | PPM, 40 kHz |
| 0x08 | Sony SIRC 20 | PPM, 40 kHz |
| 0x09 | Samsung | PPM (NEC-like), 38 kHz |
| 0x0A | Sharp | PPM, 38 kHz |
| 0x0B | Daikin | PPM (multi-frame), 38 kHz |
| 0x0C | Mitsubishi | PPM, 33 kHz |
| 0x0D | JVC | PPM (NEC-like), 38 kHz |
| 0x0E | Panasonic | PPM, 36.7 kHz |
| 0x0F | IRDA-SIR | NRZ UART, 115.2 kbps |
| 0x10 | Raw Manchester | Manchester, auto-detect carrier |
| 0xFF | Raw waveform | No encoding, raw timing data |

## 8. Build Instructions

### 8.1 Prerequisites

```bash
# Install ARM GCC toolchain
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi

# Install OpenOCD for flashing
sudo apt-get install openocd

# Install dfu-util for USB DFU flashing
sudo apt-get install dfu-util
```

### 8.2 Building Firmware

```bash
cd firmware
make clean
make
# Output: build/infrared-phantom.elf, build/infrared-phantom.bin, build/infrared-phantom.hex
```

### 8.3 Flashing

```bash
# Option 1: OpenOCD via SWD
openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
    -c "program build/infrared-phantom.elf verify reset exit"

# Option 2: USB DFU (hold USER button during power-on)
dfu-util -a 0 -s 0x08000000:leave -D build/infrared-phantom.bin

# Option 3: STM32CubeProgrammer (GUI)
```

### 8.4 Building Companion App

```bash
cd app
npm install
npx react-native run-android    # Android
npx react-native run-ios         # iOS (requires macOS)
```

## 9. BLE C2 Protocol (nRF52840)

### 9.1 BLE Service Definition

| Service | UUID | Characteristics |
|---------|------|-----------------|
| IR Command | 0x1RP0 | CAP_CTRL (0x1RP1), TX_CTRL (0x1RP2), FUZZ_CTRL (0x1RP3) |
| IR Data | 0x1RP4 | CAP_DATA (0x1RP5, notify), TX_DATA (0x1RP6, write) |
| Device Info | 0x180A | MODEL (0x2A24), FIRMWARE (0x2A26), SERIAL (0x2A27) |
| Status | 0x1RP7 | MODE (0x1RP8, notify), BATTERY (0x1RP9, notify) |

### 9.2 BLE Data Flow

```
Phone App → BLE Write (TX_CTRL) → nRF52840 → UART TX → STM32H743 → IR Engine
STM32H743 → UART RX → nRF52840 → BLE Notify (CAP_DATA) → Phone App
```

UART framing (115200, 8N1):
```
SOF (0xAA) | LEN (2 bytes) | CMD (1 byte) | DATA (LEN bytes) | CHECKSUM (1 byte) | EOF (0x55)
```

## 10. Memory Map — Firmware Layout

| Address | Size | Content |
|---------|------|---------|
| 0x08000000 | 16 KB | Vector table + bootloader |
| 0x08004000 | 16 KB | Bootloader (DFU + integrity check) |
| 0x08008000 | 480 KB | Application firmware |
| 0x08080000 | 16 KB | Configuration storage (settings, calibration) |
| 0x08084000 | 496 KB | OTA update partition |
| 0x080FC000 | 16 KB | Signature/certificate area |

### 10.1 RAM Layout

| Address | Size | Content |
|---------|------|---------|
| 0x20000000 | 128 KB | DTCM: DMA buffers, capture ring, ISR stacks |
| 0x24000000 | 512 KB | AXI SRAM: Protocol engine, fuzz engine, USB buffers |
| 0x30000000 | 128 KB | SRAM1: SD card buffers, filesystem |
| 0x30020000 | 32 KB | SRAM2: BLE UART ring buffer |
| 0x38000000 | 64 KB | SRAM4: Backup registers, persistent config |