/*
 * protocol.h — Host command protocol driver header
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "../board.h"

/* Command codes */
#define CMD_FIRE            0x01
#define CMD_SET_PARAMS      0x02
#define CMD_GET_RESULT      0x03
#define CMD_START_SWEEP     0x04
#define CMD_STOP_SWEEP      0x05
#define CMD_GET_WAVEFORM    0x06
#define CMD_SET_TRIGGER     0x07
#define CMD_SET_PATTERNS    0x08
#define CMD_GET_STATUS      0x09
#define CMD_LOAD_FPGA       0x0A
#define CMD_ERASE_LOG       0x0B
#define CMD_GET_LOG_ENTRY   0x0C
#define CMD_PING            0x0D
#define CMD_GET_VERSION     0x0E

/* Response codes */
#define RESP_OK             0x80
#define RESP_ERROR          0x81
#define RESP_RESULT         0x82
#define RESP_SWEEP_PROGRESS 0x83
#define RESP_SWEEP_COMPLETE 0x84
#define RESP_STATUS         0x85
#define RESP_WAVEFORM       0x86
#define RESP_VERSION        0x87

/* Protocol framing */
#define FRAME_START_BYTE    0xA5
#define FRAME_END_BYTE      0x5A

void protocol_poll(void);
void protocol_send_sweep_progress(const sweep_status_t *status,
                                  const glitch_result_t *result);
void protocol_send_sweep_complete(const sweep_status_t *status);

#endif /* PROTOCOL_H */