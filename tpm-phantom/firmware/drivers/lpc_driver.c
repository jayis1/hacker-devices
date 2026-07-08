/*
 * tpm-phantom — drivers/lpc_driver.c
 * LPC (Low Pin Count) bus snoop driver.
 *
 * The LPC bus carries TPM 2.0 traffic in systems that use the LPC
 * interface (Intel platforms, many embedded boards, server BMCs via
 * the "LPC-ish" eSPI bridge). The tpm-phantom passively snoops the
 * LPC bus to reconstruct TPM register reads/writes.
 *
 * LPC protocol summary (relevant to TPM):
 *  - LCLK: 24/33 MHz clock
 *  - LFRAME#: frame strobe (active low)
 *  - LAD[3:0]: 4-bit multiplexed address/data
 *  - Start cycle: CT (cycle type) + ADDR + data phase
 *  - TPM uses cycle type 0x0 (IO read) and 0x1 (IO write) at
 *    addresses 0x0000-0x00FF (TPM locality registers) and 0xFED40000+
 *
 * The driver uses a GPIO edge-triggered state machine on LFRAME# and
 * LAD bus changes. Because the bus is 4 bits, every nibble is captured
 * on each LCLK rising edge. A TIM input-capture channel on LCLK
 * measures bus speed to adapt the sampling point.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "lpc_driver.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ===================================================================
 * Internal state
 * =================================================================== */
static volatile lpc_capture_state_t g_state = LPC_STATE_IDLE;
static volatile uint8_t  g_nibble_idx = 0;
static volatile uint32_t g_addr = 0;
static volatile uint32_t g_data = 0;
static volatile uint8_t  g_ct = 0;           /* cycle type */
static volatile uint8_t  g_cycle_dir = 0;    /* 1=read,0=write */
static volatile uint8_t  g_target_id = 0;    /* TPM target ID nibble */
static volatile uint8_t  g_field = 0;        /* 0=CT/ID 1=ADDR 2=DATA */

static lpc_transaction_t g_tx_buf[LPC_FRAME_MAX];
static volatile uint16_t g_tx_head = 0;
static volatile uint16_t g_tx_tail = 0;

static volatile uint32_t g_lclk_count = 0;
static volatile uint32_t g_last_lclk_us = 0;

/* ===================================================================
 * Public statistics
 * =================================================================== */
volatile uint32_t lpc_total_transactions = 0;
volatile uint32_t lpc_tpm_transactions = 0;
volatile uint32_t lpc_crc_errors = 0;
volatile uint32_t lpc_frame_errors = 0;

/* ===================================================================
 * Nibble sampling — called on each LCLK rising edge via EXTI interrupt.
 * Reads LAD[3:0] as a single GPIO port read.
 * =================================================================== */
static inline uint8_t read_lad_nibble(void)
{
    /* LAD0..3 on PB0..PB3 — read IDR[3:0] */
    uint32_t idr = GPIOB->IDR;
    return (uint8_t)(idr & 0x0FUL);
}

/* ===================================================================
 * Push a completed transaction into the ring buffer.
 * =================================================================== */
static void push_transaction(uint8_t dir, uint32_t addr, uint32_t data,
                             uint8_t ct, uint32_t timestamp)
{
    uint16_t next = (g_tx_head + 1) % LPC_FRAME_MAX;
    if (next == g_tx_tail) {
        lpc_frame_errors++;
        return;  /* overflow — drop */
    }
    lpc_transaction_t *t = &g_tx_buf[g_tx_head];
    t->direction = dir;
    t->address   = addr & 0xFFFFFFUL;
    t->data      = data & 0xFFUL;
    t->cycle_type = ct;
    t->timestamp = timestamp;
    t->is_tpm    = lpc_is_tpm_address(addr);
    g_tx_head = next;
    lpc_total_transactions++;
    if (t->is_tpm)
        lpc_tpm_transactions++;
}

/* ===================================================================
 * LPC cycle type decoding (Intel LPC spec, table 4-1)
 * 0b0000 = IO read
 * 0b0001 = IO write
 * 0b0010 = Memory read
 * 0b0011 = Memory write
 * 0b0100 = Firmware memory read
 * 0b0101 = Firmware memory write
 * =================================================================== */
static uint8_t decode_cycle_type(uint8_t ct_nibble)
{
    if (ct_nibble == 0x0 || ct_nibble == 0x1) return ct_nibble;  /* IO */
    if (ct_nibble == 0x2 || ct_nibble == 0x3) return ct_nibble;  /* MEM */
    if (ct_nibble == 0x4 || ct_nibble == 0x5) return ct_nibble;  /* FWH */
    return 0xFF;  /* unknown */
}

static uint8_t is_read_cycle(uint8_t ct) { return (ct == 0x0 || ct == 0x2 || ct == 0x4); }

/* ===================================================================
 * LFRAME# edge handler — called from EXTI9 on LFRAME# falling/rising.
 * Falling edge = start of cycle, rising = end.
 * =================================================================== */
void lpc_on_lframe_edge(uint8_t level)
{
    if (level == 0) {
        /* Falling edge: begin new cycle */
        g_state = LPC_STATE_CYCLE_START;
        g_nibble_idx = 0;
        g_addr = 0;
        g_data = 0;
        g_ct = 0;
        g_field = 0;
    } else {
        /* Rising edge: end of cycle — finalize transaction */
        if (g_state == LPC_STATE_DATA_PHASE || g_state == LPC_STATE_WAIT_DATA) {
            uint32_t ts = lpc_get_timestamp_us();
            push_transaction(g_cycle_dir, g_addr, g_data, g_ct, ts);
        }
        g_state = LPC_STATE_IDLE;
    }
}

/* ===================================================================
 * LCLK rising-edge sampler — called from EXTI8 interrupt.
 *
 * LPC nibble ordering (IO read/write, big-endian MSB-first within each
 * field):
 *   Nibble 0: cycle type (CT[3:0])
 *   Nibble 1: address[31:28] / target ID (IO uses 16-bit addr; for TPM
 *             the high nibbles are 0xFED4)
 *   Nibbles 2-7: address[27:0] (shifted; 28 bits = 7 nibbles)
 *   For IO read/write: only 16-bit address — 4 nibbles after CT.
 *   Data: read = 1 nibble data + 1 nibble (status/data) per byte; write
 *         = 2 nibbles per byte.
 *
 * This implementation handles the IO cycle (the only one TPM uses on
 * LPC). It expects:
 *   nib 0: CT
 *   nib 1-4: ADDR[15:12], [11:8], [7:4], [3:0]
 *   For writes: nib 5-6: DATA[7:4], [3:0]
 *   For reads: host drives nothing; TPM responds (we capture TPM's
 *   response nibbles).
 * =================================================================== */
void lpc_on_lclk_rising(void)
{
    g_lclk_count++;
    if (g_state == LPC_STATE_IDLE)
        return;  /* not in a cycle */

    uint8_t nib = read_lad_nibble();
    uint8_t idx = g_nibble_idx;

    switch (g_state) {
    case LPC_STATE_CYCLE_START:
        /* First nibble: cycle type */
        g_ct = decode_cycle_type(nib);
        if (g_ct == 0xFF) {
            lpc_frame_errors++;
            g_state = LPC_STATE_IDLE;
            return;
        }
        g_cycle_dir = is_read_cycle(g_ct) ? 1 : 0;
        g_state = LPC_STATE_ADDR_PHASE;
        g_nibble_idx = 1;
        break;

    case LPC_STATE_ADDR_PHASE:
        /* Shift address nibble; IO cycle = 4 addr nibbles (16-bit) */
        g_addr = (g_addr << 4) | nib;
        g_nibble_idx++;
        if (g_nibble_idx == 5) {  /* 4 addr nibbles consumed */
            if (g_cycle_dir == 0) {
                /* Write: data follows */
                g_state = LPC_STATE_DATA_PHASE;
            } else {
                /* Read: TPM drives data back */
                g_state = LPC_STATE_WAIT_DATA;
            }
        }
        break;

    case LPC_STATE_DATA_PHASE:
        /* Write data nibbles (2 per byte) */
        g_data = (g_data << 4) | nib;
        g_nibble_idx++;
        if (g_nibble_idx == 7) {
            /* Full byte received */
        }
        break;

    case LPC_STATE_WAIT_DATA:
        /* Read: capture TPM response nibble */
        g_data = (g_data << 4) | nib;
        g_nibble_idx++;
        break;

    default:
        break;
    }
}

/* ===================================================================
 * Check if address falls in TPM range.
 * TPM on LPC typically uses IO ports:
 *   - Legacy (1.2): 0x4E/0x4F (command/status/data)
 *   - TIS 2.0:     0xFED40000+ (memory-mapped but LPC FWH mirror),
 *                  or IO 0x0000-0x00FF of locality space
 * =================================================================== */
uint8_t lpc_is_tpm_address(uint32_t addr)
{
    /* Common TPM TIS IO ranges */
    if (addr >= 0xFED40000UL && addr <= 0xFED44FFFUL) return 1;  /* TIS MMIO window */
    if (addr == 0x4E || addr == 0x4F) return 1;                 /* legacy */
    if (addr >= 0x0020 && addr <= 0x002F) return 1;             /* some FWH TPM */
    /* Locality 0-4 registers (offset within 0xFED40000) */
    if ((addr & 0xFFFFF000UL) == 0xFED40000UL) return 1;
    return 0;
}

/* ===================================================================
 * GPIO + EXTI + TIM initialization for LPC capture
 * =================================================================== */
void lpc_capture_init(void)
{
    /* Enable GPIOA, GPIOB, GPIOC clocks */
    SET_BIT(RCC_AHB4ENR, BIT(0) | BIT(1) | BIT(2));

    /* Configure LPC pins as input (high speed, pull-up where applicable) */
    gpio_set_mode(LPC_LCLK_PORT,    LPC_LCLK_PIN,    GPIO_MODE_INPUT);
    gpio_set_speed(LPC_LCLK_PORT,   LPC_LCLK_PIN,    GPIO_SPEED_VHIGH);
    gpio_set_pupd(LPC_LCLK_PORT,    LPC_LCLK_PIN,    GPIO_PUPD_NONE);

    gpio_set_mode(LPC_LFRAME_PORT,  LPC_LFRAME_PIN,  GPIO_MODE_INPUT);
    gpio_set_speed(LPC_LFRAME_PORT, LPC_LFRAME_PIN,  GPIO_SPEED_VHIGH);
    gpio_set_pupd(LPC_LFRAME_PORT,  LPC_LFRAME_PIN,  GPIO_PUPD_PU);  /* active-low */

    for (int i = 0; i < 4; i++) {
        gpio_set_mode(LPC_LAD0_PORT, i, GPIO_MODE_INPUT);
        gpio_set_speed(LPC_LAD0_PORT, i, GPIO_SPEED_VHIGH);
        gpio_set_pupd(LPC_LAD0_PORT, i, GPIO_PUPD_NONE);
    }

    gpio_set_mode(LPC_LPCPD_PORT,   LPC_LPCPD_PIN,   GPIO_MODE_INPUT);
    gpio_set_pupd(LPC_LPCPD_PORT,   LPC_LPCPD_PIN,   GPIO_PUPD_PU);
    gpio_set_mode(LPC_SERIRQ_PORT,  LPC_SERIRQ_PIN,  GPIO_MODE_INPUT);
    gpio_set_mode(LPC_CLKRUN_PORT,  LPC_CLKRUN_PIN,  GPIO_MODE_INPUT);

    /* TIM2: input capture on LCLK (PA8 = TIM1_CH1, but use TIM2 TI2 via
     * internal connection — actually PA8 is TIM1_CH1; we use TIM1 for
     * clock counting. For simplicity, software EXTI handles edges.) */

    /* Reset state */
    g_state = LPC_STATE_IDLE;
    g_tx_head = 0;
    g_tx_tail = 0;
    g_lclk_count = 0;
    lpc_total_transactions = 0;
    lpc_tpm_transactions = 0;
}

/* ===================================================================
 * Start/stop capture
 * =================================================================== */
void lpc_capture_start(void) { g_state = LPC_STATE_IDLE; led_lpc_on(); }
void lpc_capture_stop(void)  { g_state = LPC_STATE_IDLE; led_lpc_off(); }

/* ===================================================================
 * Pop a captured transaction (non-blocking).
 * Returns 1 if a transaction was dequeued, 0 if empty.
 * =================================================================== */
uint8_t lpc_pop_transaction(lpc_transaction_t *out)
{
    if (g_tx_head == g_tx_tail)
        return 0;
    *out = g_tx_buf[g_tx_tail];
    g_tx_tail = (g_tx_tail + 1) % LPC_FRAME_MAX;
    return 1;
}

/* ===================================================================
 * Timestamp — uses TIM6 free-running counter at 1 MHz
 * =================================================================== */
static volatile uint32_t g_tim6_overflow = 0;

void lpc_timer_init(void)
{
    SET_BIT(RCC_APB1ENR, BIT(4));   /* TIM6 */
    TIM6->PSC = (APB1_CLK_HZ / 1000000UL) - 1;  /* 1 MHz tick = 1 us */
    TIM6->ARR = 0xFFFFFFFFUL;
    TIM6->DIER = TIM_DIER_UIE;
    SET_BIT(TIM6->CR1, TIM_CR1_CEN);
    NVIC_ISER0 = BIT(IRQ_TIM2 - 0);  /* placeholder — TIM6 IRQ is #54 */
}

uint32_t lpc_get_timestamp_us(void)
{
    uint32_t cnt = TIM6->CNT;
    return g_tim6_overflow * 0xFFFF + cnt;
}

/* TIM6 overflow handler (weak; linked by startup) */
void TIM6_DAC_IRQHandler(void)
{
    if (TIM6->SR & TIM_SR_UIF) {
        TIM6->SR = 0;
        g_tim6_overflow++;
    }
}

/* ===================================================================
 * LPC injection (experimental — drive LAD lines as master)
 *
 * WARNING: This requires tri-stating the host's LAD driver. The
 * tpm-phantom uses series resistors (100Ω) on LAD lines so the host
 * can still drive the bus; for injection we switch our GPIO to output
 * and drive. This is aggressive and may cause contention on live
 * systems — use only on test benches.
 *
 * Writes a single IO write cycle to a 16-bit address.
 * =================================================================== */
uint8_t lpc_inject_io_write(uint16_t addr, uint8_t data)
{
    /* Switch LAD[3:0] to output push-pull */
    for (int i = 0; i < 4; i++) {
        gpio_set_mode(LPC_LAD0_PORT, i, GPIO_MODE_OUTPUT);
        gpio_set_otype(LPC_LAD0_PORT, i, GPIO_OTYPE_PP);
    }
    gpio_set_mode(LPC_LFRAME_PORT, LPC_LFRAME_PIN, GPIO_MODE_OUTPUT);
    gpio_set_otype(LPC_LFRAME_PORT, LPC_LFRAME_PIN, GPIO_OTYPE_PP);

    /* Drive LFRAME# low to start cycle */
    gpio_write(LPC_LFRAME_PORT, LPC_LFRAME_PIN, 0);

    /* Send cycle type nibble: IO write = 0x1 */
    gpio_write(LPC_LAD0_PORT, 0, (0x1 >> 0) & 1);
    gpio_write(LPC_LAD0_PORT, 1, (0x1 >> 1) & 1);
    gpio_write(LPC_LAD0_PORT, 2, (0x1 >> 2) & 1);
    gpio_write(LPC_LAD0_PORT, 3, (0x1 >> 3) & 1);
    /* Toggle LCLK would be needed if we drove clock; on LPC the host
     * drives LCLK. For injection we rely on the host's clock being
     * present — this is a semi-passive injection. */

    /* Send address nibbles (MSB first) */
    for (int n = 3; n >= 0; n--) {
        uint8_t nib = (addr >> (n * 4)) & 0xF;
        for (int b = 0; b < 4; b++)
            gpio_write(LPC_LAD0_PORT, b, (nib >> b) & 1);
    }

    /* Send data nibbles (MSB first) */
    for (int n = 1; n >= 0; n--) {
        uint8_t nib = (data >> (n * 4)) & 0xF;
        for (int b = 0; b < 4; b++)
            gpio_write(LPC_LAD0_PORT, b, (nib >> b) & 1);
    }

    /* Release LFRAME# */
    gpio_write(LPC_LFRAME_PORT, LPC_LFRAME_PIN, 1);

    /* Switch LAD back to input (high-Z) */
    for (int i = 0; i < 4; i++)
        gpio_set_mode(LPC_LAD0_PORT, i, GPIO_MODE_INPUT);
    gpio_set_mode(LPC_LFRAME_PORT, LPC_LFRAME_PIN, GPIO_MODE_INPUT);

    return 1;
}

/* ===================================================================
 * SERIRQ monitoring — count IRQ pulses (TPM interrupt activity)
 * =================================================================== */
volatile uint32_t lpc_serirq_count = 0;
void lpc_on_serirq_edge(void) { lpc_serirq_count++; }

/* ===================================================================
 * Reset all stats
 * =================================================================== */
void lpc_reset_stats(void)
{
    lpc_total_transactions = 0;
    lpc_tpm_transactions = 0;
    lpc_crc_errors = 0;
    lpc_frame_errors = 0;
    lpc_serirq_count = 0;
}