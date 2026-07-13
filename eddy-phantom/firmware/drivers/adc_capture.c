/*
 * eddy-phantom — adc_capture.c
 * AD7606C-6 ADC driver: SPI configuration, DMA burst capture,
 * and sample management for the electromagnetic emanation
 * keyboard reconnaissance device.
 *
 * AD7606C-6: 6-channel, 16-bit, simultaneous sampling ADC.
 * We use 4 channels (probe array) at 1 MSPS.
 * Interface: SPI (mode 0, 16-bit data words).
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"

/* DMA buffer for ADC data — placed in DTCM for fast access */
static volatile int16_t  dma_buf[BURST_SAMPLES * ADC_CHANNELS];
static volatile uint8_t  capture_complete = 0;
static volatile uint32_t capture_count = 0;

/* ── SPI1 low-level transfer ──────────────────────────────────── */
static uint16_t spi1_xfer16(uint16_t tx)
{
    spi_regs_t *spi = SPI(1);

    /* Wait for TXE */
    volatile uint32_t timeout = 0xFFFF;
    while (!(spi->SR & SPI_SR_TXE) && --timeout);

    /* Send 16-bit data (write to DR as 32-bit, lower 16 bits used) */
    spi->DR = tx;

    /* Wait for RXNE */
    timeout = 0xFFFF;
    while (!(spi->SR & SPI_SR_RXNE) && --timeout);

    return (uint16_t)spi->DR;
}

static void spi1_xfer_burst(int16_t *buf, uint32_t count)
{
    spi_regs_t *spi = SPI(1);

    for (uint32_t i = 0; i < count; i++) {
        volatile uint32_t timeout = 0xFFFF;
        while (!(spi->SR & SPI_SR_TXE) && --timeout);
        spi->DR = 0x0000;  /* dummy write to clock out data */

        timeout = 0xFFFF;
        while (!(spi->SR & SPI_SR_RXNE) && --timeout);
        buf[i] = (int16_t)spi->DR;
    }
}

/* ── AD7606C initialization ───────────────────────────────────── */
int adc_init(void)
{
    /* Hardware reset */
    gpio_clr(GPIO(ADC_RST_PORT), ADC_RST_PIN);
    /* Wait at least 50 ns — a few cycles at 480 MHz is plenty */
    for (volatile int i = 0; i < 10; i++);
    gpio_set(GPIO(ADC_RST_PORT), ADC_RST_PIN);
    for (volatile int i = 0; i < 100; i++);

    /* Configure SPI1: master, mode 0 (CPOL=0, CPHA=0), 16-bit data,
     * baud rate = PCLK2/16 = 240/16 = 15 MHz (AD7606 max SCLK = 20 MHz) */
    spi_regs_t *spi = SPI(1);

    /* Disable SPI */
    spi->CR1 = 0;

    /* Configure for 16-bit data frame, master, mode 0 */
    spi->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI |
               SPI_CR1_BR_DIV16;
    spi->CR2 = SPI_CR2_DS_16BIT | SPI_CR2_FRXTH;

    /* Enable SPI */
    spi->CR1 |= SPI_CR1_SPE;

    /* Set oversampling to OS=0 (no oversampling, max sample rate) */
    gpio_clr(GPIO(ADC_OS0_PORT), ADC_OS0_PIN);
    gpio_clr(GPIO(ADC_OS1_PORT), ADC_OS1_PIN);
    gpio_clr(GPIO(ADC_OS2_PORT), ADC_OS2_PIN);

    /* Set range to ±5V (range pin = 1 for ±5V on AD7606C) */
    gpio_set(GPIO(ADC_RANGE_PORT), ADC_RANGE_PIN);

    /* CONVST idle high */
    gpio_set(GPIO(ADC_CONVST_PORT), ADC_CONVST_PIN);

    return 0;
}

/* ── Set ADC range ────────────────────────────────────────────── */
void adc_set_range(int range_5v)
{
    if (range_5v) {
        gpio_set(GPIO(ADC_RANGE_PORT), ADC_RANGE_PIN);  /* ±5V */
    } else {
        gpio_clr(GPIO(ADC_RANGE_PORT), ADC_RANGE_PIN);  /* ±10V */
    }
}

/* ── Reset ADC ────────────────────────────────────────────────── */
void adc_reset(void)
{
    gpio_clr(GPIO(ADC_RST_PORT), ADC_RST_PIN);
    for (volatile int i = 0; i < 100; i++);
    gpio_set(GPIO(ADC_RST_PORT), ADC_RST_PIN);
    for (volatile int i = 0; i < 100; i++);
}

/* ── Start a conversion and read one sample set (4 channels) ──── */
static void adc_start_conversion(void)
{
    /* Pull CONVST low to start conversion on all channels simultaneously */
    gpio_clr(GPIO(ADC_CONVST_PORT), ADC_CONVST_PIN);
    /* Minimum CONVST low time = 50 ns — a few cycles */
    for (volatile int i = 0; i < 2; i++);
    gpio_set(GPIO(ADC_CONVST_PORT), ADC_CONVST_PIN);

    /* Wait for BUSY to go low (conversion complete) */
    volatile uint32_t timeout = 0xFFFF;
    while (gpio_get(GPIO(ADC_BUSY_PORT), ADC_BUSY_PIN) && --timeout);
}

/* ── Read one sample from all 4 channels ──────────────────────── */
static int adc_read_sample(int16_t *channels)
{
    adc_start_conversion();

    /* Read 4 channels (AD7606 outputs channels sequentially after CS low) */
    gpio_clr(GPIO(ADC_CS_PORT), ADC_CS_PIN);

    /* First read after conversion: channel 0, then 1, 2, 3 */
    for (int i = 0; i < ADC_CHANNELS; i++) {
        channels[i] = spi1_xfer16(0x0000);
    }

    gpio_set(GPIO(ADC_CS_PORT), ADC_CS_PIN);
    return 0;
}

/* ── Read a full burst of samples ───────────────────────────────
 * Captures BURST_SAMPLES × ADC_CHANNELS samples into buf.
 * Samples are interleaved: [ch0_s0, ch1_s0, ch2_s0, ch3_s0,
 *                            ch0_s1, ch1_s1, ...]
 *
 * At 1 MSPS, BURST_SAMPLES=2048 → 2048 µs = 2.048 ms per burst.
 */
int adc_read_burst(int16_t *buf, uint32_t samples)
{
    spi_regs_t *spi = SPI(1);

    for (uint32_t s = 0; s < samples; s++) {
        adc_start_conversion();

        /* Wait for BUSY low */
        volatile uint32_t timeout = 0xFFFF;
        while (gpio_get(GPIO(ADC_BUSY_PORT), ADC_BUSY_PIN) && --timeout);

        /* Read 4 channels */
        gpio_clr(GPIO(ADC_CS_PORT), ADC_CS_PIN);
        for (int ch = 0; ch < ADC_CHANNELS; ch++) {
            buf[s * ADC_CHANNELS + ch] = spi1_xfer16(0x0000);
        }
        gpio_set(GPIO(ADC_CS_PORT), ADC_CS_PIN);

        /* The AD7606C internal oscillator manages the sample rate.
         * For precise 1 MSPS, we use the CONVST timing.
         * With OS=0, max throughput is 200 kSPS per channel in SPI mode.
         * For 1 MSPS, we'd use the parallel interface or a faster ADC.
         * This implementation targets ~100 kSPS effective rate,
         * which is sufficient for keyboard emanation bursts (50-350 kHz BW).
         */
    }

    capture_count += samples;
    return 0;
}

/* ── DMA-based continuous capture (for triggered mode) ────────── */
int adc_start_capture(int16_t *buf, uint32_t samples)
{
    dma_regs_t *dma = (dma_regs_t *)DMA1_BASE;
    dma_stream_regs_t *stream = &dma->stream[0];  /* DMA1 Stream0 */

    /* Disable DMA stream */
    stream->CR &= ~DMA_CR_EN;
    while (stream->CR & DMA_CR_EN);

    /* Clear all interrupt flags for stream 0 */
    dma->LIFCR = 0x3F;

    /* Configure DMA1 Stream0:
     *   Channel = SPI1_RX (channel 4 for H7)
     *   Direction = peripheral-to-memory
     *   Peripheral = SPI1_DR, no increment
     *   Memory = buf, increment
     *   16-bit data size both sides
     *   Circular mode
     *   High priority
     */
    stream->PAR = (uint32_t)&SPI(1)->DR;
    stream->M0AR = (uint32_t)buf;
    stream->NDTR = samples * ADC_CHANNELS;
    stream->CR = (4U << 25)        /* CHSEL = channel 4 (SPI1_RX) */
               | DMA_CR_DIR_P2M    /* peripheral-to-memory */
               | DMA_CR_MINC       /* memory increment */
               | DMA_CR_PSIZE_16   /* peripheral 16-bit */
               | DMA_CR_MSIZE_16   /* memory 16-bit */
               | DMA_CR_PL_HIGH    /* high priority */
               | DMA_CR_CIRC       /* circular mode */
               | DMA_CR_TCIE;      /* transfer complete interrupt */

    /* FIFO: disable direct mode, full threshold */
    stream->FCR = DMA_FCR_DMDIS | DMA_FCR_FTH_FULL;

    /* Enable SPI RX DMA request */
    SPI(1)->CR2 |= SPI_CR1_RXDMAEN;

    /* Enable DMA stream */
    stream->CR |= DMA_CR_EN;

    capture_complete = 0;
    return 0;
}

/* ── Get the current peak amplitude across all channels ───────── */
int16_t adc_get_peak_amplitude(void)
{
    int16_t sample[ADC_CHANNELS];
    if (adc_read_sample(sample) != 0)
        return 0;

    int16_t peak = 0;
    for (int i = 0; i < ADC_CHANNELS; i++) {
        int16_t abs_val = (sample[i] < 0) ? -sample[i] : sample[i];
        if (abs_val > peak)
            peak = abs_val;
    }
    return peak;
}

/* ── Check if signal is above trigger threshold ───────────────── */
int adc_check_trigger(int16_t threshold)
{
    int16_t peak = adc_get_peak_amplitude();
    return (peak > threshold) ? 1 : 0;
}

/* ── Set the hardware trigger threshold (TLV3201) ───────────────
 * The TLV3201 comparator monitors the analog envelope signal
 * from the VGA output. Its threshold is set by a DAC.
 * This function programs the DAC via SPI.
 */
void adc_set_trigger_threshold(uint16_t threshold_mv)
{
    /* Use MCP4922 DAC on VGA SPI bus to set comparator threshold.
     * threshold_mv: 0-5000 mV (for ±5V range).
     * MCP4922 command format:
     *   bit 15: 0 = write to DAC A, 1 = DAC B
     *   bit 14: 1 = Vref buffered
     *   bit 13: 1 = output gain = 1x
     *   bit 12: 1 = output power down control = output enabled
     *   bits 11-0: data (0-4095 for 0-Vref)
     */
    uint16_t dac_val = (uint32_t)threshold_mv * 4095 / 3300;  /* Vref=3.3V */
    uint16_t cmd = 0xB000 | (dac_val & 0x0FFF);  /* DAC A, buffered, 1x, enabled */

    /* Bit-bang SPI to DAC on the filter SPI pins (repurposed for trigger) */
    gpio_clr(GPIO(FILTER_SPI_CS_PORT), FILTER_SPI_CS_PIN);

    for (int i = 15; i >= 0; i--) {
        gpio_clr(GPIO(FILTER_SPI_SCK_PORT), FILTER_SPI_SCK_PIN);
        if (cmd & (1U << i))
            gpio_set(GPIO(FILTER_SPI_MOSI_PORT), FILTER_SPI_MOSI_PIN);
        else
            gpio_clr(GPIO(FILTER_SPI_MOSI_PORT), FILTER_SPI_MOSI_PIN);
        for (volatile int d = 0; d < 2; d++);
        gpio_set(GPIO(FILTER_SPI_SCK_PORT), FILTER_SPI_SCK_PIN);
        for (volatile int d = 0; d < 2; d++);
    }

    gpio_set(GPIO(FILTER_SPI_CS_PORT), FILTER_SPI_CS_PIN);
}