/*
 * spi4.c — SPI4 peripheral driver for STM32F423
 * USB DMA Phantom firmware
 *
 * Copyright (c) 2026 Hacker Devices
 * SPDX-License-Identifier: MIT
 */

#include "spi4.h"
#include "registers.h"
#include "board.h"

void spi4_init(void) {
    /* Enable SPI4 clock */
    RCC->APB1ENR |= RCC_APB1ENR_SPI4EN;

    /* Ensure SPI is disabled before configuration */
    SPI4->CR1 &= ~SPI_CR1_SPE;

    /* Configure SPI4: Master, Mode 0, 8-bit, MSB first
     * Clock: APB1/16 = 60MHz/16 ≈ 3.75 MHz (safe for both flash and XIO2001)
     * Increase later after verification */
    SPI4->CR1 = SPI_CR1_MSTR      /* Master mode */
              | (3UL << SPI_CR1_BR_SHIFT) /* BR: fPCLK/16 */
              | SPI_CR1_SSM           /* Software slave management */
              | SPI_CR1_SSI;          /* Internal slave select */

    /* 8-bit data size, no DMA initially, NSS pulse not needed */
    SPI4->CR2 = 0;

    /* Enable SPI */
    SPI4->CR1 |= SPI_CR1_SPE;

    /* Initialize CS pins (both high = inactive) */
    GPIOC->ODR |= (1UL << GPIOC_FLASH_CS);   /* PC5 high */
    GPIOC->ODR |= (1UL << GPIOC_XIO_SPI_CS);  /* PC4 high */
}

void spi4_select(uint16_t cs) {
    if (cs == SPI4_CS_FLASH) {
        GPIOC->ODR &= ~(1UL << GPIOC_FLASH_CS);   /* Assert FLASH_CS# low */
    } else if (cs == SPI4_CS_XIO) {
        GPIOC->ODR &= ~(1UL << GPIOC_XIO_SPI_CS);  /* Assert XIO_CS# low */
    }
}

void spi4_deselect(uint16_t cs) {
    if (cs == SPI4_CS_FLASH) {
        GPIOC->ODR |= (1UL << GPIOC_FLASH_CS);   /* Deassert FLASH_CS# high */
    } else if (cs == SPI4_CS_XIO) {
        GPIOC->ODR |= (1UL << GPIOC_XIO_SPI_CS);  /* Deassert XIO_CS# high */
    }
}

uint8_t spi4_transfer(uint8_t tx_byte) {
    /* Wait for TX empty flag */
    while (!(SPI4->SR & SPI_SR_TXE));

    /* Write data register */
    SPI4->DR = tx_byte;

    /* Wait for RX not empty flag */
    while (!(SPI4->SR & SPI_SR_RXNE));

    /* Read and return data register */
    return (uint8_t)(SPI4->DR & 0xFF);
}

void spi4_dma_transfer(const uint8_t *tx_buf, uint8_t *rx_buf,
                       uint16_t len, uint16_t cs) {
    /* Enable DMA2 clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;

    /* Configure DMA2 Stream 0 (RX) and Stream 4 (TX) for SPI4 */

    /* Disable DMA streams first */
    DMA2_Stream_TypeDef *stream_rx = (DMA2_Stream_TypeDef *)(0x40026400UL + 0x1C * 0);
    DMA2_Stream_TypeDef *stream_tx = (DMA2_Stream_TypeDef *)(0x40026400UL + 0x1C * 4);

    stream_rx->CR &= ~DMA_SxCR_EN;
    stream_tx->CR &= ~DMA_SxCR_EN;
    while (stream_rx->CR & DMA_SxCR_EN);
    while (stream_tx->CR & DMA_SxCR_EN);

    /* Select target chip */
    spi4_select(cs);

    /* Configure SPI4 for DMA mode */
    SPI4->CR2 = SPI_CR2_RXNEIE | SPI_CR2_DMAEN | SPI_CR2_TXDMAEN;

    /* Configure RX DMA (DMA2 Stream 0, Channel 4 = SPI4_RX) */
    stream_rx->PAR = (uint32_t)&(SPI4->DR);
    stream_rx->M0AR = (uint32_t)rx_buf;
    stream_rx->NDTR = len;
    stream_rx->CR = (4UL << DMA_SxCR_CHSEL_SHIFT)  /* Channel 4 */
                  | (0UL << 6)                        /* Peripheral-to-memory */
                  | (0UL << 8)                        /* 8-bit peripheral */
                  | (1UL << 10)                       /* 16-bit memory */
                  | (1UL << DMA_SxCR_MINC)            /* Memory increment */
                  | DMA_SxCR_TCIE;                    /* Transfer complete IRQ */

    /* Configure TX DMA (DMA2 Stream 4, Channel 5 = SPI4_TX) */
    stream_tx->PAR = (uint32_t)&(SPI4->DR);
    stream_tx->M0AR = (uint32_t)tx_buf;
    stream_tx->NDTR = len;
    stream_tx->CR = (5UL << DMA_SxCR_CHSEL_SHIFT)  /* Channel 5 */
                  | (1UL << DMA_SxCR_DIR_SHIFT)       /* Memory-to-peripheral */
                  | (0UL << 8)                        /* 8-bit peripheral */
                  | (1UL << 10)                       /* 16-bit memory */
                  | (1UL << DMA_SxCR_MINC)            /* Memory increment */
                  | DMA_SxCR_EN;                       /* Enable */

    /* Enable RX DMA last (so we start receiving) */
    stream_rx->CR |= DMA_SxCR_EN;

    /* Wait for RX DMA to complete */
    while (!(DMA2->LISR & (1UL << 5)));  /* TCIF0 */

    /* Clear DMA flags */
    DMA2->LIFCR = (1UL << 5);

    /* Deselect chip */
    spi4_deselect(cs);

    /* Disable DMA on SPI */
    SPI4->CR2 = 0;
}

void spi4_dma_wait_complete(void) {
    while (!(DMA2->LISR & (1UL << 5)));
    DMA2->LIFCR = (1UL << 5);
}