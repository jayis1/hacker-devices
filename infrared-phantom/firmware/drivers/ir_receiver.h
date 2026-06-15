/*
 * ir_receiver.h — IR receiver driver for TSMP58000
 * Captures raw IR waveforms via ADC1 and digital edges via TIM2
 * STM32H743VIT6 @ 480 MHz
 */

#ifndef IR_RECEIVER_H
#define IR_RECEIVER_H

#include <stdint.h>
#include "board.h"
#include "registers.h"

/* Capture configuration */
#define IR_ADC_SAMPLE_RATE      2000000U    /* 2 Msps */
#define IR_ADC_RESOLUTION       12U          /* 12-bit */
#define IR_DMA_BUF_SIZE         8192U        /* 4 ms at 2 Msps */
#define IR_TIM_CAPTURE_BUF_SIZE 2048U        /* Timestamp buffer */

/* Protocol detection thresholds (in microseconds) */
#define NEC_LEADER_MARK_US      9000U
#define NEC_LEADER_SPACE_US     4500U
#define NEC_BIT_MARK_US         560U
#define NEC_BIT_ONE_US          1690U
#define NEC_BIT_ZERO_US         560U

#define RC5_LEADER_US           889U
#define RC6_LEADER_US           2666U
#define SONY_LEADER_US          2400U

/* Detected protocol result */
typedef struct {
    uint8_t protocol;           /* IR_PROTO_xxx */
    uint16_t address;           /* Decoded address */
    uint16_t command;           /* Decoded command */
    uint8_t toggle;             /* Toggle bit (RC5/RC6) */
    uint32_t timestamp_ms;     /* System tick at detection */
    uint16_t raw_length;        /* Number of timing entries */
    uint32_t raw_data[128];    /* Raw timing data (mark/space pairs) */
} ir_frame_t;

/* Capture statistics */
typedef struct {
    uint32_t total_frames;      /* Total frames decoded */
    uint32_t unknown_frames;    /* Frames that couldn't be decoded */
    uint32_t noise_events;      /* Noise-triggered events */
    uint32_t last_carrier_hz;   /* Detected carrier frequency */
    uint8_t  last_protocol;     /* Last detected protocol */
    uint16_t last_address;      /* Last decoded address */
    uint16_t last_command;      /* Last decoded command */
} ir_stats_t;

/* Function prototypes */
void ir_receiver_init(void);
void ir_receiver_start(uint32_t sample_rate);
void ir_receiver_stop(void);
void ir_receiver_poll(void);
ir_frame_t *ir_receiver_get_last_frame(void);
ir_stats_t *ir_receiver_get_stats(void);
void ir_receiver_reset_stats(void);

/* DMA interrupt handlers */
void adc_dma_half_complete(void);
void adc_dma_full_complete(void);
void tim2_capture_callback(void);

#endif /* IR_RECEIVER_H */