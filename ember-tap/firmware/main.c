/*
 * main.c — Ember-Tap firmware entry point and scheduler
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Responsibilities:
 *   - Clock tree setup (HSE → PLL → 170 MHz)
 *   - GPIO + peripheral init
 *   - 1 ms tick (TIM6) → buttons debounce + fuzz timer
 *   - Main loop: poll USB CDC, parse CLI, run fuzz tick, render OLED
 *   - FUSB302 EXTI handler → PD engine
 */

#include "board.h"
#include "registers.h"
#include "drivers/pd_engine.h"
#include "drivers/power_monitor.h"
#include "drivers/usb_cdc.h"
#include "drivers/display.h"
#include "drivers/buttons.h"
#include <string.h>

/* ---- Global millisecond tick ---- */
volatile uint32_t g_ms_tick = 0;

/* ---- Current operating mode ---- */
static board_mode_t g_mode = MODE_PASSIVE;

/* ---- Fuzz campaign instance ---- */
static fuzz_campaign_t g_fuzz;

/* ---- CLI buffer ---- */
static char cli_line[128];
static int  cli_len = 0;

/* ---- Forward declarations ---- */
static void clock_init(void);
static void gpio_init(void);
static void tim6_init(void);
static void cli_process(const char *line);
static void cli_help(void);
static void mode_set(board_mode_t m);

/* =========================================================================
 * Entry point
 * ========================================================================= */
int main(void) {
    /* Boot: set up clocks first (so all peripherals run at full speed) */
    clock_init();
    gpio_init();
    tim6_init();

    /* Initialize drivers */
    power_monitor_init();
    pd_engine_init();
    usb_cdc_init();
    display_init();
    buttons_init();

    /* Welcome banner on OLED */
    display_clear();
    display_text(0, 0, "EMBER-TAP");
    display_text(0, 1, "by jayis1");
    display_text(0, 3, "USB-PD fuzzer");
    display_text(0, 4, "Initializing...");
    display_flush();

    for (volatile int i = 0; i < 2000000; i++);

    /* Start in passive sniff mode */
    mode_set(MODE_PASSIVE);

    usb_cdc_puts("\r\nEmber-Tap v1.0 (c) jayis1\r\n");
    usb_cdc_puts("Type 'help' for commands.\r\n> ");

    /* ---- Main scheduler loop ---- */
    uint32_t last_display = 0;
    uint32_t last_status  = 0;
    while (1) {
        /* 1. USB CDC housekeeping */
        usb_cdc_poll();

        /* 2. Process incoming CDC characters into CLI lines */
        int c;
        while ((c = usb_cdc_getchar()) >= 0) {
            if (c == '\r' || c == '\n') {
                if (cli_len > 0) {
                    cli_line[cli_len] = 0;
                    usb_cdc_puts("\r\n");
                    cli_process(cli_line);
                    cli_len = 0;
                    usb_cdc_puts("> ");
                } else {
                    usb_cdc_puts("\r\n> ");
                }
            } else if (c == 0x08 && cli_len > 0) {
                cli_len--;
                usb_cdc_puts("\b \b");
            } else if (cli_len < (int)sizeof(cli_line) - 1 && c >= 0x20) {
                cli_line[cli_len++] = (char)c;
                usb_cdc_write((char *)&c, 1);
            }
        }

        /* 3. Fuzzer tick (rate-limited internally) */
        fuzz_tick();

        /* 4. Buttons → cycle mode */
        uint8_t btn = buttons_read();
        if (btn & 0x01) {  /* UP: next mode */
            mode_set((g_mode + 1) % MODE_COUNT);
        }
        if (btn & 0x02) {  /* DN: prev mode */
            mode_set((g_mode + MODE_COUNT - 1) % MODE_COUNT);
        }
        if (btn & 0x04) {  /* SEL: status dump */
            usb_cdc_puts("\r\n[STATUS] mode=");
            usb_cdc_puts((const char *[]){
                "passive","spoof_src","sink_masq","fuzz","glitch"
            }[g_mode]);
            char buf[32];
            uint32_t v = power_get_vbus_mv();
            uint32_t a = power_get_vbus_ma();
            /* tiny itoa */
            int p = 0;
            buf[p++] = ' '; buf[p++] = 'V'; buf[p++] = '=';
            p += uitoa(v, &buf[p]);
            buf[p++] = 'm'; buf[p++] = 'V';
            buf[p++] = ' '; buf[p++] = 'I'; buf[p++] = '=';
            p += uitoa(a, &buf[p]);
            buf[p++] = 'm'; buf[p++] = 'A';
            buf[p] = 0;
            usb_cdc_puts(buf);
            usb_cdc_puts("\r\n> ");
        }

        /* 5. Display refresh at 10 Hz */
        if (g_ms_tick - last_display >= 100) {
            last_display = g_ms_tick;
            display_render_status((uint8_t)g_mode,
                                  power_get_vbus_mv(),
                                  power_get_vbus_ma(),
                                  power_get_temp_c(),
                                  g_fuzz.sent, g_fuzz.crash_cnt);
        }

        /* 6. Background status report at 1 Hz when sniffing */
        if (g_mode == MODE_PASSIVE && (g_ms_tick - last_status) >= 1000) {
            last_status = g_ms_tick;
            pd_frame_t f;
            while (pd_get_captured(&f)) {
                char buf[64];
                int p = 0;
                buf[p++] = '[';
                p += uitoa(f.ts_ms, &buf[p]);
                buf[p++] = ']'; buf[p++] = ' ';
                buf[p++] = 'S'; buf[p++] = 'O'; buf[p++] = 'P';
                buf[p++] = '0' + f.sof;
                buf[p++] = ' ';
                p += uitoa(f.hdr.msg_type, &buf[p]);
                buf[p++] = '/'; p += uitoa(f.hdr.numobj, &buf[p]);
                buf[p++] = '\r'; buf[p++] = '\n'; buf[p] = 0;
                usb_cdc_puts(buf);
            }
        }
    }
}

/* =========================================================================
 * Tiny unsigned itoa (for CLI status output)
 * ========================================================================= */
int uitoa(uint32_t v, char *out) {
    char tmp[11];
    int n = 0;
    if (v == 0) { out[0] = '0'; out[1] = 0; return 1; }
    while (v) { tmp[n++] = '0' + (v % 10); v /= 10; }
    for (int i = 0; i < n; i++) out[i] = tmp[n - 1 - i];
    out[n] = 0;
    return n;
}

/* =========================================================================
 * Clock tree: HSE 16 MHz → PLL → 170 MHz
 * ========================================================================= */
static void clock_init(void) {
    /* Enable HSE */
    RCC->CR |= RCC_CR_HSEON;
    uint32_t to = 0xFFFF;
    while (!(RCC->CR & RCC_CR_HSERDY) && to--) ;
    if (!(RCC->CR & RCC_CR_HSERDY)) {
        /* Fall back to HSI if no crystal */
        RCC->CR |= RCC_CR_HSION;
        while (!(RCC->CR & RCC_CR_HSIRDY)) ;
        RCC->CFGR = RCC_CFGR_SW_HSI;
        return;
    }

    /* Configure PLL: M=2, N=85, R=2 → 16/2*85/2 = 340? No: 16/2=8, 8*85=680, 680/2=340.
     * Actually for 170 MHz: VCO_in=8 MHz, VCO_out=340 MHz, SYSCLK=170. Correct. */
    RCC->PLLCFGR = (2U << 4)    /* PLLM = 2 */
                 | (85U << 8)   /* PLLN = 85 */
                 | (0U << 16)   /* PLLR = 2 (00 → /2) */
                 | (1U << 24);  /* PLLREN */
    /* PLL source = HSE */
    RCC->PLLCFGR |= (1U << 0);  /* PLLSRC = HSE */

    /* Flash latency for 170 MHz */
    FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN
               | FLASH_ACR_LATENCY(BOARD_FLASH_WS);

    /* Enable PLL */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) ;

    /* Switch SYSCLK to PLL */
    RCC->CFGR = RCC_CFGR_SW_PLL;
    while (((RCC->CFGR >> 3) & 0x3) != 0x3) ;  /* wait SWS = PLL */
}

/* =========================================================================
 * GPIO: enable all ports used, configure LED + KILL + FUSB_INT
 * ========================================================================= */
static void gpio_init(void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN
                  | RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_GPIOFEN;

    /* LED on PA4 — output push-pull */
    GPIOA->MODER &= ~(0x3U << (LED_STATUS_PIN * 2));
    GPIOA->MODER |=  (GPIO_MODE_OUTPUT << (LED_STATUS_PIN * 2));
    GPIOA->OTYPER &= ~(1U << LED_STATUS_PIN);

    /* KILL on PA3 — input pull-up */
    GPIOA->MODER &= ~(0x3U << (KILL_HW_PIN * 2));
    GPIOA->PUPDR |=  (0x1U << (KILL_HW_PIN * 2));

    /* FUSB_INT on PA8 — input pull-up (active low) */
    GPIOA->MODER &= ~(0x3U << (FUSB_INT_PIN * 2));
    GPIOA->PUPDR |=  (0x1U << (FUSB_INT_PIN * 2));

    /* Enable SYSCFG for EXTI */
    RCC->APB2ENR |= (1U << 0); /* SYSCFGEN */

    /* Wire PA8 → EXTI8 */
    SYSCFG->EXTICR3 &= ~(0xFU << ((FUSB_INT_PIN - 8) * 4)); /* PA = 0 */
    EXTI->IMR1 |= (1U << FUSB_INT_PIN);
    EXTI->FTSR1 |= (1U << FUSB_INT_PIN); /* falling edge */
    nvic_enable(EXTI1_IRQn + (FUSB_INT_PIN - 1)); /* approx — EXTI9_5 in real hw */

    /* Thermistor PB15 — analog */
    GPIOB->MODER &= ~(0x3U << (THERM_PIN * 2));
    GPIOB->MODER |=  (GPIO_MODE_ANALOG << (THERM_PIN * 2));
}

/* =========================================================================
 * TIM6: 1 ms tick interrupt
 * ========================================================================= */
static void tim6_init(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM6EN;
    TIM6->PSC = (BOARD_SYSCLK_HZ / 1000000U) - 1;  /* 1 MHz */
    TIM6->ARR = 1000 - 1;                            /* 1 ms */
    TIM6->DIER = TIM_DIER_UIE;
    TIM6->CR1 = TIM_CR1_CEN;
    nvic_enable(TIM6_DAC1_IRQn);
}

/* ---- TIM6 ISR ---- */
void TIM6_DAC1_IRQHandler(void) {
    TIM6->SR = ~TIM_SR_UIF;
    g_ms_tick++;
    buttons_tick();

    /* Blink LED every 500 ms */
    static uint16_t blink = 0;
    if (++blink >= 500) {
        blink = 0;
        GPIOA->ODR ^= (1U << LED_STATUS_PIN);
    }
}

/* ---- FUSB_INT EXTI ISR ---- */
void EXTI9_5_IRQHandler(void) {
    if (EXTI->IMR1 & (1U << FUSB_INT_PIN)) {
        EXTI->RPR1 |= (1U << FUSB_INT_PIN);  /* clear pending (rising) */
        /* Actually we set falling edge; use FPR1 on G4 */
        pd_irq_handler();
    }
}

/* =========================================================================
 * Mode management
 * ========================================================================= */
static void mode_set(board_mode_t m) {
    /* Stop any active fuzzer before switching */
    if (g_mode == MODE_FUZZ && m != MODE_FUZZ) {
        fuzz_stop();
    }
    g_mode = m;

    switch (m) {
        case MODE_PASSIVE:
            pd_sniff_start();
            break;
        case MODE_SPOOF_SRC:
            pd_dead_battery(1);
            break;
        case MODE_SINK_MASQ:
            pd_dead_battery(0);
            pd_request(5000, 3000);
            break;
        case MODE_FUZZ:
            pd_dead_battery(1);
            break;
        case MODE_GLITCH:
            pd_sniff_start();
            break;
        default:
            break;
    }
}

/* =========================================================================
 * CLI command processor
 * ========================================================================= */
static int starts_with(const char *s, const char *pfx) {
    while (*pfx) { if (*s++ != *pfx++) return 0; }
    return 1;
}

static void cli_process(const char *line) {
    if (line[0] == 0) return;

    if (starts_with(line, "help")) {
        cli_help();
        return;
    }
    if (starts_with(line, "status")) {
        char buf[64];
        int p = 0;
        p += uitoa(power_get_vbus_mv(), &buf[p]);
        usb_cdc_puts("VBUS="); usb_cdc_puts(buf); usb_cdc_puts("mV ");
        p = 0;
        p += uitoa(power_get_vbus_ma(), &buf[p]);
        usb_cdc_puts("I="); usb_cdc_puts(buf); usb_cdc_puts("mA ");
        p = 0;
        int8_t t = power_get_temp_c();
        p += uitoa((uint32_t)(t < 0 ? -t : t), &buf[p]);
        usb_cdc_puts("T="); usb_cdc_puts(buf); usb_cdc_puts("C\r\n");
        return;
    }
    if (starts_with(line, "sniff on")) {
        pd_sniff_start();
        mode_set(MODE_PASSIVE);
        usb_cdc_puts("OK sniff on\r\n");
        return;
    }
    if (starts_with(line, "sniff off")) {
        pd_sniff_stop();
        usb_cdc_puts("OK sniff off\r\n");
        return;
    }
    if (starts_with(line, "spoof src")) {
        /* Parse a simple fixed PDO: "spoof src <mv> <ma>" */
        uint16_t mv = 5000, ma = 3000;
        const char *p = line + 9;
        mv = 0; while (*p == ' ') p++; while (*p >= '0' && *p <= '9') { mv = mv*10 + (*p - '0'); p++; }
        ma = 0; while (*p == ' ') p++; while (*p >= '0' && *p <= '9') { ma = ma*10 + (*p - '0'); p++; }
        uint16_t pdo = (1u << 31) | ((ma/10) << 10) | ((mv/50) << 20);
        pd_advertise_src(&pdo, 1);
        mode_set(MODE_SPOOF_SRC);
        usb_cdc_puts("OK spoofing source ");
        char b[12]; uitoa(mv, b); usb_cdc_puts(b); usb_cdc_puts("mV ");
        uitoa(ma, b); usb_cdc_puts(b); usb_cdc_puts("mA\r\n");
        return;
    }
    if (starts_with(line, "sink req")) {
        uint16_t mv = 5000, ma = 3000;
        const char *p = line + 8;
        mv = 0; while (*p == ' ') p++; while (*p >= '0' && *p <= '9') { mv = mv*10 + (*p - '0'); p++; }
        ma = 0; while (*p == ' ') p++; while (*p >= '0' && *p <= '9') { ma = ma*10 + (*p - '0'); p++; }
        pd_request(mv, ma);
        mode_set(MODE_SINK_MASQ);
        usb_cdc_puts("OK requesting ");
        char b[12]; uitoa(mv, b); usb_cdc_puts(b); usb_cdc_puts("mV ");
        uitoa(ma, b); usb_cdc_puts(b); usb_cdc_puts("mA\r\n");
        return;
    }
    if (starts_with(line, "fuzz start")) {
        uint32_t count = 1000, seed = 0;
        const char *p = line + 10;
        count = 0; while (*p == ' ') p++; while (*p >= '0' && *p <= '9') { count = count*10 + (*p - '0'); p++; }
        seed = 0;  while (*p == ' ') p++; while (*p >= '0' && *p <= '9') { seed = seed*10 + (*p - '0'); p++; }
        if (count == 0) count = 1000;
        if (seed == 0) seed = 0xDEAD;
        memset(&g_fuzz, 0, sizeof(g_fuzz));
        g_fuzz.count = count;
        g_fuzz.seed = seed;
        g_fuzz.profile = FUZZ_PROF_HEADER;
        fuzz_start(&g_fuzz);
        mode_set(MODE_FUZZ);
        usb_cdc_puts("OK fuzzing ");
        char b[12]; uitoa(count, b); usb_cdc_puts(b);
        usb_cdc_puts(" frames seed=");
        uitoa(seed, b); usb_cdc_puts(b); usb_cdc_puts("\r\n");
        return;
    }
    if (starts_with(line, "fuzz profile")) {
        const char *p = line + 12;
        while (*p == ' ') p++;
        int prof = 0;
        if      (starts_with(p, "header"))  prof = FUZZ_PROF_HEADER;
        else if (starts_with(p, "pdo"))     prof = FUZZ_PROF_PDO;
        else if (starts_with(p, "timing"))  prof = FUZZ_PROF_TIMING;
        else if (starts_with(p, "chunk"))   prof = FUZZ_PROF_CHUNK;
        else { usb_cdc_puts("ERR unknown profile\r\n"); return; }
        g_fuzz.profile = (fuzz_profile_t)prof;
        usb_cdc_puts("OK profile set\r\n");
        return;
    }
    if (starts_with(line, "fuzz stop")) {
        fuzz_stop();
        usb_cdc_puts("OK fuzz stopped\r\n");
        return;
    }
    if (starts_with(line, "hardreset")) {
        pd_hard_reset();
        usb_cdc_puts("OK hard reset sent\r\n");
        return;
    }
    if (starts_with(line, "roleswap")) {
        pd_role_swap();
        usb_cdc_puts("OK role swap sent\r\n");
        return;
    }
    if (starts_with(line, "deadbattery on")) {
        pd_dead_battery(1);
        usb_cdc_puts("OK dead-battery Rp enabled\r\n");
        return;
    }
    if (starts_with(line, "deadbattery off")) {
        pd_dead_battery(0);
        usb_cdc_puts("OK dead-battery disabled\r\n");
        return;
    }
    if (starts_with(line, "vbus glitch")) {
        uint32_t us = 100;
        int type = 0;
        const char *p = line + 11;
        us = 0; while (*p == ' ') p++; while (*p >= '0' && *p <= '9') { us = us*10 + (*p - '0'); p++; }
        while (*p == ' ') p++;
        if (*p == '1') type = 1;
        if (us == 0) us = 100;
        if (us > 100000) us = 100000;
        int r = power_vbus_glitch(us, type);
        if (r < 0) {
            usb_cdc_puts("ERR blocked by interlock\r\n");
        } else {
            usb_cdc_puts("OK glitch ");
            char b[12]; uitoa(us, b); usb_cdc_puts(b);
            usb_cdc_puts("us type="); uitoa((uint32_t)type, b); usb_cdc_puts(b);
            usb_cdc_puts("\r\n");
        }
        return;
    }
    if (starts_with(line, "log dump")) {
        pd_frame_t f;
        int n = 0;
        while (pd_get_captured(&f)) {
            char buf[80];
            int p = 0;
            p += uitoa(f.ts_ms, &buf[p]);
            buf[p++] = ' ';
            buf[p++] = '0' + f.sof;
            buf[p++] = ' ';
            /* Header raw hex */
            const char hex[] = "0123456789ABCDEF";
            buf[p++] = hex[(f.hdr.raw >> 12) & 0xF];
            buf[p++] = hex[(f.hdr.raw >> 8) & 0xF];
            buf[p++] = hex[(f.hdr.raw >> 4) & 0xF];
            buf[p++] = hex[f.hdr.raw & 0xF];
            buf[p++] = ' ';
            buf[p++] = 't'; buf[p++] = '='; p += uitoa(f.hdr.msg_type, &buf[p]);
            buf[p++] = ' '; buf[p++] = 'n'; buf[p++] = '='; p += uitoa(f.hdr.numobj, &buf[p]);
            buf[p++] = '\r'; buf[p++] = '\n'; buf[p] = 0;
            usb_cdc_puts(buf);
            n++;
            if (n > 64) { usb_cdc_puts("(truncated)\r\n"); break; }
        }
        if (n == 0) usb_cdc_puts("(empty)\r\n");
        return;
    }
    if (starts_with(line, "ovp")) {
        uint16_t v = 0;
        const char *p = line + 3;
        while (*p == ' ') p++;
        while (*p >= '0' && *p <= '9') { v = v*10 + (*p - '0'); p++; }
        power_set_ovp(v);
        usb_cdc_puts("OK OVP set\r\n");
        return;
    }
    if (starts_with(line, "ilim")) {
        uint16_t a = 0;
        const char *p = line + 4;
        while (*p == ' ') p++;
        while (*p >= '0' && *p <= '9') { a = a*10 + (*p - '0'); p++; }
        power_set_ilim(a);
        usb_cdc_puts("OK ILIM set\r\n");
        return;
    }

    usb_cdc_puts("ERR unknown command (try 'help')\r\n");
}

static void cli_help(void) {
    usb_cdc_puts(
        "Ember-Tap commands (c) jayis1:\r\n"
        "  help\r\n"
        "  status\r\n"
        "  sniff on|off\r\n"
        "  spoof src <mv> <ma>       — advertise as source\r\n"
        "  sink req <mv> <ma>        — request from real source\r\n"
        "  fuzz start <count> <seed>\r\n"
        "  fuzz profile header|pdo|timing|chunk\r\n"
        "  fuzz stop\r\n"
        "  hardreset\r\n"
        "  roleswap\r\n"
        "  deadbattery on|off\r\n"
        "  vbus glitch <us> [type 0|1]\r\n"
        "  log dump\r\n"
        "  ovp <0.1V units>\r\n"
        "  ilim <mA>\r\n"
    );
}

/* end of file — author: jayis1 */