/*
 * spi_dma.h — SPI DMA Helper for STM32F401
 *
 * Provides DMA configuration helpers for SPI1 (CC1101) and SPI2 (nRF24L01+).
 * Uses DMA2 for SPI1 and DMA1 for SPI2 per STM32F401 reference manual.
 */

#ifndef SPI_DMA_H
#define SPI_DMA_H

#include <stdint.h>
#include <stddef.h>

/**
 * spi_dma_init - Enable DMA clocks
 */
void spi_dma_init(void);

/**
 * spi1_dma_rx_start - Start DMA transfer from SPI1 RX (CC1101)
 * @param buf Destination buffer
 * @param len Number of bytes
 * Returns: 0 on success
 */
int spi1_dma_rx_start(uint8_t *buf, uint16_t len);

/**
 * spi1_dma_tx_start - Start DMA transfer to SPI1 TX (CC1101)
 * @param buf Source buffer
 * @param len Number of bytes
 * Returns: 0 on success
 */
int spi1_dma_tx_start(const uint8_t *buf, uint16_t len);

/**
 * spi1_dma_wait_rx - Wait for SPI1 DMA RX transfer complete
 * @param timeout_ms Timeout in ms
 * Returns: 0 on success, -2 on timeout
 */
int spi1_dma_wait_rx(uint32_t timeout_ms);

/**
 * spi1_dma_wait_tx - Wait for SPI1 DMA TX transfer complete
 * @param timeout_ms Timeout in ms
 * Returns: 0 on success, -2 on timeout
 */
int spi1_dma_wait_tx(uint32_t timeout_ms);

/**
 * spi2_dma_rx_start - Start DMA transfer from SPI2 RX (nRF24L01+)
 * @param buf Destination buffer
 * @param len Number of bytes
 * Returns: 0 on success
 */
int spi2_dma_rx_start(uint8_t *buf, uint16_t len);

/**
 * spi2_dma_tx_start - Start DMA transfer to SPI2 TX (nRF24L01+)
 * @param buf Source buffer
 * @param len Number of bytes
 * Returns: 0 on success
 */
int spi2_dma_tx_start(const uint8_t *buf, uint16_t len);

#endif /* SPI_DMA_H */