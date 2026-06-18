/*
 * GHOSTBUS — Covert SWD/JTAG Hardware Debug Implant
 * JTAG Engine — IEEE 1149.1 TAP state machine & bit-bang driver
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#include "jtag_driver.h"

/* =========================================================================
 * TAP state transition table
 *
 * Index = current state. Bit 0 = TMS=0 next state, Bit 1 = TMS=1 next state.
 * (Packed as 8-bit; high nibble = TMS=1 next, low nibble = TMS=0 next.)
 * ========================================================================= */

static const uint8_t jtag_tap_next[16] = {
    /* TEST_LOGIC_RESET: TMS=0→RUN_IDLE, TMS=1→TEST_LOGIC_RESET */
    0x10, /* low=RUN_TEST_IDLE(1), high=TEST_LOGIC_RESET(0) */
    /* RUN_TEST_IDLE:    TMS=0→RUN_IDLE, TMS=1→SELECT_DR */
    0x12, /* low=1, high=2 */
    /* SELECT_DR_SCAN:   TMS=0→CAPTURE_DR, TMS=1→SELECT_IR */
    0x39, /* low=3, high=9 */
    /* CAPTURE_DR:       TMS=0→SHIFT_DR, TMS=1→EXIT1_DR */
    0x45, /* low=4, high=5 */
    /* SHIFT_DR:         TMS=0→SHIFT_DR, TMS=1→EXIT1_DR */
    0x45, /* low=4, high=5 */
    /* EXIT1_DR:         TMS=0→PAUSE_DR, TMS=1→UPDATE_DR */
    0x68, /* low=6, high=8 */
    /* PAUSE_DR:         TMS=0→PAUSE_DR, TMS=1→EXIT2_DR */
    0x76, /* low=6, high=7 */
    /* EXIT2_DR:         TMS=0→SHIFT_DR, TMS=1→UPDATE_DR */
    0x48, /* low=4, high=8 */
    /* UPDATE_DR:        TMS=0→RUN_IDLE, TMS=1→SELECT_DR */
    0x12, /* low=1, high=2 */
    /* SELECT_IR_SCAN:   TMS=0→CAPTURE_IR, TMS=1→TEST_LOGIC_RESET */
    0xA0, /* low=10, high=0 */
    /* CAPTURE_IR:       TMS=0→SHIFT_IR, TMS=1→EXIT1_IR */
    0xBC, /* low=11, high=12 */
    /* SHIFT_IR:         TMS=0→SHIFT_IR, TMS=1→EXIT1_IR */
    0xBC, /* low=11, high=12 */
    /* EXIT1_IR:         TMS=0→PAUSE_IR, TMS=1→UPDATE_IR */
    0xDE, /* low=13, high=14 */
    /* PAUSE_IR:         TMS=0→PAUSE_IR, TMS=1→EXIT2_IR */
    0xED, /* low=13, high=15 */
    /* EXIT2_IR:         TMS=0→SHIFT_IR, TMS=1→UPDATE_IR */
    0xBE, /* low=11, high=14 */
    /* UPDATE_IR:        TMS=0→RUN_IDLE, TMS=1→SELECT_DR */
    0x12  /* low=1, high=2 */
};

/* =========================================================================
 * Internal state
 * ========================================================================= */

static GPIO_TypeDef *jtag_tck_port;
static uint8_t       jtag_tck_pin;
static GPIO_TypeDef *jtag_tms_port;
static uint8_t       jtag_tms_pin;
static GPIO_TypeDef *jtag_tdi_port;
static uint8_t       jtag_tdi_pin;
static GPIO_TypeDef *jtag_tdo_port;
static uint8_t       jtag_tdo_pin;

static uint32_t      jtag_clock_hz = 1000000UL;
static uint32_t      jtag_clk_half_us = 0;
static jtag_tap_state_t jtag_state = JTAG_TEST_LOGIC_RESET;

static inline void jtag_delay_us(uint32_t us)
{
    volatile uint32_t count = us * (SYSTEM_CLOCK_HZ / 3000000UL);
    while (count--) { __asm volatile ("nop"); }
}

static inline void jtag_clk_pulse(void)
{
    gpio_set(jtag_tck_port, jtag_tck_pin);
    jtag_delay_us(jtag_clk_half_us);
    gpio_clear(jtag_tck_port, jtag_tck_pin);
    jtag_delay_us(jtag_clk_half_us);
}

static inline void jtag_set_tms(uint8_t tms)
{
    if (tms) gpio_set(jtag_tms_port, jtag_tms_pin);
    else     gpio_clear(jtag_tms_port, jtag_tms_pin);
}

static inline void jtag_set_tdi(uint8_t tdi)
{
    if (tdi) gpio_set(jtag_tdi_port, jtag_tdi_pin);
    else     gpio_clear(jtag_tdi_port, jtag_tdi_pin);
}

static inline uint8_t jtag_read_tdo(void)
{
    return gpio_read(jtag_tdo_port, jtag_tdo_pin);
}

static jtag_tap_state_t jtap_step(jtag_tap_state_t cur, uint8_t tms)
{
    uint8_t entry = jtag_tap_next[cur];
    /* Low nibble = TMS=0 next, high nibble (>>4) = TMS=1 next */
    uint8_t next = (uint8_t)(tms ? (entry >> 4) : (entry & 0x0F));
    return (jtag_tap_state_t)next;
}

/* =========================================================================
 * Public API
 * ========================================================================= */

void jtag_init(probe_channel_t tck_ch, probe_channel_t tms_ch,
                probe_channel_t tdi_ch, probe_channel_t tdo_ch)
{
    jtag_tck_port = probe_pinmap[tck_ch].port;
    jtag_tck_pin  = probe_pinmap[tck_ch].pin;
    jtag_tms_port = probe_pinmap[tms_ch].port;
    jtag_tms_pin  = probe_pinmap[tms_ch].pin;
    jtag_tdi_port = probe_pinmap[tdi_ch].port;
    jtag_tdi_pin  = probe_pinmap[tdi_ch].pin;
    jtag_tdo_port = probe_pinmap[tdo_ch].port;
    jtag_tdo_pin  = probe_pinmap[tdo_ch].pin;

    /* TCK, TMS, TDI: outputs */
    gpio_set_mode(jtag_tck_port, jtag_tck_pin, GPIO_MODE_OUTPUT);
    gpio_set_otyper(jtag_tck_port, jtag_tck_pin, GPIO_OTYPE_PP);
    gpio_set_ospeed(jtag_tck_port, jtag_tck_pin, GPIO_OSPEED_HIGH);

    gpio_set_mode(jtag_tms_port, jtag_tms_pin, GPIO_MODE_OUTPUT);
    gpio_set_otyper(jtag_tms_port, jtag_tms_pin, GPIO_OTYPE_PP);

    gpio_set_mode(jtag_tdi_port, jtag_tdi_pin, GPIO_MODE_OUTPUT);
    gpio_set_otyper(jtag_tdi_port, jtag_tdi_pin, GPIO_OTYPE_PP);

    /* TDO: input, pull-up (idle high in many parts) */
    gpio_set_mode(jtag_tdo_port, jtag_tdo_pin, GPIO_MODE_INPUT);
    gpio_set_pupd(jtag_tdo_port, jtag_tdo_pin, GPIO_PUPD_PULLUP);

    gpio_clear(jtag_tck_port, jtag_tck_pin);
    gpio_clear(jtag_tms_port, jtag_tms_pin);
    gpio_clear(jtag_tdi_port, jtag_tdi_pin);

    jtag_set_clock(1000000UL);
    jtag_state = JTAG_TEST_LOGIC_RESET;
}

void jtag_set_clock(uint32_t hz)
{
    if (hz == 0) return;
    jtag_clock_hz = hz;
    jtag_clk_half_us = 500000UL / hz;
    if (jtag_clk_half_us == 0) jtag_clk_half_us = 1;
}

uint32_t jtag_get_clock(void)
{
    return jtag_clock_hz;
}

void jtag_tap_reset(void)
{
    /* TMS=1 for 5+ TCK cycles puts TAP in TEST_LOGIC_RESET */
    jtag_set_tms(1);
    for (int i = 0; i < 6; i++) jtag_clk_pulse();
    jtag_state = JTAG_TEST_LOGIC_RESET;
}

void jtag_goto_state(jtag_tap_state_t target)
{
    int max_steps = 16;
    while (jtag_state != target && max_steps-- > 0) {
        /* Compute TMS to move toward target.
         * Simple heuristic: use a fixed shortest-path via reset+select.
         * For robustness, drive to TLR then back via standard path. */
        uint8_t tms;
        if (jtag_state == JTAG_TEST_LOGIC_RESET) {
            if (target == JTAG_TEST_LOGIC_RESET) { break; }
            tms = 0; /* go to RUN_TEST_IDLE */
        } else if (jtag_state == JTAG_RUN_TEST_IDLE) {
            if (target == JTAG_RUN_TEST_IDLE) { break; }
            tms = 1; /* SELECT_DR_SCAN */
        } else {
            /* Walk using table; choose TMS that decreases distance.
             * For simplicity, pick TMS=0 if low-nibble equals or
             * progresses toward target; else TMS=1. */
            uint8_t entry = jtag_tap_next[jtag_state];
            jtag_tap_state_t n0 = (jtag_tap_state_t)(entry & 0x0F);
            jtag_tap_state_t n1 = (jtag_tap_state_t)(entry >> 4);
            tms = (n0 == target) ? 0 : (n1 == target) ? 1 :
                  (n0 > jtag_state) ? 0 : 1;
        }
        jtag_set_tms(tms);
        jtag_clk_pulse();
        jtag_state = jtap_step(jtag_state, tms);
    }
}

jtag_tap_state_t jtag_current_state(void)
{
    return jtag_state;
}

gb_status_t jtag_shift_ir(uint32_t instruction, uint8_t ir_len)
{
    if (ir_len == 0 || ir_len > 32) return GB_ERR_PARAM;
    jtag_goto_state(JTAG_SHIFT_IR);
    /* Shift ir_len-1 bits with TMS=0, last bit with TMS=1 (to EXIT1_IR) */
    for (uint8_t i = 0; i < ir_len; i++) {
        uint8_t bit = (uint8_t)((instruction >> i) & 1);
        jtag_set_tdi(bit);
        if (i == ir_len - 1) jtag_set_tms(1);
        jtag_clk_pulse();
    }
    jtag_state = JTAG_EXIT1_IR;
    jtag_goto_state(JTAG_RUN_TEST_IDLE);
    return GB_OK;
}

gb_status_t jtag_shift_dr(uint32_t *tdi, uint32_t *tdo, uint32_t bits)
{
    if (bits == 0 || bits > 64) return GB_ERR_PARAM;
    jtag_goto_state(JTAG_SHIFT_DR);
    uint32_t in_val  = tdi ? *tdi : 0;
    uint32_t out_val = 0;
    for (uint32_t i = 0; i < bits; i++) {
        uint8_t bit = (uint8_t)((in_val >> i) & 1);
        jtag_set_tdi(bit);
        if (i == bits - 1) jtag_set_tms(1);
        /* Sample TDO on the clock edge (after rising; we sample just before next) */
        uint8_t rd = jtag_read_tdo();
        jtag_clk_pulse();
        if (i < 32) out_val |= ((uint32_t)rd << i);
    }
    jtag_state = JTAG_EXIT1_DR;
    if (tdo) *tdo = out_val;
    jtag_goto_state(JTAG_RUN_TEST_IDLE);
    return GB_OK;
}

gb_status_t jtag_read_idcode(uint32_t *idcode)
{
    if (!idcode) return GB_ERR_PARAM;
    /* After TLR, IDCODE is loaded into DR by default on most TAPs */
    jtag_tap_reset();
    jtag_goto_state(JTAG_SHIFT_DR);
    /* Shift 32 bits out; first bit captured in CAPTURE_DR */
    uint32_t val = 0;
    for (int i = 0; i < 32; i++) {
        jtag_set_tdi(0); /* don't care input */
        if (i == 31) jtag_set_tms(1);
        uint8_t rd = jtag_read_tdo();
        jtag_clk_pulse();
        val |= ((uint32_t)rd << i);
    }
    jtag_state = JTAG_EXIT1_DR;
    jtag_goto_state(JTAG_RUN_TEST_IDLE);
    *idcode = val;
    /* Validate: IDCODE LSB must be 1 (per JTAG spec) */
    if ((val & 1) == 0) return GB_ERR_NO_TARGET;
    return GB_OK;
}

gb_status_t jtag_read_dr(uint32_t *data, uint8_t bits)
{
    if (!data || bits == 0 || bits > 32) return GB_ERR_PARAM;
    jtag_goto_state(JTAG_SHIFT_DR);
    uint32_t val = 0;
    for (uint8_t i = 0; i < bits; i++) {
        jtag_set_tdi(0);
        if (i == bits - 1) jtag_set_tms(1);
        uint8_t rd = jtag_read_tdo();
        jtag_clk_pulse();
        val |= ((uint32_t)rd << i);
    }
    jtag_state = JTAG_EXIT1_DR;
    jtag_goto_state(JTAG_RUN_TEST_IDLE);
    *data = val;
    return GB_OK;
}

gb_status_t jtag_write_dr(uint32_t data, uint8_t bits)
{
    if (bits == 0 || bits > 32) return GB_ERR_PARAM;
    jtag_goto_state(JTAG_SHIFT_DR);
    for (uint8_t i = 0; i < bits; i++) {
        jtag_set_tdi((uint8_t)((data >> i) & 1));
        if (i == bits - 1) jtag_set_tms(1);
        jtag_clk_pulse();
    }
    jtag_state = JTAG_EXIT1_DR;
    jtag_goto_state(JTAG_UPDATE_DR);
    jtag_goto_state(JTAG_RUN_TEST_IDLE);
    return GB_OK;
}

gb_status_t jtag_scan_chain(jtag_target_info_t *info)
{
    if (!info) return GB_ERR_PARAM;
    memset(info, 0, sizeof(*info));

    jtag_tap_reset();
    jtag_goto_state(JTAG_SHIFT_DR);

    /* Shift out a long stream of 0s while reading TDO; count consecutive
     * 1-bits at the start to determine chain length (each TAP returns its
     * IDCODE then BYPASS zeros). */
    uint32_t idcodes[8];
    uint8_t  count = 0;
    uint32_t cur_id = 0;
    uint8_t  bits_read = 0;
    /* Read up to 256 bits to enumerate up to 8 TAPs */
    for (uint16_t i = 0; i < 256 && count < 8; i++) {
        jtag_set_tdi(0);
        uint8_t rd = jtag_read_tdo();
        if (i == 255) jtag_set_tms(1);
        jtag_clk_pulse();
        cur_id |= ((uint32_t)rd << bits_read);
        bits_read++;
        if (bits_read == 32) {
            if (cur_id != 0 && (cur_id & 1) == 1) {
                idcodes[count++] = cur_id;
                info->chain_idcodes[count - 1] = cur_id;
                cur_id = 0;
                bits_read = 0;
            } else {
                /* Hit the BYPASS register (all zeros) — chain end */
                break;
            }
        }
    }
    info->chain_count = count;
    if (count > 0) {
        info->idcode = idcodes[0];
    }
    jtag_state = JTAG_EXIT1_DR;
    jtag_goto_state(JTAG_RUN_TEST_IDLE);
    return (count > 0) ? GB_OK : GB_ERR_NO_TARGET;
}

gb_status_t jtag_boundary_scan(uint32_t *sample_buf, uint8_t bits)
{
    if (!sample_buf || bits == 0 || bits > 32) return GB_ERR_PARAM;
    /* SAMPLE/PRELOAD instruction code is typically 0x01 but vendor-specific */
    jtag_shift_ir(JTAG_INST_SAMPLE_PRELOAD, 4);
    jtag_goto_state(JTAG_CAPTURE_DR);
    jtag_goto_state(JTAG_SHIFT_DR);
    uint32_t val = 0;
    for (uint8_t i = 0; i < bits; i++) {
        jtag_set_tdi(0);
        if (i == bits - 1) jtag_set_tms(1);
        uint8_t rd = jtag_read_tdo();
        jtag_clk_pulse();
        val |= ((uint32_t)rd << i);
    }
    jtag_state = JTAG_EXIT1_DR;
    jtag_goto_state(JTAG_UPDATE_DR);
    jtag_goto_state(JTAG_RUN_TEST_IDLE);
    *sample_buf = val;
    return GB_OK;
}