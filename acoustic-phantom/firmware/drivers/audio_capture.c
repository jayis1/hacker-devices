/*
 * ACOUSTIC-PHANTOM — Audio capture driver implementation
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * Manages I²S DMA capture from the 4-mic MEMS array and the WM8904
 * codec. Uses double-buffered DMA with half/full transfer interrupts
 * to deliver 25 ms audio frames to the processing pipeline.
 */

#include <string.h>
#include "audio_capture.h"
#include "registers.h"

/* ---- DMA buffers -------------------------------------------------------
 * Two ping-pong buffers, each holding FRAME_SAMPLES (1200) samples per
 * channel. The DMA fills one while the DSP consumes the other.
 * ------------------------------------------------------------------------ */
#define BUF_FRAMES  2

static int16_t s_dma_buf[BUF_FRAMES][NUM_MICS + 2][FRAME_SAMPLES];
static volatile uint8_t  s_dma_half_ready = 0;
static volatile uint8_t  s_dma_full_ready = 0;
static volatile uint16_t s_frame_seq = 0;
static audio_frame_t s_ready_frame;
static volatile uint8_t s_frame_ready = 0;
static uint8_t s_capturing = 0;

/* ---- I²S2 init (MEMS mic array) ---------------------------------------- */
static void i2s2_init(void)
{
    /* Enable SPI2 clock already done in clock_init */
    /* Configure I2S2 in master Rx mode, 16-bit data, 48 kHz */
    SPI2->I2SCFGR = I2SCFGR_I2SMOD | I2SCFGR_I2SCFG_RX;
    /* I2S clock = 48000 * 256 = 12.288 MHz, derived from PLL1 Q output */
    SPI2->I2SPR = 2;  /* odd=0, MCKOE=1, divider=2 → ~12.288 MHz */
    SPI2->CR1 |= SPI_CR1_SPE;
}

/* ---- I²S3 init (WM8904 codec) ------------------------------------------ */
static void i2s3_init(void)
{
    SPI3->I2SCFGR = I2SCFGR_I2SMOD | I2SCFGR_I2SCFG_RX;
    SPI3->I2SPR = 2;
    SPI3->CR1 |= SPI_CR1_SPE;
}

/* ---- DMA configuration for I²S2 RX ------------------------------------- */
static void dma_i2s2_rx_init(void)
{
    /* DMA1 Stream0, Channel 0 (SPI2_RX), circular, 16-bit, double-buffer */
    volatile uint32_t *dma_cr   = DMA1_Stream0;
    volatile uint32_t *dma_ndtr = (volatile uint32_t *)(DMA1_BASE + 0x010 + 0x04);
    volatile uint32_t *dma_par  = (volatile uint32_t *)(DMA1_BASE + 0x010 + 0x08);
    volatile uint32_t *dma_m0ar = (volatile uint32_t *)(DMA1_BASE + 0x010 + 0x0C);
    volatile uint32_t *dma_m1ar = (volatile uint32_t *)(DMA1_BASE + 0x010 + 0x10);
    volatile uint32_t *dma_fcr  = (volatile uint32_t *)(DMA1_BASE + 0x010 + 0x14);

    *dma_cr = 0;  /* disable before config */
    *dma_m0ar = (uint32_t)s_dma_buf[0];
    *dma_m1ar = (uint32_t)s_dma_buf[1];
    *dma_par  = (uint32_t)&SPI2->DR;
    *dma_ndtr = FRAME_SAMPLES * (NUM_MICS + 2);
    *dma_fcr  = 0x00000005U;  /* direct mode, FIFO error interrupt off */

    /* Circular, peripheral-to-memory, 16-bit/16-bit, mem increment,
       half + full transfer interrupts */
    *dma_cr = DMA_CR_MINC | DMA_CR_CIRC | DMA_CR_TCIE | DMA_CR_HTIE |
              DMA_CR_PSIZE_16 | DMA_CR_MSIZE_16 | DMA_CR_DBM;

    /* Enable DMA interrupt in NVIC */
    NVIC_ISER0 |= (1U << DMA1_Stream0_IRQn);
}

/* ---- DMA ISR: half and full transfer ----------------------------------- */
void audio_capture_dma_half_cb(void)
{
    s_dma_half_ready = 1;
}

void audio_capture_dma_full_cb(void)
{
    s_dma_full_ready = 1;
}

/* DMA1 Stream0 interrupt handler */
void DMA1_Stream0_IRQHandler(void)
{
    /* Check half-transfer flag (HIFCR bit 4 in DMA HISR/LISR) */
    volatile uint32_t *dma_lisr = (volatile uint32_t *)(DMA1_BASE + 0x000);

    /* Simplified: check transfer complete and half transfer */
    if (*dma_lisr & (1U << 4)) {  /* HTIF0 */
        /* Clear flag via LIFCR */
        volatile uint32_t *dma_lifcr = (volatile uint32_t *)(DMA1_BASE + 0x008);
        *dma_lifcr = (1U << 4);
        audio_capture_dma_half_cb();
    }
    if (*dma_lisr & (1U << 5)) {  /* TCIF0 */
        volatile uint32_t *dma_lifcr = (volatile uint32_t *)(DMA1_BASE + 0x008);
        *dma_lifcr = (1U << 5);
        audio_capture_dma_full_cb();
    }
}

/* ---- Assemble a frame from the DMA buffer ------------------------------ */
static void assemble_frame(int buf_idx)
{
    audio_frame_t *f = &s_ready_frame;

    /* De-interleave: DMA buffer is [mic0, mic1, mic2, mic3, piezo, ambient]
       per sample, FRAME_SAMPLES samples deep. */
    for (int i = 0; i < FRAME_SAMPLES; i++) {
        f->mic[0][i]    = s_dma_buf[buf_idx][0][i];
        f->mic[1][i]    = s_dma_buf[buf_idx][1][i];
        f->mic[2][i]    = s_dma_buf[buf_idx][2][i];
        f->mic[3][i]    = s_dma_buf[buf_idx][3][i];
        f->piezo[i]     = s_dma_buf[buf_idx][4][i];
        f->ambient[i]   = s_dma_buf[buf_idx][5][i];
    }

    f->seq = s_frame_seq++;
    f->timestamp_ms = board_millis();
    s_frame_ready = 1;
}

/* ---- Public API -------------------------------------------------------- */
void audio_capture_init(void)
{
    i2s2_init();
    i2s3_init();
    dma_i2s2_rx_init();
    memset(s_dma_buf, 0, sizeof(s_dma_buf));
    s_capturing = 0;
}

void audio_capture_start(void)
{
    /* Enable DMA stream */
    volatile uint32_t *dma_cr = DMA1_Stream0;
    *dma_cr |= DMA_CR_EN;

    /* Enable I²S2 RX */
    SPI2->I2SCFGR |= I2SCFGR_I2SCFG_RX;

    s_capturing = 1;
    s_frame_ready = 0;
}

void audio_capture_stop(void)
{
    /* Disable DMA */
    volatile uint32_t *dma_cr = DMA1_Stream0;
    *dma_cr &= ~DMA_CR_EN;

    s_capturing = 0;
}

uint8_t audio_capture_get_frame(audio_frame_t *frame)
{
    if (!s_capturing) return 0;

    /* Check for new data from DMA */
    if (s_dma_half_ready) {
        s_dma_half_ready = 0;
        assemble_frame(0);
    } else if (s_dma_full_ready) {
        s_dma_full_ready = 0;
        assemble_frame(1);
    }

    if (s_frame_ready) {
        s_frame_ready = 0;
        memcpy(frame, &s_ready_frame, sizeof(audio_frame_t));
        return 1;
    }
    return 0;
}

void audio_capture_wipe(void)
{
    memset(s_dma_buf, 0, sizeof(s_dma_buf));
    memset(&s_ready_frame, 0, sizeof(s_ready_frame));
    s_frame_ready = 0;
    s_dma_half_ready = 0;
    s_dma_full_ready = 0;
    s_frame_seq = 0;
}