/*
 * flash_ops.c — Flash Memory Operations for ECHO-Phantom
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Erase and write operations for the STM32H743 Bank 2 flash,
 * used to store inject audio clips.
 *
 * STM32H743 Flash layout:
 *   Bank 1: 1 MB (sectors 0-7, 128 KB each)
 *   Bank 2: 1 MB (sectors 0-7, 128 KB each)
 *
 * Inject clips are stored in Bank 2, using 128 KB sectors.
 * Each clip occupies up to 64 KB (half a sector).
 */

#include "board.h"
#include "registers.h"
#include <string.h>

/* Flash key values for unlocking */
#define FLASH_KEY1  0x45670123U
#define FLASH_KEY2  0xCDEF89ABU

/* ========================================================================
 *  Unlock flash for writing
 * ========================================================================
 */

static void flash_unlock(void)
{
    FLASH_KEYR = FLASH_KEY1;
    FLASH_KEYR = FLASH_KEY2;
}

/* ========================================================================
 *  Lock flash
 * ========================================================================
 */

static void flash_lock(void)
{
    FLASH_CR |= BIT(0);  /* LOCK bit */
}

/* ========================================================================
 *  Wait for flash operation to complete
 * ========================================================================
 */

static int flash_wait_busy(void)
{
    uint32_t timeout = 100000;
    while ((FLASH_SR & BIT(0)) && timeout > 0) {  /* BSY bit */
        timeout--;
    }
    if (timeout == 0)
        return -1;

    /* Check for errors */
    if (FLASH_SR & (BIT(1) | BIT(3) | BIT(4) | BIT(5))) {
        /* Clear error flags */
        FLASH_CCR = FLASH_SR;
        return -1;
    }

    return 0;
}

/* ========================================================================
 *  Erase a flash sector
 *
 *  Bank 2 sector addresses (128 KB each):
 *    Sector 0: 0x08080000
 *    Sector 1: 0x080A0000
 *    Sector 2: 0x080C0000
 *    ...
 * ========================================================================
 */

int flash_erase_sector(uint32_t addr)
{
    int ret;

    flash_unlock();

    /* Wait for any ongoing operation */
    ret = flash_wait_busy();
    if (ret != 0) {
        flash_lock();
        return ret;
    }

    /* Determine sector number from address */
    uint32_t sector;
    if (addr >= 0x08080000U && addr < 0x08100000U) {
        /* Bank 2 */
        sector = (addr - 0x08080000U) / 0x20000U;  /* 128 KB sectors */
        FLASH_CR |= BIT(3);   /* Select Bank 2 */
    } else if (addr >= 0x08000000U && addr < 0x08080000U) {
        /* Bank 1 */
        sector = (addr - 0x08000000U) / 0x20000U;
        FLASH_CR &= ~BIT(3);  /* Select Bank 1 */
    } else {
        flash_lock();
        return -1;  /* Invalid address */
    }

    /* Set sector number */
    FLASH_CR &= ~(0x7U << 8);   /* Clear SNB field */
    FLASH_CR |= (sector << 8);  /* Set sector number */

    /* Set sector erase size to 128 KB (value 3) */
    FLASH_CR &= ~(0x3U << 4);  /* Clear PSIZE */
    FLASH_CR |= (0x3U << 4);   /* 64-bit (word) program size */

    /* Set erase bit */
    FLASH_CR |= BIT(1);  /* SER: Sector Erase */
    FLASH_CR |= BIT(16); /* STRT: Start */

    /* Wait for completion */
    ret = flash_wait_busy();

    /* Clear SER bit */
    FLASH_CR &= ~BIT(1);

    flash_lock();
    return ret;
}

/* ========================================================================
 *  Write data to flash
 *
 *  The STM32H743 flash is 256-bit (32-byte) word programmable.
 *  We write in 32-byte chunks.
 * ========================================================================
 */

int flash_write(uint32_t addr, const uint8_t *data, uint32_t len)
{
    int ret;

    flash_unlock();

    /* Wait for any ongoing operation */
    ret = flash_wait_busy();
    if (ret != 0) {
        flash_lock();
        return ret;
    }

    /* Clear any error flags */
    FLASH_CCR = FLASH_SR;

    /* Set programming mode */
    FLASH_CR |= BIT(2);  /* PG: Programming */

    /* Program in 32-byte (256-bit) chunks */
    uint32_t offset = 0;
    while (offset < len) {
        /* Wait for flash to be ready */
        ret = flash_wait_busy();
        if (ret != 0) {
            FLASH_CR &= ~BIT(2);  /* Clear PG */
            flash_lock();
            return ret;
        }

        /* Write 32 bytes (8 words) at once */
        uint32_t chunk = (len - offset < 32) ? (len - offset) : 32;
        uint32_t words_to_write = (chunk + 3) / 4;

        volatile uint32_t *dst = (volatile uint32_t *)(addr + offset);
        const uint32_t *src = (const uint32_t *)(data + offset);

        for (uint32_t i = 0; i < words_to_write && i < 8; i++) {
            dst[i] = src[i];
        }

        /* Wait for write to complete */
        ret = flash_wait_busy();
        if (ret != 0) {
            FLASH_CR &= ~BIT(2);
            flash_lock();
            return ret;
        }

        offset += chunk;
    }

    /* Clear PG bit */
    FLASH_CR &= ~BIT(2);

    flash_lock();
    return 0;
}

/* ========================================================================
 *  Read data from flash (trivial — flash is memory-mapped)
 * ========================================================================
 */

int flash_read(uint32_t addr, uint8_t *data, uint32_t len)
{
    memcpy(data, (const void *)addr, len);
    return 0;
}