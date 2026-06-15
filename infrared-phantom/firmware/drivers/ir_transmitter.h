/*
 * ir_transmitter.h — IR transmitter driver for TSAL6100 LED array
 * Generates modulated IR signals via TIM3 PWM + DMA
 * STM32H743VIT6 @ 480 MHz
 */

#ifndef IR_TRANSMITTER_H
#define IR_TRANSMITTER_H

#include <stdint.h>
#include "board.h"
#include "registers.h"

/* Default carrier parameters */
#define IR_CARRIER_NEC_HZ           38000U    /* 38 kHz NEC */
#define IR_CARRIER_RC5_HZ           36000U    /* 36 kHz RC5 */
#define IR_CARRIER_SONY_HZ          40000U    /* 40 kHz Sony */
#define IR_CARRIER_DEFAULT_HZ       38000U    /* Default 38 kHz */
#define IR_DUTY_CYCLE_DEFAULT       33U        /* 33% duty cycle */

/* TIM3 clock frequency (APB1 timer = APB1 × 2 = 240 MHz) */
#define TIM3_CLOCK_HZ               240000000U

/* Maximum waveform buffer size (in DMA entries) */
#define IR_TX_WAVEFORM_MAX          2048U

/* Fuzz mode mutation strategies */
#define FUZZ_STRATEGY_BITFLIP       0x01U
#define FUZZ_STRATEGY_RANGE_SWEEP   0x02U
#define FUZZ_STRATEGY_RANDOM         0x03U
#define FUZZ_STRATEGY_DICTIONARY    0x04U

/* Fuzz field targets */
#define FUZZ_FIELD_ADDRESS          0x01U
#define FUZZ_FIELD_COMMAND          0x02U
#define FUZZ_FIELD_TOGGLE           0x03U
#define FUZZ_FIELD_EXTENDED          0x04U

/* Transmit status */
typedef enum {
    IR_TX_IDLE = 0,
    IR_TX_ACTIVE,
    IR_TX_COMPLETE,
    IR_TX_ERROR
} ir_tx_status_t;

/* Transmit configuration */
typedef struct {
    uint8_t  protocol;           /* IR_PROTO_xxx */
    uint16_t carrier_hz;         /* Carrier frequency (Hz) */
    uint8_t  duty_cycle;         /* Duty cycle (%) */
    uint16_t address;            /* Address bits */
    uint16_t command;            /* Command bits */
    uint8_t  repeat_count;       /* Number of repeats (0 = single) */
    uint16_t repeat_interval_ms; /* Time between repeats */
} ir_tx_config_t;

/* Fuzz configuration */
typedef struct {
    uint8_t  protocol;           /* Target protocol */
    uint8_t  field;              /* FUZZ_FIELD_xxx */
    uint8_t  strategy;           /* FUZZ_STRATEGY_xxx */
    uint16_t base_address;       /* Starting address */
    uint16_t base_command;       /* Starting command */
    uint16_t count;              /* Number of mutations */
    uint16_t delay_ms;           /* Delay between transmissions */
} ir_fuzz_config_t;

/* Function prototypes */
void ir_transmitter_init(void);
ir_tx_status_t ir_transmitter_send(const ir_tx_config_t *config);
ir_tx_status_t ir_transmitter_send_raw(const uint16_t *timing, uint16_t count,
                                        uint16_t carrier_hz, uint8_t duty_cycle);
void ir_transmitter_stop(void);
ir_tx_status_t ir_transmitter_get_status(void);
void ir_transmitter_poll(void);

/* Fuzz functions */
void ir_fuzz_start(const ir_fuzz_config_t *config);
void ir_fuzz_stop(void);
void ir_fuzz_poll(void);

#endif /* IR_TRANSMITTER_H */