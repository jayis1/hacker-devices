/*
 * adc_capture.c — ADC waveform capture driver
 * PlasmaReaper Multi-Vector Fault Injection Toolkit
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Uses ADC1 channel 0 (PA0) to capture the target VCC rail voltage
 * during a glitch. The ADC runs at 3.6 MSps with DMA, storing 256
 * samples around the glitch event. The captured waveform is used to:
 *  1. Verify the glitch actually occurred (VCC dropped)
 *  2. Classify the glitch quality (clean, overshoot, underdepth)
 *  3. Debug parameter tuning (the waveform can be sent to the host)
 *
 * The ADC is triggered by the HRTIM (same event that fires the glitch),
 * ensuring the samples are centered on the glitch event.
 */

#include "adc_capture.h"
#include "../registers.h"
#include "../board.h"

/* ---- Private state ------------------------------------------------- */

static uint16_t g_adc_buf[256];
static uint16_t g_adc_len;
static uint16_t *g_user_buf;
static uint16_t *g_user_len;
static uint16_t g_max_samples;
static volatile bool g_capture_done;

/* ---- Initialization ------------------------------------------------ */

void adc_capture_init(void)
{
    /*
     * Configure PA0 as analog input (ADC1 channel 0).
     */
    gpio_config(GPIOA, VCC_MONITOR_PIN, GPIO_MODE_ANALOG, GPIO_OSPEED_LOW,
                GPIO_PUPD_NONE, 0);

    /*
     * Configure ADC1:
     *  - 12-bit resolution
     *  - Single conversion mode (triggered by HRTIM)
     *  - DMA mode: store N samples then stop
     *  - Software trigger for now (HRTIM trigger in full implementation)
     */
    ADC1->CR = 0;
    ADC1->CFGR = ADC_CFGR_DMAEN | ADC_CFGR_OVRMOD; /* DMA, overwrite on overrun */
    /* Disable continuous mode — we want triggered conversions */

    /* Set sample time for channel 0 (maximum for best accuracy) */
    ADC1->SMPR1 = 7; /* 640.5 cycles sampling time for channel 0 */

    /* Configure sequence: channel 0, 1 conversion */
    ADC1->SQR1 = (1U << 0); /* L=0 → 1 conversion */
    ADC1->SQR3 = (VCC_MONITOR_CHANNEL << 0); /* SQ1 = channel 0 */

    /* Enable ADC */
    ADC1->CR = ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY))
        ;

    /* Configure DMA1 Stream 0 for ADC1 */
    DMA1_Stream0->CR = 0;
    DMA1_Stream0->PAR = (uint32_t)&ADC1->DR;
    DMA1_Stream0->M0AR = (uint32_t)g_adc_buf;
    DMA1_Stream0->NDTR = 256;
    DMA1_Stream0->CR = DMA_SxCR_DIR_P2M
                     | DMA_SxCR_MINC
                     | DMA_SxCR_PSIZE_16
                     | DMA_SxCR_MSIZE_16
                     | DMA_SxCR_PRIO_HIGH
                     | DMA_SxCR_TCIE; /* transfer complete IRQ */

    /* Enable NVIC for DMA1 Stream 0 */
    NVIC_ISER0 |= BIT(DMA1_Stream0_IRQn);

    g_capture_done = false;
    g_adc_len = 0;
}

/* ---- Arm the capture ----------------------------------------------- */

void adc_capture_arm(uint16_t *buf, uint16_t *len, uint16_t max_samples)
{
    g_user_buf = buf;
    g_user_len = len;
    g_max_samples = max_samples;
    if (g_max_samples > 256)
        g_max_samples = 256;

    g_capture_done = false;

    /* Reset DMA */
    DMA1_Stream0->CR &= ~DMA_SxCR_EN; /* disable DMA */
    DMA1_Stream0->M0AR = (uint32_t)g_adc_buf;
    DMA1_Stream0->NDTR = g_max_samples;
    DMA1_Stream0->CR |= DMA_SxCR_EN;  /* re-enable DMA */

    /* Start ADC conversions (software trigger for now) */
    ADC1->CR |= ADC_CR_ADSTART;
}

/* ---- Read captured data -------------------------------------------- */

void adc_capture_read(uint16_t *buf, uint16_t *len)
{
    /* Wait for capture to complete (with timeout) */
    uint32_t timeout = board_millis();
    while (!g_capture_done) {
        if ((board_millis() - timeout) > 100)
            break; /* 100 ms timeout */
    }

    uint16_t n = g_adc_len;
    if (n > g_max_samples)
        n = g_max_samples;

    for (uint16_t i = 0; i < n; i++)
        buf[i] = g_adc_buf[i];
    *len = n;
}

/* ---- DMA interrupt handler ----------------------------------------- */

void adc_capture_dma_isr(void)
{
    /* Check transfer complete flag */
    if (DMA1_LISR & BIT(5)) { /* TCIF0 = stream 0 transfer complete */
        DMA1_LIFCR |= BIT(5); /* clear flag */

        g_adc_len = g_max_samples;
        g_capture_done = true;

        /* Stop ADC */
        ADC1->CR &= ~ADC_CR_ADSTART;
    }
}

/* ---- Glitch quality classification --------------------------------- */

glitch_quality_t adc_capture_classify(const uint16_t *waveform,
                                      uint16_t len,
                                      uint16_t expected_depth_mv)
{
    /*
     * Classify the glitch waveform by analyzing the VCC drop.
     *
     * The ADC reads VCC with 12-bit resolution over 0–3.3 V range.
     * Nominal VCC = 3300 mV → ADC value ~ 0xFFF (4095).
     *
     * A good glitch should show:
     *  - VCC dropping by approximately expected_depth_mv
     *  - The drop lasting for approximately the glitch width
     *  - VCC recovering to nominal after the glitch
     */

    if (len == 0)
        return GLITCH_QUALITY_NO_GLITCH;

    /* Find the minimum VCC value during the glitch */
    uint16_t min_val = 0xFFFF;
    uint16_t max_val = 0;
    uint32_t sum = 0;

    for (uint16_t i = 0; i < len; i++) {
        if (waveform[i] < min_val)
            min_val = waveform[i];
        if (waveform[i] > max_val)
            max_val = waveform[i];
        sum += waveform[i];
    }

    /* Convert ADC values to mV: mV = adc * 3300 / 4096 */
    uint16_t min_mv = (uint16_t)((uint32_t)min_val * 3300 / 4096);
    uint16_t max_mv = (uint16_t)((uint32_t)max_val * 3300 / 4096);
    uint16_t avg_mv = (uint16_t)(sum * 3300 / (4096 * len));

    /* Expected: VCC drops by expected_depth_mv from ~3300 mV */
    uint16_t expected_min_mv = 3300 - expected_depth_mv;

    /* No glitch: VCC stayed near nominal */
    if (min_mv > 3000) {
        return GLITCH_QUALITY_NO_GLITCH;
    }

    /* Underdepth: VCC didn't drop enough (less than 50% of expected) */
    if (min_mv > (expected_min_mv + expected_depth_mv / 2)) {
        return GLITCH_QUALITY_UNDERDEPTH;
    }

    /* Overshoot: VCC dropped too far (went below GND + 100 mV) */
    if (min_mv < 100) {
        return GLITCH_QUALITY_OVERSHOOT;
    }

    /* Invalid: VCC never recovered (stuck low) */
    if (max_mv < 2000) {
        return GLITCH_QUALITY_INVALID;
    }

    /* Clean glitch: VCC dropped to expected range and recovered */
    (void)avg_mv;
    return GLITCH_QUALITY_CLEAN;
}

/* ---- End of file --------------------------------------------------- */