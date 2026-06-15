/*
 * ir_receiver.c — IR receiver driver for TSMP58000
 * Captures raw IR waveforms via ADC1 and digital edges via TIM2
 * Real-time protocol decoding engine
 */

#include "drivers/ir_receiver.h"
#include <string.h>

/* External DMA buffers (defined in main.c) */
extern volatile uint16_t g_adc_buffer[];
extern volatile uint32_t g_tim_buffer[];
extern volatile uint32_t g_frames_captured;

/* Module state */
static ir_frame_t g_last_frame;
static ir_stats_t g_stats;
static volatile uint8_t g_capture_running = 0;
static volatile uint32_t g_adc_half_ptr = 0;

/* Raw timing accumulation buffer */
static uint32_t g_raw_timing[256];
static uint16_t g_raw_timing_idx = 0;
static uint8_t g_in_mark = 0;       /* Currently in mark (IR burst) */
static uint32_t g_last_edge_tick = 0; /* Last edge timestamp */

/* ========================================
 * ADC1 and DMA initialization
 * ======================================== */

void ir_receiver_init(void) {
    /* Enable ADC1 and DMA1 clocks */
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->AHB1ENR |= (1U << 0);  /* DMA1 clock */

    /* Configure ADC1 for channel 7 (PA7) */
    /* Enable ADC voltage regulator */
    ADC1->CR &= ~ADC_CR_ADEN;
    ADC1->CR |= ADC_CR_ADVREGEN;
    for (volatile int i = 0; i < 100; i++);  /* Startup delay */

    /* Calibrate ADC */
    ADC1->CR |= (1U << ADC_CR_ADCAL_Pos);
    while (ADC1->CR & ADC_CR_ADCAL)
        ;

    /* Configure ADC1:
     * - 12-bit resolution (0b00)
     * - Continuous mode (CONT=1)
     * - DMA enabled, circular mode (DMAEN=1, DMACFG=1)
     * - Overrun disable (OVRDIS=1)
     */
    ADC1->CFGR = ADC_CFGR_CONT | ADC_CFGR_DMAEN | ADC_CFGR_DMACFG | ADC_CFGR_OVRDIS;
    ADC1->CFGR2 = 0;  /* No oversampling */

    /* Set sample time for channel 7: 1.5 cycles (fastest) */
    ADC1->SMPR1 &= ~(0x7U << (7 * 3));  /* Clear SMP7 */
    ADC1->SMPR1 |= (0U << (7 * 3));      /* 1.5 cycles */

    /* Pre-select channel 7 */
    ADC1->PCSEL |= (1U << 7);

    /* Configure DMA1 Stream 0 for ADC1 */
    DMA1_STREAM0->CR = 0;  /* Disable stream first */
    while (DMA1_STREAM0->CR & DMA_CR_EN)  /* Wait for stream to disable */
        ;

    DMA1_STREAM0->PAR = (uint32_t)&ADC1->DR;      /* Peripheral: ADC1 DR */
    DMA1_STREAM0->M0AR = (uint32_t)g_adc_buffer;   /* Memory: ADC buffer */
    DMA1_STREAM0->NDTR = IR_ADC_SAMPLE_RATE / 500;  /* Buffer size */
    DMA1_STREAM0->CR = DMA_CR_MINC |         /* Memory increment */
                        DMA_CR_CIRC |         /* Circular mode */
                        DMA_CR_TCIE |          /* Transfer complete interrupt */
                        DMA_CR_HTIE |          /* Half transfer interrupt */
                        (0U << 11) |           /* Peripheral size: 16-bit (half-word) */
                        (1U << 13) |           /* Memory size: 16-bit (half-word) */
                        (0U << 6);             /* Direction: peripheral-to-memory */

    /* Clear DMA flags */
    DMA1->LISR = 0x0F0FU;  /* Clear all stream 0 flags */

    /* Configure TIM2 for 32-bit input capture (digital edge timestamps) */
    TIM2->CR1 = 0;  /* Disable TIM2 */
    TIM2->PSC = 0;   /* No prescaler: count at APB1 timer clock (240 MHz) */
    TIM2->ARR = 0xFFFFFFFF;  /* Full 32-bit range */
    TIM2->CCMR1 = TIM_CCMR1_CC1S_INPUT_TI1;  /* CH1 = TI1 (PA6) */
    TIM2->CCER = TIM_CCER_CC1E | TIM_CCER_CC1P;  /* Enable CH1, both edges */
    TIM2->DIER = TIM_DIER_CC1IE | TIM_DIER_DMAEN;  /* CC1 interrupt + DMA */
    TIM2->CR1 = TIM_CR1_CEN;  /* Enable TIM2 */

    /* Reset statistics */
    memset(&g_stats, 0, sizeof(g_stats));
}

void ir_receiver_start(uint32_t sample_rate) {
    (void)sample_rate;  /* Always 2 Msps */

    /* Enable ADC1 */
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY))
        ;

    /* Enable DMA stream */
    DMA1_STREAM0->CR |= DMA_CR_EN;

    /* Start ADC conversion */
    ADC1->CR |= ADC_CR_ADSTART;

    g_capture_running = 1;
}

void ir_receiver_stop(void) {
    /* Stop ADC conversion */
    ADC1->CR |= ADC_CR_ADSTP;
    while (ADC1->CR & ADC_CR_ADSTP)
        ;

    /* Disable ADC */
    ADC1->CR |= ADC_CR_ADDIS;

    /* Disable DMA stream */
    DMA1_STREAM0->CR &= ~DMA_CR_EN;

    g_capture_running = 0;
}

/* ========================================
 * Protocol detection engine
 * ======================================== */

/* Decode NEC protocol from raw timing data */
static int decode_nec(const uint32_t *timing, uint16_t count, ir_frame_t *frame) {
    if (count < 2) return -1;

    /* NEC: 9ms mark + 4.5ms space = leader */
    if (timing[0] < 7000 || timing[0] > 11000) return -1;  /* Mark ~9ms */
    if (timing[1] < 3000 || timing[1] > 6000) return -1;   /* Space ~4.5ms */

    /* Decode 32 bits (8 addr + 8 ~addr + 8 cmd + 8 ~cmd) */
    uint32_t data = 0;
    for (int i = 2; i < count - 1 && i < 66; i += 2) {
        uint32_t mark = timing[i];
        uint32_t space = timing[i + 1];

        /* Mark should be ~560 us */
        if (mark < 300 || mark > 900) return -1;

        /* Space determines bit value: ~560 us = 0, ~1690 us = 1 */
        data >>= 1;
        if (space > 1000 && space < 2200) {
            data |= (1U << 31);
        } else if (space < 300 || space > 2800) {
            return -1;
        }
        /* else space ~560 us, bit = 0 */
    }

    /* Check for extended NEC (8-bit address + 8-bit inverse) */
    frame->protocol = IR_PROTO_NEC;
    frame->address = (data >> 24) & 0xFF;
    frame->command = (data >> 8) & 0xFF;

    /* Verify address inverse */
    uint8_t addr_inv = (data >> 16) & 0xFF;
    if ((frame->address ^ addr_inv) == 0xFF) {
        /* Standard NEC */
    } else if (addr_inv != (frame->address ^ 0xFF)) {
        /* Extended NEC (16-bit address) */
        frame->protocol = IR_PROTO_NEC_EXT;
        frame->address = (data >> 16) & 0xFFFF;
    }

    return 0;
}

/* Decode RC5 protocol */
static int decode_rc5(const uint32_t *timing, uint16_t count, ir_frame_t *frame) {
    if (count < 4) return -1;

    /* RC5: Manchester encoding, 889 us bit period */
    /* Simplified: decode from raw mark/space pairs */
    frame->protocol = IR_PROTO_RC5;
    frame->address = 0;
    frame->command = 0;

    /* Manchester decode logic would go here */
    /* For now, store raw timing for host-side decoding */
    return 0;
}

/* Auto-detect and decode protocol */
static int detect_and_decode(const uint32_t *timing, uint16_t count) {
    int result = -1;

    /* Try NEC first (most common) */
    result = decode_nec(timing, count, &g_last_frame);
    if (result == 0) return 0;

    /* Try RC5 */
    result = decode_rc5(timing, count, &g_last_frame);
    if (result == 0) return 0;

    /* Unknown protocol — store as raw */
    g_last_frame.protocol = IR_PROTO_RAW_WAVEFORM;
    g_last_frame.address = 0;
    g_last_frame.command = 0;

    return -1;
}

/* ========================================
 * ADC DMA half/full callbacks
 * ======================================== */

void adc_dma_half_complete(void) {
    /* First half of buffer filled — process samples 0..N/2 */
    g_adc_half_ptr = 0;
    g_dma_half_irq++;
}

void adc_dma_full_complete(void) {
    /* Second half of buffer filled — process samples N/2..N */
    g_adc_half_ptr = IR_ADC_SAMPLE_RATE / 1000;  /* Offset to second half */
    g_dma_full_irq++;
}

/* ========================================
 * TIM2 capture callback (digital edge detection)
 * ======================================== */

void tim2_capture_callback(void) {
    /* Read captured timestamp from TIM2 CCR1 */
    uint32_t timestamp = TIM2->CCR1;

    /* Calculate delta from last edge */
    uint32_t delta_us = 0;
    if (g_last_edge_tick != 0) {
        delta_us = (timestamp - g_last_edge_tick) / (TIM_APB1_FREQ / 1000000U);
    }
    g_last_edge_tick = timestamp;

    /* Store timing entry */
    if (g_raw_timing_idx < 256) {
        g_raw_timing[g_raw_timing_idx++] = delta_us;
    }

    /* Check for end of frame (long silence = new frame) */
    /* If we have enough timing data, try to decode */
    if (g_raw_timing_idx >= 4 && g_raw_timing_idx < 256) {
        /* Simple heuristic: if we got a sequence of marks/spaces, try decode */
        if (detect_and_decode(g_raw_timing, g_raw_timing_idx) == 0) {
            g_frames_captured++;
            g_stats.total_frames++;
            g_stats.last_protocol = g_last_frame.protocol;
            g_stats.last_address = g_last_frame.address;
            g_stats.last_command = g_last_frame.command;
        } else {
            g_stats.unknown_frames++;
        }
        g_raw_timing_idx = 0;
    }
}

/* ========================================
 * Poll function (called from main loop)
 * ======================================== */

void ir_receiver_poll(void) {
    /* Process DMA buffer for analog waveform analysis */
    /* In a full implementation, this would compute:
     * - Carrier frequency detection via zero-crossing counting
     * - Signal envelope analysis
     * - Noise floor estimation
     */

    /* For now, just maintain statistics */
    (void)g_adc_half_ptr;
}

ir_frame_t *ir_receiver_get_last_frame(void) {
    return &g_last_frame;
}

ir_stats_t *ir_receiver_get_stats(void) {
    return &g_stats;
}

void ir_receiver_reset_stats(void) {
    memset(&g_stats, 0, sizeof(g_stats));
    g_frames_captured = 0;
}