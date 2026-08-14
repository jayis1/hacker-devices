/*
 * bc_role.h — Bus Controller role
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef BC_ROLE_H
#define BC_ROLE_H

#include <stdint.h>

/* Fuzz masks (bits of fpga_link_set_fuzz) */
#define FUZZ_PARITY   0x0001
#define FUZZ_SYNC     0x0002
#define FUZZ_BADLEN   0x0004
#define FUZZ_GAP      0x0008
#define FUZZ_FAULTINJ 0x0010
#define FUZZ_MODECODE 0x0020

typedef struct {
    uint8_t  ch;            /* 0=A, 1=B */
    uint8_t  rt;             /* target RT address */
    uint8_t  tx;            /* 1 = RT→BC, 0 = BC→RT */
    uint8_t  sa;             /* subaddress */
    uint8_t  wc;             /* word count */
    uint8_t  n_data;         /* actual data words present (BC→RT) */
    uint16_t data[32];       /* data words (BC→RT) */
    uint16_t gap_us;        /* inter-message gap */
} bc_msg_t;

void     bc_role_init(void);
void     bc_role_start(void);
void     bc_role_stop(void);
void     bc_role_tick(uint32_t now_ms);
void     bc_role_cli(int argc, char argv[8][32]);
uint32_t bc_role_frame_count(void);
void     bc_role_fuzz_mode_sweep(uint8_t rt_start, uint8_t rt_end);

#endif /* BC_ROLE_H */