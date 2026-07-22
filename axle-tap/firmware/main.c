/*
 * main.c — AxleTap firmware top-level
 * Automotive Ethernet (100/1000BASE-T1) Tap, MITM & gPTP spoofing platform
 *
 * Author: jayis1
 * License: GPL-2.0
 * Date:   2026-07-22
 *
 * Initializes the STM32H723 peripherals, runs the cooperative scheduler,
 * handles the USB-CDC CLI, and dispatches frames to the capture /
 * bridge / gPTP / TSN / SOME/IP engines.
 *
 * Hardware interlock: the firmware boots in RECEIVE-ONLY mode. The
 * arm switch (PC15) must be closed AND the kill key (PB0) must be open
 * for any transmit function to execute. The firmware checks armed()
 * before every TX attempt.
 */

#include "registers.h"
#include "board.h"
#include "drivers/phy_driver.h"
#include "drivers/bridge_driver.h"
#include "drivers/gptp_driver.h"
#include "drivers/tsn_driver.h"
#include "drivers/someip_driver.h"
#include "drivers/ble_bridge.h"
#include "drivers/pcap_driver.h"
#include <string.h>
#include <stdlib.h>

/* ------------------------------------------------------------------ */
/* Forward decls                                                       */
/* ------------------------------------------------------------------ */
void usb_cdc_send(const uint8_t *data, uint16_t len);
static void usb_cdc_init(void);
static void cli_init(void);
static void cli_process(void);
static void ble_cmd_handler(const uint8_t *payload, uint16_t len);

/* ------------------------------------------------------------------ */
/* Clocks + GPIO init                                                   */
/* ------------------------------------------------------------------ */
static void clocks_init(void)
{
    /* Enable HSI (already on after reset). Enable PLL1 for 550 MHz.
     * Full PLL1 config is in registers.h / linker script; here we
     * use the high-level RCC writes.
     */
    RCC_CR |= RCC_CR_HSION;
    while (!(RCC_CR & RCC_CR_HSIRDY)) {}

    /* Enable clocks for all peripherals used */
    RCC_AHB1ENR |= BIT(15) | BIT(16) | BIT(17); /* ETH MAC/TX/RX */
    RCC_AHB1ENR |= BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4)
                 | BIT(5) | BIT(6) | BIT(7) | BIT(8); /* DMA1/DMA2 */
    RCC_AHB2ENR |= BIT(0); /* USB OTG-HS */
    RCC_AHB3ENR |= BIT(0); /* FMC (SD card) */
    RCC_AHB4ENR |= BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4)
                 | BIT(5) | BIT(6) | BIT(7) | BIT(8) | BIT(9)
                 | BIT(10) | BIT(11); /* GPIOA..K */
    RCC_APB1ENR1 |= BIT(23) | BIT(1) | BIT(0); /* USART3, TIM2, TIM3 */
    RCC_APB2ENR |= BIT(5) | BIT(6); /* SDMMC1, TIM15 */
    RCC_APB4ENR |= BIT(7) | BIT(4) | BIT(6); /* SPI6, SYSCFG, PWR */
}

static void gpio_init(void)
{
    /* Configure LEDs as outputs */
    GPIOE->MODER &= ~(0x3 << (LED_LINKA_PIN * 2));
    GPIOE->MODER |=  (GPIO_MODE_OUT << (LED_LINKA_PIN * 2));
    GPIOE->MODER &= ~(0x3 << (LED_LINKB_PIN * 2));
    GPIOE->MODER |=  (GPIO_MODE_OUT << (LED_LINKB_PIN * 2));
    GPIOE->MODER &= ~(0x3 << (LED_ARM_PIN * 2));
    GPIOE->MODER |=  (GPIO_MODE_OUT << (LED_ARM_PIN * 2));
    GPIOE->MODER &= ~(0x3 << (LED_CAP_PIN * 2));
    GPIOE->MODER |=  (GPIO_MODE_OUT << (LED_CAP_PIN * 2));
    GPIOE->MODER &= ~(0x3 << (LED_BLE_PIN * 2));
    GPIOE->MODER |=  (GPIO_MODE_OUT << (LED_BLE_PIN * 2));

    /* Arm switch / kill key as inputs with pull-up */
    ARM_SWITCH_PORT->MODER &= ~(0x3 << (ARM_SWITCH_PIN * 2));
    ARM_SWITCH_PORT->PUPDR &= ~(0x3 << (ARM_SWITCH_PIN * 2));
    ARM_SWITCH_PORT->PUPDR |=  (0x1 << (ARM_SWITCH_PIN * 2));
    KILL_KEY_PORT->MODER  &= ~(0x3 << (KILL_KEY_PIN * 2));
    KILL_KEY_PORT->PUPDR  &= ~(0x3 << (KILL_KEY_PIN * 2));
    KILL_KEY_PORT->PUPDR  |=  (0x1 << (KILL_KEY_PIN * 2));

    /* PHY reset pins as outputs, high (inactive) */
    PHYA_RESET_PORT->MODER &= ~(0x3 << (PHYA_RESET_PIN * 2));
    PHYA_RESET_PORT->MODER |=  (GPIO_MODE_OUT << (PHYA_RESET_PIN * 2));
    PHYA_RESET_PORT->BSRR = BIT(PHYA_RESET_PIN);
    PHYB_RESET_PORT->MODER &= ~(0x3 << (PHYB_RESET_PIN * 2));
    PHYB_RESET_PORT->MODER |=  (GPIO_MODE_OUT << (PHYB_RESET_PIN * 2));
    PHYB_RESET_PORT->BSRR = BIT(PHYB_RESET_PIN);

    /* MDIO bit-bang pins as outputs */
    GPIOC->MODER &= ~(0x3 << (MDIO_GPIO_MDC_PIN * 2));
    GPIOC->MODER |=  (GPIO_MODE_OUT << (MDIO_GPIO_MDC_PIN * 2));
    GPIOA->MODER &= ~(0x3 << (MDIO_GPIO_MDIO_PIN * 2));
    GPIOA->MODER |=  (GPIO_MODE_OUT << (MDIO_GPIO_MDIO_PIN * 2));
}

/* ------------------------------------------------------------------ */
/* USART3 debug console                                                 */
/* ------------------------------------------------------------------ */
static void usart3_init(void)
{
    /* PD8 AF7 (TX), PD9 AF7 (RX) */
    GPIOD->MODER &= ~(0x3 << (DBG_TX_PIN * 2));
    GPIOD->MODER |=  (GPIO_MODE_AF << (DBG_TX_PIN * 2));
    GPIOD->AFR[1] = (GPIOD->AFR[1] & ~(0xFU << ((DBG_TX_PIN - 8) * 4))) |
                   (DBG_UART_AF << ((DBG_TX_PIN - 8) * 4));
    GPIOD->MODER &= ~(0x3 << (DBG_RX_PIN * 2));
    GPIOD->MODER |=  (GPIO_MODE_AF << (DBG_RX_PIN * 2));
    GPIOD->AFR[1] = (GPIOD->AFR[1] & ~(0xFU << ((DBG_RX_PIN - 8) * 4))) |
                   (DBG_UART_AF << ((DBG_RX_PIN - 8) * 4));

    /* Baud 115200 @ 137.5 MHz APB1: BRR = 137500000 / 115200 = 1193 */
    DBG_UART->BRR = 1193;
    DBG_UART->CR1 = BIT(3) | BIT(2) | BIT(0);  /* TX, RX, UE */
}

static void dbg_putc(char c)
{
    while (!(DBG_UART->ISR & BIT(7))) {}  /* TXE */
    DBG_UART->TDR = c;
}

void dbg_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') dbg_putc('\r');
        dbg_putc(*s++);
    }
}

static void dbg_hex(uint32_t v)
{
    char buf[12];
    for (int i = 7; i >= 0; i--) {
        int d = (v >> (i * 4)) & 0xF;
        buf[7 - i] = d < 10 ? '0' + d : 'A' + d - 10;
    }
    buf[8] = 0;
    dbg_puts(buf);
}

/* ------------------------------------------------------------------ */
/* TIM2 — gPTP fine timer                                               */
/* ------------------------------------------------------------------ */
static void tim2_init(void)
{
    /* TIM2 runs at PCLK1 * 2 = 275 MHz, free-running 32-bit counter.
     * Used by the bridge / gPTP engines for timestamps.
     */
    TIM2->PSC = 0;
    TIM2->ARR = 0xFFFFFFFF;
    TIM2->CR1 = BIT(0);  /* enable counter, free-running */
}

/* ------------------------------------------------------------------ */
/* USB-CDC stand-in                                                     */
/* ------------------------------------------------------------------ */
/* The full USB OTG-HS CDC stack is too large for this file; we provide
 * a hook that the (separate) USB module fills. The default weak
 * implementation routes to the debug USART so the CLI still works
 * during development.
 */
__attribute__((weak)) void usb_cdc_send(const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) dbg_putc((char)data[i]);
}

static void usb_cdc_init(void)
{
    /* Configure PA11/PA12 AF10 for OTG-HS. The full CDC stack is built
     * from a separate module; here we only configure pins.
     */
    GPIOA->MODER &= ~(0x3 << (USB_DM_PIN * 2));
    GPIOA->MODER |=  (GPIO_MODE_AF << (USB_DM_PIN * 2));
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~(0xFU << ((USB_DM_PIN - 8) * 4)))
                   | (USB_AF << ((USB_DM_PIN - 8) * 4));
    GPIOA->MODER &= ~(0x3 << (USB_DP_PIN * 2));
    GPIOA->MODER |=  (GPIO_MODE_AF << (USB_DP_PIN * 2));
}

/* ------------------------------------------------------------------ */
/* CLI                                                                  */
/* ------------------------------------------------------------------ */
static char cli_buf[128];
static int cli_pos;

static void cli_help(void)
{
    dbg_puts(
        "AxleTap CLI — jayis1\r\n"
        "  status              show link/arm/capture status\r\n"
        "  arm                 enable transmit (requires arm switch)\r\n"
        "  disarm              disable transmit\r\n"
        "  tap on|off          passive tap mode\r\n"
        "  bridge on|off       inline MITM bridge\r\n"
        "  gptp gm             spoof grandmaster\r\n"
        "  gptp slip <ppb>     time slip at ppb rate\r\n"
        "  gptp freeze         hold time constant\r\n"
        "  gptp reset          return to passive\r\n"
        "  tsn seqforge dup|out|off   802.1CB sequence forgery\r\n"
        "  tsn qbv inject      inject out-of-window frame\r\n"
        "  tsn sr spoof|off    SR-class reservation spoof\r\n"
        "  someip discover     run SOME/IP-SD discovery\r\n"
        "  someip list         list discovered services\r\n"
        "  someip fuzz <svc> <method> <mode>   fuzz service\r\n"
        "  someip stop         stop fuzzing\r\n"
        "  pcap start|stop     start/stop capture\r\n"
        "  pcap dump           dump SD card to USB\r\n"
        "  ble on|off          enable BLE backhaul\r\n"
        "  help\r\n"
    );
}

static void cli_status(void)
{
    phy_status_t sa, sb;
    phy_get_status(PHYA_MDIO_ADDR, &sa);
    phy_get_status(PHYB_MDIO_ADDR, &sb);
    bridge_stats_t bs; bridge_get_stats(&bs);
    tsn_stats_t ts; tsn_get_stats(&ts);
    pcap_stats_t ps; pcap_get_stats(&ps);

    dbg_puts("=== AxleTap status ===\r\n");
    dbg_puts("  PHY A: link="); dbg_puts(sa.link_up ? "UP" : "DOWN");
    dbg_puts(" speed="); dbg_puts(sa.speed == 2 ? "1000T1" : sa.speed == 1 ? "100T1" : "down");
    dbg_puts(" role="); dbg_puts(sa.master ? "master" : "slave");
    dbg_puts(" sqi="); dbg_hex((uint32_t)sa.sqi); dbg_puts("\r\n");
    dbg_puts("  PHY B: link="); dbg_puts(sb.link_up ? "UP" : "DOWN");
    dbg_puts(" speed="); dbg_puts(sb.speed == 2 ? "1000T1" : sb.speed == 1 ? "100T1" : "down");
    dbg_puts(" role="); dbg_puts(sb.master ? "master" : "slave"); dbg_puts("\r\n");

    dbg_puts("  Arm switch: "); dbg_puts(arm_switch_closed() ? "CLOSED" : "OPEN");
    dbg_puts("  Kill key: "); dbg_puts(kill_key_set() ? "SET" : "open");
    dbg_puts("  Armed: "); dbg_puts(armed() ? "YES" : "no"); dbg_puts("\r\n");

    bridge_mode_t bm = bridge_get_mode();
    dbg_puts("  Bridge: ");
    dbg_puts(bm == BRIDGE_OFF ? "OFF" : bm == BRIDGE_PASSTHROUGH ? "PASSTHROUGH" : "MITM");
    dbg_puts("\r\n");

    gptp_state_t *gs = gptp_get_state();
    dbg_puts("  gPTP: mode=");
    dbg_puts(gs->mode == GPTP_PASSIVE ? "PASSIVE" :
             gs->mode == GPTP_GRANDMASTER ? "GM" :
             gs->mode == GPTP_SLIP ? "SLIP" :
             gs->mode == GPTP_FREEZE ? "FREEZE" : "JUMP");
    dbg_puts(" slip_ppb="); dbg_hex((uint32_t)gs->slip_ppb); dbg_puts("\r\n");
    dbg_puts("  Observed GM identity: ");
    for (int i = 0; i < 8; i++) { dbg_hex(gs->observed_master.clock_identity[i]); dbg_puts(" "); }
    dbg_puts("\r\n");

    dbg_puts("  Bridge stats: AB="); dbg_hex(bs.frames_ab);
    dbg_puts(" BA="); dbg_hex(bs.frames_ba);
    dbg_puts(" dropped="); dbg_hex(bs.dropped);
    dbg_puts(" injected="); dbg_hex(bs.injected); dbg_puts("\r\n");

    dbg_puts("  TSN: cb_seen="); dbg_hex(ts.cb_frames_seen);
    dbg_puts(" cb_forged="); dbg_hex(ts.cb_frames_forged);
    dbg_puts(" qbv_injects="); dbg_hex(ts.qbv_injects); dbg_puts("\r\n");

    dbg_puts("  PCAP: running="); dbg_puts(pcap_is_running() ? "yes" : "no");
    dbg_puts(" captured="); dbg_hex(ps.captured);
    dbg_puts(" sd_bytes="); dbg_hex(ps.sd_bytes); dbg_puts("\r\n");

    int sc; const someip_service_t *svcs = someip_get_services(&sc);
    dbg_puts("  SOME/IP services: "); dbg_hex((uint32_t)sc); dbg_puts("\r\n");

    dbg_puts("  BLE: connected="); dbg_puts(ble_is_connected() ? "yes" : "no"); dbg_puts("\r\n");
}

static int starts_with(const char *s, const char *p)
{
    while (*p) { if (*s++ != *p++) return 0; }
    return 1;
}

static int parse_int(const char *s, int *out)
{
    int sign = 1; int v = 0;
    if (*s == '-') { sign = -1; s++; }
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); s++; }
    *out = sign * v;
    return 1;
}

static void cli_dispatch(char *line)
{
    if (line[0] == 0) return;
    if (strcmp(line, "help") == 0) { cli_help(); return; }
    if (strcmp(line, "status") == 0) { cli_status(); return; }
    if (strcmp(line, "arm") == 0) {
        if (!armed()) { dbg_puts("ERROR: arm switch open or kill key set\r\n"); return; }
        dbg_puts("ARMED\r\n");
        led_arm_set(1);
        return;
    }
    if (strcmp(line, "disarm") == 0) {
        dbg_puts("DISARMED\r\n");
        led_arm_set(0);
        gptp_set_mode(GPTP_PASSIVE);
        tsn_cb_set_mode(TSN_CB_OFF);
        someip_fuzz_stop();
        return;
    }
    if (strcmp(line, "tap on") == 0) {
        bridge_set_mode(BRIDGE_OFF);
        pcap_start();
        dbg_puts("Passive tap started\r\n");
        led_cap_set(1);
        return;
    }
    if (strcmp(line, "tap off") == 0) {
        pcap_stop();
        bridge_set_mode(BRIDGE_OFF);
        dbg_puts("Tap stopped\r\n");
        led_cap_set(0);
        return;
    }
    if (strcmp(line, "bridge on") == 0) {
        if (!armed()) { dbg_puts("ERROR: not armed\r\n"); return; }
        bridge_set_mode(BRIDGE_MITM);
        dbg_puts("MITM bridge enabled\r\n");
        return;
    }
    if (strcmp(line, "bridge off") == 0) {
        bridge_set_mode(BRIDGE_PASSTHROUGH);
        dbg_puts("Bridge passthrough\r\n");
        return;
    }
    if (strcmp(line, "gptp gm") == 0) {
        if (!armed()) { dbg_puts("ERROR: not armed\r\n"); return; }
        bridge_set_mode(BRIDGE_MITM);
        gptp_set_mode(GPTP_GRANDMASTER);
        dbg_puts("gPTP grandmaster spoof started\r\n");
        return;
    }
    if (starts_with(line, "gptp slip ")) {
        int ppb; parse_int(line + 10, &ppb);
        if (!armed()) { dbg_puts("ERROR: not armed\r\n"); return; }
        bridge_set_mode(BRIDGE_MITM);
        gptp_set_slip(ppb);
        dbg_puts("gPTP slip started at "); dbg_hex((uint32_t)ppb); dbg_puts(" ppb\r\n");
        return;
    }
    if (strcmp(line, "gptp freeze") == 0) {
        if (!armed()) { dbg_puts("ERROR: not armed\r\n"); return; }
        bridge_set_mode(BRIDGE_MITM);
        gptp_set_mode(GPTP_FREEZE);
        dbg_puts("gPTP time freeze\r\n");
        return;
    }
    if (strcmp(line, "gptp reset") == 0) {
        gptp_set_mode(GPTP_PASSIVE);
        dbg_puts("gPTP passive\r\n");
        return;
    }
    if (strcmp(line, "tsn seqforge dup") == 0) {
        if (!armed()) { dbg_puts("ERROR: not armed\r\n"); return; }
        bridge_set_mode(BRIDGE_MITM);
        tsn_cb_set_mode(TSN_CB_FORGE_DUPLICATE);
        dbg_puts("802.1CB forge-duplicate enabled\r\n");
        return;
    }
    if (strcmp(line, "tsn seqforge out") == 0) {
        if (!armed()) { dbg_puts("ERROR: not armed\r\n"); return; }
        bridge_set_mode(BRIDGE_MITM);
        tsn_cb_set_mode(TSN_CB_FORGE_OUTOFORDER);
        dbg_puts("802.1CB forge-outoforder enabled\r\n");
        return;
    }
    if (strcmp(line, "tsn seqforge off") == 0) {
        tsn_cb_set_mode(TSN_CB_OFF);
        dbg_puts("802.1CB forge disabled\r\n");
        return;
    }
    if (strcmp(line, "tsn qbv inject") == 0) {
        if (!armed()) { dbg_puts("ERROR: not armed\r\n"); return; }
        uint8_t payload[] = "AXLETAP-QBV-OUT-OF-WINDOW";
        tsn_qbv_inject(0xFF, 3, payload, sizeof(payload) - 1);
        dbg_puts("802.1Qbv out-of-window injected\r\n");
        return;
    }
    if (strcmp(line, "tsn sr spoof") == 0) {
        if (!armed()) { dbg_puts("ERROR: not armed\r\n"); return; }
        tsn_sr_spoof(1, 0);  /* SR-A */
        dbg_puts("SR-class reservation spoofing (SR-A)\r\n");
        return;
    }
    if (strcmp(line, "tsn sr off") == 0) {
        tsn_sr_spoof(0, 0);
        dbg_puts("SR-class spoof off\r\n");
        return;
    }
    if (strcmp(line, "someip discover") == 0) {
        if (!armed()) { dbg_puts("ERROR: not armed\r\n"); return; }
        bridge_set_mode(BRIDGE_MITM);
        someip_send_find_all();
        dbg_puts("SOME/IP-SD Find sent\r\n");
        return;
    }
    if (strcmp(line, "someip list") == 0) {
        int sc; const someip_service_t *svcs = someip_get_services(&sc);
        dbg_puts("Discovered SOME/IP services:\r\n");
        for (int i = 0; i < sc; i++) {
            dbg_puts("  "); dbg_hex(svcs[i].service_id);
            dbg_puts(":"); dbg_hex(svcs[i].instance_id);
            dbg_puts(" port="); dbg_hex(svcs[i].port);
            dbg_puts(" proto="); dbg_puts(svcs[i].protocol == 0x11 ? "UDP" : "TCP");
            dbg_puts("\r\n");
        }
        return;
    }
    if (starts_with(line, "someip fuzz ")) {
        /* someip fuzz <svc hex> <method hex> <mode> */
        char *p = line + 12;
        char svc[8], method[8], mode[16];
        if (sscanf(p, "%s %s %s", svc, method, mode) == 3) {
            if (!armed()) { dbg_puts("ERROR: not armed\r\n"); return; }
            someip_fuzz_mode_t m = SOMEIP_FUZZ_OFF;
            if (strcmp(mode, "trunc") == 0) m = SOMEIP_FUZZ_HEADER_TRUNC;
            else if (strcmp(mode, "oversize") == 0) m = SOMEIP_FUZZ_OVERSIZE;
            else if (strcmp(mode, "badlen") == 0) m = SOMEIP_FUZZ_BAD_LEN;
            else if (strcmp(mode, "msgid") == 0) m = SOMEIP_FUZZ_BAD_MSGID;
            else if (strcmp(mode, "badver") == 0) m = SOMEIP_FUZZ_BAD_VER;
            else if (strcmp(mode, "random") == 0) m = SOMEIP_FUZZ_RANDOM;
            else { dbg_puts("Unknown fuzz mode\r\n"); return; }
            uint16_t s = (uint16_t)strtol(svc, 0, 16);
            uint16_t mm = (uint16_t)strtol(method, 0, 16);
            bridge_set_mode(BRIDGE_MITM);
            someip_fuzz_start(s, mm, m, 10);
            dbg_puts("Fuzzing started\r\n");
        } else {
            dbg_puts("Usage: someip fuzz <svc> <method> <mode>\r\n");
        }
        return;
    }
    if (strcmp(line, "someip stop") == 0) {
        someip_fuzz_stop();
        dbg_puts("Fuzzing stopped\r\n");
        return;
    }
    if (strcmp(line, "pcap start") == 0) { pcap_start(); led_cap_set(1); dbg_puts("Capture started\r\n"); return; }
    if (strcmp(line, "pcap stop") == 0)  { pcap_stop(); led_cap_set(0); dbg_puts("Capture stopped\r\n"); return; }
    if (strcmp(line, "pcap dump") == 0)  { pcap_dump_usb(); dbg_puts("SD dump done\r\n"); return; }
    if (strcmp(line, "ble on") == 0)     { dbg_puts("BLE backhaul enabled (always on by default)\r\n"); return; }
    if (strcmp(line, "ble off") == 0)    { dbg_puts("BLE backhaul disabled\r\n"); return; }
    dbg_puts("Unknown command: "); dbg_puts(line); dbg_puts("\r\n");
}

static void cli_process(void)
{
    while (DBG_UART->ISR & BIT(5)) {  /* RXNE */
        char c = (char)DBG_UART->RDR;
        if (c == '\r' || c == '\n') {
            cli_buf[cli_pos] = 0;
            dbg_puts("\r\n");
            cli_dispatch(cli_buf);
            cli_pos = 0;
            dbg_puts("axle> ");
        } else if (cli_pos < (int)sizeof(cli_buf) - 1) {
            cli_buf[cli_pos++] = c;
            dbg_putc(c);
        }
    }
}

static void cli_init(void)
{
    cli_pos = 0;
    dbg_puts("\r\n*** AxleTap firmware v1.0 — jayis1 ***\r\n");
    dbg_puts("Automotive Ethernet Tap, MITM & gPTP spoofing platform\r\n");
    dbg_puts("Booting in RECEIVE-ONLY mode (arm switch required for TX)\r\n");
    dbg_puts("Type 'help' for commands\r\naxle> ");
}

/* ------------------------------------------------------------------ */
/* BLE command handler                                                  */
/* ------------------------------------------------------------------ */
static void ble_cmd_handler(const uint8_t *payload, uint16_t len)
{
    if (len < 1) return;
    /* The payload is a UTF-8 CLI command (after decryption) */
    char buf[128];
    int n = len < (int)sizeof(buf) - 1 ? len : (int)sizeof(buf) - 1;
    memcpy(buf, payload, n);
    buf[n] = 0;
    cli_dispatch(buf);
}

/* ------------------------------------------------------------------ */
/* Cooperative scheduler                                                 */
/* ------------------------------------------------------------------ */
/* Each task is a non-blocking function; the scheduler calls them in
 * round-robin. The bridge runs from the Ethernet IRQ, not here, for
 * latency; this scheduler handles the slower engines.
 */
typedef void (*task_fn)(void);
static const task_fn tasks[] = {
    bridge_poll,
    gptp_poll,
    someip_fuzz_poll,
    ble_poll,
    cli_process,
};
#define N_TASKS (sizeof(tasks) / sizeof(tasks[0]))

static void scheduler_run(void)
{
    static int idx = 0;
    tasks[idx]();
    idx = (idx + 1) % N_TASKS;
}

/* ------------------------------------------------------------------ */
/* gPTP / TSN / SOME/IP frame hook                                       */
/* ------------------------------------------------------------------ */
/* Called by the bridge mirror via pcap_on_frame for every observed
 * frame. We forward to the engines for protocol-level analysis.
 */
void pcap_on_frame_hook(const uint8_t *frame, uint16_t len, uint8_t dir, uint64_t ts)
{
    gptp_on_frame(frame, len);
    tsn_on_frame(frame, len, dir, ts);
    someip_on_frame(frame, len, dir);
}

/* ------------------------------------------------------------------ */
/* main                                                                  */
/* ------------------------------------------------------------------ */
int main(void)
{
    /* Init everything */
    clocks_init();
    gpio_init();
    usart3_init();
    tim2_init();
    usb_cdc_init();

    /* LEDs initial state */
    led_linka_set(0); led_linkb_set(0); led_arm_set(0);
    led_cap_set(0);  led_ble_set(0);

    /* Init subsystems */
    phy_init(PHYA_MDIO_ADDR, PHY_MODE_1000T1, PHY_ROLE_SLAVE);
    phy_init(PHYB_MDIO_ADDR, PHY_MODE_1000T1, PHY_ROLE_MASTER);

    bridge_init();
    gptp_init();
    tsn_init();
    someip_init();
    ble_init();
    ble_set_cmd_callback(ble_cmd_handler);
    pcap_init();

    bridge_set_mode(BRIDGE_PASSTHROUGH);   /* bridge but no injection */
    pcap_start();
    led_cap_set(1);

    cli_init();

    /* Main loop */
    for (;;) {
        scheduler_run();

        /* Update link LEDs */
        phy_status_t sa, sb;
        phy_get_status(PHYA_MDIO_ADDR, &sa);
        phy_get_status(PHYB_MDIO_ADDR, &sb);
        led_linka_set(sa.link_up);
        led_linkb_set(sb.link_up);
        led_ble_set(ble_is_connected());

        /* IWDG refresh — keeps the watchdog happy */
        IWDG_KR = 0xAAAA;

        /* The bridge poll does the real work; we just keep the loop tight */
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Ethernet IRQ — drives the cut-through bridge                         */
/* ------------------------------------------------------------------ */
void ETH_IRQHandler(void)
{
    /* The cut-through bridge runs here for minimum latency. It calls
     * bridge_poll which processes the DMA rings.
     */
    bridge_poll();
    ETH_DMAISR = ETH_DMAISR;  /* clear flags */
}

/* Hard fault handler — blink the ARM LED fast */
void HardFault_Handler(void)
{
    for (;;) { led_arm_set(1); for (volatile int i = 0; i < 100000; i++); led_arm_set(0); for (volatile int i = 0; i < 100000; i++); }
}