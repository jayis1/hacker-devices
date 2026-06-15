/*
 * w25q128.h — W25Q128JVSIQ 16 MB SPI NOR Flash driver
 * Connected via SPI1 on STM32H743
 */

#ifndef W25Q128_H
#define W25Q128_H

#include <stdint.h>

/* Flash specifications */
#define W25Q128_PAGE_SIZE       256U        /* 256 bytes per page */
#define W25Q128_SECTOR_SIZE     4096U       /* 4 KB per sector */
#define W25Q128_BLOCK_SIZE      65536U      /* 64 KB per block */
#define W25Q128_TOTAL_SIZE      16777216U   /* 16 MB */
#define W25Q128_NUM_SECTORS     4096U
#define W25Q128_NUM_PAGES       65536U

/* JEDEC ID for W25Q128JVSIQ */
#define W25Q128_JEDEC_EXPECTED 0xEF4018U    /* Manufacturer: Winbond (0xEF), Type: 0x40, Capacity: 0x18 (128Mbit) */

/* Function prototypes */
void w25q128_init(void);
uint32_t w25q128_read_jedec_id(void);
void w25q128_read(uint32_t addr, uint8_t *buf, uint16_t len);
void w25q128_write_page(uint32_t addr, const uint8_t *buf, uint16_t len);
void w25q128_erase_sector(uint32_t addr);
void w25q128_erase_block_64k(uint32_t addr);
void w25q128_erase_chip(void);
void w25q128_write_enable(void);
void w25q128_write_disable(void);
uint8_t w25q128_read_status(uint8_t reg);
void w25q128_wait_busy(void);

/* SPI1 CS control macros */
#define W25Q128_CS_LOW()    GPIOA->ODR &= ~(1U << SPI1_NSS_PIN)
#define W25Q128_CS_HIGH()   GPIOA->ODR |= (1U << SPI1_NSS_PIN)

#endif /* W25Q128_H */