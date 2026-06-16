/**
 * @file psram_driver.c
 * @brief QSPI PSRAM and NOR Flash Driver Implementation
 *
 * Manages APS6404L-3SQR-SN 8MB PSRAM via nRF52840 QSPI peripheral.
 * Provides ring buffer implementation for CAN frame storage.
 * Also manages W25Q128JVSIQ 16MB NOR Flash for DBC/script storage.
 *
 * QSPI clock: 64 MHz (PCLK64M / 1)
 * PSRAM: Quad I/O mode, 6 dummy cycles
 * Flash: Quad I/O mode, 8 dummy cycles
 */

#include "psram_driver.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/*===========================================================================
 * STATIC VARIABLES
 *===========================================================================*/

static bool g_qspi_initialized = false;
static bool g_psram_ready = false;
static bool g_flash_ready = false;

/* QSPI transfer buffer (max 256 bytes per EasyDMA transfer) */
static uint8_t g_qspi_buf[256] __attribute__((aligned(4)));

/*===========================================================================
 * PRIVATE HELPERS
 *===========================================================================*/

/**
 * @brief Wait for QSPI peripheral to be ready
 */
static int qspi_wait_ready(void) {
    uint32_t timeout = 1000000;  /* ~1 second at 64 MHz */
    while (!NRF_QSPI->EVENTS_READY) {
        if (--timeout == 0) return PSRAM_ERR_TIMEOUT;
    }
    NRF_QSPI->EVENTS_READY = 0;
    return PSRAM_ERR_OK;
}

/**
 * @brief Send a custom instruction to a QSPI device
 *
 * @param csn       CSN pin (0=PSRAM, 1=Flash)
 * @param opcode    8-bit instruction opcode
 * @param tx_data   Optional TX data after opcode
 * @param tx_len    TX data length
 * @param rx_data   Optional RX data buffer
 * @param rx_len    RX data length
 * @param dummy_cycles Number of dummy cycles after opcode
 * @return PSRAM_ERR_OK on success
 */
static int qspi_custom_instruction(uint8_t csn, uint8_t opcode,
                                    const uint8_t *tx_data, uint8_t tx_len,
                                    uint8_t *rx_data, uint8_t rx_len,
                                    uint8_t dummy_cycles) {
    int ret;

    /* Wait for QSPI ready */
    ret = qspi_wait_ready();
    if (ret != PSRAM_ERR_OK) return ret;

    /* Configure custom instruction */
    NRF_QSPI->CINSTRCONF =
        ((uint32_t)opcode << QSPI_CINSTRCONF_OPCODE_Pos) |
        ((uint32_t)rx_len << QSPI_CINSTRCONF_LEN_Pos) |
        ((dummy_cycles > 0 ? 1UL : 0UL) << QSPI_CINSTRCONF_DUMMEN_Pos);

    /* Set data bytes if any */
    if (tx_len > 0 && tx_data) {
        NRF_QSPI->CINSTRDAT0 = tx_data[0];
    }
    if (tx_len > 1 && tx_data) {
        NRF_QSPI->CINSTRDAT1 = tx_data[1];
    }

    /* Set duration (opcode + tx bytes + dummy cycles + rx bytes) */
    uint32_t duration = 1 + tx_len + dummy_cycles + rx_len;
    NRF_QSPI->DURATION = duration;

    /* Select device */
    if (csn == 0) {
        NRF_QSPI->PSEL_CSN0 = QSPI_CSN_PSRAM_PIN;
        NRF_QSPI->PSEL_CSN1 = 0xFFFFFFFF;  /* Disconnect */
    } else {
        NRF_QSPI->PSEL_CSN0 = 0xFFFFFFFF;
        NRF_QSPI->PSEL_CSN1 = QSPI_CSN_FLASH_PIN;
    }

    /* Set RX buffer if expecting data */
    if (rx_len > 0 && rx_data) {
        NRF_QSPI->CINSTRDAT0 = 0;  /* Clear data bytes */
        NRF_QSPI->CINSTRDAT1 = 0;
        /* Note: Custom instruction RX goes to CINSTRDAT registers, not EasyDMA */
        /* For longer reads, use READSTART instead */
    }

    /* Execute */
    NRF_QSPI->TASKS_ACTIVATE = 1;

    /* Wait for completion */
    ret = qspi_wait_ready();
    if (ret != PSRAM_ERR_OK) return ret;

    /* Read back data from CINSTRDAT registers */
    if (rx_len > 0 && rx_data) {
        uint32_t dat0 = NRF_QSPI->CINSTRDAT0;
        uint32_t dat1 = NRF_QSPI->CINSTRDAT1;
        if (rx_len >= 1) rx_data[0] = (uint8_t)(dat0 & 0xFF);
        if (rx_len >= 2) rx_data[1] = (uint8_t)((dat0 >> 8) & 0xFF);
        if (rx_len >= 3) rx_data[2] = (uint8_t)((dat0 >> 16) & 0xFF);
        if (rx_len >= 4) rx_data[3] = (uint8_t)((dat0 >> 24) & 0xFF);
        if (rx_len >= 5) rx_data[4] = (uint8_t)(dat1 & 0xFF);
        if (rx_len >= 6) rx_data[5] = (uint8_t)((dat1 >> 8) & 0xFF);
        if (rx_len >= 7) rx_data[6] = (uint8_t)((dat1 >> 16) & 0xFF);
        if (rx_len >= 8) rx_data[7] = (uint8_t)((dat1 >> 24) & 0xFF);
    }

    return PSRAM_ERR_OK;
}

/*===========================================================================
 * PSRAM INITIALIZATION
 *===========================================================================*/

/**
 * @brief Initialize QSPI and PSRAM
 */
int psram_init(void) {
    int ret;

    if (g_qspi_initialized) return PSRAM_ERR_OK;

    /* Configure QSPI pins */
    NRF_QSPI->PSEL_SCK  = QSPI_SCK_PIN;
    NRF_QSPI->PSEL_IO0  = QSPI_IO0_PIN;
    NRF_QSPI->PSEL_IO1  = QSPI_IO1_PIN;
    NRF_QSPI->PSEL_IO2  = QSPI_IO2_PIN;
    NRF_QSPI->PSEL_IO3  = QSPI_IO3_PIN;
    NRF_QSPI->PSEL_CSN0 = QSPI_CSN_PSRAM_PIN;
    NRF_QSPI->PSEL_CSN1 = QSPI_CSN_FLASH_PIN;

    /* Configure interface for PSRAM */
    NRF_QSPI->IFCONFIG0 =
        (QSPI_IFCONFIG0_ADDRMODE_24BIT << QSPI_IFCONFIG0_ADDRMODE_Pos) |
        (QSPI_IFCONFIG0_PPSIZE_256 << QSPI_IFCONFIG0_PPSIZE_Pos) |
        (QSPI_IFCONFIG0_READOC_READ4IO << QSPI_IFCONFIG0_READOC_Pos) |
        (QSPI_IFCONFIG0_WRITEOC_PP4IO << QSPI_IFCONFIG0_WRITEOC_Pos);

    NRF_QSPI->IFCONFIG1 =
        (QSPI_IFCONFIG1_SCKFREQ_DIV1 << QSPI_IFCONFIG1_SCKFREQ_Pos) |  /* 64 MHz */
        (QSPI_IFCONFIG1_SAMPLE_SHIFT << QSPI_IFCONFIG1_SAMPLE_Pos) |
        (QSPI_IFCONFIG1_SPI_MODE0 << QSPI_IFCONFIG1_SPI_MODE_Pos);

    /* Enable QSPI */
    NRF_QSPI->ENABLE = QSPI_ENABLE_ENABLE;

    /* Activate QSPI (must be done after enable) */
    NRF_QSPI->TASKS_ACTIVATE = 1;
    ret = qspi_wait_ready();
    if (ret != PSRAM_ERR_OK) {
        NRF_QSPI->ENABLE = QSPI_ENABLE_DISABLE;
        return ret;
    }

    /* Enter quad mode on PSRAM */
    ret = qspi_custom_instruction(0, PSRAM_ENTER_QUAD_OPCODE, NULL, 0, NULL, 0, 0);
    if (ret != PSRAM_ERR_OK) {
        NRF_QSPI->ENABLE = QSPI_ENABLE_DISABLE;
        return ret;
    }

    g_qspi_initialized = true;
    g_psram_ready = true;

    return PSRAM_ERR_OK;
}

/**
 * @brief Deinitialize QSPI
 */
int psram_deinit(void) {
    if (!g_qspi_initialized) return PSRAM_ERR_NOT_INIT;

    /* Exit quad mode on PSRAM */
    qspi_custom_instruction(0, 0xF5, NULL, 0, NULL, 0, 0);  /* RESET_QUAD */

    NRF_QSPI->ENABLE = QSPI_ENABLE_DISABLE;
    g_qspi_initialized = false;
    g_psram_ready = false;
    g_flash_ready = false;

    return PSRAM_ERR_OK;
}

/*===========================================================================
 * RING BUFFER OPERATIONS
 *===========================================================================*/

/**
 * @brief Initialize a ring buffer
 */
int psram_ring_init(ring_buffer_t *ring, uint32_t base_offset, uint32_t size) {
    if (!g_psram_ready) return PSRAM_ERR_NOT_INIT;
    if (!ring || size == 0 || base_offset + size > PSRAM_SIZE) {
        return PSRAM_ERR_INVALID_PARAM;
    }

    memset(ring, 0, sizeof(ring_buffer_t));
    ring->base_offset = base_offset;
    ring->buffer_size = size;
    ring->write_ptr = 0;
    ring->read_ptr = 0;
    ring->frame_count = 0;
    ring->overflow_count = 0;
    ring->initialized = true;

    return PSRAM_ERR_OK;
}

/**
 * @brief Write data to ring buffer
 */
int psram_ring_write(ring_buffer_t *ring, const uint8_t *data, uint32_t len) {
    uint32_t avail;
    int ret;

    if (!ring || !ring->initialized || !data) return PSRAM_ERR_INVALID_PARAM;
    if (!g_psram_ready) return PSRAM_ERR_NOT_INIT;

    /* Check available space */
    avail = ring->buffer_size - psram_ring_available(ring);
    if (len > avail) {
        ring->overflow_count++;
        return PSRAM_ERR_BUFFER_FULL;
    }

    /* Write to PSRAM at write_ptr */
    uint32_t phys_addr = ring->base_offset + ring->write_ptr;

    /* Handle wrap-around */
    uint32_t space_to_end = ring->buffer_size - ring->write_ptr;
    if (len <= space_to_end) {
        /* Single contiguous write */
        ret = psram_direct_write(phys_addr, data, len);
        if (ret != PSRAM_ERR_OK) return ret;
        ring->write_ptr = (ring->write_ptr + len) % ring->buffer_size;
    } else {
        /* Split write: first part to end, second part from start */
        ret = psram_direct_write(phys_addr, data, space_to_end);
        if (ret != PSRAM_ERR_OK) return ret;

        ret = psram_direct_write(ring->base_offset, data + space_to_end, len - space_to_end);
        if (ret != PSRAM_ERR_OK) return ret;

        ring->write_ptr = len - space_to_end;
    }

    ring->frame_count++;

    return PSRAM_ERR_OK;
}

/**
 * @brief Read data from ring buffer
 */
int psram_ring_read(ring_buffer_t *ring, uint8_t *data, uint32_t len, uint32_t *frames_read) {
    uint32_t avail;
    int ret;

    if (!ring || !ring->initialized || !data) return PSRAM_ERR_INVALID_PARAM;
    if (!g_psram_ready) return PSRAM_ERR_NOT_INIT;

    avail = psram_ring_available(ring);
    if (avail == 0) {
        if (frames_read) *frames_read = 0;
        return PSRAM_ERR_BUFFER_EMPTY;
    }

    /* Clamp read length to available data */
    if (len > avail) len = avail;

    /* Read from PSRAM at read_ptr */
    uint32_t phys_addr = ring->base_offset + ring->read_ptr;

    /* Handle wrap-around */
    uint32_t space_to_end = ring->buffer_size - ring->read_ptr;
    if (len <= space_to_end) {
        ret = psram_direct_read(phys_addr, data, len);
        if (ret != PSRAM_ERR_OK) return ret;
        ring->read_ptr = (ring->read_ptr + len) % ring->buffer_size;
    } else {
        ret = psram_direct_read(phys_addr, data, space_to_end);
        if (ret != PSRAM_ERR_OK) return ret;

        ret = psram_direct_read(ring->base_offset, data + space_to_end, len - space_to_end);
        if (ret != PSRAM_ERR_OK) return ret;

        ring->read_ptr = len - space_to_end;
    }

    /* Calculate frames read (each frame is CAN_FRAME_ENTRY_SIZE bytes) */
    if (frames_read) {
        *frames_read = len / 32;  /* CAN_FRAME_ENTRY_SIZE = 32 */
    }

    ring->frame_count -= (len / 32);

    return PSRAM_ERR_OK;
}

/**
 * @brief Peek at ring buffer data
 */
int psram_ring_peek(ring_buffer_t *ring, uint8_t *data, uint32_t len) {
    uint32_t avail;
    int ret;

    if (!ring || !ring->initialized || !data) return PSRAM_ERR_INVALID_PARAM;
    if (!g_psram_ready) return PSRAM_ERR_NOT_INIT;

    avail = psram_ring_available(ring);
    if (avail == 0) return PSRAM_ERR_BUFFER_EMPTY;
    if (len > avail) len = avail;

    /* Read without advancing read_ptr */
    uint32_t phys_addr = ring->base_offset + ring->read_ptr;
    uint32_t space_to_end = ring->buffer_size - ring->read_ptr;

    if (len <= space_to_end) {
        ret = psram_direct_read(phys_addr, data, len);
    } else {
        ret = psram_direct_read(phys_addr, data, space_to_end);
        if (ret == PSRAM_ERR_OK) {
            ret = psram_direct_read(ring->base_offset, data + space_to_end, len - space_to_end);
        }
    }

    return ret;
}

/**
 * @brief Clear ring buffer
 */
int psram_ring_clear(ring_buffer_t *ring) {
    if (!ring || !ring->initialized) return PSRAM_ERR_INVALID_PARAM;

    ring->write_ptr = 0;
    ring->read_ptr = 0;
    ring->frame_count = 0;

    return PSRAM_ERR_OK;
}

/**
 * @brief Get available bytes in ring buffer
 */
uint32_t psram_ring_available(ring_buffer_t *ring) {
    if (!ring || !ring->initialized) return 0;

    if (ring->write_ptr >= ring->read_ptr) {
        return ring->write_ptr - ring->read_ptr;
    } else {
        return ring->buffer_size - ring->read_ptr + ring->write_ptr;
    }
}

/*===========================================================================
 * DIRECT PSRAM ACCESS
 *===========================================================================*/

/**
 * @brief Direct read from PSRAM using QSPI EasyDMA
 */
int psram_direct_read(uint32_t addr, uint8_t *data, uint32_t len) {
    int ret;

    if (!g_psram_ready) return PSRAM_ERR_NOT_INIT;
    if (addr + len > PSRAM_SIZE) return PSRAM_ERR_INVALID_PARAM;
    if (len > 256) return PSRAM_ERR_INVALID_PARAM;  /* Max EasyDMA transfer */

    /* Wait for QSPI ready */
    ret = qspi_wait_ready();
    if (ret != PSRAM_ERR_OK) return ret;

    /* Configure read */
    NRF_QSPI->READ_SRC = addr;
    NRF_QSPI->READ_DST = (uint32_t)(len <= sizeof(g_qspi_buf) ? g_qspi_buf : (uintptr_t)data);
    NRF_QSPI->READ_CNT = len;

    /* Select PSRAM */
    NRF_QSPI->PSEL_CSN0 = QSPI_CSN_PSRAM_PIN;
    NRF_QSPI->PSEL_CSN1 = 0xFFFFFFFF;

    /* Start read */
    NRF_QSPI->TASKS_READSTART = 1;

    /* Wait for completion */
    ret = qspi_wait_ready();
    if (ret != PSRAM_ERR_OK) return ret;

    /* Copy from internal buffer if used */
    if (data != g_qspi_buf && len <= sizeof(g_qspi_buf)) {
        memcpy(data, g_qspi_buf, len);
    }

    return PSRAM_ERR_OK;
}

/**
 * @brief Direct write to PSRAM using QSPI EasyDMA
 */
int psram_direct_write(uint32_t addr, const uint8_t *data, uint32_t len) {
    int ret;

    if (!g_psram_ready) return PSRAM_ERR_NOT_INIT;
    if (addr + len > PSRAM_SIZE) return PSRAM_ERR_INVALID_PARAM;
    if (len > 256) return PSRAM_ERR_INVALID_PARAM;

    /* Wait for QSPI ready */
    ret = qspi_wait_ready();
    if (ret != PSRAM_ERR_OK) return ret;

    /* Copy to internal buffer if needed */
    if (data != g_qspi_buf && len <= sizeof(g_qspi_buf)) {
        memcpy(g_qspi_buf, data, len);
    }

    /* Configure write */
    NRF_QSPI->WRITE_SRC = (uint32_t)(len <= sizeof(g_qspi_buf) ? g_qspi_buf : (uintptr_t)data);
    NRF_QSPI->WRITE_DST = addr;
    NRF_QSPI->WRITE_CNT = len;

    /* Select PSRAM */
    NRF_QSPI->PSEL_CSN0 = QSPI_CSN_PSRAM_PIN;
    NRF_QSPI->PSEL_CSN1 = 0xFFFFFFFF;

    /* Start write */
    NRF_QSPI->TASKS_WRITESTART = 1;

    /* Wait for completion */
    ret = qspi_wait_ready();
    if (ret != PSRAM_ERR_OK) return ret;

    return PSRAM_ERR_OK;
}

/*===========================================================================
 * NOR FLASH OPERATIONS
 *===========================================================================*/

/**
 * @brief Initialize NOR Flash
 */
int flash_init(void) {
    uint8_t id[3];
    int ret;

    if (g_flash_ready) return PSRAM_ERR_OK;
    if (!g_qspi_initialized) {
        ret = psram_init();
        if (ret != PSRAM_ERR_OK) return ret;
    }

    /* Read JEDEC ID to verify flash presence */
    ret = flash_get_id(id);
    if (ret != PSRAM_ERR_OK) return ret;

    /* Expected: Manufacturer 0xEF (Winbond), Memory Type 0x40, Capacity 0x18 (128Mb) */
    if (id[0] != 0xEF || id[1] != 0x40 || id[2] != 0x18) {
        return PSRAM_ERR_FLASH_BUSY;  /* Wrong device */
    }

    g_flash_ready = true;
    return PSRAM_ERR_OK;
}

/**
 * @brief Read from NOR Flash
 */
int flash_read(uint32_t addr, uint8_t *data, uint32_t len) {
    int ret;

    if (!g_flash_ready) return PSRAM_ERR_NOT_INIT;
    if (addr + len > FLASH_SIZE) return PSRAM_ERR_INVALID_PARAM;
    if (len > 256) return PSRAM_ERR_INVALID_PARAM;

    ret = qspi_wait_ready();
    if (ret != PSRAM_ERR_OK) return ret;

    /* Configure for flash read */
    NRF_QSPI->IFCONFIG0 &= ~(QSPI_IFCONFIG0_READOC_Msk);
    NRF_QSPI->IFCONFIG0 |= (QSPI_IFCONFIG0_READOC_READ4IO << QSPI_IFCONFIG0_READOC_Pos);

    NRF_QSPI->READ_SRC = addr;
    NRF_QSPI->READ_DST = (uint32_t)(len <= sizeof(g_qspi_buf) ? g_qspi_buf : (uintptr_t)data);
    NRF_QSPI->READ_CNT = len;

    NRF_QSPI->PSEL_CSN0 = 0xFFFFFFFF;
    NRF_QSPI->PSEL_CSN1 = QSPI_CSN_FLASH_PIN;

    NRF_QSPI->TASKS_READSTART = 1;

    ret = qspi_wait_ready();
    if (ret != PSRAM_ERR_OK) return ret;

    if (data != g_qspi_buf && len <= sizeof(g_qspi_buf)) {
        memcpy(data, g_qspi_buf, len);
    }

    return PSRAM_ERR_OK;
}

/**
 * @brief Write to NOR Flash
 */
int flash_write(uint32_t addr, const uint8_t *data, uint32_t len) {
    int ret;

    if (!g_flash_ready) return PSRAM_ERR_NOT_INIT;
    if (addr + len > FLASH_SIZE) return PSRAM_ERR_INVALID_PARAM;

    /* Write enable */
    ret = qspi_custom_instruction(1, FLASH_WRITE_ENABLE_OPCODE, NULL, 0, NULL, 0, 0);
    if (ret != PSRAM_ERR_OK) return ret;

    /* Write data in 256-byte pages */
    while (len > 0) {
        uint32_t chunk = (len > 256) ? 256 : len;

        ret = qspi_wait_ready();
        if (ret != PSRAM_ERR_OK) return ret;

        NRF_QSPI->WRITE_SRC = (uint32_t)(chunk <= sizeof(g_qspi_buf) ? g_qspi_buf : (uintptr_t)data);
        NRF_QSPI->WRITE_DST = addr;
        NRF_QSPI->WRITE_CNT = chunk;

        NRF_QSPI->PSEL_CSN0 = 0xFFFFFFFF;
        NRF_QSPI->PSEL_CSN1 = QSPI_CSN_FLASH_PIN;

        if (data != g_qspi_buf && chunk <= sizeof(g_qspi_buf)) {
            memcpy(g_qspi_buf, data, chunk);
        }

        NRF_QSPI->TASKS_WRITESTART = 1;

        ret = qspi_wait_ready();
        if (ret != PSRAM_ERR_OK) return ret;

        /* Wait for write to complete (poll status register) */
        uint8_t status;
        uint32_t timeout = 100000;
        do {
            ret = qspi_custom_instruction(1, FLASH_READ_STATUS_OPCODE, NULL, 0, &status, 1, 0);
            if (ret != PSRAM_ERR_OK) return ret;
            if (!(status & 0x01)) break;  /* BUSY bit clear */
            for (volatile uint32_t d = 0; d < 100; d++) { __NOP(); }
        } while (--timeout > 0);

        if (timeout == 0) return PSRAM_ERR_TIMEOUT;

        addr += chunk;
        data += chunk;
        len -= chunk;
    }

    return PSRAM_ERR_OK;
}

/**
 * @brief Erase a 4KB sector
 */
int flash_erase_sector(uint32_t addr) {
    int ret;

    if (!g_flash_ready) return PSRAM_ERR_NOT_INIT;

    /* Write enable */
    ret = qspi_custom_instruction(1, FLASH_WRITE_ENABLE_OPCODE, NULL, 0, NULL, 0, 0);
    if (ret != PSRAM_ERR_OK) return ret;

    /* Send sector erase command with address */
    uint8_t cmd[4];
    cmd[0] = FLASH_ERASE_4K_OPCODE;
    cmd[1] = (uint8_t)((addr >> 16) & 0xFF);
    cmd[2] = (uint8_t)((addr >> 8) & 0xFF);
    cmd[3] = (uint8_t)(addr & 0xFF);

    ret = qspi_custom_instruction(1, FLASH_ERASE_4K_OPCODE, cmd, 3, NULL, 0, 0);
    if (ret != PSRAM_ERR_OK) return ret;

    /* Wait for erase to complete */
    uint8_t status;
    uint32_t timeout = 500000;  /* Erase can take up to ~400ms */
    do {
        ret = qspi_custom_instruction(1, FLASH_READ_STATUS_OPCODE, NULL, 0, &status, 1, 0);
        if (ret != PSRAM_ERR_OK) return ret;
        if (!(status & 0x01)) break;
        for (volatile uint32_t d = 0; d < 1000; d++) { __NOP(); }
    } while (--timeout > 0);

    if (timeout == 0) return PSRAM_ERR_TIMEOUT;

    return PSRAM_ERR_OK;
}

/**
 * @brief Erase entire flash chip
 */
int flash_erase_chip(void) {
    int ret;

    if (!g_flash_ready) return PSRAM_ERR_NOT_INIT;

    /* Write enable */
    ret = qspi_custom_instruction(1, FLASH_WRITE_ENABLE_OPCODE, NULL, 0, NULL, 0, 0);
    if (ret != PSRAM_ERR_OK) return ret;

    /* Chip erase command */
    ret = qspi_custom_instruction(1, FLASH_ERASE_CHIP_OPCODE, NULL, 0, NULL, 0, 0);
    if (ret != PSRAM_ERR_OK) return ret;

    /* Wait for erase (can take up to 80 seconds for 16MB!) */
    uint8_t status;
    uint32_t timeout = 10000000;  /* ~10 seconds at 64 MHz */
    do {
        ret = qspi_custom_instruction(1, FLASH_READ_STATUS_OPCODE, NULL, 0, &status, 1, 0);
        if (ret != PSRAM_ERR_OK) return ret;
        if (!(status & 0x01)) break;
        for (volatile uint32_t d = 0; d < 10000; d++) { __NOP(); }
    } while (--timeout > 0);

    if (timeout == 0) return PSRAM_ERR_TIMEOUT;

    return PSRAM_ERR_OK;
}

/**
 * @brief Get flash JEDEC ID
 */
int flash_get_id(uint8_t id_out[3]) {
    int ret;

    if (!g_qspi_initialized) return PSRAM_ERR_NOT_INIT;

    /* Read JEDEC ID (opcode 0x9F) */
    ret = qspi_custom_instruction(1, 0x9F, NULL, 0, id_out, 3, 0);
    if (ret != PSRAM_ERR_OK) return ret;

    return PSRAM_ERR_OK;
}
