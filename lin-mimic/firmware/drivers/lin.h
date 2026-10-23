/* LIN protocol engine API. Author: jayis1. */
#ifndef LIN_MIMIC_LIN_H
#define LIN_MIMIC_LIN_H
#include <stdint.h>
#include <stdbool.h>
typedef enum { LIN_SIDE_CONTROLLER=0, LIN_SIDE_TARGET=1 } lin_side_t;
typedef enum { LIN_CLASSIC=0, LIN_ENHANCED=1, LIN_AUTO=2 } lin_checksum_t;
typedef enum { RX_WAIT_BREAK, RX_WAIT_SYNC, RX_WAIT_PID, RX_DATA } lin_rx_state_t;
typedef struct {
 uint32_t timestamp_us; uint8_t pid; uint8_t id; uint8_t data[8];
 uint8_t length; uint8_t checksum; lin_checksum_t checksum_type;
 lin_side_t source; bool parity_ok; bool checksum_ok; bool collision;
} lin_frame_t;
typedef struct {
 lin_rx_state_t state; lin_frame_t pending; uint8_t count; uint32_t last_byte_us;
 uint32_t frames; uint32_t parity_errors; uint32_t checksum_errors;
} lin_decoder_t;
uint8_t lin_make_pid(uint8_t id);
bool lin_pid_valid(uint8_t pid);
uint8_t lin_checksum(const uint8_t *data,uint8_t length,uint8_t pid,lin_checksum_t type);
void lin_decoder_init(lin_decoder_t *decoder,lin_side_t side);
bool lin_decoder_break(lin_decoder_t *decoder,uint32_t now);
bool lin_decoder_byte(lin_decoder_t *decoder,uint8_t byte,uint32_t now,lin_frame_t *out);
bool lin_decoder_timeout(lin_decoder_t *decoder,uint32_t now,lin_frame_t *out);
void lin_send_header(lin_side_t side,uint8_t id);
void lin_send_response(lin_side_t side,const lin_frame_t *frame);
#endif
