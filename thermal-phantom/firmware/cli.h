/*
 * cli.h - Serial command-line interface header
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPLv3
 */

#ifndef CLI_H
#define CLI_H

#include <stdint.h>

void cli_init(void);
void cli_process(void);
void cli_print(const char *str);
void cli_printf(const char *fmt, ...);

#endif /* CLI_H */