/*
 * console.h — USB-CDC text command console for CC-Stiletto.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

#ifndef CC_STILETTO_CONSOLE_H
#define CC_STILETTO_CONSOLE_H

#include <stdint.h>

/* Initialize the USB-CDC peripheral and the line-oriented command parser. */
void console_init(void);

/* Poll for incoming characters from USB host. Call from main loop. */
void console_poll(void);

/* Emit a printf-style event/status line to the host (newline-terminated). */
void console_emit(const char *fmt, ...) __attribute__((format(printf,1,2)));

/* Process one complete line of input. Exposed for testing. */
void console_handle_line(const char *line);

#endif /* CC_STILETTO_CONSOLE_H */