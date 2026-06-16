/**
 * @file psram_driver.h
 * @brief QSPI PSRAM Driver API for APS6404L-3SQR-SN
 *
 * Provides ring buffer management for CAN frame storage in external PSRAM.
 * Also provides direct read/write access and NOR Flash management.
 *
 * PSRAM: APS6404L-3SQR-SN (8 MB, QSPI, 80 MHz max)
 * Flash: W25Q128JVSIQ (16 MB, QSPI, 133 MHz max)
 */

#ifndef PSRAM_DRIVER_H
#define PSRAM_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * MEMORY MAP
 *===========================================================================*/

#define PSRAM_BASE_ADDR             0x00800000UL
#define PSRAM_SIZE                  (8 * 1024 * 1024)   /* 8 MB */
#define PSRAM_CH0_BUFFER_OFFSET     0x00000000UL
#define PSRAM_CH0_BUFFER_SIZE       (512 * 1024)        /* 512 KB */
#define PSRAM_CH1_BUFFER_OFFSET     0x00080000UL
#define PSRAM_CH1_BUFFER_SIZE       (512 * 1024)
#define PSRAM_SCRIPT_OFFSET         0x00100000UL
#define PSRAM_SCRIPT_SIZE           (1 * 1024 * 1024)   /* 1 MB */
#define PSRAM_DBC_OFFSET            0x00200000UL
#define PSRAM_DBC_SIZE              (2 * 1024 * 1024)   /* 2 MB */

#define FLASH_BASE_ADDR             0x01000000UL
#define FLASH_SIZE                  (16 * 1024 * 1024)  /* 16 MB */
#define FLASH_DFU_IMAGE_OFFSET      0x00000000UL
#define FLASH_DFU_IMAGE_SIZE        (1 * 1024 * 1024)
#define FLASH_DBC_STORE_OFFSET      0x00100000UL
#define FLASH_DBC_STORE_SIZE        (4 * 1024 * 1024)
#define FLASH_SCRIPT_STORE_OFFSET   0x00500000UL
#define FLASH_SCRIPT_STORE_SIZE     (2 * 1024 * 1024)

/*===========================================================================
 * RING BUFFER
 *===========================================================================*/

/** Ring buffer state structure */
typedef struct {
    uint32_t write_ptr;         /* Current write position (byte offset from base) */
    uint32_t read_ptr;          /* Current read position (byte offset from base) */
    uint32_t frame_count;       /* Number of frames currently in buffer */
    uint32_t overflow_count;    /* Number of dropped frames due to buffer full */
    uint32_t base_offset;       /* Base offset in PSRAM */
    uint32_t buffer_size;       /* Total buffer size in bytes */
    bool     initialized;
} ring_buffer_t;

/*===========================================================================
 * ERROR CODES
 *===========================================================================*/

#define PSRAM_ERR_OK              0
#define PSRAM_ERR_NOT_INIT        -1
#define PSRAM_ERR_QSPI            -2
#define PSRAM_ERR_TIMEOUT         -3
#define PSRAM_ERR_BUFFER_FULL     -4
#define PSRAM_ERR_BUFFER_EMPTY    -5
#define PSRAM_ERR_INVALID_PARAM   -6
#define PSRAM_ERR_FLASH_BUSY      -7
#define PSRAM_ERR_FLASH_WRITE_PROT -8

/*===========================================================================
 * PSRAM API
 *===========================================================================*/

/**
 * @brief Initialize QSPI peripheral and PSRAM device
 *
 * Configures nRF52840 QSPI for 64 MHz operation, enters quad mode on PSRAM.
 *
 * @return PSRAM_ERR_OK on success
 */
int psram_init(void);

/**
 * @brief Deinitialize QSPI peripheral
 *
 * @return PSRAM_ERR_OK on success
 */
int psram_deinit(void);

/**
 * @brief Initialize a ring buffer in PSRAM
 *
 * @param ring        Pointer to ring buffer state
 * @param base_offset Base offset in PSRAM for this buffer
 * @param size        Buffer size in bytes
 * @return PSRAM_ERR_OK on success
 */
int psram_ring_init(ring_buffer_t *ring, uint32_t base_offset, uint32_t size);

/**
 * @brief Write data to ring buffer
 *
 * Writes len bytes at the current write pointer, then advances it.
 * If buffer is full, returns PSRAM_ERR_BUFFER_FULL.
 *
 * @param ring Ring buffer state
 * @param data Data to write
 * @param len  Number of bytes to write
 * @return PSRAM_ERR_OK on success
 */
int psram_ring_write(ring_buffer_t *ring, const uint8_t *data, uint32_t len);

/**
 * @brief Read data from ring buffer
 *
 * Reads len bytes from the current read pointer, then advances it.
 * If fewer than len bytes available, reads what's available.
 *
 * @param ring        Ring buffer state
 * @param data        Buffer to store read data
 * @param len         Number of bytes to read
 * @param frames_read Output: number of complete frames read
 * @return PSRAM_ERR_OK on success, PSRAM_ERR_BUFFER_EMPTY if no data
 */
int psram_ring_read(ring_buffer_t *ring, uint8_t *data, uint32_t len, uint32_t *frames_read);

/**
 * @brief Peek at ring buffer data without advancing read pointer
 *
 * @param ring Ring buffer state
 * @param data Buffer to store peeked data
 * @param len  Number of bytes to peek
 * @return PSRAM_ERR_OK on success
 */
int psram_ring_peek(ring_buffer_t *ring, uint8_t *data, uint32_t len);

/**
 * @brief Clear ring buffer (reset pointers)
 *
 * @param ring Ring buffer state
 * @return PSRAM_ERR_OK on success
 */
int psram_ring_clear(ring_buffer_t *ring);

/**
 * @brief Get number of bytes available in ring buffer
 *
 * @param ring Ring buffer state
 * @return Number of bytes available to read
 */
uint32_t psram_ring_available(ring_buffer_t *ring);

/**
 * @brief Direct read from PSRAM (bypasses ring buffer)
 *
 * @param addr Byte address in PSRAM (0 to PSRAM_SIZE-1)
 * @param data Buffer to store read data
 * @param len  Number of bytes to read
 * @return PSRAM_ERR_OK on success
 */
int psram_direct_read(uint32_t addr, uint8_t *data, uint32_t len);

/**
 * @brief Direct write to PSRAM (bypasses ring buffer)
 *
 * @param addr Byte address in PSRAM
 * @param data Data to write
 * @param len  Number of bytes to write
 * @return PSRAM_ERR_OK on success
 */
int psram_direct_write(uint32_t addr, const uint8_t *data, uint32_t len);

/*===========================================================================
 * NOR FLASH API
 *===========================================================================*/

/**
 * @brief Initialize NOR Flash device
 *
 * Verifies device ID, configures QSPI for flash access.
 *
 * @return PSRAM_ERR_OK on success
 */
int flash_init(void);

/**
 * @brief Read data from NOR Flash
 *
 * @param addr Byte address in flash
 * @param data Buffer to store read data
 * @param len  Number of bytes to read
 * @return PSRAM_ERR_OK on success
 */
int flash_read(uint32_t addr, uint8_t *data, uint32_t len);

/**
 * @brief Write data to NOR Flash (handles page boundaries)
 *
 * @param addr Byte address in flash
 * @param data Data to write
 * @param len  Number of bytes to write
 * @return PSRAM_ERR_OK on success
 */
int flash_write(uint32_t addr, const uint8_t *data, uint32_t len);

/**
 * @brief Erase a 4KB sector in NOR Flash
 *
 * @param addr Byte address within sector to erase
 * @return PSRAM_ERR_OK on success
 */
int flash_erase_sector(uint32_t addr);

/**
 * @brief Erase entire NOR Flash chip
 *
 * @return PSRAM_ERR_OK on success
 */
int flash_erase_chip(void);

/**
 * @brief Get NOR Flash manufacturer/device ID
 *
 * @param id_out Buffer to store 3-byte ID (manufacturer, memory type, capacity)
 * @return PSRAM_ERR_OK on success
 */
int flash_get_id(uint8_t id_out[3]);

#ifdef __cplusplus
}
#endif

#endif /* PSRAM_DRIVER_H */
