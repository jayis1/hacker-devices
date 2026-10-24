/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include <stdint.h>
extern uint32_t _estack, _sidata, _sdata, _edata, _sbss, _ebss;
extern int main(void);
extern void SysTick_Handler(void);
void Reset_Handler(void);
static void fault(void)
{
    /* Watchdog, if initialized, resets us. GPIO never drives a contact. */
    for (;;) { }
}
__attribute__((section(".vectors"), used))
void (*const vectors[])(void) = {
    (void (*)(void))(&_estack), Reset_Handler, fault, fault,
    fault, fault, fault, 0, 0, 0, 0, fault, fault, 0, fault, SysTick_Handler
};
void Reset_Handler(void)
{
    uint32_t *src = &_sidata;
    for (uint32_t *dst = &_sdata; dst < &_edata; )
        *dst++ = *src++;
    for (uint32_t *dst = &_sbss; dst < &_ebss; )
        *dst++ = 0;
    (void)main();
    fault();
}
