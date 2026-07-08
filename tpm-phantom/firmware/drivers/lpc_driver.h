/*
 * tpm-phantom — drivers/lpc_driver.h
 * LPC bus snoop driver interface.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef TPM_PHANTOM_LPC_DRIVER_H
#define TPM_PHANTOM_LPC_DRIVER_H

#include <stdint.h>

/* LPC capture state machine */
typedef enum {
    LPC_STATE_IDLE = 0,
    LPC_STATE_CYCLE_START,
    LPC_STATE_ADDR_PHASE,
    LPC_STATE_DATA_PHASE,
    LPC_STATE_WAIT_DATA
} lpc_capture_state_t;

/* Captured LPC transaction */
typedef struct {
    uint8_t  direction;     /* 1=read, 0=write */
    uint32_t address;       /* 16-bit (IO) or 32-bit (MEM) */
    uint32_t data;          /* 8-bit data value */
    uint8_t  cycle_type;    /* 0=IOR 1=IOW 2=MEMR 3=MEMW 4=FWHR 5=FWRW */
    uint32_t timestamp;     /* microseconds since capture start */
    uint8_t  is_tpm;        /* 1 if address in TPM range */
} lpc_transaction_t;

#define LPC_FRAME_MAX 64

/* Public stats */
extern volatile uint32_t lpc_total_transactions;
extern volatile uint32_t lpc_tpm_transactions;
extern volatile uint32_t lpc_crc_errors;
extern volatile uint32_t lpc_frame_errors;
extern volatile uint32_t lpc_serirq_count;

/* Init / start / stop */
void lpc_capture_init(void);
void lpc_capture_start(void);
void lpc_capture_stop(void);

/* Transaction queue */
uint8_t lpc_pop_transaction(lpc_transaction_t *out);

/* Address classify */
uint8_t lpc_is_tpm_address(uint32_t addr);

/* Interrupt handlers (called from EXTI dispatch) */
void lpc_on_lframe_edge(uint8_t level);
void lpc_on_lclk_rising(void);
void lpc_on_serirq_edge(void);

/* Timestamp */
void lpc_timer_init(void);
uint32_t lpc_get_timestamp_us(void);

/* Injection */
uint8_t lpc_inject_io_write(uint16_t addr, uint8_t data);

/* Stats */
void lpc_reset_stats(void);

#endif /* TPM_PHANTOM_LPC_DRIVER_H */