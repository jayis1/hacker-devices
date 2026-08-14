/*
 * startup.c — STM32G474 vector table + Reset_Handler + weak IRQ defaults
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Provides the interrupt vector table (at 0x08000000) and a minimal
 * Reset_Handler that copies .data, zeroes .bss, and calls main().
 * All other IRQs default to an infinite loop (can be overridden).
 */

#include "registers.h"

extern int main(void);

/* Forward declarations for handlers defined in main.c */
void Reset_Handler(void);
void TIM6_DAC_IRQHandler(void);
void EXTI15_10_IRQHandler(void);
void USB_HP_IRQHandler(void);
void USB_LP_IRQHandler(void);

/* Symbols from linker.ld */
extern uint32_t _etext, _sdata, _edata, _sbss, _ebss, _stack_top;

/* ---- Default handler ---- */
__attribute__((weak, noreturn))
void Default_Handler(void) {
    while (1) { __asm volatile("wfi"); }
}

/* ---- Reset handler: copy data, zero bss, call main ---- */
__attribute__((section(".isr_vector"), used))
void (* const g_pfnVectors[])(void) = {
    (void (*)(void))(&_stack_top),   /* initial SP */
    Reset_Handler,
    Default_Handler,   /* NMI */
    Default_Handler,   /* HardFault */
    Default_Handler,   /* MemManage */
    Default_Handler,   /* BusFault */
    Default_Handler,   /* UsageFault */
    0, 0, 0, 0,         /* reserved */
    Default_Handler,   /* SVC */
    Default_Handler,   /* DebugMon */
    0,                  /* reserved */
    Default_Handler,   /* PendSV */
    Default_Handler,   /* SysTick */
    /* IRQ 0..15 external */
    Default_Handler,   /* WWDG */
    Default_Handler,   /* PVD */
    Default_Handler,   /* TAMP */
    Default_Handler,   /* RTC */
    Default_Handler,   /* FLASH */
    Default_Handler,   /* RCC */
    Default_Handler,   /* EXTI0 */
    Default_Handler,   /* EXTI1 */
    Default_Handler,   /* EXTI2 */
    Default_Handler,   /* EXTI3 */
    Default_Handler,   /* EXTI4 */
    Default_Handler,   /* DMA1ch1 */
    Default_Handler,   /* DMA1ch2 */
    Default_Handler,   /* DMA1ch3 */
    Default_Handler,   /* DMA1ch4 */
    Default_Handler,   /* DMA1ch5 */
    Default_Handler,   /* DMA1ch6 */
    Default_Handler,   /* DMA1ch7 */
    Default_Handler,   /* ADC1_2 */
    Default_Handler,   /* USB_HP */
    Default_Handler,   /* USB_LP */
    Default_Handler,   /* CAN */
    Default_Handler,   /* EXTI9_5 */
    TIM6_DAC_IRQHandler,
    Default_Handler,   /* TIM7 */
    Default_Handler,   /* SPI1 */
    Default_Handler,   /* SPI2 */
    Default_Handler,   /* USART1 */
    Default_Handler,   /* USART2 */
    Default_Handler,   /* USART3 */
    Default_Handler,   /* EXTI15_10 */
    Default_Handler,   /* RTC_alarm */
    Default_Handler,   /* ... */
};

__attribute__((noreturn))
void Reset_Handler(void) {
    /* Copy .data from flash to RAM */
    uint32_t *src = &_etext;
    uint32_t *dst = &_sdata;
    while (dst < &_edata) *dst++ = *src++;
    /* Zero .bss */
    dst = &_sbss;
    while (dst < &_ebss) *dst++ = 0;
    /* Call main */
    (void)main();
    while (1) { }
}