/* Credential Canary minimal Cortex-M4 startup
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdint.h>
extern int main(void);
extern uint32_t _estack, _etext, _sdata, _edata, _sbss, _ebss;
void Reset_Handler(void);
void Default_Handler(void) { for (;;) { } }
void NMI_Handler(void) __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void TIM2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USART1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USART2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
__attribute__((section(".isr_vector"))) void (*const vectors[])(void) = {
 (void (*)(void))(&_estack), Reset_Handler, NMI_Handler, HardFault_Handler,
 Default_Handler, Default_Handler, Default_Handler, 0, 0, 0, 0,
 Default_Handler, Default_Handler, 0, Default_Handler, Default_Handler,
 [16 + 28] = TIM2_IRQHandler, [16 + 37] = USART1_IRQHandler, [16 + 38] = USART2_IRQHandler
};
void Reset_Handler(void) {
 uint32_t *src = &_etext;
 for (uint32_t *dst = &_sdata; dst < &_edata;) *dst++ = *src++;
 for (uint32_t *dst = &_sbss; dst < &_ebss;) *dst++ = 0u;
 (void)main();
 for (;;) { }
}
