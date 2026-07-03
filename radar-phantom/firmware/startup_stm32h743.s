/*
 * startup_stm32h743.s — minimal vector table + startup for RadarPhantom
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Provides the initial stack pointer and vector table. The Reset_Handler
 * is implemented in C (board_init.c) so this file only needs the vector
 * table and a default handler alias.
 */
    .syntax unified
    .cpu cortex-m7
    .thumb

    .section .isr_vector,"a",%progbits
    .global g_pfnVectors
g_pfnVectors:
    .word _estack
    .word Reset_Handler
    .word NMI_Handler
    .word HardFault_Handler
    .word MemManage_Handler
    .word BusFault_Handler
    .word UsageFault_Handler
    .word 0
    .word 0
    .word 0
    .word 0
    .word Default_Handler        /* SVC */
    .word Default_Handler        /* DebugMon */
    .word 0
    .word Default_Handler        /* PendSV */
    .word SysTick_Handler
    /* IRQ 0..15 are above; peripheral IRQs 16+ are aliased to Default */
    .rept 128
    .word Default_Handler
    .endr

    .section .text.Default_Handler,"ax",%progbits
Default_Handler:
    b Default_Handler

    .weak NMI_Handler
    .weak HardFault_Handler
    .weak MemManage_Handler
    .weak BusFault_Handler
    .weak UsageFault_Handler
    .weak SysTick_Handler
    .thumb_set NMI_Handler, Default_Handler
    .thumb_set HardFault_Handler, Default_Handler
    .thumb_set MemManage_Handler, Default_Handler
    .thumb_set BusFault_Handler, Default_Handler
    .thumb_set UsageFault_Handler, Default_Handler
    /* SysTick_Handler is defined in board_init.c — keep it strong */

    .end