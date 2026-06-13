/*
 * spi_dma.c — SPI DMA Helper Implementation for STM32F401
 *
 * Configures DMA1/DMA2 streams for SPI1 and SPI2 transfers.
 * Uses polling for completion (no DMA interrupts in this version).
 */

#include "spi_dma.h"
#include "board.h"
#include "registers.h"

extern volatile uint32_t systick_ms;

void spi_dma_init(void)
{
    /* Enable DMA1 and DMA2 clocks */
    RCC_AHB1ENR |= (RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMA2EN);
}

static void dma_clear_flags(uint32_t base, uint8_t stream)
{
    /* Clear all interrupt flags for the stream */
    if (stream < 4) {
        DMAx_LIFCR(base) = (0x3DU << (stream * 6));
    } else {
        DMAx_HIFCR(base) = (0x3DU << ((stream - 4) * 6));
    }
}

int spi1_dma_rx_start(uint8_t *buf, uint16_t len)
{
    /* DMA2 Stream 0, Channel 3: SPI1_RX */
    uint8_t stream = 0;
    
    /* Disable stream */
    DMAx_SxCR(DMA2_BASE, stream) &= ~DMA_SxCR_EN;
    while (DMAx_SxCR(DMA2_BASE, stream) & DMA_SxCR_EN)
        ;
    
    /* Clear flags */
    dma_clear_flags(DMA2_BASE, stream);
    
    /* Configure stream */
    DMAx_SxPAR(DMA2_BASE, stream) = (uint32_t)&SPIx_DR(SPI1_BASE);
    DMAx_SxM0AR(DMA2_BASE, stream) = (uint32_t)buf;
    DMAx_SxNDTR(DMA2_BASE, stream) = len;
    
    DMAx_SxCR(DMA2_BASE, stream) = DMA_DIR_PERIPH_TO_MEM
                                  | DMA_SxCR_MINC
                                  | DMA_MSIZE_BYTE
                                  | DMA_PSIZE_BYTE
                                  | (3U << DMA_SxCR_CHSEL_SHIFT);  /* Channel 3 */
    
    /* Enable SPI1 RX DMA */
    SPIx_CR2(SPI1_BASE) |= SPI_CR2_RXDMAEN;
    
    /* Enable DMA stream */
    DMAx_SxCR(DMA2_BASE, stream) |= DMA_SxCR_EN;
    
    return 0;
}

int spi1_dma_tx_start(const uint8_t *buf, uint16_t len)
{
    /* DMA2 Stream 3, Channel 3: SPI1_TX */
    uint8_t stream = 3;
    
    DMAx_SxCR(DMA2_BASE, stream) &= ~DMA_SxCR_EN;
    while (DMAx_SxCR(DMA2_BASE, stream) & DMA_SxCR_EN)
        ;
    
    dma_clear_flags(DMA2_BASE, stream);
    
    DMAx_SxPAR(DMA2_BASE, stream) = (uint32_t)&SPIx_DR(SPI1_BASE);
    DMAx_SxM0AR(DMA2_BASE, stream) = (uint32_t)buf;
    DMAx_SxNDTR(DMA2_BASE, stream) = len;
    
    DMAx_SxCR(DMA2_BASE, stream) = DMA_DIR_MEM_TO_PERIPH
                                  | DMA_SxCR_MINC
                                  | DMA_MSIZE_BYTE
                                  | DMA_PSIZE_BYTE
                                  | (3U << DMA_SxCR_CHSEL_SHIFT);
    
    SPIx_CR2(SPI1_BASE) |= SPI_CR2_TXDMAEN;
    
    DMAx_SxCR(DMA2_BASE, stream) |= DMA_SxCR_EN;
    
    return 0;
}

int spi1_dma_wait_rx(uint32_t timeout_ms)
{
    uint32_t deadline = systick_ms + timeout_ms;
    while (systick_ms < deadline) {
        if (!(DMAx_SxCR(DMA2_BASE, 0) & DMA_SxCR_EN)) {
            /* Disable DMA in SPI and return */
            SPIx_CR2(SPI1_BASE) &= ~SPI_CR2_RXDMAEN;
            return 0;
        }
    }
    SPIx_CR2(SPI1_BASE) &= ~SPI_CR2_RXDMAEN;
    return -2;
}

int spi1_dma_wait_tx(uint32_t timeout_ms)
{
    uint32_t deadline = systick_ms + timeout_ms;
    while (systick_ms < deadline) {
        if (!(DMAx_SxCR(DMA2_BASE, 3) & DMA_SxCR_EN)) {
            SPIx_CR2(SPI1_BASE) &= ~SPI_CR2_TXDMAEN;
            return 0;
        }
    }
    SPIx_CR2(SPI1_BASE) &= ~SPI_CR2_TXDMAEN;
    return -2;
}

int spi2_dma_rx_start(uint8_t *buf, uint16_t len)
{
    /* DMA1 Stream 3, Channel 0: SPI2_RX */
    uint8_t stream = 3;
    
    DMAx_SxCR(DMA1_BASE, stream) &= ~DMA_SxCR_EN;
    while (DMAx_SxCR(DMA1_BASE, stream) & DMA_SxCR_EN)
        ;
    
    dma_clear_flags(DMA1_BASE, stream);
    
    DMAx_SxPAR(DMA1_BASE, stream) = (uint32_t)&SPIx_DR(SPI2_BASE);
    DMAx_SxM0AR(DMA1_BASE, stream) = (uint32_t)buf;
    DMAx_SxNDTR(DMA1_BASE, stream) = len;
    
    DMAx_SxCR(DMA1_BASE, stream) = DMA_DIR_PERIPH_TO_MEM
                                  | DMA_SxCR_MINC
                                  | DMA_MSIZE_BYTE
                                  | DMA_PSIZE_BYTE
                                  | (0U << DMA_SxCR_CHSEL_SHIFT);  /* Channel 0 */
    
    SPIx_CR2(SPI2_BASE) |= SPI_CR2_RXDMAEN;
    DMAx_SxCR(DMA1_BASE, stream) |= DMA_SxCR_EN;
    
    return 0;
}

int spi2_dma_tx_start(const uint8_t *buf, uint16_t len)
{
    /* DMA1 Stream 5, Channel 0: SPI2_TX */
    uint8_t stream = 5;
    
    DMAx_SxCR(DMA1_BASE, stream) &= ~DMA_SxCR_EN;
    while (DMAx_SxCR(DMA1_BASE, stream) & DMA_SxCR_EN)
        ;
    
    dma_clear_flags(DMA1_BASE, stream);
    
    DMAx_SxPAR(DMA1_BASE, stream) = (uint32_t)&SPIx_DR(SPI2_BASE);
    DMAx_SxM0AR(DMA1_BASE, stream) = (uint32_t)buf;
    DMAx_SxNDTR(DMA1_BASE, stream) = len;
    
    DMAx_SxCR(DMA1_BASE, stream) = DMA_DIR_MEM_TO_PERIPH
                                  | DMA_SxCR_MINC
                                  | DMA_MSIZE_BYTE
                                  | DMA_PSIZE_BYTE
                                  | (0U << DMA_SxCR_CHSEL_SHIFT);
    
    SPIx_CR2(SPI2_BASE) |= SPI_CR2_TXDMAEN;
    DMAx_SxCR(DMA1_BASE, stream) |= DMA_SxCR_EN;
    
    return 0;
}