/*
 * main.c — Powerline-Reaper firmware entry point + cooperative scheduler
 *
 * Hardware: STM32H743 + QCA7420 PLC + nRF52840 BLE + MicroSD + W25Q128 + MAX17048
 *
 * This is bare-metal C with a cooperative state-machine scheduler. The QCA7420
 * DATA_IND IRQ is the only priority interrupt (it must be serviced within one
 * beacon interval ~40ms; the capture path targets <10us jitter for SNR tagging).
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"
#include "drivers/qca7420.h"
#include "drivers/plc_capture.h"
#include "drivers/plc_topo.h"
#include "drivers/nmk_attack.h"
#include "drivers/ble_uart.h"
#include "drivers/sd_pcap.h"
#include "drivers/usb_cdc_ecm.h"
#include "drivers/flash.h"
#include "drivers/fuel_gauge.h"
#include "drivers/chacha20poly1305.h"

/* ---- Forward declarations ---- */
static void clock_init(void);
static void gpio_init(void);
static void nvic_init(void);
static void led_init(void);
static void led_set_rgb(uint16_t r, uint16_t g, uint16_t b);
static void tamper_isr(void);
static void button_isr(void);
static void zeroize(void);

/* ---- Global state ---- */
volatile board_mode_t  g_mode     = MODE_INIT;
volatile uint32_t      g_caps     = 0;
volatile uint32_t      g_uptime_ms = 0;
volatile uint32_t      g_plc_irq_count = 0;
volatile int            g_tampered = 0;

/* SysTick 1ms counter */
static volatile uint32_t systick_ms = 0;

/* ---- SysTick ---- */
void SysTick_Handler(void) {
    systick_ms++;
    g_uptime_ms = systick_ms;
}

static uint32_t millis(void) {
    return systick_ms;
}

static uint32_t elapsed_since(uint32_t t0) {
    return systick_ms - t0;
}

/* ---- Millis delay ---- */
static void delay_ms(uint32_t ms) {
    uint32_t t0 = systick_ms;
    while ((systick_ms - t0) < ms) { /* spin */ }
}

/* ---- EXTI ISR dispatch ---- */
/* PLC IRQ on PD7 -> EXTI9_5
 * BLE IRQ on PC14 -> EXTI15_10
 * Button on PC4 -> EXTI4
 * Tamper on PC0 -> EXTI0
 */
void EXTI0_IRQHandler(void) { /* Tamper */
    /* PC0 pending bit 0 */
    volatile uint32_t *exti_pr1 = (volatile uint32_t *)(EXTI_BASE + 0x88);
    if (*exti_pr1 & (1U << 0)) {
        *exti_pr1 = (1U << 0);
        tamper_isr();
    }
}

void EXTI4_IRQHandler(void) { /* Button */
    volatile uint32_t *exti_pr1 = (volatile uint32_t *)(EXTI_BASE + 0x88);
    if (*exti_pr1 & (1U << 4)) {
        *exti_pr1 = (1U << 4);
        button_isr();
    }
}

void EXTI9_5_IRQHandler(void) { /* PLC IRQ PD7 (bit 7) */
    volatile uint32_t *exti_pr1 = (volatile uint32_t *)(EXTI_BASE + 0x88);
    if (*exti_pr1 & (1U << 7)) {
        *exti_pr1 = (1U << 7);
        g_plc_irq_count++;
        qca7420_irq_handler();
    }
}

void EXTI15_10_IRQHandler(void) { /* BLE IRQ PC14 (bit 14) */
    volatile uint32_t *exti_pr1 = (volatile uint32_t *)(EXTI_BASE + 0x88);
    if (*exti_pr1 & (1U << 14)) {
        *exti_pr1 = (1U << 14);
        ble_uart_irq_handler();
    }
}

/* ---- Tamper / zeroize ---- */
static void tamper_isr(void) {
    g_tampered = 1;
    g_mode = MODE_FAULT;
    zeroize();
    led_set_rgb(0xFF, 0, 0); /* solid red = tampered */
}

static void button_isr(void) {
    static uint32_t press_t0 = 0;
    uint32_t now = systick_ms;
    if ((GPIOC->IDR) & (1U << 4)) {
        /* released — measure hold */
        uint32_t hold = now - press_t0;
        if (hold >= 6000) {
            zeroize();
        } else if (hold >= 50) {
            /* tap: cycle view mode */
            if (g_mode == MODE_SNIFF)      g_mode = MODE_BRIDGE;
            else if (g_mode == MODE_BRIDGE) g_mode = MODE_SNIFF;
            else                            g_mode = MODE_SNIFF;
        }
    } else {
        press_t0 = now;
    }
}

static void zeroize(void) {
    /* Wipe keys in BKPSRAM + crypto-erase SD */
    qca7420_set_promisc(0);
    qca7420_wipe_keys();
    nmk_attack_abort();
    sd_pcap_crypto_erase();
    flash_wipe_keys();
    memset((void *)0x38800000UL, 0, 4096); /* BKPSRAM region */
}

/* ---- LED ---- */
static void led_set_rgb(uint16_t r, uint16_t g, uint16_t b) {
    volatile uint32_t *tim_ccr = (volatile uint32_t *)((char *)TIM2_BASE + 0x34);
    tim_ccr[0] = r; /* CCR1 R */
    tim_ccr[1] = g; /* CCR2 G */
    tim_ccr[2] = b; /* CCR3 B */
}

static void led_init(void) {
    /* Enable TIM2 + GPIOA/B */
    RCC_APB1LENR_TIM2 = 1U; /* TIM2 clock enable */
    /* PWM 1 kHz, period 1000 ticks */
    volatile uint32_t *tim_cr1  = (volatile uint32_t *)(TIM2_BASE + 0x00);
    volatile uint32_t *tim_psc  = (volatile uint32_t *)(TIM2_BASE + 0x28);
    volatile uint32_t *tim_arr  = (volatile uint32_t *)(TIM2_BASE + 0x2C);
    volatile uint32_t *tim_ccmr1 = (volatile uint32_t *)(TIM2_BASE + 0x18);
    volatile uint32_t *tim_ccmr2 = (volatile uint32_t *)(TIM2_BASE + 0x1C);
    volatile uint32_t *tim_ccer  = (volatile uint32_t *)(TIM2_BASE + 0x20);
    volatile uint32_t *tim_bdtr  = (volatile uint32_t *)(TIM2_BASE + 0x44);
    *tim_psc = 240 - 1; /* 240 MHz / 240 = 1 MHz tick */
    *tim_arr = 1000 - 1; /* 1 kHz */
    /* CH1/2/3 PWM mode 1, active high */
    *tim_ccmr1 = (6U << 4) | (1U << 3) | (6U << 12) | (1U << 11); /* OC1M=PWM1, OC1PE ; OC2M=PWM1, OC2PE */
    *tim_ccmr2 = (6U << 4) | (1U << 3);                          /* OC3M=PWM1, OC3PE */
    *tim_ccer = (1U << 0) | (1U << 4) | (1U << 8);              /* CC1E CC2E CC3E */
    *tim_bdtr = (1U << 15);                                     /* MOE */
    *tim_cr1 |= (1U << 7) | (1U << 0);                         /* ARPE + CEN */
    led_set_rgb(0, 0, 0);
}

static void led_status(void) {
    /* Reflect g_mode + caps in color:
     * INIT  : dim white pulse
     * LINK  : blue
     * SNIFF : green slow blink
     * BRIDGE: cyan
     * INJECT: red blink
     * NMK   : magenta
     * TUNNEL: yellow
     * COVERT: dim green
     * FAULT : solid red
     */
    uint32_t t = systick_ms;
    uint32_t blink = (t >> 9) & 1; /* ~0.5s period */
    switch (g_mode) {
    case MODE_INIT:       led_set_rgb(40, 40, 40); break;
    case MODE_LINKING:    led_set_rgb(0, 0, 200); break;
    case MODE_SNIFF:      led_set_rgb(blink ? 0 : 200, blink ? 200 : 0, 0); break;
    case MODE_BRIDGE:     led_set_rgb(0, 180, 180); break;
    case MODE_INJECT:     led_set_rgb(blink ? 200 : 0, 0, 0); break;
    case MODE_NMK_ATTACK: led_set_rgb(200, 0, 200); break;
    case MODE_TUNNEL:     led_set_rgb(180, 180, 0); break;
    case MODE_COVERT:     led_set_rgb(0, 40, 0); break;
    case MODE_FAULT:      led_set_rgb(255, 0, 0); break;
    default:              led_set_rgb(0, 0, 0); break;
    }
}

/* ---- Clock tree ---- */
static void clock_init(void) {
    /* HSE 25 MHz -> PLL -> 480 MHz SYS (VOS0)
     * Simplified — full sequence in the SDK startup; this sets the key fields.
     */
    volatile uint32_t *rcc_cr   = (volatile uint32_t *)(RCC_BASE + 0x00);
    volatile uint32_t *rcc_pll1 = (volatile uint32_t *)(RCC_BASE + 0x06C);
    volatile uint32_t *rcc_cfgr = (volatile uint32_t *)(RCC_BASE + 0x10);
    /* Enable HSE and wait */
    *rcc_cr |= (1U << 16); /* HSEON */
    while (!(*rcc_cr & (1U << 17))) { } /* HSERDY */
    /* PLL1 config: M=5 (25/5=5), N=192 (5*192=960), P=2 -> 480 MHz, Q=8, R=8 */
    *rcc_pll1 = (5U << 4) | (192U << 8) | (1U << 24) /* PLL1P = /2 */ | (8U << 20);
    *rcc_cr |= (1U << 24); /* PLL1ON */
    while (!(*rcc_cr & (1U << 25))) { } /* PLL1RDY */
    /* Voltage scale VOS0 (max perf) */
    volatile uint32_t *pwr_d3cr = (volatile uint32_t *)(PWR_BASE + 0x18);
    *pwr_d3cr |= (3U << 14);
    while (!(*pwr_d3cr & (1U << 13))) { }
    /* Flash latency: 8 wait states @ 240 MHz HCLK */
    *(volatile uint32_t *)0x52002020 = 8;
    /* Switch SYSCLK to PLL1 */
    *rcc_cfgr = (*rcc_cfgr & ~0x7) | 3U;
    while (((*rcc_cfgr >> 3) & 0x7) != 3) { }
    /* APB dividers: HCLK=/2 (240), APB1=/2 (120), APB2=/2, APB4=/2 */
    volatile uint32_t *rcc_d1cfgr = (volatile uint32_t *)(RCC_BASE + 0x18);
    volatile uint32_t *rcc_d2cfgr = (volatile uint32_t *)(RCC_BASE + 0x1C);
    volatile uint32_t *rcc_d3cfgr = (volatile uint32_t *)(RCC_BASE + 0x20);
    *rcc_d1cfgr = (*rcc_d1cfgr & ~0xF) | (8U << 0); /* HPRE=/2 */
    *rcc_d2cfgr = (*rcc_d2cfgr & ~0x07) | (4U << 0) /* APB1=/2 */
                | ((4U << 8) /* APB2=/2 */);
    *rcc_d3cfgr = (*rcc_d3cfgr & ~0x7) | (4U << 0); /* APB4=/2 */

    /* Enable peripheral clocks */
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOA | RCC_AHB4ENR_GPIOB
                  | RCC_AHB4ENR_GPIOC | RCC_AHB4ENR_GPIOD;
    RCC_AHB1ENR |= RCC_AHB1ENR_DMA1 | RCC_AHB1ENR_DMAMUX1
                  | RCC_AHB1ENR_USB1OTGHS;
    RCC_AHB3ENR |= RCC_AHB3ENR_QSPI;
    RCC_APB1ENR |= RCC_APB1ENR_SPI2;
    RCC_APB1ENR1 |= RCC_APB1ENR1_USART3 | RCC_APB1ENR1_I2C1;
    RCC_APB1LENR_TIM2 = 1U;
    RCC_APB4ENR |= RCC_APB4ENR_SDMMC2 | RCC_APB4ENR_SYSCFG;
    RCC_AHB2ENR |= RCC_AHB2ENR_ADC12;

    /* SysTick 1 ms at 240 MHz HCLK */
    *(volatile uint32_t *)0xE000E014 = 240000 - 1; /* LOAD */
    *(volatile uint32_t *)0xE000E018 = 0;          /* VAL */
    *(volatile uint32_t *)0xE000E010 = 0x7;        /* CLKSOURCE=AHB, TICKINT, ENABLE */
}

/* ---- GPIO init ---- */
static void gpio_init(void) {
    /* Helper to set mode/af/speed/pupd on a pin */
    /* Quick setup for the critical pins: PLC SPI2 on PD3-PD6 (AF5), IRQ PD7 (in),
     * BLE UART3 PC10/11 (AF7), SDMMC2 on PD6/PD7/PB14/15/4/5 (AF12),
     * QSPI on PB2/PB6/PC9/PC10/PD11/12 (AF9),
     * LED PWM PA15/PB3/PB10 (AF1), button PC4 (in), tamper PC0 (in),
     * current ADC PA0 (analog), USB PA11/12 (AF10), I2C1 PB8/9 (AF4).
     */

    /* Set PC4 button input + pull-up */
    GPIOC->PUPDR = (GPIOC->PUPDR & ~(3U << (4*2))) | (1U << (4*2));
    GPIOC->MODER &= ~(3U << (4*2)); /* input */

    /* PC0 tamper input + pull-up */
    GPIOC->PUPDR = (GPIOC->PUPDR & ~(3U << (0*2))) | (1U << (0*2));
    GPIOC->MODER &= ~(3U << (0*2));

    /* PA0 analog (current sense) */
    GPIOA->MODER |= (3U << (0*2)); /* analog */
    GPIOA->PUPDR &= ~(3U << (0*2));

    /* PA11/PA12 USB AF10 */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (11*2))) | (2U << (11*2));
    GPIOA->AFRH = (GPIOA->AFRH & ~(0xFU << ((11-8)*4))) | (10U << ((11-8)*4));
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (12*2))) | (2U << (12*2));
    GPIOA->AFRH = (GPIOA->AFRH & ~(0xFU << ((12-8)*4))) | (10U << ((12-8)*4));

    /* PD9 PLC LDO enable (output, low = off) */
    GPIOD->MODER |= (1U << (9*2));
    GPIOD->ODR &= ~(1U << 9);

    /* PC13 BLE RST (output, high = run) */
    GPIOC->MODER |= (1U << (13*2));
    GPIOC->ODR |= (1U << 13);
}

static void nvic_init(void) {
    /* Enable EXTI0 (tamper, IRQ 6), EXTI4 (button, IRQ 23),
     * EXTI9_5 (PLC, IRQ 23? actually IRQ 23 is EXTI9_5), EXTI15_10 (BLE, IRQ 40)
     */
    volatile uint32_t *nvic_iser0 = (volatile uint32_t *)0xE000E100;
    volatile uint32_t *nvic_iser1 = (volatile uint32_t *)0xE000E104;
    /* EXTI0  = IRQ6  -> ISER0 bit 6 */
    /* EXTI4  = IRQ10 -> ISER0 bit 10 */
    /* EXTI9_5= IRQ23 -> ISER0 bit 23 */
    /* EXTI15_10 = IRQ40 -> ISER1 bit 8 */
    *nvic_iser0 = (1U << 6) | (1U << 10) | (1U << 23);
    *nvic_iser1 = (1U << 8);

    /* Priority: PLC IRQ highest */
    volatile uint8_t *nvic_ipr = (volatile uint8_t *)0xE000E400;
    nvic_ipr[23] = 0x00;   /* PLC IRQ highest priority */
    nvic_ipr[40] = 0x20;   /* BLE IRQ lower */
    nvic_ipr[6]  = 0x40;   /* Tamper */
    nvic_ipr[10] = 0x60;   /* Button lowest */
}

/* ---- PLC event callback (called from qca7420 IRQ-driven RX path) ---- */
static void on_plc_frame(const qca7420_frame_t *f) {
    /* Feed capture ring + topology + (if injecting) deauth detection */
    plc_capture_push(f);
    plc_topo_observe(f);
    if (g_mode == MODE_BRIDGE) {
        usb_cdc_ecm_inject(f);
    }
    if (g_mode == MODE_TUNNEL) {
        ble_uart_tunnel_tx(f);
    }
}

/* ---- Main scheduler ---- */
int main(void) {
    /* Boot sequence */
    clock_init();
    gpio_init();
    nvic_init();
    led_init();
    delay_ms(50);

    led_set_rgb(80, 80, 80); /* white boot */
    g_mode = MODE_INIT;

    /* Init subsystems in dependency order */
    flash_init();              /* external W25Q128 (payloads + key backup) */
    fuel_gauge_init();         /* MAX17048 */
    sd_pcap_init();            /* SD + FatFs */
    ble_uart_init();           /* nRF52840 UART */
    qca7420_init(on_plc_frame);/* QCA7420 SPI + PIB load */
    usb_cdc_ecm_init();        /* composite USB gadget */
    plc_capture_init();
    plc_topo_init();
    nmk_attack_init();

    /* Power up the PLC LDO and reset QCA7420 */
    GPIOD->ODR |= (1U << 9);   /* PLC_PWR_EN high */
    delay_ms(10);
    qca7420_reset();
    delay_ms(5);

    /* Try to sync to the PLC segment (default NMK = "HomePlugAV") */
    g_mode = MODE_LINKING;
    if (qca7420_sync(5000) == 0) {
        g_caps |= CAP_PLC_LINK;
        g_mode = MODE_SNIFF;   /* default to passive capture */
        qca7420_set_promisc(1);
    } else {
        g_mode = MODE_COVERT; /* no link → battery covert mode */
    }

    /* ---- Cooperative scheduler loop ----
     * Each subsystem gets a non-blocking "tick" call. The PLC IRQ is the
     * only preemptive handler; everything else runs to completion here.
     */
    uint32_t topo_t0 = 0, stat_t0 = 0, fuel_t0 = 0, led_t0 = 0;
    while (1) {
        uint32_t now = systick_ms;

        /* PLC driver tick: drain RX FIFO, process beacons, run rogue CCo FSM */
        qca7420_poll();

        /* Capture ring → SD flush (every 250 ms) */
        if (g_caps & CAP_SD_PRESENT) {
            sd_pcap_flush_tick(now);
        }

        /* Topology refresh (every 1 s) */
        if ((now - topo_t0) >= 1000) {
            topo_t0 = now;
            plc_topo_refresh();
        }

        /* USB bridge tick */
        usb_cdc_ecm_tick();

        /* BLE tick (covert mode: keep BLE alive, tunnel frames) */
        ble_uart_tick();

        /* NMK attack worker */
        if (g_mode == MODE_NMK_ATTACK) {
            nmk_attack_step();
        }

        /* Status / battery (every 5 s) */
        if ((now - fuel_t0) >= 5000) {
            fuel_t0 = now;
            if (fuel_gauge_voltage_mv() < 3300) {
                g_caps &= ~CAP_BATTERY_OK;
            } else {
                g_caps |= CAP_BATTERY_OK;
            }
            /* Over-temperature protection */
            int16_t t = flash_read_temp();
            if (t > 75) {
                g_mode = MODE_FAULT;
            }
        }

        /* LED update (every 100 ms) */
        if ((now - led_t0) >= 100) {
            led_t0 = now;
            led_status();
        }

        /* WFI between iterations to save power in covert mode */
        if (g_mode == MODE_COVERT || g_mode == MODE_SNIFF) {
            __asm volatile ("wfi");
        }
    }
}

/* ---- Hard fault handler ---- */
void HardFault_Handler(void) {
    /* Disable interrupts, light red LED, halt */
    __asm volatile ("cpsid i");
    led_set_rgb(255, 0, 0);
    while (1) { }
}

/* ---- Default weak handlers (so link doesn't fail) ---- */
void MemManage_Handler(void)    { while (1) { } }
void BusFault_Handler(void)     { while (1) { } }
void UsageFault_Handler(void)   { while (1) { } }
void SVC_Handler(void)          { }
void DebugMon_Handler(void)     { }
void PendSV_Handler(void)       { }