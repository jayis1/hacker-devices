/*
 * tpm-phantom — drivers/spi_tpm_driver.h
 * SPI TPM 2.0 capture driver interface.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef TPM_PHANTOM_SPI_TPM_DRIVER_H
#define TPM_PHANTOM_SPI_TPM_DRIVER_H

#include <stdint.h>
#include "lpc_driver.h"  /* for lpc_get_timestamp_us() */

#define SPI_TPM_CMD_READ   0x83
#define SPI_TPM_CMD_WRITE  0x00
#define SPI_TPM_MAX_DATA   64
#define SPI_TPM_QUEUE_SIZE 64
#define SPI_CHUNK_SIZE     64

typedef enum {
    SPI_TPM_IDLE = 0,
    SPI_TPM_CMD_PHASE,
    SPI_TPM_ADDR_PHASE,
    SPI_TPM_DATA_PHASE
} spi_tpm_state_t;

typedef struct {
    uint8_t  command;     /* 0x83=READ, 0x00=WRITE */
    uint32_t address;    /* 24-bit TPM register address */
    uint8_t  data[SPI_TPM_MAX_DATA];
    uint8_t  data_len;
    uint32_t timestamp;
    uint8_t  is_tpm;
} spi_tpm_transaction_t;

extern volatile uint32_t spi_tpm_total_transactions;
extern volatile uint32_t spi_tpm_read_count;
extern volatile uint32_t spi_tpm_write_count;
extern volatile uint32_t spi_tpm_dma_overruns;
extern volatile uint32_t spi_tpm_cs_errors;
extern volatile uint32_t spi_tpm_bytes_captured;
extern volatile uint32_t spi_tpm_wait_states;

void spi_tpm_capture_init(void);
void spi_tpm_capture_start(void);
void spi_tpm_capture_stop(void);
uint8_t spi_tpm_pop_transaction(spi_tpm_transaction_t *out);
uint8_t spi_tpm_is_tpm_address(uint32_t addr);

void spi_tpm_on_cs_edge(uint8_t level);
void spi_tpm_count_wait_state(void);

uint8_t spi_tpm_inject_read_response(const uint8_t *data, uint8_t len);
void spi_tpm_inject_disable(void);
uint8_t spi_tpm_inject_active(void);

void spi_tpm_reset_stats(void);

#endif /* TPM_PHANTOM_SPI_TPM_DRIVER_H */