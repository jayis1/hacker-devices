/*
 * GHOSTBUS — Covert SWD/JTAG Hardware Debug Implant
 * Discovery Engine — impedance-guided pin auto-discovery
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#include "discovery_driver.h"
#include "swd_driver.h"
#include "jtag_driver.h"

/* =========================================================================
 * ADC sampling for channel voltage reading
 * ========================================================================= */

static void adc_init(void)
{
    /* Enable ADC clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_ADC1EN;
    (void)RCC->AHB1ENR; /* read-back to ensure write took effect */
    /* Power down, configure, power up */
    ADC1->CR = 0;
    ADC1->CFGR = ADC_CFGR_CONT;
    /* Enable */
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) { /* wait ready */ }
}

static uint16_t adc_sample_channel(uint8_t channel)
{
    /* Configure channel selection (simplified: assumes single-ended) */
    /* On STM32H5, channel is in ADC_SEQ[0]. For brevity, we use a
     * simplified configuration: select channel via ADC_SQR1. */
    /* Enable ADC, start conversion, wait EOC, read DR. */
    ADC1->CR &= ~ADC_CR_ADEN;
    /* (Real code would configure SQR1 here.) */
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) { }
    /* Start conversion */
    ADC1->CR |= (1U << 2); /* ADSTART bit */
    while (!(ADC1->ISR & ADC_ISR_EOC)) { }
    return (uint16_t)(ADC1->DR & 0x0FFF);
}

void discovery_init(void)
{
    adc_init();
}

/* =========================================================================
 * Channel voltage reading
 *
 * Each probe channel (when not actively driven) can be read via ADC.
 * We configure the channel as analog input, sample, then restore.
 * ========================================================================= */

uint16_t discovery_read_channel_voltage(uint8_t channel)
{
    if (channel >= PROBE_CH_MAX) return 0;
    if (probe_pinmap[channel].adc_channel == 0xFF) return 0;
    /* Set channel as analog input */
    GPIO_TypeDef *port = probe_pinmap[channel].port;
    uint8_t pin = probe_pinmap[channel].pin;
    gpio_set_mode(port, pin, GPIO_MODE_ANALOG);
    /* Allow signal to settle */
    volatile uint32_t delay = 1000; while (delay--) { }
    uint16_t raw = adc_sample_channel(probe_pinmap[channel].adc_channel);
    /* Convert 12-bit ADC to millivolts assuming 3.3 V VRef */
    uint32_t mv = ((uint32_t)raw * 3300UL) / 4096UL;
    return (uint16_t)mv;
}

/* =========================================================================
 * Resistance measurement between two channels
 *
 * Drive ch_a high with a known series resistor, read ch_b voltage,
 * compute divider. This is a simplified continuity probe: low resistance
 * to GND indicates likely connection.
 * ========================================================================= */

uint16_t discovery_measure_resistance(uint8_t ch_a, uint8_t ch_b)
{
    if (ch_a >= PROBE_CH_MAX || ch_b >= PROBE_CH_MAX) return 0xFFFF;
    /* Drive ch_a as output high */
    GPIO_TypeDef *port_a = probe_pinmap[ch_a].port;
    uint8_t pin_a = probe_pinmap[ch_a].pin;
    gpio_set_mode(port_a, pin_a, GPIO_MODE_OUTPUT);
    gpio_set_otyper(port_a, pin_a, GPIO_OTYPE_PP);
    gpio_set(port_a, pin_a);
    /* Read ch_b voltage via ADC if available */
    uint16_t mv = discovery_read_channel_voltage(ch_b);
    /* Restore ch_a to input */
    gpio_set_mode(port_a, pin_a, GPIO_MODE_INPUT);
    gpio_set_pupd(port_a, pin_a, GPIO_PUPD_NONE);
    /* Return raw mV; lower means lower resistance to driven pin */
    return mv;
}

/* =========================================================================
 * Find GND channel: the channel with voltage closest to 0 mV when
 * target is powered, or lowest resistance to the device's own GND
 * reference (which we identify via a continuity scan against a known
 * driven channel).
 * ========================================================================= */

uint8_t discovery_find_gnd(void)
{
    uint8_t best = PROBE_CH7; /* default to dedicated GND sense */
    uint16_t best_mv = 0xFFFF;
    for (uint8_t ch = 0; ch < PROBE_CH_MAX; ch++) {
        if (probe_pinmap[ch].adc_channel == 0xFF) continue;
        uint16_t mv = discovery_read_channel_voltage(ch);
        /* GND pins read very close to 0 V */
        if (mv < 100 && mv < best_mv) {
            best_mv = mv;
            best = ch;
        }
    }
    return best;
}

/* =========================================================================
 * Find VRef channel: highest-voltage channel (typically 1.2-5.0 V)
 * ========================================================================= */

uint8_t discovery_find_vref(uint8_t gnd_ch)
{
    uint8_t best = PROBE_CH4;
    uint16_t best_mv = 0;
    for (uint8_t ch = 0; ch < PROBE_CH_MAX; ch++) {
        if (ch == gnd_ch) continue;
        if (probe_pinmap[ch].adc_channel == 0xFF) continue;
        uint16_t mv = discovery_read_channel_voltage(ch);
        if (mv > best_mv) {
            best_mv = mv;
            best = ch;
        }
    }
    return best;
}

/* =========================================================================
 * Pull characterization: drive a weak pull-up then weak pull-down and
 * observe the settled voltage. SWDIO/TMS typically have onboard pulls.
 * ========================================================================= */

typedef struct {
    uint16_t pullup_mv;
    uint16_t pulldown_mv;
} pull_profile_t;

static pull_profile_t discovery_profile_channel(uint8_t ch)
{
    pull_profile_t prof = { 0, 0 };
    if (probe_pinmap[ch].adc_channel == 0xFF) return prof;
    GPIO_TypeDef *port = probe_pinmap[ch].port;
    uint8_t pin = probe_pinmap[ch].pin;
    /* Weak pull-up */
    gpio_set_mode(port, pin, GPIO_MODE_INPUT);
    gpio_set_pupd(port, pin, GPIO_PUPD_PULLUP);
    volatile uint32_t d = 5000; while (d--) { }
    prof.pullup_mv = discovery_read_channel_voltage(ch);
    /* Weak pull-down */
    gpio_set_pupd(port, pin, GPIO_PUPD_PULLDOWN);
    d = 5000; while (d--) { }
    prof.pulldown_mv = discovery_read_channel_voltage(ch);
    /* Restore to no-pull */
    gpio_set_pupd(port, pin, GPIO_PUPD_NONE);
    return prof;
}

/* =========================================================================
 * SWD protocol probe: try a given (swdio, swclk) pair, perform line reset,
 * read DP_IDCODE. Returns confidence 0-100.
 * ========================================================================= */

static uint8_t discovery_try_swd(uint8_t swdio_ch, uint8_t swclk_ch,
                                   discovery_result_t *result)
{
    swd_init((probe_channel_t)swdio_ch, (probe_channel_t)swclk_ch);
    swd_set_clock(100000UL); /* slow & safe */
    gb_status_t st = swd_line_reset();
    if (st != GB_OK) return 0;
    uint32_t idcode = 0;
    st = swd_read_dp(SWD_DP_IDCODE, &idcode);
    if (st != GB_OK || idcode == 0 || idcode == 0xFFFFFFFF) return 0;
    /* Validate IDCODE: ARM CoreSight-DP typically has specific patterns.
     * Accept any non-zero IDCODE with valid parity (already checked). */
    result->idcode = idcode;
    result->swdio_ch = swdio_ch;
    result->swclk_ch = swclk_ch;
    /* Confidence based on signal quality: slow-clock read success = high */
    return 85;
}

/* =========================================================================
 * JTAG protocol probe: try (tck, tms, tdi, tdo) combinations.
 * ========================================================================= */

static uint8_t discovery_try_jtag(uint8_t tck_ch, uint8_t tms_ch,
                                    uint8_t tdi_ch, uint8_t tdo_ch,
                                    discovery_result_t *result)
{
    jtag_init((probe_channel_t)tck_ch, (probe_channel_t)tms_ch,
              (probe_channel_t)tdi_ch, (probe_channel_t)tdo_ch);
    jtag_set_clock(100000UL);
    jtag_tap_reset();
    uint32_t idcode = 0;
    gb_status_t st = jtag_read_idcode(&idcode);
    if (st != GB_OK || idcode == 0 || idcode == 0xFFFFFFFF) return 0;
    /* LSB must be 1 per JTAG spec */
    if ((idcode & 1) == 0) return 0;
    result->idcode = idcode;
    result->tck_ch = tck_ch;
    result->tms_ch = tms_ch;
    result->tdi_ch = tdi_ch;
    result->tdo_ch = tdo_ch;
    return 80;
}

/* =========================================================================
 * Main discovery scan: enumerate candidate pinouts and try each protocol.
 * ========================================================================= */

gb_status_t discovery_scan(probe_protocol_t *protocols, uint8_t n_protocols,
                            discovery_result_t *result, uint32_t timeout_ms)
{
    if (!result || !protocols) return GB_ERR_PARAM;
    memset(result, 0, sizeof(*result));
    result->protocol = PROTO_UNKNOWN;
    result->confidence = 0;

    discovery_init();

    /* Step 1: find GND and VRef */
    uint8_t gnd_ch = discovery_find_gnd();
    uint8_t vref_ch = discovery_find_vref(gnd_ch);
    result->gnd_ch = gnd_ch;
    result->vref_ch = vref_ch;
    result->vref_mv = discovery_read_channel_voltage(vref_ch);

    /* If VRef is too low, target may be unpowered — try power injection
     * (omitted here for safety; operator triggers it explicitly). */
    if (result->vref_mv < 1000) {
        return GB_ERR_NO_TARGET;
    }

    /* Step 2: characterize pulls on remaining channels */
    pull_profile_t profiles[PROBE_CH_MAX];
    for (uint8_t ch = 0; ch < PROBE_CH_MAX; ch++) {
        if (ch == gnd_ch || ch == vref_ch) continue;
        if (probe_pinmap[ch].adc_channel == 0xFF) continue;
        profiles[ch] = discovery_profile_channel(ch);
    }

    /* Step 3: enumerate candidate pinouts and try each requested protocol.
     * Build a candidate list of "data" channels (non-power). */
    uint8_t data_chs[6];
    uint8_t n_data = 0;
    for (uint8_t ch = 0; ch < PROBE_CH_MAX; ch++) {
        if (ch == gnd_ch || ch == vref_ch) continue;
        if (probe_pinmap[ch].adc_channel != 0xFF) continue; /* skip ADC-only */
        data_chs[n_data++] = ch;
    }
    if (n_data < 2) return GB_ERR_NO_TARGET;

    /* Step 4: try SWD on all ordered pairs of data channels */
    for (uint8_t p = 0; p < n_protocols; p++) {
        if (protocols[p] == PROTO_SWD) {
            for (uint8_t i = 0; i < n_data; i++) {
                for (uint8_t j = 0; j < n_data; j++) {
                    if (i == j) continue;
                    uint8_t conf = discovery_try_swd(
                        data_chs[i], data_chs[j], result);
                    if (conf > result->confidence) {
                        result->confidence = conf;
                        result->protocol = PROTO_SWD;
                    }
                    if (conf >= 80) goto done;
                }
            }
        }
        if (protocols[p] == PROTO_JTAG) {
            /* Try ordered 4-tuples (limited search) */
            for (uint8_t a = 0; a < n_data && n_data >= 4; a++) {
                for (uint8_t b = 0; b < n_data; b++) {
                    if (b == a) continue;
                    for (uint8_t c = 0; c < n_data; c++) {
                        if (c == a || c == b) continue;
                        for (uint8_t d = 0; d < n_data; d++) {
                            if (d == a || d == b || d == c) continue;
                            uint8_t conf = discovery_try_jtag(
                                data_chs[a], data_chs[b],
                                data_chs[c], data_chs[d], result);
                            if (conf > result->confidence) {
                                result->confidence = conf;
                                result->protocol = PROTO_JTAG;
                            }
                            if (conf >= 80) goto done;
                        }
                    }
                }
            }
        }
    }

done:
    if (result->confidence == 0) return GB_ERR_NO_TARGET;
    return GB_OK;
}

void discovery_test_all_channels(void)
{
    discovery_init();
    for (uint8_t ch = 0; ch < PROBE_CH_MAX; ch++) {
        if (probe_pinmap[ch].adc_channel == 0xFF) continue;
        uint16_t mv = discovery_read_channel_voltage(ch);
        (void)mv; /* would log via debug UART */
    }
}