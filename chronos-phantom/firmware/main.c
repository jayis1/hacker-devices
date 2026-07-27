/*
 * main.c — Chronos-Phantom main firmware
 *
 * System initialization, main loop, mode dispatcher, and peripheral setup.
 * This is the entry point for the STM32H753 firmware.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <string.h>
#include "board.h"
#include "registers.h"
#include "ptp_engine.h"
#include "ntp_engine.h"
#include "eth_bridge.h"
#include "tcxo_drvr.h"
#include "skew_gen.h"
#include "covert_codec.h"
#include "ble_link.h"
#include "imux_tamper.h"

/* --------------------------------------------------------------------- */
/*  Global state                                                          */
/* --------------------------------------------------------------------- */
static ptp_engine_state_t  g_ptp;
static ntp_engine_state_t  g_ntp;
static eth_bridge_t        g_bridge;
static tcxo_state_t        g_tcxo;
static covert_state_t     g_covert;
static ble_link_t         g_ble;
static tamper_t           g_tamper;
static op_mode_t          g_mode = MODE_PASSIVE_SNIFF;

/* Frame buffers (aligned for DMA) */
static uint8_t  g_rx_frame_a[ETH_BUF_SIZE] __attribute__((aligned(32)));
static uint8_t  g_rx_frame_b[ETH_BUF_SIZE] __attribute__((aligned(32)));
static uint8_t  g_tx_frame[ETH_BUF_SIZE]   __attribute__((aligned(32)));
static uint8_t  g_announce_frame[ETH_BUF_SIZE] __attribute__((aligned(32)));

/* SysTick millisecond counter */
static volatile uint32_t g_sys_ms = 0;
static volatile uint64_t g_sys_ns = 0;  /* rough nanosecond counter */

/* GNSS 1-PPS tick counter */
static volatile uint32_t g_pps_count = 0;

/* Capture flag (controlled by app) */
static uint8_t g_capturing = 0;

/* --------------------------------------------------------------------- */
/*  Forward declarations for low-level init functions                     */
/* --------------------------------------------------------------------- */
static void clock_init(void);
static void gpio_init(void);
static void eth_init(void);
static void usart1_init(void);
static void usart3_init(void);
static void dac_init(void);
static void i2c2_init(void);
static void systick_init(void);
static void nvic_enable(int irqn);

/* UART RX ring buffer for BLE (USART1) */
static uint8_t  g_ble_rx_ring[256];
static volatile uint16_t g_ble_rx_head = 0;
static volatile uint16_t g_ble_rx_tail = 0;

/* GNSS NMEA line buffer (USART3) */
static uint8_t  g_gnss_line[GNSS_UART_BUF_SIZE];
static volatile uint16_t g_gnss_idx = 0;

/* --------------------------------------------------------------------- */
/*  SysTick handler (1 ms tick)                                           */
/* --------------------------------------------------------------------- */
void SysTick_Handler(void)
{
    g_sys_ms++;
    g_sys_ns += 1000000ULL;  /* rough — real HW uses PTP timestamp */
}

/* --------------------------------------------------------------------- */
/*  USART1 RX handler (BLE UART)                                          */
/* --------------------------------------------------------------------- */
void USART1_IRQHandler(void)
{
    if (USART1->ISR & USART_ISR_RXNE) {
        uint8_t b = (uint8_t)(USART1->RDR & 0xFF);
        uint16_t next = (g_ble_rx_head + 1) & 0xFF;
        if (next != g_ble_rx_tail) {
            g_ble_rx_ring[g_ble_rx_head] = b;
            g_ble_rx_head = next;
        }
    }
}

/* --------------------------------------------------------------------- */
/*  USART3 RX handler (GNSS UART)                                         */
/* --------------------------------------------------------------------- */
void USART3_IRQHandler(void)
{
    if (USART3->ISR & USART_ISR_RXNE) {
        uint8_t b = (uint8_t)(USART3->RDR & 0xFF);
        if (b == '\n' || g_gnss_idx >= GNSS_UART_BUF_SIZE - 1) {
            g_gnss_line[g_gnss_idx] = 0;
            g_gnss_idx = 0;
            /* In a full implementation, we'd parse $GPRMC/$GPGGA here
             * to extract UTC time and use it for TCXO disciplining.
             * For now, we just discard the line.
             */
        } else if (b != '\r') {
            g_gnss_line[g_gnss_idx++] = b;
        }
    }
}

/* --------------------------------------------------------------------- */
/*  TIM8 Capture/Compare handler (1-PPS capture from GNSS)               */
/* --------------------------------------------------------------------- */
void TIM8_CC_IRQHandler(void)
{
    /* Check CC2 flag (PC8 = TIM8_CH2) */
    if (TIM8->SR & (1 << 2)) {
        TIM8->SR &= ~(1 << 2);
        g_pps_count++;
        /* In real HW we'd read the captured timer value here, compute
         * the offset between PPS and TCXO, and feed tcxo_pps_handler().
         */
        tcxo_pps_handler(&g_tcxo, 0);  /* offset = 0 (simplified) */
    }
}

/* --------------------------------------------------------------------- */
/*  Low-level initialization functions                                    */
/* --------------------------------------------------------------------- */
static void nvic_enable(int irqn)
{
    /* Each ISER covers 32 IRQs */
    int reg = irqn / 32;
    int bit = irqn % 32;
    volatile uint32_t *iser = (volatile uint32_t *)0xE000E100;
    iser[reg] |= (1 << bit);
}

static void clock_init(void)
{
    /* Enable HSE and wait for ready */
    RCC->CR |= (1 << 16);  /* HSEON */
    while (!(RCC->CR & (1 << 17))) ;  /* HSERDY */

    /* Configure PLL1: HSE / 1 * 96 / 2 = 480 MHz
     * PLL1M=1, PLL1N=96 (wrong—H7 calc), but simplified for this exercise.
     * In real code we'd set D1CFGR, PLL1CFGR etc. per RM0433.
     */
    RCC->D1CFGR = 0;   /* D1CPRE = 1, HPRE = 1 → 480 MHz D1 */
    RCC->D2CFGR = (1 << 4);  /* APB1 = D2/2 = 240 MHz (too fast, simplified) */
    RCC->CFGR = 0;

    /* Enable peripheral clocks */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN |
                    RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_GPIODEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN | RCC_APB2ENR_TIM8EN;
    RCC->APB1LENR |= RCC_APB1LENR_USART3EN | RCC_APB1LENR_I2C2EN |
                      RCC_APB1LENR_TIM3EN | RCC_APB1LENR_TIM4EN;

    /* Enable ETH MAC clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_ETHMACEN | RCC_AHB1ENR_ETHMACTXEN |
                     RCC_AHB1ENR_ETHMACRXEN;

    /* Enable DAC clock (simplified — actual bit is in APB1 or AHB1
     * depending on H7 variant) */
}

static void gpio_config(uint8_t port_idx, uint8_t pin, uint8_t mode,
                          uint8_t otype, uint8_t ospeed, uint8_t pupd,
                          uint8_t af)
{
    gpio_t *gpio;
    switch (port_idx) {
    case 0: gpio = GPIOA; break;
    case 1: gpio = GPIOB; break;
    case 2: gpio = GPIOC; break;
    case 3: gpio = GPIOD; break;
    default: return;
    }
    gpio->MODER &= ~(3 << (pin * 2));
    gpio->MODER |= (mode << (pin * 2));
    gpio->OTYPER &= ~(1 << pin);
    gpio->OTYPER |= (otype << pin);
    gpio->OSPEEDR &= ~(3 << (pin * 2));
    gpio->OSPEEDR |= (ospeed << (pin * 2));
    gpio->PUPDR &= ~(3 << (pin * 2));
    gpio->PUPDR |= (pupd << (pin * 2));
    if (af != 0) {
        if (pin < 8)
            gpio->AFRL |= ((uint32_t)af << (pin * 4));
        else
            gpio->AFRH |= ((uint32_t)af << ((pin - 8) * 4));
    }
}

static void gpio_init(void)
{
    /* ETH RMII pins: PA1(RXD0), PA2(RXD1), PA3(CRS_DV), PA7(TXEN),
     *               PC1(MDIO), PC14(TXD0), PC15(TXD1), PD0(MDC), PD2(TX_CLK)
     * All in AF mode 11 (Ethernet).
     */
    gpio_config(0, 1, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH,
                GPIO_PUPD_NONE, 11);
    gpio_config(0, 2, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH,
                GPIO_PUPD_NONE, 11);
    gpio_config(0, 3, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH,
                GPIO_PUPD_NONE, 11);
    gpio_config(0, 7, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH,
                GPIO_PUPD_NONE, 11);
    gpio_config(2, 1, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH,
                GPIO_PUPD_NONE, 11);
    gpio_config(2, 14, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH,
                GPIO_PUPD_NONE, 11);
    gpio_config(2, 15, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH,
                GPIO_PUPD_NONE, 11);
    gpio_config(3, 0, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH,
                GPIO_PUPD_NONE, 11);
    gpio_config(3, 2, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH,
                GPIO_PUPD_NONE, 11);

    /* USART1: PA8(TX, AF7), PA9(RX, AF7) */
    gpio_config(0, 8, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_VHIGH,
                GPIO_PUPD_NONE, 7);
    gpio_config(0, 9, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_VHIGH,
                GPIO_PUPD_PULLUP, 7);

    /* USART3: PB10(TX, AF7), PB11(RX, AF7) */
    gpio_config(1, 10, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH,
                GPIO_PUPD_NONE, 7);
    gpio_config(1, 11, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH,
                GPIO_PUPD_PULLUP, 7);

    /* I2C2: PB12(SCL, AF4), PB13(SDA, AF4) */
    gpio_config(1, 12, GPIO_MODE_AF, GPIO_OTYPE_OD, GPIO_OSPEED_HIGH,
                GPIO_PUPD_PULLUP, 4);
    gpio_config(1, 13, GPIO_MODE_AF, GPIO_MODE_AF, GPIO_OSPEED_HIGH,
                GPIO_PUPD_PULLUP, 4);

    /* DAC1_OUT1: PA4 (analog mode) */
    gpio_config(0, 4, GPIO_MODE_ANALOG, 0, 0, GPIO_PUPD_NONE, 0);

    /* TIM8_CH2 (1-PPS input): PC8 (AF3) */
    gpio_config(2, 8, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH,
                GPIO_PUPD_NONE, 3);

    /* Mode button: PB14 (input, pull-up) */
    gpio_config(1, 14, GPIO_MODE_INPUT, 0, 0, GPIO_PUPD_PULLUP, 0);

    /* LED PWM: PB0(TIM3_CH3, AF2), PB1(TIM3_CH4, AF2),
     *         PB6(TIM4_CH1, AF2), PB7(TIM4_CH2, AF2)
     */
    gpio_config(1, 0, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH,
                GPIO_PUPD_NONE, 2);
    gpio_config(1, 1, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH,
                GPIO_PUPD_NONE, 2);
    gpio_config(1, 6, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH,
                GPIO_PUPD_NONE, 2);
    gpio_config(1, 7, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH,
                GPIO_PUPD_NONE, 2);

    /* GNSS power enable: PC9 (output, push-pull) */
    gpio_config(2, 9, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_OSPEED_LOW,
                GPIO_PUPD_NONE, 0);
    GPIOC->BSRR = (1 << 9);   /* enable GNSS power */
}

static void usart1_init(void)
{
    /* Disable USART */
    USART1->CR1 = 0;
    /* Set baud rate: APB2 = 120 MHz, baud = 2 Mbps
     * BRR = APB2 / baud = 120000000 / 2000000 = 60
     */
    USART1->BRR = 60;
    /* Enable TX, RX, RXNE interrupt */
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE |
                   USART_CR1_RXNEIE;
    nvic_enable(82);  /* USART1_IRQn = 82 on H7 */
}

static void usart3_init(void)
{
    USART3->CR1 = 0;
    USART3->BRR = 120000000UL / GNSS_UART_BAUD;  /* e.g., 38400 → 3125 */
    USART3->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE |
                   USART_CR1_RXNEIE;
    nvic_enable(39);  /* USART3_IRQn */
}

static void dac_init(void)
{
    /* Enable DAC1 channel 1 */
    DAC1->CR = 0;
    DAC1->CR = DAC_CR_EN1 | (7 << 3);  /* EN1 + SW trigger select */
    DAC1->DHR12R1 = TCXO_DAC_CENTER;   /* center the TCXO */
    DAC1->SWTRIGR = 1;
}

static void i2c2_init(void)
{
    /* Configure I2C2 at 100 kHz (standard mode)
     * TIMINGR: for 120 MHz APB1 → ~100 kHz. Simplified.
     */
    I2C2->TIMINGR = 0x307075B1u;  /* standard 100 kHz from 120 MHz */
    I2C2->CR1 = (1 << 0);   /* PE */
}

static void eth_init(void)
{
    /* Configure Ethernet MAC in RMII mode, promiscuous (for sniffing) */
    ETH_MAC->MACCR = ETH_MACCR_FES | ETH_MACCR_DM | ETH_MACCR_RE |
                      ETH_MACCR_TE;
    /* Promiscuous mode: pass all frames to host */
    ETH_MAC->MACFFR = (1 << 4);   /* PR (promiscuous) */

    /* Configure DMA */
    ETH_DMA->DMABMR = (1 << 0);   /* SWR — software reset */
    /* Wait for reset to complete */
    while (ETH_DMA->DMABMR & 1) ;
    ETH_DMA->DMAOMR = ETH_DMAOMR_SR | ETH_DMAOMR_ST;

    /* In real HW we'd set up RX/TX descriptor rings here */
}

static void systick_init(void)
{
    /* 1 ms tick at 240 MHz HCLK */
    SYST_RVR = 240000 - 1;
    SYST_CVR = 0;
    SYST_CSR = 0x7;   /* CLKSOURCE=HCLK, TICKINT=1, ENABLE=1 */
}

/* --------------------------------------------------------------------- */
/*  LED control (PWM-based dimming)                                        */
/* --------------------------------------------------------------------- */
static void leds_init(void)
{
    /* TIM3 CH3/CH4 for LED1/LED2, TIM4 CH1/CH2 for LED3/LED4
     * PWM at ~1 kHz (PSC=0, ARR=24000 for 240MHz APB1/2)
     */
    TIM3->PSC = 0;
    TIM3->ARR = 24000;
    TIM3->CCMR1 = (TIM_CCMR_OCxM_PWM1 << 4) | (TIM_CCMR_OCxM_PWM1 << 12);
    TIM3->CCER = (1 << 8) | (1 << 12);   /* CC3E + CC4E */
    TIM3->CCR3 = 0;  /* LED1 off */
    TIM3->CCR4 = 0;
    TIM3->CR1 = TIM_CR1_CEN | TIM_CR1_ARPE;

    TIM4->PSC = 0;
    TIM4->ARR = 24000;
    TIM4->CCMR1 = (TIM_CCMR_OCxM_PWM1 << 4) | (TIM_CCMR_OCxM_PWM1 << 12);
    TIM4->CCER = (1 << 0) | (1 << 4);
    TIM4->CCR1 = 0;  /* LED3 off */
    TIM4->CCR2 = 0;
    TIM4->CR1 = TIM_CR1_CEN | TIM_CR1_ARPE;
}

static void led_set(uint8_t idx, uint16_t brightness)
{
    switch (idx) {
    case LED_LINK_A:    TIM3->CCR3 = brightness; break;
    case LED_LINK_B:    TIM3->CCR4 = brightness; break;
    case LED_PTP_LOCK:  TIM4->CCR1 = brightness; break;
    case LED_BLE_ACTIVE: TIM4->CCR2 = brightness; break;
    }
}

/* --------------------------------------------------------------------- */
/*  IMU read (LSM6DSO over I2C2)                                          */
/* --------------------------------------------------------------------- */
static int imu_read_accel(int16_t *x, int16_t *y, int16_t *z)
{
    /* LSM6DSO OUTX_L_A = 0x28, OUTX_H_A = 0x29, etc.
     * Read 6 bytes starting at 0x28.
     * Return values in mg (milli-g).
     */
    /* In real HW: I2C2 write 0x6A<<1 | 0x28, then read 6 bytes.
     * For this exercise we return zeros (no real hardware).
     */
    *x = 0; *y = 0; *z = 1000;  /* ~1 g on Z (resting flat) */
    return 0;
}

/* --------------------------------------------------------------------- */
/*  BLE UART TX pump                                                       */
/* --------------------------------------------------------------------- */
static void ble_uart_tx(const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        while (!(USART1->ISR & USART_ISR_TXE)) ;
        USART1->TDR = data[i];
    }
}

/* --------------------------------------------------------------------- */
/*  Process captured frame (send to app via BLE)                         */
/* --------------------------------------------------------------------- */
static void send_captured_frame(const uint8_t *frame, uint32_t len,
                                 uint64_t ts_ns)
{
    /* Send EVT_FRAME_CAPTURED: [ts_ns:8][len:2][frame:<=128] */
    if (len > 128) len = 128;
    uint8_t buf[140];
    memcpy(buf, &ts_ns, 8);
    buf[8] = (uint8_t)(len >> 8);
    buf[9] = (uint8_t)(len & 0xFF);
    memcpy(&buf[10], frame, len);
    ble_link_send_event(&g_ble, EVT_FRAME_CAPTURED, buf, 10 + len);
}

/* --------------------------------------------------------------------- */
/*  Main                                                                   */
/* --------------------------------------------------------------------- */
int main(void)
{
    /* Initialize hardware */
    clock_init();
    gpio_init();
    dac_init();
    i2c2_init();
    eth_init();
    usart1_init();
    usart3_init();
    leds_init();
    systick_init();

    /* Initialize subsystems */
    ptp_engine_init(&g_ptp);
    ntp_engine_init(&g_ntp);
    eth_bridge_init(&g_bridge);
    tcxo_init(&g_tcxo);
    covert_init(&g_covert);
    ble_link_init(&g_ble);
    tamper_init(&g_tamper);

    /* Default mode: passive sniff */
    g_mode = MODE_PASSIVE_SNIFF;
    g_ptp.mode = MODE_PASSIVE_SNIFF;

    /* Enable global interrupts */
    __asm volatile ("cpsie i");

    uint32_t last_tick_ms = 0;
    uint32_t last_tamper_ms = 0;
    uint32_t last_status_ms = 0;

    /* Main loop */
    while (1) {
        uint32_t now_ms = g_sys_ms;

        /* --- Process BLE RX --- */
        while (g_ble_rx_tail != g_ble_rx_head) {
            uint8_t b = g_ble_rx_ring[g_ble_rx_tail];
            g_ble_rx_tail = (g_ble_rx_tail + 1) & 0xFF;
            ble_link_uart_rx_byte(&g_ble, b);
        }
        /* Dispatch complete frames */
        ble_link_dispatch(&g_ble, &g_ptp, &g_ntp, &g_covert,
                          (uint8_t *)&g_mode);

        /* --- Pump BLE TX --- */
        if (g_ble.tx_len > 0) {
            uint8_t txbuf[BLE_UART_BUF_SIZE];
            int n = ble_link_tx_drain(&g_ble, txbuf, sizeof(txbuf));
            if (n > 0)
                ble_uart_tx(txbuf, n);
        }

        /* --- PTP engine tick --- */
        ptp_engine_tick(&g_ptp, now_ms);

        /* --- Generate Announce frames (rogue-GM mode) --- */
        if (g_ptp.mode == MODE_ROGUE_GM) {
            uint32_t alen = ptp_engine_generate_announce(&g_ptp,
                                                          g_announce_frame,
                                                          sizeof(g_announce_frame),
                                                          now_ms);
            if (alen > 0) {
                /* Transmit on Port B (simplified: write to ETH TX) */
                /* In real HW: set up TX descriptor and trigger DMA */
                (void)alen;
            }
        }

        /* --- Process received frames (simplified polling) --- */
        /* In real HW, we'd poll the ETH DMA descriptor ring here.
         * For this exercise we simulate frame processing with a
         * placeholder that runs the engine logic.
         *
         * Frame flow:
         *   1. ETH DMA receives frame → g_rx_frame_a
         *   2. ptp_engine_rx_frame() inspects/modifies → g_tx_frame
         *   3. eth_bridge_forward() decides which port to send to
         *   4. ETH DMA transmits g_tx_frame
         *
         * The actual DMA ring management is omitted for brevity
         * but the engine logic is fully exercised by the unit tests.
         */
        (void)g_rx_frame_a;
        (void)g_rx_frame_b;
        (void)g_tx_frame;

        /* --- Periodic status update to app (1 Hz) --- */
        if (now_ms - last_status_ms >= 1000) {
            last_status_ms = now_ms;
            /* Send status if BLE is connected */
            if (g_ble.connected) {
                uint8_t status[26];
                status[0] = (uint8_t)g_mode;
                status[1] = (uint8_t)g_ptp.mode;
                status[2] = g_ptp.skew.active ? 1 : 0;
                status[3] = (uint8_t)g_ptp.skew.profile;
                int64_t skew = g_ptp.current_skew_ns;
                memcpy(&status[4], &skew, 8);
                status[12] = g_ptp.observed_gm.grandmaster_priority1;
                status[13] = g_ptp.observed_gm.grandmaster_clock_quality.clock_class;
                status[14] = g_ptp.spoofed_gm.grandmaster_priority1;
                status[15] = g_ptp.spoofed_gm.grandmaster_clock_quality.clock_class;
                uint32_t capt = g_ptp.frames_captured;
                uint32_t mod = g_ptp.frames_modified;
                memcpy(&status[16], &capt, 4);
                memcpy(&status[20], &mod, 4);
                status[24] = (uint8_t)(g_ntp.requests_received & 0xFF);
                status[25] = (uint8_t)(g_ntp.responses_sent & 0xFF);
                ble_link_send_event(&g_ble, EVT_STATUS, status, 26);
            }

            /* Update LEDs */
            led_set(LED_BLE_ACTIVE, g_ble.connected ? 12000 : 0);
            led_set(LED_PTP_LOCK, g_ptp.mode == MODE_ROGUE_GM ? 12000 : 0);
        }

        /* --- Tamper check (10 Hz) --- */
        if (now_ms - last_tamper_ms >= 100) {
            last_tamper_ms = now_ms;
            int16_t ax, ay, az;
            imu_read_accel(&ax, &ay, &az);
            tamper_sample(&g_tamper, ax, ay, az);
            if (tamper_check(&g_tamper)) {
                /* Tamper detected — zeroize */
                tamper_zeroize(&g_tamper);
                covert_init(&g_covert);          /* clear covert buffers */
                g_ptp.mode = MODE_TRANSPARENT_ONLY;  /* go benign */
                g_ptp.skew.active = 0;
                g_ntp.skew_cfg.active = 0;
                ble_link_send_event(&g_ble, EVT_TAMPER,
                                     (uint8_t *)"TAMPER", 6);
            }
        }

        /* --- TCXO discipline tick --- */
        tcxo_discipline_tick(&g_tcxo, now_ms);

        /* --- Drain covert RX to app --- */
        if (g_covert.rx_len > 0) {
            uint8_t buf[COVERT_MAX_MSG];
            uint16_t n = covert_rx_drain(&g_covert, buf, sizeof(buf));
            if (n > 0)
                ble_link_send_event(&g_ble, EVT_COVERT_RX, buf, n);
        }

        /* --- Sleep until next interrupt (WFI) --- */
        __asm volatile ("wfi");
    }

    return 0;
}