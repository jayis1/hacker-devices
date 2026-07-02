/*
 * host_test_main.c — host test harness for Aurora-Phantom firmware
 * Author: jayis1   License: GPL-2.0
 *
 * Provides the standard main() that calls app_main() so the firmware can be
 * compiled and run on a host for unit testing of the control logic and
 * drivers without any ESP32/iCE40 hardware.
 */
#include <stdio.h>

extern int app_main(void);

int main(void)
{
    printf("aurora-phantom host test (author: jayis1)\n");
    return app_main();
}