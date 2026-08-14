/*
 * main.c — 1553-Phantom firmware entry point and scheduler
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Responsibilities:
 *   - Clock tree setup (HSE 16 MHz → PLL → 170 MHz)
 *   - GPIO + peripheral init (SPI1, TIM6, USB FS, EXTI)
 *   - 1 ms tick (TIM6) → button debounce + arm-latch watchdog
 *   - Main loop: poll USB CDC, parse CLI, drain FPGA RX FIFO,
 *     service role tick (BC/RT/BM/MITM), render OLED
 *   - FPGA EXTI handler → "word ready" → wake BM capture path
 */

#include "board.h"
#include "registers.h"
#include "drivers/fpga_link.h"
#include "drivers/bc_role.h"
#include "drivers/rt_role.h"
#include "drivers/bm_role.h"
#include "drivers/mitm_role.h"
#include "drivers/capture.h"
#include "drivers/display.h"
#include "drivers/buttons.h"
#include "drivers/usb_cdc.h"
#include <string.h>

/* ---- Global millisecond tick (advanced by TIM6 ISR) ---- */
volatile uint32_t g_ms_tick = 0;

/* ---- Current operating role ---- */
static role_t g_role = ROLE_BM;

/* ---- Arm latch ----
 * Two-stage: software request (from CLI "arm") AND user-button long-press
 * both required within ARM_REQUEST_TIMEOUT_MS. Held only in SRAM.
 */
static volatile uint8_t g_arm_request   = 0;   /* CLI requested arm          */
static volatile uint8_t g_arm_button    = 0;   /* button long-press observed */
static volatile uint32_t g_arm_req_ms   = 0;   /* timestamp of CLI request   */
static volatile uint8_t g_armed         = 0;   /* latch output               */
static volatile uint8_t g_injecting     = 0;   /* >0 while TX FIFO has data  */

/* ---- CLI buffer ---- */
static char cli_line[160];
static int  cli_len = 0;

/* ---- Forward declarations ---- */
static void clock_init(void);
static void gpio_init(void);
static void spi1_init(void);
static void tim6_init(void);
static void exti_init(void);
static void arm_eval(void);
static void disarm(void);
static void role_set(role_t r);
static void cli_process(const char *line);
static void cli_help(void);
static void render_status(void);

/* =========================================================================
 * Entry point
 * ========================================================================= */
int main(void) {
    /* Boot: clocks first so all peripherals run at full speed */
    clock_init();
    gpio_init();
    spi1_init();
    tim6_init();
    exti_init();

    /* Drivers */
    display_init();
    buttons_init();
    fpga_link_init();      /* resets iCE40, loads bitstream marker */
    capture_init();
    usb_cdc_init();
    bc_role_init();
    rt_role_init();
    bm_role_init();
    mitm_role_init();

    /* Welcome banner */
    display_clear();
    display_text(0, 0, DEVICE_NAME);
    display_text(0, 1, "by " DEVICE_AUTHOR);
    display_text(0, 3, "MIL-1553B implant");
    display_text(0, 4, "Booting passive...");
    display_flush();

    for (volatile int i = 0; i < 1000000; i++) { /* small settle */
        __asm volatile("nop");
    }

    /* Boot-to-passive: BM, no arm */
    role_set(ROLE_BM);

    usb_cdc_puts("\r\n" DEVICE_NAME " v" FW_VERSION " (c) " DEVICE_AUTHOR "\r\n");
    usb_cdc_puts("Passive BM. Type 'help'.\r\n> ");

    /* ---- Main scheduler loop ---- */
    uint32_t last_render = 0;
    uint32_t last_role_tick = 0;
    while (1) {
        /* 1. Drain any captured 1553 words the FPGA latched since last loop */
        fpga_link_pump();

        /* 2. Service the active role */
        uint32_t now = g_ms_tick;
        if (now - last_role_tick >= 1) {
            last_role_tick = now;
            switch (g_role) {
                case ROLE_BM:   bm_role_tick(now); break;
                case ROLE_RT:   rt_role_tick(now); break;
                case ROLE_BC:   bc_role_tick(now); break;
                case ROLE_MITM: mitm_role_tick(now); break;
                default: break;
            }
        }

        /* 3. Arm-latch watchdog: CLI request expires after timeout */
        arm_eval();

        /* 4. USB CDC CLI poll */
        int c;
        while ((c = usb_cdc_getc()) >= 0) {
            if (c == '\r' || c == '\n') {
                if (cli_len > 0) {
                    cli_line[cli_len] = 0;
                    cli_process(cli_line);
                    cli_len = 0;
                }
                usb_cdc_puts("\r\n> ");
            } else if (c == 0x08 && cli_len > 0) {
                cli_len--;
                usb_cdc_puts("\b \b");
            } else if (cli_len < (int)sizeof(cli_line) - 1 && c >= 0x20) {
                cli_line[cli_len++] = (char)c;
                usb_cdc_putc((char)c);
            }
        }

        /* 5. Render status ~5 Hz */
        if (now - last_render >= 200) {
            last_render = now;
            render_status();
        }
    }
}

/* =========================================================================
 * Clock tree: HSE 16 MHz crystal → PLL (VCO 170 MHz) → SYSCLK 170 MHz
 * ========================================================================= */
static void clock_init(void) {
    /* Enable HSE */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY)) { }

    /* PLLCFGR: source = HSE, M=2, N=85, R=2 → 170 MHz
     *   VCO = HSE / M * N = 16/2 * 85 = 680 MHz
     *   SYS = VCO / R     = 680 / 2 = 340? ... no: R=2 → 170 (HSE/2 → div2)
     * We use M=2 N=42 R=4 → (16/2*42)/4 = 84 MHz conservative for USB FS
     * Actually for 170 MHz: M=2, N=85, P=2,Q=4,R=2
     * For simplicity and stable USB FS (needs 48 MHz), we target SYS=170,
     * USB uses its own 48 MHz from HSI48 trimming. See USB driver.
     */
    RCC->PLLCFGR = (2u<<0)    /* M */
                 | (85u<<6)    /* N */
                 | (0u<<16)    /* P (not used for SYS) */
                 | (1u<<20)    /* PLLSRC = HSE */
                 | (2u<<29);   /* R = /2 → SYS=170 MHz */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) { }

    /* Flash latency 5 wait states for 170 MHz (set via FLASH_ACR; omitted here
     * for brevity — real firmware writes 0x00000005 to FLASH_ACR at 0x40022000)
     */
    REG32(0x40022000u) = 0x05u | (1u<<8);  /* ACR: LATENCY=5 | PRFTEN */

    /* Switch SYSCLK to PLL */
    RCC->CFGR = (RCC->CFGR & ~0x3u) | RCC_CFGR_SW_PLL;
    while (((RCC->CFGR >> RCC_CFGR_SWS_SHIFT) & 0x3u) != RCC_CFGR_SW_PLL) { }
}

/* =========================================================================
 * GPIO
 * ========================================================================= */
static void gpio_init(void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOA | RCC_AHB2ENR_GPIOB | RCC_AHB2ENR_GPIOC;

    /* OLED control pins */
    GPIO_OUT_PP(GPIOA, PA0_OLED_DC);
    GPIO_OUT_PP(GPIOA, PA1_OLED_RST);
    GPIO_OUT_PP(GPIOC, PC13_OLED_CS);

    /* SPI1 to FPGA: SCK/MISO/MOSI alt AF5, CS as output */
    GPIO_ALT(GPIOA, PA5_SPI1_SCK,  5);
    GPIO_ALT(GPIOA, PA6_SPI1_MISO, 5);
    GPIO_ALT(GPIOA, PA7_SPI1_MOSI, 5);
    GPIO_OUT_PP(GPIOA, PA4_FPGA_CS);
    GPIO_SET(GPIOA, PA4_FPGA_CS);   /* idle high (active low) */

    /* FPGA IRQ + reset */
    GPIO_IN_FLT(GPIOA, PA11_FPGA_IRQ);
    GPIO_OUT_PP(GPIOA, PA12_FPGA_RST);

    /* USB DM/DP alt AF11 */
    GPIO_ALT(GPIOA, PA8_USB_DM, 11);
    GPIO_ALT(GPIOA, PA9_USB_DP, 11);

    /* User button, pull-up, falling edge */
    GPIO_IN_PUP(GPIOA, PA10_BTN);

    /* LEDs */
    GPIO_OUT_PP(GPIOA, PA15_LED_PWR);
    GPIO_OUT_PP(GPIOB, PB0_LED_ACTA);
    GPIO_OUT_PP(GPIOB, PB1_LED_ACTB);
    GPIO_OUT_PP(GPIOB, PB3_LED_ARM_RED);
    GPIO_OUT_PP(GPIOB, PB4_LED_INJ_GRN);

    /* Panic sense (capacitive pad, treated as digital for now) */
    GPIO_IN_PDN(GPIOB, PB10_PANIC_SENSE);

    /* TX gate (arm latch) — default OFF */
    GPIO_OUT_PP(GPIOB, PB11_TX_GATE);
    GPIO_CLR(GPIOB, PB11_TX_GATE);

    /* Power LED on */
    GPIO_SET(GPIOA, PA15_LED_PWR);

    /* I2C1 for MAX17048 fuel gauge */
    GPIO_ALT(GPIOB, PB8_I2C1_SCL, 4);
    GPIO_ALT(GPIOB, PB9_I2C1_SDA, 4);
}

/* =========================================================================
 * SPI1 master to iCE40 — 24 MHz (PCLK/8), 16-bit frames, NSS software
 * ========================================================================= */
static void spi1_init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_SPI1;

    SPI1->CR1 = 0;   /* disable before config */
    SPI1->CR2 = (15u << SPI_CR2_DS_SHIFT) | SPI_CR2_FRXTH;  /* 16-bit frames */
    /* BR = /8, master, CPOL=0, CPHA=0, SSM, SSI */
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI
              | (2u << SPI_CR1_BR_SHIFT)  /* /8 */
              | 0;
    SPI1->CR1 |= SPI_CR1_SPE;   /* enable */
}

/* =========================================================================
 * TIM6 — 1 ms tick (170 MHz / (1700 PSC) / 100 ARR)
 * ========================================================================= */
static void tim6_init(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM6;
    TIM6->PSC = 1700 - 1;   /* 170 MHz / 1700 = 100 kHz tick */
    TIM6->ARR = 100 - 1;    /* 100 ticks → 1 kHz = 1 ms */
    TIM6->DIER = TIM6_DIER_UIE;
    TIM6->CR1  = TIM6_CR1_CEN;

    /* Enable TIM6 IRQ in NVIC */
    NVIC_ISER = (1u << IRQ_TIM6);
}

/* =========================================================================
 * EXTI for button + FPGA IRQ
 * ========================================================================= */
static void exti_init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFG;
    /* Button PA10 → EXTI10, falling edge */
    EXTI->IMR1  |= (1u << 10);
    EXTI->FTSR1 |= (1u << 10);
    /* FPGA IRQ PA11 → EXTI11, falling edge (word ready) */
    EXTI->IMR1  |= (1u << 11);
    EXTI->FTSR1 |= (1u << 11);
    NVIC_ISER = (1u << IRQ_EXTI10_15);
}

/* =========================================================================
 * Arm latch evaluation
 * ========================================================================= */
static void arm_eval(void) {
    /* Expire stale CLI requests */
    if (g_arm_request && (g_ms_tick - g_arm_req_ms) > ARM_REQUEST_TIMEOUT_MS) {
        g_arm_request = 0;
        usb_cdc_puts("\r\narm: CLI request timed out\r\n");
    }
    /* Both required to arm */
    if (!g_armed && g_arm_request && g_arm_button) {
        g_armed = 1;
        GPIO_SET(GPIOB, PB3_LED_ARM_RED);
        GPIO_SET(GPIOB, PB11_TX_GATE);   /* enable HI-1573 tx drivers */
        usb_cdc_puts("\r\nARMED. TX drivers enabled.\r\n");
    }
}

static void disarm(void) {
    g_armed = 0;
    g_arm_request = 0;
    g_arm_button  = 0;
    g_injecting = 0;
    GPIO_CLR(GPIOB, PB11_TX_GATE);
    GPIO_CLR(GPIOB, PB3_LED_ARM_RED);
    GPIO_CLR(GPIOB, PB4_LED_INJ_GRN);
    fpga_link_disarm();
    usb_cdc_puts("\r\nDISARMED. TX drivers off, FIFO drained.\r\n");
}

/* =========================================================================
 * Role switching
 * ========================================================================= */
static void role_set(role_t r) {
    if (r == g_role) return;
    if (ROLE_CAN_TX(r) && !g_armed) {
        usb_cdc_puts("\r\nrole: TX-capable role requires arm first\r\n");
        return;
    }
    /* Stop current role cleanly */
    switch (g_role) {
        case ROLE_BC:   bc_role_stop(); break;
        case ROLE_RT:   rt_role_stop(); break;
        case ROLE_MITM: mitm_role_stop(); break;
        case ROLE_BM:   bm_role_stop();  break;
        default: break;
    }
    g_role = r;
    switch (r) {
        case ROLE_BC:   bc_role_start(); break;
        case ROLE_RT:   rt_role_start(); break;
        case ROLE_MITM: mitm_role_start(); break;
        case ROLE_BM:   bm_role_start();  break;
        default: break;
    }
    /* MITM/BC/RT need arm; passive BM does not */
    if (!ROLE_CAN_TX(r) && g_armed) {
        disarm();
    }
    usb_cdc_puts("\r\nrole -> ");
    const char *names[ROLE_COUNT] = {"BM","RT","BC","MITM"};
    usb_cdc_puts(names[r]);
    usb_cdc_puts("\r\n");
}

/* =========================================================================
 * CLI
 * ========================================================================= */
static void cli_help(void) {
    usb_cdc_puts(
        "Commands:\r\n"
        "  help\r\n"
        "  role bm|rt|bc|mitm\r\n"
        "  arm                (then long-press button)\r\n"
        "  disarm\r\n"
        "  status\r\n"
        "  rt set <addr>      | rt tx <sa> <hex...>\r\n"
        "  rt fault <mask>    | rt mode <code> [data]\r\n"
        "  bc load csv <line> | bc run | bc stop\r\n"
        "  bc gap <us>        | bc fuzz <mask>\r\n"
        "  mitm add <ch> <match> <repl>\r\n"
        "  mitm drop <ch> <match> | mitm clear\r\n"
        "  bm start | bm stop | bm flush\r\n"
        "  bm export csv|1553cap|pcapng\r\n"
        "  log level debug|info|warn\r\n"
        "  reboot\r\n");
}

static void cli_process(const char *line) {
    /* Tokenize */
    static char argv[8][32];
    int argc = 0, i = 0, j = 0;
    while (line[i] && argc < 8) {
        while (line[i] == ' ') i++;
        if (!line[i]) break;
        j = 0;
        while (line[i] && line[i] != ' ' && j < 31) {
            argv[argc][j++] = line[i++];
        }
        argv[argc][j] = 0;
        argc++;
    }
    if (argc == 0) return;

    if (!strcmp(argv[0], "help")) { cli_help(); return; }

    if (!strcmp(argv[0], "role") && argc == 2) {
        if      (!strcmp(argv[1],"bm"))   role_set(ROLE_BM);
        else if (!strcmp(argv[1],"rt"))   role_set(ROLE_RT);
        else if (!strcmp(argv[1],"bc"))   role_set(ROLE_BC);
        else if (!strcmp(argv[1],"mitm")) role_set(ROLE_MITM);
        else usb_cdc_puts("role: bad arg\r\n");
        return;
    }

    if (!strcmp(argv[0], "arm")) {
        g_arm_request = 1;
        g_arm_req_ms = g_ms_tick;
        usb_cdc_puts("arm: long-press button within 5s\r\n");
        return;
    }
    if (!strcmp(argv[0], "disarm")) { disarm(); return; }

    if (!strcmp(argv[0], "status")) {
        char b[64];
        const char *names[ROLE_COUNT] = {"BM","RT","BC","MITM"};
        int n = snprintf_lite(b, sizeof(b),
            "role=%s armed=%d inj=%d cap=%u chA_err=%u chB_err=%u",
            names[g_role], g_armed, g_injecting,
            capture_count(),
            fpga_link_chan_errs(0), fpga_link_chan_errs(1));
        usb_cdc_puts(b);
        usb_cdc_puts("\r\n");
        return;
    }

    if (!strcmp(argv[0], "rt"))   { rt_role_cli(argc, argv); return; }
    if (!strcmp(argv[0], "bc"))   { bc_role_cli(argc, argv); return; }
    if (!strcmp(argv[0], "mitm")) { mitm_role_cli(argc, argv); return; }
    if (!strcmp(argv[0], "bm"))   { bm_role_cli(argc, argv); return; }

    if (!strcmp(argv[0], "log") && argc == 2) {
        usb_cdc_puts("log level set\r\n");
        return;
    }
    if (!strcmp(argv[0], "reboot")) {
        usb_cdc_puts("rebooting...\r\n");
        for (volatile int k=0;k<100000;k++) __asm volatile("nop");
        REG32(0xE000ED0Cu) = 0x5FA0004;  /* NVIC_SystemReset via AIRCR */
        while (1) { }
    }

    usb_cdc_puts("unknown command; 'help'\r\n");
}

/* =========================================================================
 * OLED status render
 * ========================================================================= */
static void render_status(void) {
    const char *names[ROLE_COUNT] = {"BM","RT","BC","MITM"};
    char line[22];
    display_clear();
    snprintf_lite(line, sizeof(line), "%s %s arm=%d",
        DEVICE_NAME, names[g_role], g_armed);
    display_text(0, 0, line);
    snprintf_lite(line, sizeof(line), "cap:%u A:%u B:%u",
        capture_count(),
        fpga_link_chan_errs(0), fpga_link_chan_errs(1));
    display_text(0, 1, line);
    if (g_role == ROLE_BC) {
        snprintf_lite(line, sizeof(line), "bc frame:%u",
            bc_role_frame_count());
        display_text(0, 2, line);
    } else if (g_role == ROLE_RT) {
        snprintf_lite(line, sizeof(line), "rt emul:%d",
            rt_role_emulated_count());
        display_text(0, 2, line);
    } else if (g_role == ROLE_MITM) {
        snprintf_lite(line, sizeof(line), "mitm rules:%d",
            mitm_role_rule_count());
        display_text(0, 2, line);
    }
    display_text(0, 7, "by " DEVICE_AUTHOR);
    display_flush();
}

/* =========================================================================
 * ISRs
 * ========================================================================= */
void TIM6_DAC_IRQHandler(void) {
    if (TIM6->SR & TIM6_SR_UIF) {
        TIM6->SR = 0;            /* clear all flags */
        g_ms_tick++;
        buttons_tick(g_ms_tick);  /* debounce + long-press detect */
    }
}

/* EXTI10..15 combined: button PA10 + FPGA IRQ PA11 */
void EXTI15_10_IRQHandler(void) {
    uint32_t pr = EXTI->PR1;
    if (pr & (1u << 10)) {       /* button */
        EXTI->PR1 = (1u << 10);
        buttons_on_press(g_ms_tick);
    }
    if (pr & (1u << 11)) {       /* FPGA "word ready" */
        EXTI->PR1 = (1u << 11);
        fpga_link_on_irq();
    }
}

/* Button long-press callback (from buttons.c) */
void buttons_long_press(void) {
    /* First long-press in response to `arm` request satisfies arm latch */
    if (g_arm_request && !g_arm_button) {
        g_arm_button = 1;
        arm_eval();
    } else if (g_armed) {
        /* Second long-press (no CLI request pending) = panic disarm */
        disarm();
    } else if (g_role != ROLE_BM) {
        /* Short-cut back to passive */
        role_set(ROLE_BM);
    }
}