/*
 * GHOSTBUS — Covert SWD/JTAG Hardware Debug Implant
 * Main Entry Point — Scheduler & Command Dispatch
 *
 * Initializes all subsystems, runs the main control loop, and dispatches
 * BLE commands to the appropriate debug (SWD/JTAG/discovery) subsystem.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#include "registers.h"
#include "board.h"
#include "drivers/swd_driver.h"
#include "drivers/jtag_driver.h"
#include "drivers/discovery_driver.h"
#include "drivers/ble_bridge.h"
#include "drivers/crypto_driver.h"
#include <string.h>

/* =========================================================================
 * Firmware version
 * ========================================================================= */
#define FW_VERSION_MAJOR    1
#define FW_VERSION_MINOR    0
#define FW_VERSION_PATCH    0
#define FW_VERSION_STR      "1.0.0"

/* =========================================================================
 * Global state
 * ========================================================================= */

typedef enum {
    STATE_IDLE = 0,
    STATE_DISCOVERY,
    STATE_SWD_CONNECTED,
    STATE_JTAG_CONNECTED,
    STATE_FLASH_DUMP,
    STATE_LOCKED
} device_state_t;

static device_state_t  g_state = STATE_IDLE;
static discovery_result_t g_discovery;
static swd_target_info_t  g_swd_info;
static jtag_target_info_t g_jtag_info;
static uint8_t          g_active_session = 0;
static uint32_t         g_dump_progress = 0;
static uint32_t         g_dump_total = 0;

/* LED status colors */
typedef enum { LED_OFF = 0, LED_RED, LED_GREEN, LED_BLUE, LED_YELLOW, LED_PURPLE } led_color_t;

static void led_set(led_color_t color)
{
    /* RGB on PE12 (B), PE13 (R, default LED), PE14 (R2), PE15 (G) */
    gpio_clear(GPIOE, 12); gpio_clear(GPIOE, 13);
    gpio_clear(GPIOE, 14); gpio_clear(GPIOE, 15);
    switch (color) {
        case LED_RED:    gpio_set(GPIOE, 14); break;
        case LED_GREEN:  gpio_set(GPIOE, 15); break;
        case LED_BLUE:   gpio_set(GPIOE, 12); break;
        case LED_YELLOW:  gpio_set(GPIOE, 14); gpio_set(GPIOE, 15); break;
        case LED_PURPLE:  gpio_set(GPIOE, 14); gpio_set(GPIOE, 12); break;
        default: break;
    }
}

/* =========================================================================
 * System clock configuration
 * Configure PLL1 on HSI (64 MHz) for 250 MHz system clock.
 * PLL1: VCO = HSI / M * N = 64/8 * 250 = 2000 MHz; SYSCLK = VCO / P = 2000/8 = 250
 * ========================================================================= */

static void clock_init(void)
{
    /* Enable HSI (already default on H5 reset, but ensure) */
    RCC->CR |= (1U << 8); /* HSION */
    while (!(RCC->CR & (1U << 10))) { /* HSIRDY */ }

    /* Configure PLL1: M=8, N=250, P=8, Q=not used */
    RCC->PLL1CFGR = (8U << 0)   /* M */
                  | (250U << 8) /* N */
                  | (0U << 16);  /* P=1 (encoded 0) — but we need P=8 */
    /* (On STM32H5, PLL1DIVR holds P/Q/R divisors. Real register layout varies.
     *  This is a simplified reference; production uses the H5 HAL.) */
    RCC->PLL1DIVR = (8U << 0) /* P */
                  | (0U << 9)  /* Q */
                  | (0U << 16);/* R */
    /* Enable PLL1 */
    RCC->CR |= (1U << 24); /* PLL1ON */
    while (!(RCC->CR & (1U << 25))) { /* PLL1RDY */ }

    /* Switch SYSCLK to PLL1 */
    RCC->CFGR = (3U << 0); /* SW = PLL1 */
    while (((RCC->CFGR >> 3) & 0x3) != 3) { /* wait SWS = PLL1 */ }
}

static void gpio_clocks_init(void)
{
    /* Enable all GPIO ports we use */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN |
                    RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOHEN;
    (void)RCC->AHB1ENR;

    /* Configure LED pins as outputs */
    gpio_set_mode(GPIOE, 12, GPIO_MODE_OUTPUT);
    gpio_set_otyper(GPIOE, 12, GPIO_OTYPE_PP);
    gpio_set_mode(GPIOE, 13, GPIO_MODE_OUTPUT);
    gpio_set_otyper(GPIOE, 13, GPIO_OTYPE_PP);
    gpio_set_mode(GPIOE, 14, GPIO_MODE_OUTPUT);
    gpio_set_otyper(GPIOE, 14, GPIO_OTYPE_PP);
    gpio_set_mode(GPIOE, 15, GPIO_MODE_OUTPUT);
    gpio_set_otyper(GPIOE, 15, GPIO_OTYPE_PP);

    /* Configure button as input with pull-up */
    gpio_set_mode(GPIOC, 13, GPIO_MODE_INPUT);
    gpio_set_pupd(GPIOC, 13, GPIO_PUPD_PULLUP);
}

/* =========================================================================
 * SysTick millisecond counter
 * ========================================================================= */

static volatile uint32_t g_ticks_ms = 0;

void SysTick_Handler(void)
{
    g_ticks_ms++;
}

static void systick_init(void)
{
    SysTick_RVR = (SYSTEM_CLOCK_HZ / 1000) - 1;
    SysTick_CVR = 0;
    SysTick_CSR = SysTick_CSR_ENABLE | SysTick_CSR_CLKSRC | SysTick_CSR_TICKINT;
}

static uint32_t millis(void)
{
    return g_ticks_ms;
}

static void delay_ms(uint32_t ms)
{
    uint32_t start = millis();
    while ((millis() - start) < ms) { }
}

/* =========================================================================
 * Progress callback for flash extraction
 * ========================================================================= */

static void flash_progress(uint32_t done, uint32_t total)
{
    g_dump_progress = done;
    g_dump_total = total;
    /* Send a telemetry event every 4 KB */
    if ((done & 0xFFF) == 0 && done > 0) {
        char buf[64];
        /* Minimal integer-to-string formatting (no printf) */
        uint32_t pct = (total > 0) ? (done * 100 / total) : 0;
        buf[0] = 'P'; buf[1] = ':'; buf[2] = ' ';
        int p = 3;
        if (pct == 0) { buf[p++] = '0'; }
        else {
            char tmp[4]; int n = 0;
            while (pct > 0) { tmp[n++] = (char)('0' + (pct % 10)); pct /= 10; }
            while (n > 0) buf[p++] = tmp[--n];
        }
        buf[p++] = '%'; buf[p] = 0;
        ble_bridge_send_event(buf);
    }
}

/* =========================================================================
 * Command handlers
 * ========================================================================= */

static gb_status_t handle_discover(gb_message_t *msg)
{
    /* Parse requested protocols from payload (simplified: accept all) */
    probe_protocol_t protos[3] = { PROTO_SWD, PROTO_JTAG, PROTO_CJTAG };
    uint8_t n_protos = 3;
    if (msg && msg->payload_len >= 1) {
        n_protos = msg->payload[0];
        if (n_protos > 3) n_protos = 3;
    }

    g_state = STATE_DISCOVERY;
    led_set(LED_BLUE);

    discovery_result_t result;
    gb_status_t st = discovery_scan(protos, n_protos, &result,
                                     DISCOVERY_TIMEOUT_MS);
    if (st != GB_OK) {
        ble_bridge_send_event("{\"event\":\"discovery_failed\"}");
        g_state = STATE_IDLE;
        led_set(LED_RED);
        return st;
    }
    g_discovery = result;

    /* Send discovery result to operator */
    char evt[160];
    /* Build a JSON event (manual to avoid printf) */
    /* For brevity, we send a binary telemetry frame: [protocol][idcode(4)]
     * [swdio_ch][swclk_ch][tck_ch][tms_ch][tdi_ch][tdo_ch][gnd_ch][vref_ch]
     * [vref_mv(2)][confidence] */
    uint8_t frame[16];
    frame[0] = (uint8_t)result.protocol;
    frame[1] = (uint8_t)(result.idcode & 0xFF);
    frame[2] = (uint8_t)((result.idcode >> 8) & 0xFF);
    frame[3] = (uint8_t)((result.idcode >> 16) & 0xFF);
    frame[4] = (uint8_t)((result.idcode >> 24) & 0xFF);
    frame[5] = result.swdio_ch;
    frame[6] = result.swclk_ch;
    frame[7] = result.tck_ch;
    frame[8] = result.tms_ch;
    frame[9] = result.tdi_ch;
    frame[10] = result.tdo_ch;
    frame[11] = result.gnd_ch;
    frame[12] = result.vref_ch;
    frame[13] = (uint8_t)(result.vref_mv & 0xFF);
    frame[14] = (uint8_t)((result.vref_mv >> 8) & 0xFF);
    frame[15] = result.confidence;
    ble_bridge_send_telemetry(frame, 16);

    if (result.protocol == PROTO_SWD) {
        /* Connect SWD */
        swd_init((probe_channel_t)result.swdio_ch,
                  (probe_channel_t)result.swclk_ch);
        st = swd_connect(&g_swd_info);
        if (st == GB_OK) {
            g_state = STATE_SWD_CONNECTED;
            led_set(LED_GREEN);
        } else {
            g_state = STATE_IDLE;
            led_set(LED_RED);
        }
    } else if (result.protocol == PROTO_JTAG) {
        jtag_init((probe_channel_t)result.tck_ch,
                   (probe_channel_t)result.tms_ch,
                   (probe_channel_t)result.tdi_ch,
                   (probe_channel_t)result.tdo_ch);
        st = jtag_scan_chain(&g_jtag_info);
        if (st == GB_OK) {
            g_state = STATE_JTAG_CONNECTED;
            led_set(LED_GREEN);
        } else {
            g_state = STATE_IDLE;
            led_set(LED_RED);
        }
    }
    (void)evt;
    return st;
}

static gb_status_t handle_flash_extract(gb_message_t *msg)
{
    if (g_state != STATE_SWD_CONNECTED) return GB_ERR_NO_TARGET;
    if (!msg || msg->payload_len < 8) return GB_ERR_PARAM;
    uint32_t addr = ((uint32_t)msg->payload[0]) |
                    ((uint32_t)msg->payload[1] << 8) |
                    ((uint32_t)msg->payload[2] << 16) |
                    ((uint32_t)msg->payload[3] << 24);
    uint32_t len  = ((uint32_t)msg->payload[4]) |
                    ((uint32_t)msg->payload[5] << 8) |
                    ((uint32_t)msg->payload[6] << 16) |
                    ((uint32_t)msg->payload[7] << 24);
    if (len > FLASH_MAX_DUMP_BYTES) return GB_ERR_PARAM;

    static uint8_t dump_buf[FLASH_BLOCK_SIZE];
    g_state = STATE_FLASH_DUMP;
    led_set(LED_PURPLE);

    /* Extract in 4 KB blocks, send each over BLE */
    uint32_t offset = 0;
    while (offset < len) {
        uint32_t chunk = MIN(FLASH_BLOCK_SIZE, len - offset);
        gb_status_t st = swd_mem_read8(addr + offset, dump_buf, chunk);
        if (st != GB_OK) {
            ble_bridge_send_event("{\"event\":\"dump_failed\"}");
            g_state = STATE_SWD_CONNECTED;
            led_set(LED_GREEN);
            return st;
        }
        /* Compute SHA-256 for this block */
        uint8_t hash[32];
        crypto_sha256(dump_buf, chunk, hash);
        /* Send block: [offset(4)][len(4)][hash(32)][data...] */
        static uint8_t sendbuf[FLASH_BLOCK_SIZE + 40];
        sendbuf[0] = (uint8_t)(offset & 0xFF);
        sendbuf[1] = (uint8_t)((offset >> 8) & 0xFF);
        sendbuf[2] = (uint8_t)((offset >> 16) & 0xFF);
        sendbuf[3] = (uint8_t)((offset >> 24) & 0xFF);
        sendbuf[4] = (uint8_t)(chunk & 0xFF);
        sendbuf[5] = (uint8_t)((chunk >> 8) & 0xFF);
        sendbuf[6] = (uint8_t)((chunk >> 16) & 0xFF);
        sendbuf[7] = (uint8_t)((chunk >> 24) & 0xFF);
        memcpy(sendbuf + 8, hash, 32);
        memcpy(sendbuf + 40, dump_buf, chunk);
        /* Send as BLE data chunks (chunked internally by ble_bridge_send_data) */
        ble_bridge_send_data(g_active_session, sendbuf, 40 + chunk);
        flash_progress(offset + chunk, len);
        offset += chunk;
    }
    ble_bridge_send_event("{\"event\":\"dump_complete\"}");
    g_state = STATE_SWD_CONNECTED;
    led_set(LED_GREEN);
    return GB_OK;
}

static gb_status_t handle_mem_read(gb_message_t *msg)
{
    if (g_state != STATE_SWD_CONNECTED) return GB_ERR_NO_TARGET;
    if (!msg || msg->payload_len < 8) return GB_ERR_PARAM;
    uint32_t addr = ((uint32_t)msg->payload[0]) |
                    ((uint32_t)msg->payload[1] << 8) |
                    ((uint32_t)msg->payload[2] << 16) |
                    ((uint32_t)msg->payload[3] << 24);
    uint32_t len  = ((uint32_t)msg->payload[4]) |
                    ((uint32_t)msg->payload[5] << 8) |
                    ((uint32_t)msg->payload[6] << 16) |
                    ((uint32_t)msg->payload[7] << 24);
    if (len > 256) len = 256;
    static uint8_t rdbuf[256];
    gb_status_t st = swd_mem_read8(addr, rdbuf, len);
    if (st != GB_OK) return st;
    return ble_bridge_send_data(g_active_session, rdbuf, len);
}

static gb_status_t handle_swd_halt(void)
{
    if (g_state != STATE_SWD_CONNECTED) return GB_ERR_NO_TARGET;
    return swd_halt();
}

static gb_status_t handle_swd_resume(void)
{
    if (g_state != STATE_SWD_CONNECTED) return GB_ERR_NO_TARGET;
    return swd_resume();
}

static gb_status_t handle_power_on(void)
{
    /* Drive target-power channel high (current-limited) */
    gpio_set_mode(GPIOC, 1, GPIO_MODE_OUTPUT);
    gpio_set_otyper(GPIOC, 1, GPIO_OTYPE_PP);
    gpio_set(GPIOC, 1);
    return GB_OK;
}

static gb_status_t handle_power_off(void)
{
    gpio_clear(GPIOC, 1);
    gpio_set_mode(GPIOC, 1, GPIO_MODE_INPUT);
    return GB_OK;
}

static gb_status_t handle_status(void)
{
    uint8_t status_frame[8];
    status_frame[0] = (uint8_t)g_state;
    status_frame[1] = (uint8_t)g_discovery.protocol;
    /* Battery: would read MAX17048 via I2C; placeholder */
    status_frame[2] = 85; /* battery % */
    status_frame[3] = (uint8_t)(g_dump_progress & 0xFF);
    status_frame[4] = (uint8_t)((g_dump_progress >> 8) & 0xFF);
    status_frame[5] = (uint8_t)((g_dump_progress >> 16) & 0xFF);
    status_frame[6] = (uint8_t)((g_dump_progress >> 24) & 0xFF);
    status_frame[7] = ble_bridge_is_connected();
    return ble_bridge_send_telemetry(status_frame, 8);
}

static gb_status_t handle_lock(void)
{
    g_state = STATE_LOCKED;
    led_set(LED_RED);
    ble_bridge_disconnect();
    return GB_OK;
}

static gb_status_t handle_jtag_scan(gb_message_t *msg)
{
    if (g_state != STATE_JTAG_CONNECTED) return GB_ERR_NO_TARGET;
    /* Send chain info */
    uint8_t chain_frame[40];
    memset(chain_frame, 0, sizeof(chain_frame));
    chain_frame[0] = g_jtag_info.chain_count;
    chain_frame[1] = g_jtag_info.ir_len;
    chain_frame[2] = (uint8_t)(g_jtag_info.idcode & 0xFF);
    chain_frame[3] = (uint8_t)((g_jtag_info.idcode >> 8) & 0xFF);
    chain_frame[4] = (uint8_t)((g_jtag_info.idcode >> 16) & 0xFF);
    chain_frame[5] = (uint8_t)((g_jtag_info.idcode >> 24) & 0xFF);
    for (int i = 0; i < 8; i++) {
        chain_frame[6 + i*4] = (uint8_t)(g_jtag_info.chain_idcodes[i] & 0xFF);
        chain_frame[7 + i*4] = (uint8_t)((g_jtag_info.chain_idcodes[i] >> 8) & 0xFF);
        chain_frame[8 + i*4] = (uint8_t)((g_jtag_info.chain_idcodes[i] >> 16) & 0xFF);
        chain_frame[9 + i*4] = (uint8_t)((g_jtag_info.chain_idcodes[i] >> 24) & 0xFF);
    }
    return ble_bridge_send_telemetry(chain_frame, 40);
}

/* =========================================================================
 * Command dispatcher
 * ========================================================================= */

static void dispatch_command(gb_message_t *msg)
{
    gb_status_t st = GB_OK;
    switch (msg->type) {
        case GB_MSG_DISCOVER:       st = handle_discover(msg); break;
        case GB_MSG_SWD_HALT:        st = handle_swd_halt(); break;
        case GB_MSG_SWD_RESUME:      st = handle_swd_resume(); break;
        case GB_MSG_FLASH_EXTRACT:   st = handle_flash_extract(msg); break;
        case GB_MSG_MEM_READ:        st = handle_mem_read(msg); break;
        case GB_MSG_POWER_ON:        st = handle_power_on(); break;
        case GB_MSG_POWER_OFF:       st = handle_power_off(); break;
        case GB_MSG_JTAG_SCAN:       st = handle_jtag_scan(msg); break;
        case GB_MSG_STATUS:         st = handle_status(); break;
        case GB_MSG_LOCK:            st = handle_lock(); break;
        default: st = GB_ERR_UNSUPPORTED; break;
    }
    /* Send a status response */
    uint8_t resp[2] = { (uint8_t)msg->type, (uint8_t)(st & 0xFF) };
    ble_bridge_send_telemetry(resp, 2);
}

/* =========================================================================
 * Button handling
 * ========================================================================= */

static uint8_t button_pressed(void)
{
    return (gpio_read(BUTTON_PORT, BUTTON_PIN) == 0) ? 1 : 0; /* active low */
}

static void handle_button(void)
{
    static uint32_t press_start = 0;
    static uint8_t  was_pressed = 0;
    if (button_pressed()) {
        if (!was_pressed) {
            press_start = millis();
            was_pressed = 1;
        }
    } else {
        if (was_pressed) {
            uint32_t dur = millis() - press_start;
            if (dur > 50 && dur < 500) {
                /* Short press: re-run discovery if idle */
                if (g_state == STATE_IDLE) {
                    gb_message_t msg = { .type = GB_MSG_DISCOVER,
                                          .payload = NULL, .payload_len = 0 };
                    dispatch_command(&msg);
                }
            } else if (dur >= 2000) {
                /* Long press: lock/disconnect */
                handle_lock();
            }
            was_pressed = 0;
        }
    }
}

/* =========================================================================
 * Main
 * ========================================================================= */

int main(void)
{
    /* Disable interrupts during init */
    __asm volatile ("cpsid i");

    /* Initialize clocks and GPIO */
    clock_init();
    gpio_clocks_init();
    systick_init();

    led_set(LED_RED);
    delay_ms(200);

    /* Initialize subsystems */
    crypto_init();
    ble_bridge_init();
    discovery_init();

    led_set(LED_OFF);
    delay_ms(100);

    /* Enable interrupts */
    __asm volatile ("cpsie i");

    /* Main loop */
    uint32_t last_status = 0;
    while (1) {
        /* Poll for BLE commands */
        gb_message_t msg;
        int rc = ble_bridge_recv_message(&msg, 50);
        if (rc == 0) {
            dispatch_command(&msg);
        }

        /* Handle button */
        handle_button();

        /* Periodic status beacon (every 5 s) */
        if (ble_bridge_is_connected() && (millis() - last_status) > 5000) {
            handle_status();
            last_status = millis();
        }

        /* Low-power: WFI between iterations if idle */
        if (g_state == STATE_IDLE && !button_pressed()) {
            __asm volatile ("wfi");
        }
    }

    return 0;
}

/* =========================================================================
 * HardFault handler — blink LED red rapidly to indicate fault
 * ========================================================================= */

void HardFault_Handler(void)
{
    while (1) {
        led_set(LED_RED);
        delay_ms(50);
        led_set(LED_OFF);
        delay_ms(50);
    }
}

/* EOF — GHOSTBUS main.c — Author: jayis1 */