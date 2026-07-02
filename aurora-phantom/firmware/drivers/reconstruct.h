/*
 * reconstruct.h — compressed-sensing / sparse reconstruction
 * Author: jayis1   License: GPL-2.0
 */
#ifndef AURORA_RECONSTRUCT_H
#define AURORA_RECONSTRUCT_H
#include <stdint.h>

int        reconstruct_init(void);
void       reconstruct_pull_frame(void);        /* pull 256 IQ mags from FPGA */
const uint16_t *reconstruct_frame_buffer(void); /* 256 upsampled mags */
uint32_t   reconstruct_frame_words(void);       /* always 256 */
void       reconstruct_set_upsample(uint8_t factor); /* 1,2,4 */

#endif