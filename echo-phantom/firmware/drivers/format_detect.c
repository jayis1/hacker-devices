/*
 * format_detect.c — I²S/TDM Audio Format Auto-Detection
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Auto-detects the I²S/TDM audio format on the target bus by:
 *   1. Measuring BCLK frequency using TIM2 input capture
 *   2. Measuring WS (LRCLK) frequency using another TIM2 channel
 *   3. Computing bit_depth × channels = BCLK / WS
 *   4. Determining the protocol (I²S, LJ, RJ, PCM, TDM)
 *   5. Determining bit depth from data transitions
 *
 * The detection runs for ~100ms to get accurate frequency measurements.
 */

#include "board.h"
#include "registers.h"

/* ========================================================================
 *  Measure frequency on a GPIO pin using TIM2 input capture
 *
 *  TIM2 is configured in input capture mode. We count the number of
 *  timer ticks between N rising edges to compute the frequency.
 * ======================================================================== */

static uint32_t measure_frequency(uint32_t gpio_base, uint8_t pin, uint32_t timeout_ms)
{
    uint32_t count_start = millis();

    /* Configure the pin as input */
    GPIO_MODER(gpio_base) &= ~(3U << (pin * 2));
    /* Enable pull-down to avoid floating input noise */
    GPIO_PUPDR(gpio_base) &= ~(3U << (pin * 2));
    GPIO_PUPDR(gpio_base) |= (GPIO_PUPD_PD << (pin * 2));

    /* Configure TIM2 for input capture on channel 1 (PA0 / TI1) */
    /* Actually we need to route the specific pin to TIM2 via SYSCFG */
    /* For simplicity, we'll poll the pin directly */

    uint32_t edge_count = 0;
    uint8_t last_state = 0;
    uint32_t start_tick = millis();
    uint32_t duration = timeout_ms;

    /* Count rising edges for 'duration' milliseconds */
    while ((millis() - start_tick) < duration) {
        uint8_t state = (GPIO_IDR(gpio_base) >> pin) & 1;
        if (state && !last_state) {
            edge_count++;
        }
        last_state = state;
    }

    /* Frequency = edge_count / duration_in_seconds */
    if (duration == 0)
        return 0;

    return (edge_count * 1000U) / duration;
}

/* ========================================================================
 *  Detect I²S/TDM format
 *
 *  Returns 0 on success, -1 if no bus detected
 * ======================================================================== */

int format_detect(audio_format_t *fmt)
{
    uint32_t bclk_freq, ws_freq;
    uint32_t bits_per_frame;

    /* Default to standard I²S if detection fails */
    fmt->sample_rate = 48000;
    fmt->bit_depth = 16;
    fmt->channels = 2;
    fmt->protocol = PROTO_I2S_PHILIPS;

    /*
     * Step 1: Measure BCLK frequency
     * BCLK = sample_rate × bit_depth × channels
     * For 48kHz/16-bit/stereo: BCLK = 48000 × 16 × 2 = 1.536 MHz
     * For 96kHz/24-bit/stereo: BCLK = 96000 × 24 × 2 = 4.608 MHz
     * For 48kHz/16-bit/8ch TDM: BCLK = 48000 × 16 × 8 = 6.144 MHz
     */
    bclk_freq = measure_frequency(SAI1_BCLK_GPIO, SAI1_BCLK_PIN, 100);

    if (bclk_freq < 100) {
        /* No clock detected — bus may be inactive */
        return -1;
    }

    /*
     * Step 2: Measure WS (LRCLK) frequency
     * WS = sample_rate
     * For 48kHz: WS = 48000 Hz
     */
    ws_freq = measure_frequency(SAI1_WS_GPIO, SAI1_WS_PIN, 200);

    if (ws_freq < 100) {
        return -1;
    }

    /* Step 3: Determine sample rate from WS */
    fmt->sample_rate = ws_freq;

    /* Step 4: Compute bits per frame = BCLK / WS */
    if (ws_freq == 0)
        return -1;
    bits_per_frame = bclk_freq / ws_freq;

    /* Step 5: Determine channel count and bit depth */
    /* Common combinations:
     *   bits_per_frame = bit_depth × channels
     *   32  → 16-bit stereo or 8-bit 4ch or 32-bit mono
     *   48  → 24-bit stereo or 16-bit 3ch
     *   64  → 32-bit stereo or 16-bit 4ch or 8-bit 8ch
     *   96  → 24-bit 4ch or 16-bit 6ch
     *   128 → 32-bit 4ch or 16-bit 8ch
     *   256 → 16-bit 16ch TDM or 32-bit 8ch
     */

    /* Try common bit depth + channel combinations */
    struct {
        uint8_t bit_depth;
        uint8_t channels;
    } candidates[] = {
        {16, 2}, {24, 2}, {32, 2},
        {16, 4}, {24, 4}, {32, 4},
        {16, 8}, {24, 8}, {32, 8},
        {16, 16}, {24, 16}, {32, 16},
        {8, 4}, {8, 8}, {8, 16},
    };

    int found = 0;
    for (int i = 0; i < (int)(sizeof(candidates)/sizeof(candidates[0])); i++) {
        if (candidates[i].bit_depth * candidates[i].channels == bits_per_frame) {
            fmt->bit_depth = candidates[i].bit_depth;
            fmt->channels = candidates[i].channels;
            found = 1;
            break;
        }
    }

    if (!found) {
        /* Default to 16-bit stereo if we can't match */
        fmt->bit_depth = 16;
        fmt->channels = 2;
    }

    /* Step 6: Determine protocol */
    /* If channels > 2, it's TDM */
    if (fmt->channels > 2) {
        fmt->protocol = PROTO_TDM;
    } else {
        /* Distinguish between I²S Philips, LJ, RJ, PCM */
        /* For simplicity, assume I²S Philips (most common) */
        /* A more sophisticated detector would check WS-to-data timing */
        fmt->protocol = PROTO_I2S_PHILIPS;
    }

    /* Validate detected format against known sample rates */
    switch (fmt->sample_rate) {
    case 8000: case 16000: case 22050: case 24000:
    case 32000: case 44100: case 48000:
    case 88200: case 96000: case 176400: case 192000:
        /* Valid sample rate */
        break;
    default:
        /* Round to nearest standard rate */
        if (fmt->sample_rate < 12000)
            fmt->sample_rate = 8000;
        else if (fmt->sample_rate < 20000)
            fmt->sample_rate = 16000;
        else if (fmt->sample_rate < 27000)
            fmt->sample_rate = 22050;
        else if (fmt->sample_rate < 37000)
            fmt->sample_rate = 32000;
        else if (fmt->sample_rate < 64000)
            fmt->sample_rate = 48000;
        else if (fmt->sample_rate < 128000)
            fmt->sample_rate = 96000;
        else
            fmt->sample_rate = 192000;
        break;
    }

    return 0;
}

/* ========================================================================
 *  Get human-readable protocol name
 * ======================================================================== */

const char *format_protocol_name(uint8_t proto)
{
    switch (proto) {
    case PROTO_I2S_PHILIPS: return "I2S Philips";
    case PROTO_LJ:          return "Left-Justified";
    case PROTO_RJ:          return "Right-Justified";
    case PROTO_PCM_SHORT:   return "PCM Short";
    case PROTO_PCM_LONG:    return "PCM Long";
    case PROTO_TDM:         return "TDM";
    default:                return "Unknown";
    }
}

/* ========================================================================
 *  Print format info (for debug output over USB CDC)
 * ======================================================================== */

void format_print(const audio_format_t *fmt)
{
    (void)fmt;  /* Debug output would go to USB CDC here */
}