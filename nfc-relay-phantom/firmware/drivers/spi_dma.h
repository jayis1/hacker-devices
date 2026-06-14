/*
 * SPI DMA Driver for NFC Relay Phantom
 * Handles SPI1 (PN5180) and SPI2 (W25Q128) with DMA support
 *
 * Copyright (c) 2026 Hacker Devices. Licensed under GPL-2.0.
 */

#ifndef SPI_DMA_H
#define SPI_DMA_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define SPI1_NS_LOW()       (GPIOA->ODR &= ~(1 << 4))
#define SPI1_NS_HIGH()      (GPIOA->ODR |= (1 << 4))
#define SPI2_NS_LOW()       (GPIOB->ODR &= ~(1 << 12))
#define SPI2_NS_HIGH()      (GPIOB->ODR |= (1 << 12))

typedef enum {
    SPI_BUS_1 = 0,  /* PN5180 */
    SPI_BUS_2 = 1,  /* W25Q128 */
} spi_bus_t;

void spi_init(void);
void spi_transfer_blocking(spi_bus_t bus, const uint8_t *tx, uint8_t *rx, size_t len);
void spi_transfer_dma(spi_bus_t bus, const uint8_t *tx, uint8_t *rx, size_t len);
bool spi_dma_busy(spi_bus_t bus);
void spi_dma_wait(spi_bus_t bus);

#endif /* SPI_DMA_H */