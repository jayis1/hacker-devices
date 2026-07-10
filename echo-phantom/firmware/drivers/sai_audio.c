/*
 * sai_audio.c — SAI Audio Interface Driver for ECHO-Phantom
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Configures the STM32H743 SAI1 peripheral for I²S/TDM audio:
 *   - Block A (SAI1_A): RX from microphone/codec (slave mode)
 *   - Block B (SAI1_B): TX to host AP/DSP (master mode for clocks)
 *
 * Uses DMA double-buffering for zero-copy audio frame transfer.
 * Block A receives audio from the mic into alternating SRAM buffers,
 * while Block B transmits the processed audio to the host AP.
 */

#include "board.h"
#include "registers.h"

/* External DMA buffer declarations from main.c */
extern int32_t g_rx_buf0[AUDIO_FRAME_SAMPLES];
extern int32_t g_rx_buf1[AUDIO_FRAME_SAMPLES];
extern int32_t g_tx_buf0[AUDIO_FRAME_SAMPLES];
extern int32_t g_tx_buf1[AUDIO_FRAME_SAMPLES];
extern volatile uint8_t g_rx_ready;

/* ========================================================================
 *  SAI data size encoding
 * ======================================================================== */

static uint32_t sai_data_size(uint8_t bit_depth)
{
    switch (bit_depth) {
    case 8:  return SAI_DS_8BIT;
    case 16: return SAI_DS_16BIT;
    case 24: return SAI_DS_24BIT;
    case 32: return SAI_DS_32BIT;
    default: return SAI_DS_16BIT;
    }
}

/* ========================================================================
 *  Compute SAI MCLK divider
 *  SAI clock = PLL3_P / 2 (typically ~25 MHz)
 *  MCLK_out = SAI_clock / (MCKDIV * 2)
 *  We want MCLK_out = sample_rate * 256
 * ======================================================================== */

static uint32_t compute_mckdiv(uint32_t sample_rate)
{
    /* SAI1 clock from PLL3_P = 50 MHz (our config) */
    uint32_t sai_clk = 50000000U;
    uint32_t mclk_target = sample_rate * 256U;
    uint32_t div = sai_clk / (mclk_target * 2U);
    if (div > 63) div = 63;
    if (div < 1) div = 1;
    return div;
}

/* ========================================================================
 *  Compute SAI BCLK frame divider
 *  BCLK = MCLK / (FRL+1) * 2
 *  For standard I²S stereo: BCLK = sample_rate * bit_depth * 2
 * ======================================================================== */

static uint32_t compute_bclk_div(uint32_t mclk, uint32_t sample_rate, uint8_t bit_depth, uint8_t channels)
{
    uint32_t bclk_target = sample_rate * bit_depth * channels;
    uint32_t div = mclk / bclk_target;
    if (div > 255) div = 255;
    if (div < 1) div = 1;
    return div;
}

/* ========================================================================
 *  Initialize SAI1 Block A (RX from microphone) and Block B (TX to host)
 * ======================================================================== */

int sai_audio_init(const audio_format_t *fmt)
{
    /* Disable both blocks */
    SAI1_CR1A &= ~SAI_CR1_EN;
    SAI1_CR1B &= ~SAI_CR1_EN;

    /* Configure global SAI1 — both blocks synchronous */
    SAI1_GCR = 0x00000000U;  /* No synchronization — each block independent */

    /* --- Configure Block A (RX from mic, slave mode) --- */
    SAI1_CR1A = 0;
    SAI1_CR2A = 0;

    /* Mode: slave RX */
    SAI1_CR1A |= SAI_MODE_SLAVE_RX;
    /* Data size */
    SAI1_CR1A |= sai_data_size(fmt->bit_depth);
    /* Clock strobing: rising edge (standard I²S) */
    if (fmt->protocol == PROTO_I2S_PHILIPS || fmt->protocol == PROTO_LJ) {
        SAI1_CR1A |= SAI_CR1_CKSTR;  /* Clock strobing on rising edge */
    }
    /* No master clock output on Block A (we receive it from mic) */
    SAI1_CR1A &= ~SAI_CR1_MCKEN;
    SAI1_CR1A |= SAI_CR1_NODIV;  /* No MCLK divider — we're slave */

    /* Frame configuration for Block A */
    uint32_t frame_length = fmt->bit_depth * fmt->channels;
    uint32_t frame_sync_active = fmt->bit_depth / 2;  /* WS active for half frame */
    SAI1_FRCRA = 0;
    SAI1_FRCRA |= (frame_length - 1) << SAI_FRCR_FRL_Pos;  /* Frame length */
    SAI1_FRCRA |= (frame_sync_active - 1) << SAI_FRCR_FSALL_Pos;  /* FS active length */
    if (fmt->protocol == PROTO_I2S_PHILIPS) {
        SAI1_FRCRA |= SAI_FRCR_FSDEF;  /* FS toggling mode */
        SAI1_FRCRA |= SAI_FRCR_FSOFF;  /* FS offset by 1 BCLK */
    }
    SAI1_FRCRA |= SAI_FRCR_FSPOL;  /* FS active high */

    /* Slot configuration for Block A */
    SAI1_SLOTA = 0;
    if (fmt->channels > 2) {
        /* TDM mode */
        SAI1_SLOTA |= ((fmt->channels - 1) << SAI_SLOTR_NBSLOT_Pos);
        SAI1_SLOTA |= 0xFFFFU << SAI_SLOTR_SLOTEN_Pos;  /* Enable all slots */
    } else {
        /* Stereo mode — enable slots 0 and 1 */
        SAI1_SLOTA |= 1U << SAI_SLOTR_NBSLOT_Pos;  /* 2 slots (value = N-1) */
        SAI1_SLOTA |= 0x0003U << SAI_SLOTR_SLOTEN_Pos;  /* Enable slots 0,1 */
    }
    SAI1_SLOTA |= (fmt->bit_depth / 2 - 1) << SAI_SLOTR_FBOFF_Pos;  /* First bit offset */

    /* --- Configure Block B (TX to host, master mode) --- */
    SAI1_CR1B = 0;
    SAI1_CR2B = 0;

    /* Mode: master TX */
    SAI1_CR1B |= SAI_MODE_MASTER_TX;
    /* Data size */
    SAI1_CR1B |= sai_data_size(fmt->bit_depth);
    /* Clock strobing */
    if (fmt->protocol == PROTO_I2S_PHILIPS || fmt->protocol == PROTO_LJ) {
        SAI1_CR1B |= SAI_CR1_CKSTR;
    }

    /* Master clock and divider for Block B */
    SAI1_CR1B |= SAI_CR1_MCKEN;  /* Enable MCLK output */
    SAI1_CR1B &= ~SAI_CR1_NODIV;  /* Use MCLK divider */
    uint32_t mckdiv = compute_mckdiv(fmt->sample_rate);
    SAI1_CR1B &= ~(0x3FU << SAI_CR1_MCKDIV_Pos);
    SAI1_CR1B |= (mckdiv << SAI_CR1_MCKDIV_Pos);
    SAI1_CR1B |= SAI_CR1_OUTDRIV;  /* Drive clock immediately */

    /* Frame configuration for Block B (same as A) */
    SAI1_FRCRB = 0;
    SAI1_FRCRB |= (frame_length - 1) << SAI_FRCR_FRL_Pos;
    SAI1_FRCRB |= (frame_sync_active - 1) << SAI_FRCR_FSALL_Pos;
    if (fmt->protocol == PROTO_I2S_PHILIPS) {
        SAI1_FRCRB |= SAI_FRCR_FSDEF;
        SAI1_FRCRB |= SAI_FRCR_FSOFF;
    }
    SAI1_FRCRB |= SAI_FRCR_FSPOL;

    /* Slot configuration for Block B (same as A) */
    SAI1_SLOTB = 0;
    if (fmt->channels > 2) {
        SAI1_SLOTB |= ((fmt->channels - 1) << SAI_SLOTR_NBSLOT_Pos);
        SAI1_SLOTB |= 0xFFFFU << SAI_SLOTR_SLOTEN_Pos;
    } else {
        SAI1_SLOTB |= 1U << SAI_SLOTR_NBSLOT_Pos;
        SAI1_SLOTB |= 0x0003U << SAI_SLOTR_SLOTEN_Pos;
    }
    SAI1_SLOTB |= (fmt->bit_depth / 2 - 1) << SAI_SLOTR_FBOFF_Pos;

    /* Enable FIFO threshold and companding mode */
    SAI1_CR2A |= 0x3U << 14;  /* FIFO threshold = 1/4 full */
    SAI1_CR2B |= 0x3U << 14;

    /* Flush FIFOs */
    SAI1_CR2A |= SAI_CR2_FFLUSH;
    SAI1_CR2B |= SAI_CR2_FFLUSH;

    /* Clear any pending flags */
    SAI1_CLRFR = 0x07;
    SAI1_CLRFB = 0x07;

    /* Enable SAI interrupts (frequency error, mute, overrun) */
    SAI1_IMA = 0x07;
    SAI1_IMB = 0x07;

    /* Enable NVIC for SAI1 */
    NVIC_ISER(2) |= BIT(IRQ_SAI1 - 64);

    return 0;
}

/* ========================================================================
 *  Start SAI audio DMA streaming
 * ======================================================================== */

void sai_audio_start(void)
{
    /* --- Configure DMA1 Stream 0 for SAI1_A RX --- */
    DMA_SCR(0) = 0;  /* Disable stream */
    while (DMA_SCR(0) & DMA_SCR_EN)  /* Wait for EN to clear */
        ;

    DMA_SCR(0) |= DMA_SCR_CHSEL_SAI1_A;  /* Channel select = SAI1_A */
    DMA_SCR(0) |= (DMA_DIR_P2M << DMA_SCR_DIR_Pos);  /* Peripheral to memory */
    DMA_SCR(0) |= (0x2U << DMA_SCR_PSIZE_Pos);   /* Peripheral: 32-bit */
    DMA_SCR(0) |= (0x2U << DMA_SCR_MSIZE_Pos);   /* Memory: 32-bit */
    DMA_SCR(0) |= DMA_SCR_MINC;  /* Memory increment */
    DMA_SCR(0) |= DMA_SCR_CIRC;  /* Circular mode (double buffer) */
    DMA_SCR(0) |= DMA_SCR_HTIE | DMA_SCR_TCIE | DMA_SCR_TEIE;  /* Interrupts */

    DMA_SPAR(0) = (uint32_t)&SAI1_DR;  /* Peripheral address = SAI1_A data register */
    DMA_SM0AR(0) = (uint32_t)g_rx_buf0;  /* Memory 0 address */
    DMA_SM1AR(0) = (uint32_t)g_rx_buf1;  /* Memory 1 address */
    DMA_SNDTR(0) = AUDIO_FRAME_SAMPLES;  /* Number of data items */

    /* DMA FIFO configuration */
    DMA_SFCR(0) = (0x3U << 0) | BIT(2);  /* FIFO full, direct mode disabled, FIFO threshold 1/2 full */

    /* Clear DMA flags */
    DMA_LIFCR = 0x3DU;

    /* --- Configure DMA1 Stream 1 for SAI1_B TX --- */
    DMA_SCR(1) = 0;
    while (DMA_SCR(1) & DMA_SCR_EN)
        ;

    DMA_SCR(1) |= DMA_SCR_CHSEL_SAI1_B;
    DMA_SCR(1) |= (DMA_DIR_M2P << DMA_SCR_DIR_Pos);  /* Memory to peripheral */
    DMA_SCR(1) |= (0x2U << DMA_SCR_PSIZE_Pos);
    DMA_SCR(1) |= (0x2U << DMA_SCR_MSIZE_Pos);
    DMA_SCR(1) |= DMA_SCR_MINC;
    DMA_SCR(1) |= DMA_SCR_CIRC;
    DMA_SCR(1) |= DMA_SCR_TCIE | DMA_SCR_TEIE;

    DMA_SPAR(1) = (uint32_t)&SAI1_DB;
    DMA_SM0AR(1) = (uint32_t)g_tx_buf0;
    DMA_SM1AR(1) = (uint32_t)g_tx_buf1;
    DMA_SNDTR(1) = AUDIO_FRAME_SAMPLES;

    DMA_SFCR(1) = (0x3U << 0) | BIT(2);

    DMA_LIFCR = 0x3D0U;

    /* --- Enable SAI1 Block B (TX) first, then Block A (RX) --- */
    SAI1_CR1B |= SAI_CR1_EN;
    while (!(SAI1_SRB & SAI_SR_FREQ))  /* Wait for BCLK to start */
        ;

    SAI1_CR1A |= SAI_CR1_EN;
    while (!(SAI1_SR & SAI_SR_FREQ))
        ;

    /* --- Enable DMA streams --- */
    DMA_SCR(1) |= DMA_SCR_EN;  /* Enable TX DMA first */
    DMA_SCR(0) |= DMA_SCR_EN;  /* Enable RX DMA */
}

/* ========================================================================
 *  Stop SAI audio
 * ======================================================================== */

void sai_audio_stop(void)
{
    /* Disable DMA streams */
    DMA_SCR(0) &= ~DMA_SCR_EN;
    DMA_SCR(1) &= ~DMA_SCR_EN;

    while ((DMA_SCR(0) & DMA_SCR_EN) || (DMA_SCR(1) & DMA_SCR_EN))
        ;

    /* Disable SAI blocks */
    SAI1_CR1A &= ~SAI_CR1_EN;
    SAI1_CR1B &= ~SAI_CR1_EN;

    /* Flush FIFOs */
    SAI1_CR2A |= SAI_CR2_FFLUSH;
    SAI1_CR2B |= SAI_CR2_FFLUSH;
}

/* ========================================================================
 *  Blocking single-frame RX/TX (for testing without DMA)
 * ======================================================================== */

int sai_audio_rx(uint32_t *buf, uint16_t samples)
{
    for (uint16_t i = 0; i < samples; i++) {
        while (!(SAI1_SR & SAI_SR_FREQ))
            ;
        buf[i] = SAI1_DR;
    }
    return 0;
}

int sai_audio_tx(const uint32_t *buf, uint16_t samples)
{
    for (uint16_t i = 0; i < samples; i++) {
        while (!(SAI1_SRB & BIT(1)))  /* Wait for TX FIFO not full */
            ;
        SAI1_DB = buf[i];
    }
    return 0;
}