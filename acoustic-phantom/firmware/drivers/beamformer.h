/*
 * ACOUSTIC-PHANTOM — Beamformer driver
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * Delay-and-sum beamformer for the 4-element linear MEMS microphone
 * array. Steers a directional pickup pattern toward the target,
 * rejecting off-axis ambient noise. Also estimates the source bearing
 * by scanning all steering directions and picking the one with maximum
 * output energy.
 */
#ifndef BEAMFORMER_H
#define BEAMFORMER_H

#include <stdint.h>
#include "board.h"
#include "audio_capture.h"

void     beamformer_init(void);
void     beamformer_process(audio_frame_t *frame);
void     beamformer_set_steering(int16_t angle_deg);
int16_t  beamformer_get_bearing(void);
void     beamformer_auto_steer(audio_frame_t *frame);

/* Get the beamformed mono output from the last processed frame */
void     beamformer_get_output(int16_t *out, uint16_t max_samples);

#endif /* BEAMFORMER_H */