/*
 * hart-bleeder — console.h
 * USB-CDC console interface for operator command & control.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef HART_BLEEDER_CONSOLE_H
#define HART_BLEEDER_CONSOLE_H

#include <stdint.h>

int  console_init(void);
void console_printf(const char *fmt, ...);
int  console_getline(char *buf, uint16_t max);
void console_task(void);          /* call from main loop — dispatch cmds */
void console_help(void);

/* Command handler type */
typedef int (*cmd_handler_t)(int argc, char **argv);

int  console_register(const char *name, const char *help, cmd_handler_t fn);

#endif /* HART_BLEEDER_CONSOLE_H */