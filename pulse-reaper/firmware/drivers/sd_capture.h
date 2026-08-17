/*
 * sd_capture.h — microSD capture (pcapng writer)
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef PULSEREAPER_SD_CAPTURE_H
#define PULSEREAPER_SD_CAPTURE_H

#include <stdint.h>
#include "protocol_detect.h"

void sd_capture_init(void);

/* Open a new capture file. Returns 0 on success. */
int  sd_capture_open(const char *filename);

/* Write raw bytes to the capture. */
int  sd_capture_write(const uint8_t *data, uint16_t len);

/* Format a parsed frame as a pcapng Enhanced Packet Block.
 * Returns the number of bytes written into out_buf (0 on error). */
uint16_t sd_capture_format_pcapng(const pr_parsed_t *parsed,
                                  uint8_t *out_buf, uint16_t max_len);

/* Close the capture file (flush + write trailing block). */
int  sd_capture_close(void);

#endif