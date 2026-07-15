/*
 * tesla-phantom — drivers/adc_capture.c
 * AD7606C-6 ADC configuration and DMA-based multi-channel capture.
 * 16-bit, 6-channel, 800 kSPS simultaneous sampling for SCA traces.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"

/* AD7606C-6 interface via SPI2:
 * - CONVST (PB5): start conversion (falling edge)
 * - BUSY  (PB6): conversion in progress (high = busy)
 * - CS    (PB4): SPI chip select
 * - RESET (PB7): reset ADC
 * - RANGE (PB8): input range (0 = ±10V, 1 = ±5V)
 * - SCLK  (PI1), DOUTA (PI2), DIN (PI3)
 *
 * Conversion sequence:
 * 1. Pull CONVST low → conversion starts
 * 2. Wait for BUSY to go low
 * 3. Pull CS low
 * 4. Read 16 bits × 6 channels via SPI (DOUTA outputs CH1 first)
 * 5. Pull CS high
 *
 * For DMA capture, we use a continuous conversion mode where TIM6
 * triggers CONVST at the desired rate, and DMA2 stream 0 reads the
 * SPI2 DR register into the external SRAM buffer.
 */

#define ADC_NUM_CHANNELS  6

static uint16_t current_rate_khz = 800;
static uint8_t  current_range_5v = 1;
static uint8_t  current_gain_db = 0;
static uint16_t current_filter_cutoff = 50000;

/* External callback (defined in main.c) */
extern void adc_dma_complete(void);

/* ── SPI2 init for AD7606 ─────────────────────────────────────────── */
static void adc_spi_init(void) {
    volatile spi_regs_t *s = SPI2;

    s->CR1 = 0;  /* disable */
    s->CR2 = SPI_CR2_DS_16BIT;  /* 16-bit data */
    s->CR1 = SPI_CR1_MSTR
           | SPI_CR1_SSM
           | SPI_CR1_SSI
           | SPI_CR1_BR_DIV8   /* 120 MHz / 8 = 15 MHz SPI clock */
           | SPI_CR1_CPHA;     /* CPHA=1 for AD7606 */
    s->CR1 |= SPI_CR1_SPE;
}

static void adc_reset_pulse(void) {
    gpio_clr(GPIO(GPIOB), ADC_RESET_PIN);
    delay_us(1);
    gpio_set(GPIO(GPIOB), ADC_RESET_PIN);
    delay_us(1);
}

static void adc_convst_pulse(void) {
    gpio_clr(GPIO(GPIOB), ADC_CONVST_PIN);
    delay_us(1);
    gpio_set(GPIO(GPIOB), ADC_CONVST_PIN);
}

static void adc_wait_busy(void) {
    while (gpio_get(GPIO(GPIOB), ADC_BUSY_PIN)) { }
}

/* ── VGA gain control (AD8331 via SPI digital pot) ────────────────── */
static void vga_set_gain_spi(uint8_t gain_db) {
    /* The AD8331 gain is controlled by a digital potentiometer
     * (AD5144) connected via SPI2 (sharing with ADC).
     * Gain range: 0–48 dB, mapped to 0–255 wiper position.
     * We write to the AD5144 RDAC register. */
    uint8_t wiper = (uint8_t)((uint32_t)gain_db * 255U / 48U);

    /* Select VGA CS (PB9) */
    gpio_clr(GPIO(GPIOB), VGA_CS1_PIN);

    /* Write to AD5144: [0x00] [0x01] [wiper] — simplified */
    /* Actually we use the SPI in 8-bit mode for the pot.
     * For simplicity, we switch SPI temporarily to 8-bit. */
    volatile spi_regs_t *s = SPI2;
    s->CR1 &= ~SPI_CR1_SPE;
    s->CR2 = SPI_CR2_DS_8BIT;
    s->CR1 |= SPI_CR1_SPE;

    spi_xfer(s, 0x00);  /* command: write RDAC */
    spi_xfer(s, 0x01);  /* address: channel 1 */
    spi_xfer(s, wiper); /* data: wiper position */

    gpio_set(GPIO(GPIOB), VGA_CS1_PIN);

    /* Restore 16-bit mode for ADC */
    s->CR1 &= ~SPI_CR1_SPE;
    s->CR2 = SPI_CR2_DS_16BIT;
    s->CR1 |= SPI_CR1_SPE;
}

/* ── LTC1564 filter cutoff control ────────────────────────────────── */
static void filter_set_cutoff_spi(uint16_t cutoff_hz) {
    /* The LTC1564-7 cutoff is programmed via a 6-bit code
     * sent through an SPI interface (shared SPI2).
     * Cutoff range: 1 kHz to 150 kHz in 64 steps.
     * Code = (cutoff_hz / 150000) * 63 */
    uint8_t code;
    if (cutoff_hz >= 150000) code = 63;
    else if (cutoff_hz <= 1000) code = 0;
    else code = (uint8_t)((uint32_t)cutoff_hz * 63U / 150000U);

    /* Select filter CS (PB11) */
    gpio_clr(GPIO(GPIOB), FILTER_CS_PIN);

    /* Switch to 8-bit mode */
    volatile spi_regs_t *s = SPI2;
    s->CR1 &= ~SPI_CR1_SPE;
    s->CR2 = SPI_CR2_DS_8BIT;
    s->CR1 |= SPI_CR1_SPE;

    spi_xfer(s, code);

    gpio_set(GPIO(GPIOB), FILTER_CS_PIN);

    /* Restore 16-bit mode */
    s->CR1 &= ~SPI_CR1_SPE;
    s->CR2 = SPI_CR2_DS_16BIT;
    s->CR1 |= SPI_CR1_SPE;
}

/* ── DMA configuration for continuous capture ─────────────────────── */
static void adc_setup_dma(int16_t *buf, uint32_t samples) {
    /* DMA2 stream 0, channel 3 (SPI2_RX)
     * Peripheral: SPI2->DR
     * Memory: buf (external SRAM)
     * Direction: peripheral-to-memory
     * Data width: 16-bit (both peripheral and memory)
     * Memory increment, no peripheral increment
     */
    volatile uint32_t *dma_cr   = (volatile uint32_t *)(DMA2_BASE + 0x10);
    volatile uint32_t *dma_ndtr = (volatile uint32_t *)(DMA2_BASE + 0x14);
    volatile uint32_t *dma_par  = (volatile uint32_t *)(DMA2_BASE + 0x18);
    volatile uint32_t *dma_m0ar = (volatile uint32_t *)(DMA2_BASE + 0x1C);
    volatile uint32_t *dma_fcr  = (volatile uint32_t *)(DMA2_BASE + 0x24);

    /* Disable DMA stream */
    *dma_cr = 0;
    delay_us(1);

    /* Clear DMA flags */
    volatile uint32_t *dma_lifcr = (volatile uint32_t *)(DMA2_BASE + 0x08);
    *dma_lifcr = 0x0000003F;

    /* Total transfer: samples × channels (interleaved) */
    uint32_t total = samples * ADC_NUM_CHANNELS;

    *dma_par  = (uint32_t)(&(SPI2->DR));
    *dma_m0ar = (uint32_t)buf;
    *dma_ndtr = total;

    /* Configure: P2M, 16-bit, minc, chsel=3 (SPI2_RX), TCIE */
    *dma_cr = DMA_SxCR_DIR_P2M
            | DMA_SxCR_MINC
            | DMA_SxCR_PSIZE_16
            | DMA_SxCR_MSIZE_16
            | DMA_SxCR_CHSEL(3)
            | DMA_SxCR_TCIE;

    /* FIFO: direct mode (no FIFO) */
    *dma_fcr = 0;

    /* Enable DMA stream */
    *dma_cr |= DMA_SxCR_EN;

    /* Enable SPI RX DMA */
    SPI2->CR2 |= SPI_CR2_RXDMAEN;
}

/* ── Public API ───────────────────────────────────────────────────── */

int adc_init(void) {
    adc_spi_init();

    /* Reset ADC */
    adc_reset_pulse();
    delay_ms(1);

    /* Set default range to ±5V */
    gpio_set(GPIO(GPIOB), ADC_RANGE_PIN);
    current_range_5v = 1;

    /* Set default VGA gain to 0 dB */
    vga_set_gain_spi(0);
    current_gain_db = 0;

    /* Set default filter cutoff to 50 kHz */
    filter_set_cutoff_spi(50000);
    current_filter_cutoff = 50000;

    return 0;
}

int adc_start_capture(int16_t *buf, uint32_t samples) {
    if (samples == 0) return -1;
    if (buf == NULL) return -1;

    /* Configure DMA transfer */
    adc_setup_dma(buf, samples);

    /* Start first conversion — in continuous mode, the ADC
     * will keep producing data as we read it via SPI/DMA.
     * For the AD7606, we pulse CONVST and then read all channels.
     * In DMA mode, we use a timer to trigger periodic CONVST pulses
     * while DMA reads the SPI data. */
    adc_convst_pulse();

    return 0;
}

int adc_read_burst(int16_t *buf, uint32_t samples) {
    /* Blocking read of N samples × 6 channels */
    volatile spi_regs_t *s = SPI2;

    for (uint32_t i = 0; i < samples; i++) {
        /* Start conversion */
        adc_convst_pulse();
        adc_wait_busy();

        /* Read 6 channels (16-bit each) */
        gpio_clr(GPIO(GPIOB), ADC_CS_PIN);
        for (int ch = 0; ch < ADC_NUM_CHANNELS; ch++) {
            buf[i * ADC_NUM_CHANNELS + ch] = (int16_t)spi_xfer(s, 0xFFFF);
        }
        gpio_set(GPIO(GPIOB), ADC_CS_PIN);
    }

    return 0;
}

void adc_set_range(int range_5v) {
    if (range_5v)
        gpio_set(GPIO(GPIOB), ADC_RANGE_PIN);
    else
        gpio_clr(GPIO(GPIOB), ADC_RANGE_PIN);
    current_range_5v = range_5v;
}

void adc_reset(void) {
    adc_reset_pulse();
    delay_ms(1);
}

int adc_set_oversampling(uint8_t ratio) {
    /* AD7606C supports hardware oversampling (2, 4, 8, 16, 32, 64).
     * Configured via OS[2:0] pins. In our design, these are tied
     * to GPIO or SPI-configurable. For simplicity, we note it here. */
    (void)ratio;
    return 0;
}

int adc_set_filter_cutoff(uint16_t cutoff_hz) {
    filter_set_cutoff_spi(cutoff_hz);
    current_filter_cutoff = cutoff_hz;
    return 0;
}

int adc_set_vga_gain(uint8_t gain_db) {
    if (gain_db > 48) gain_db = 48;
    vga_set_gain_spi(gain_db);
    current_gain_db = gain_db;
    return 0;
}

/* ── DMA2 Stream 0 interrupt handler ──────────────────────────────── */
void dma2_stream0_irq(void) {
    /* Check transfer complete flag */
    volatile uint32_t *dma_lisr = (volatile uint32_t *)(DMA2_BASE + 0x00);
    if (*dma_lisr & (1U << 5)) {  /* TCIF0 */
        /* Clear flag */
        volatile uint32_t *dma_lifcr = (volatile uint32_t *)(DMA2_BASE + 0x08);
        *dma_lifcr = (1U << 5);

        /* Disable SPI DMA */
        SPI2->CR2 &= ~SPI_CR2_RXDMAEN;

        /* Notify main */
        adc_dma_complete();
    }
}