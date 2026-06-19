/**
 * main.c — PhotonStrike M7 application core
 * Author: jayis1
 * License: GPL-2.0
 *
 * The M7 runs the scan automaton, the DFA solver, the BLE protocol, SD
 * logging, and the USB CDC console. The M4 (m4_core.c) handles the
 * real-time trigger arbiter and safety interlock; the M7 talks to it
 * through a small mailbox in D3 SRAM.
 *
 * No external RTOS is used — a cooperative main loop plus interrupt-
 * driven UART/SD DMA is sufficient and keeps the hot path predictable.
 * The laser is never gated by the M7; only the M4 and the hardware
 * interlock can enable it.
 *
 * Build: arm-none-eabi-gcc via the Makefile (target: photonstrike_m7.elf)
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "board.h"
#include "registers.h"

/* Driver prototypes (see drivers/*.c) */
#include "drivers/gpio.h"
#include "drivers/laser.h"
#include "drivers/trigger.h"
#include "drivers/fpga.h"
#include "drivers/mems.h"
#include "drivers/dfa.h"
#include "drivers/ble_uart.h"
#include "drivers/sdmmc.h"
#include "drivers/usb_dev.h"
#include "drivers/timing.h"

/* ─── M4 mailbox (D3 SRAM) ────────────────────────────────────────────── */
typedef struct __attribute__((packed)) {
    uint32_t magic;            /* M4_BOOT_MAGIC when M4 is alive             */
    uint32_t safety_word;      /* bit0 interlock bit1 key bit2 estop bit3 temp*/
    uint32_t arm_req;          /* M7 → M4: request arm                       */
    uint32_t disarm_req;       /* M7 → M4: request disarm                    */
    uint32_t armed_ack;        /* M4 → M7: armed confirmation                */
    uint32_t shot_count;       /* M4 → M7: shots fired since boot            */
    uint32_t last_fault;       /* M4 → M7: last driver fault code            */
} ps_m4_mailbox_t;

#define M4_MAILBOX ((volatile ps_m4_mailbox_t *)M4_RELEASE_REG)

/* ─── Globals ─────────────────────────────────────────────────────────── */
static ps_scan_desc_t  g_scan;          /* current scan descriptor         */
static bool            g_scan_active;   /* a scan is running               */
static uint32_t        g_seq;           /* global shot sequence counter    */
static uint32_t        g_shots_this_scan;
static uint32_t        g_faults_this_scan;
static uint32_t        g_dfa_faults_collected;
static char            g_log_name[16];  /* current SD log file name        */
static uint32_t        g_target_clk_hz; /* measured target clock           */

/* DFA solver state (AES-128 Piret-Quisquater) */
static ps_dfa_state_t  g_dfa;

/* Expected (correct) output for the current scan */
static uint8_t         g_expected[64];
static uint16_t        g_expected_len;
static uint32_t        g_expected_hash;

/* ─── CRC32 (used for output hashing & scan descriptor validation) ────── */
static uint32_t crc32_table[256];
static bool     crc32_init_done;

static void crc32_build_table(void)
{
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int k = 0; k < 8; k++)
            c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        crc32_table[i] = c;
    }
    crc32_init_done = true;
}

static uint32_t crc32(const uint8_t *data, uint32_t len)
{
    if (!crc32_init_done) crc32_build_table();
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < len; i++)
        crc = crc32_table[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

/* ─── Hamming weight / distance helpers ───────────────────────────────── */
static int popcount8(uint8_t v)
{
    int c = 0;
    while (v) { c += (v & 1u); v >>= 1; }
    return c;
}

static int hamming_bytes(const uint8_t *a, const uint8_t *b, uint32_t len)
{
    int d = 0;
    for (uint32_t i = 0; i < len; i++)
        d += popcount8((uint8_t)(a[i] ^ b[i]));
    return d;
}

static int bytes_differ(const uint8_t *a, const uint8_t *b, uint32_t len)
{
    int n = 0;
    for (uint32_t i = 0; i < len; i++)
        if (a[i] != b[i]) n++;
    return n;
}

/* ─── Fault classifier ────────────────────────────────────────────────── */
static ps_fault_class_t classify_fault(const uint8_t *got, uint16_t got_len,
                                       const uint8_t *expected, uint16_t exp_len)
{
    if (got_len == 0)
        return FAULT_CRASH;
    if (got_len != exp_len)
        return FAULT_MULTI_BYTE;
    if (memcmp(got, expected, exp_len) == 0)
        return FAULT_NO_CHANGE;
    int hd = hamming_bytes(got, expected, exp_len);
    int bd = bytes_differ(got, expected, exp_len);
    if (hd == 1)  return FAULT_SINGLE_BIT;
    if (bd == 1)  return FAULT_SINGLE_BYTE;
    if (bd <= 4)  return FAULT_MULTI_BYTE;   /* may still be DFA-usable */
    return FAULT_MULTI_BYTE;
}

/* ─── Safety check (M7 side — M4 has the final word) ──────────────────── */
static bool safety_ok(void)
{
    /* The M4 writes the live safety word; the M7 only reads it. */
    uint32_t w = M4_MAILBOX->safety_word;
    /* all four bits must be set for the laser to be allowed */
    return (w & 0xFu) == 0xFu;
}

/* ─── Boot the M4 core ────────────────────────────────────────────────── */
static void boot_m4(void)
{
    /* Set the M4 boot vector to the start of the M4 image in flash. */
    REG32(SCB_BASE + 0x08) = 0;  /* (M4 VTOR set internally by CM4 boot) */

    /* Drop the M4 mailbox magic so we can detect M4 liveness. */
    M4_MAILBOX->magic = 0;
    M4_MAILBOX->arm_req = 0;
    M4_MAILBOX->disarm_req = 0;
    M4_MAILBOX->armed_ack = 0;

    /* Release the M4 from reset (RCC_CCIPRx / DBGMCU CR bits on H7).
     * On STM32H7 the M4 is held in reset at boot; setting the
     * BOOT_M4 bit in RCC_MC_CFGR releases it. */
    REG32(0x58024800u + 0x80u) |= (1u << 16);   /* RCC_MC_CFGR.BOOT_M4 */

    /* Wait for the M4 to set its magic. */
    for (uint32_t i = 0; i < 1000000u; i++) {
        if (M4_MAILBOX->magic == M4_BOOT_MAGIC) break;
        ps_delay_us(1);
    }
}

/* ─── Scan control ────────────────────────────────────────────────────── */

/** Validate a scan descriptor against the compiled-in safety envelope. */
static bool scan_validate(const ps_scan_desc_t *s)
{
    if (s->magic != PS_SCAN_MAGIC) return false;
    if (s->width_start_ns < PS_MIN_PULSE_NS || s->width_stop_ns > PS_MAX_PULSE_NS)
        return false;
    if (s->width_step_ns == 0 && s->width_start_ns != s->width_stop_ns)
        return false;
    if (s->x_step == 0 || s->y_step == 0) return false;
    if (s->energy_stop > PS_MAX_ENERGY_NJ * 200u) return false;  /* DAC scale */
    if (s->shots_per_point == 0 || s->shots_per_point > 16) return false;
    if (s->delay_step_ps == 0 && s->delay_start_ps != s->delay_stop_ps)
        return false;
    return true;
}

/** Configure FPGA + MEMS + laser for a single shot at a given setpoint. */
static void program_shot(uint16_t x, uint16_t y, uint32_t delay_ps,
                         uint16_t width_ns, uint16_t energy)
{
    /* Convert to FPGA units (250 ps ticks). */
    uint32_t delay_ticks = delay_ps / 250u;
    uint32_t width_ticks = (width_ns * 1000u) / 250u;

    fpga_cmd(FPGA_CMD_MEMS_GOTO, x, y, 0);
    ps_delay_us(800);  /* MEMS settle (~2 ms worst case, usually faster) */
    fpga_cmd(FPGA_CMD_SET_DELAY,    delay_ticks, 0, 0);
    fpga_cmd(FPGA_CMD_SET_PULSE_WIDTH, width_ticks, 0, 0);
    fpga_cmd(FPGA_CMD_SET_ENERGY,   energy, 0, 0);
    laser_set_current(energy);
}

/** Arm, wait for the trigger to fire, capture the result, disarm. */
static ps_shot_t fire_one_shot(uint8_t trig_src, uint16_t x, uint16_t y,
                               uint32_t delay_ps, uint16_t width_ns,
                               uint16_t energy)
{
    ps_shot_t shot;
    memset(&shot, 0, sizeof(shot));
    shot.seq        = ++g_seq;
    shot.x_um       = x;
    shot.y_um       = y;
    shot.delay_ps   = delay_ps;
    shot.width_ns   = width_ns;
    shot.energy     = energy;
    shot.trig_src   = trig_src;
    shot.target_clk_khz = (int16_t)(g_target_clk_hz / 1000u);
    shot.laser_temp_c   = laser_read_temp();

    /* Ask the M4 to arm. */
    M4_MAILBOX->arm_req = 1;
    uint32_t to = 0;
    while (!M4_MAILBOX->armed_ack) {
        if (++to > 50000u) goto done;
        ps_delay_us(1);
    }
    M4_MAILBOX->arm_req = 0;

    /* The M4 will fire the laser when the trigger fires; it logs the
     * shot in shot_count. We wait for the shot_count to increment or
     * for a timeout, then read back the target output (supplied by
     * the operator over BLE or snooped on UART — here we poll the BLE
     * ring buffer). */
    uint32_t prev = M4_MAILBOX->shot_count;
    to = 0;
    while (M4_MAILBOX->shot_count == prev) {
        if (++to > 2000000u) {            /* 2 s timeout */
            shot.fault_class = FAULT_TIMEOUT;
            M4_MAILBOX->disarm_req = 1;
            goto done;
        }
        ps_delay_us(1);
    }

    /* Disarm immediately after the shot. */
    M4_MAILBOX->disarm_req = 1;
    ps_delay_us(50);

    /* Read back the target output from the BLE ring buffer (the operator
     * or a snoop supplies it). If none arrives in 200 ms, mark CRASH. */
    uint16_t got_len = 0;
    uint8_t  got[64];
    if (ble_take_output(got, &got_len, 64, 200)) {
        shot.output_len = (got_len > 32) ? 32 : (uint8_t)got_len;
        memcpy(shot.output, got, shot.output_len);
        shot.output_hash = crc32(got, got_len);
        shot.fault_class = classify_fault(got, got_len,
                                          g_expected, g_expected_len);
    } else {
        shot.fault_class = FAULT_CRASH;
    }

done:
    return shot;
}

/** Run the full raster scan described by g_scan. */
static void run_scan(void)
{
    g_shots_this_scan = 0;
    g_faults_this_scan = 0;
    g_dfa_faults_collected = 0;

    /* Measure the target clock once at scan start. */
    fpga_cmd(FPGA_CMD_MEASURE_CLK, 0, 0, 0);
    ps_delay_us(5000);
    g_target_clk_hz = fpga_read_clk();

    /* Open a new log file. */
    sd_open_log(g_log_name);

    /* Inform the app that the scan has started. */
    ble_send_status(BLE_MSG_STATUS, 0xFFFFFFFFu, 0, g_target_clk_hz);

    for (uint16_t x = g_scan.x_start; x <= g_scan.x_stop; x += g_scan.x_step) {
        for (uint16_t y = g_scan.y_start; y <= g_scan.y_stop; y += g_scan.y_step) {
            for (uint32_t d = g_scan.delay_start_ps; d <= g_scan.delay_stop_ps;
                 d += g_scan.delay_step_ps) {
                for (uint16_t w = g_scan.width_start_ns; w <= g_scan.width_stop_ns;
                     w += g_scan.width_step_ns) {
                    for (uint16_t e = g_scan.energy_start; e <= g_scan.energy_stop;
                         e += g_scan.energy_step) {
                        for (uint16_t r = 0; r < g_scan.shots_per_point; r++) {

                            if (!safety_ok()) {
                                ble_send_status(BLE_MSG_STATUS, 0, 0xDEADu, 0);
                                goto scan_abort;
                            }

                            program_shot(x, y, d, w, e);
                            ps_shot_t shot = fire_one_shot(g_scan.trig_src,
                                                           x, y, d, w, e);

                            g_shots_this_scan++;
                            if (shot.fault_class != FAULT_NO_CHANGE)
                                g_faults_this_scan++;

                            sd_log_shot(&shot);
                            ble_send_shot(&shot);

                            /* Feed usable faults into the DFA solver. */
                            if (g_scan.dfa_mode == 1 &&
                                (shot.fault_class == FAULT_SINGLE_BYTE ||
                                 shot.fault_class == FAULT_MULTI_BYTE) &&
                                shot.output_len >= 16 &&
                                g_dfa_faults_collected < PS_DFA_MAX_FAULTS) {
                                dfa_feed(&g_dfa,
                                         g_expected,    /* correct ciphertext */
                                         shot.output,   /* faulted ciphertext */
                                         shot.output_len);
                                g_dfa_faults_collected++;

                                /* Try to solve after each new fault. */
                                uint8_t key[16];
                                int unique = dfa_solve(&g_dfa, key);
                                ble_send_dfa(g_dfa_faults_collected, unique, key);
                                if (unique == 16) {
                                    /* Full key recovered — abort scan. */
                                    ble_send_status(BLE_MSG_STATUS,
                                                    g_shots_this_scan,
                                                    0xA11Cu, 0);
                                    goto scan_abort;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

scan_abort:
    M4_MAILBOX->disarm_req = 1;
    sd_close_log();
    g_scan_active = false;
    ble_send_status(BLE_MSG_STATUS, g_shots_this_scan, 0xF1N15Hu, 0);
}

/* ─── BLE command dispatcher ──────────────────────────────────────────── */
static void handle_ble_message(uint8_t op, const uint8_t *payload, uint16_t len)
{
    switch (op) {
    case BLE_MSG_HELLO:
        ble_send_ack(BLE_MSG_HELLO, BOARD_VERSION);
        break;

    case BLE_MSG_SCAN_DESC: {
        if (len < sizeof(ps_scan_desc_t)) {
            ble_send_nack(BLE_MSG_SCAN_DESC, 1);
            break;
        }
        memcpy(&g_scan, payload, sizeof(g_scan));
        if (!scan_validate(&g_scan)) {
            ble_send_nack(BLE_MSG_SCAN_DESC, 2);
            break;
        }
        /* Copy the expected output (sent in a follow-up message). */
        ble_send_ack(BLE_MSG_SCAN_DESC, 0);
        break;
    }

    case BLE_MSG_LOG: {
        /* The app sent us the expected (correct) output block. */
        if (len <= sizeof(g_expected)) {
            memcpy(g_expected, payload, len);
            g_expected_len = len;
            g_expected_hash = crc32(g_expected, g_expected_len);
            ble_send_ack(BLE_MSG_LOG, g_expected_len);
        } else {
            ble_send_nack(BLE_MSG_LOG, 1);
        }
        break;
    }

    case BLE_MSG_CONTROL: {
        uint8_t cmd = payload[0];
        switch (cmd) {
        case 0x01:  /* start scan */
            if (g_scan_active) { ble_send_nack(BLE_MSG_CONTROL, 1); break; }
            if (!safety_ok())  { ble_send_nack(BLE_MSG_CONTROL, 2); break; }
            if (g_scan.dfa_mode == 1)
                dfa_init(&g_dfa);
            g_scan_active = true;
            /* run_scan() is invoked from the main loop, not here, so we
             * don't block the BLE ISR. */
            break;
        case 0x02:  /* abort scan */
            g_scan_active = false;
            M4_MAILBOX->disarm_req = 1;
            ble_send_ack(BLE_MSG_CONTROL, 0);
            break;
        case 0x03:  /* laser enable (requires safety_ok) */
            if (safety_ok()) {
                laser_enable();
                ble_send_ack(BLE_MSG_CONTROL, 0);
            } else {
                ble_send_nack(BLE_MSG_CONTROL, 3);
            }
            break;
        case 0x04:  /* laser disable */
            laser_disable();
            ble_send_ack(BLE_MSG_CONTROL, 0);
            break;
        case 0x05:  /* emergency stop */
            laser_disable();
            M4_MAILBOX->disarm_req = 1;
            g_scan_active = false;
            ble_send_ack(BLE_MSG_CONTROL, 0);
            break;
        default:
            ble_send_nack(BLE_MSG_CONTROL, 0xFFu);
        }
        break;
    }

    default:
        ble_send_nack(op, 0xFFu);
    }
}

/* ─── USB CDC console (simple text commands) ──────────────────────────── */
static void usb_console_line(const char *line)
{
    if (strcmp(line, "version") == 0) {
        usb_print("PhotonStrike " BOARD_NAME " v1.0.0  author: " BOARD_AUTHOR "\r\n");
    } else if (strcmp(line, "safety") == 0) {
        uint32_t w = M4_MAILBOX->safety_word;
        usb_printf("safety_word=0x%08lx  ok=%d  interlock=%ld key=%ld estop=%ld temp=%ld\r\n",
                   (unsigned long)w, (int)safety_ok(),
                   (long)(w & 1u), (long)((w >> 1) & 1u),
                   (long)((w >> 2) & 1u), (long)((w >> 3) & 1u));
    } else if (strcmp(line, "temp") == 0) {
        usb_printf("laser_temp=%u C\r\n", laser_read_temp());
    } else if (strcmp(line, "clk") == 0) {
        fpga_cmd(FPGA_CMD_MEASURE_CLK, 0, 0, 0);
        ps_delay_us(5000);
        usb_printf("target_clk=%lu Hz\r\n", (unsigned long)fpga_read_clk());
    } else if (strcmp(line, "disarm") == 0) {
        M4_MAILBOX->disarm_req = 1;
        usb_print("disarmed\r\n");
    } else if (strcmp(line, "kill") == 0) {
        laser_disable();
        M4_MAILBOX->disarm_req = 1;
        g_scan_active = false;
        usb_print("KILLED\r\n");
    } else if (strcmp(line, "help") == 0) {
        usb_print("commands: version safety temp clk disarm kill help\r\n");
    } else {
        usb_print("unknown (try 'help')\r\n");
    }
}

/* ─── Main loop ───────────────────────────────────────────────────────── */
int main(void)
{
    /* 1. Low-level board init (clocks, GPIO defaults, peripherals). */
    ps_board_init();
    crc32_build_table();

    /* 2. Bring up the status LED first so the operator sees life. */
    ps_led_set(0, true);  /* green status on */

    /* 3. Boot the M4 (trigger + safety core). */
    boot_m4();

    /* 4. Initialize the drivers in dependency order. */
    timing_init();
    gpio_init_all();
    fpga_init();          /* loads bitstream over SPI */
    mems_init();
    laser_init();
    trigger_init();
    ble_init();
    sd_init();
    usb_init();

    /* 5. Wait for FPGA to report ready. */
    uint32_t to = 0;
    while (!fpga_ready()) {
        if (++to > 100000u) { ps_led_set(2, true); break; }
        ps_delay_us(10);
    }

    /* 6. Advertise presence on BLE. */
    ble_send_hello(BOARD_VERSION);

    /* 7. Main loop — cooperative. */
    while (1) {
        /* Run a scan if requested. */
        if (g_scan_active) {
            run_scan();       /* returns when scan done or aborted */
            continue;          /* loop back, maybe a new scan queued */
        }

        /* Pump the BLE protocol. */
        uint8_t  op;
        uint8_t  buf[256];
        uint16_t n = ble_poll(&op, buf, sizeof(buf));
        if (n > 0)
            handle_ble_message(op, buf, n);

        /* Pump the USB CDC console. */
        char line[128];
        if (usb_readline(line, sizeof(line)))
            usb_console_line(line);

        /* Blink the armed LED if the M4 reports armed. */
        if (M4_MAILBOX->armed_ack)
            ps_led_pulse(1);

        /* If the M4 reports a driver fault, light the yellow LED. */
        if (M4_MAILBOX->last_fault != 0)
            ps_led_set(2, true);
        else
            ps_led_set(2, false);

        /* Idle: a short WFI to save power between events. */
        __asm volatile ("wfi");
    }
}

/* ─── Board init (called before anything else) ────────────────────────── */
void ps_board_init(void)
{
    /* Enable the HSE and switch the system clock to 480 MHz via PLL1.
     * The exact PLL constants are STM32H7-specific; the values below
     * assume a 25 MHz HSE crystal (PhotonStrike uses an ABM8G-25.000).
     * PLL1: M=5, N=240, P=2  → VCO=1200 MHz, SYS=480 MHz. */
    volatile uint32_t *rcc = (volatile uint32_t *)RCC_BASE;

    /* Enable HSE. */
    rcc[0x00u / 4u] |= (1u << 16);                 /* RCC_CR.HSEON */
    while (!(rcc[0x00u / 4u] & (1u << 17))) ;      /* wait HSERDY   */

    /* Configure PLL1. */
    rcc[0x10u / 4u] = (5u << 4) | (240u << 8) | (0u << 16) | (2u << 17);
    rcc[0x00u / 4u] |= (1u << 24);                 /* RCC_CR.PLL1ON */
    while (!(rcc[0x00u / 4u] & (1u << 25))) ;      /* wait PLL1RDY  */

    /* Set flash latency to 4 wait states for 480 MHz. */
    REG32(FLASH_BASE + 0x00u) = 4u;                /* FLASH_ACR     */

    /* Switch SYSCLK to PLL1. */
    rcc[0x0Cu / 4u] = (0u << 0) | (0u << 4) | (0u << 8) | (0u << 12) | (0u << 16) | (3u << 20);
    while (((rcc[0x0Cu / 4u] >> 4) & 7u) != 3u) ;  /* wait SWS = PLL1 */

    /* Enable all the GPIO clocks we need. */
    rcc[RCC_AHB4ENR / 4u] |= RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN |
                             RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN |
                             RCC_AHB4ENR_GPIOEEN | RCC_AHB4ENR_GPIOHEN;

    /* Enable peripheral clocks. */
    rcc[RCC_AHB1ENR / 4u]  |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMA2EN;
    rcc[RCC_AHB3ENR / 4u]  |= RCC_AHB3ENR_ADC1EN | RCC_AHB3ENR_SDMMC1EN;
    rcc[RCC_APB1ENR1 / 4u] |= RCC_APB1ENR1_USART3EN | RCC_APB1ENR1_I2C1EN |
                              RCC_APB1ENR1_DAC1EN | RCC_APB1ENR1_TIM6EN;
    rcc[RCC_APB2ENR / 4u]  |= RCC_APB2ENR_TIM1EN | RCC_APB2ENR_SPI1EN;

    /* Set the SysTick to 1 ms. */
    REG32(SYSTICK_LOAD) = 480000u - 1u;
    REG32(SYSTICK_VAL)  = 0;
    REG32(SYSTICK_CSR)  = SYSTICK_ENABLE | SYSTICK_CLKSRC;
}

void ps_led_set(uint8_t which, bool on)
{
    volatile uint32_t *port;
    uint8_t pin;
    switch (which) {
    case 0: port = (volatile uint32_t *)GPIOE_BASE; pin = 0; break;  /* green */
    case 1: port = (volatile uint32_t *)GPIOE_BASE; pin = 1; break;  /* red   */
    case 2: port = (volatile uint32_t *)GPIOE_BASE; pin = 2; break;  /* yellow*/
    default: return;
    }
    if (on) port[GPIO_BSRR / 4u] = (1u << pin);
    else    port[GPIO_BSRR / 4u] = (1u << (pin + 16));
}

void ps_led_pulse(uint8_t which)
{
    ps_led_set(which, true);
    ps_delay_us(200);
    ps_led_set(which, false);
}

/* ─── Hard fault handler ──────────────────────────────────────────────── */
void HardFault_Handler(void)
{
    /* On any hard fault, kill the laser and hang in a visible state. */
    laser_disable();
    M4_MAILBOX->disarm_req = 1;
    ps_led_set(2, true);   /* solid yellow = fault */
    while (1) { ps_delay_us(1); }
}

/* ─── EOF ─────────────────────────────────────────────────────────────── */