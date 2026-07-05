/*
 * startup_stm32g474.s — minimal Cortex-M4 startup for DRAM-Phantom
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Provides the vector table (with initial SP reset to top of SRAM) and a
 * reset handler that copies .data from flash to SRAM, zeroes .bss, and calls
 * main(). All other IRQ handlers default to an infinite loop.
 */

.syntax unified
.cpu cortex-m4
.thumb

.extern main
.extern SystemInit

.section .isr_vector,"a",%progbits
.global g_pfnVectors
g_pfnVectors:
    .word _estack            /* Initial Stack Pointer */
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
    .word SVC_Handler
    .word DebugMon_Handler
    .word 0
    .word PendSV_Handler
    .word SysTick_Handler
    /* External interrupts — only the ones we use */
    .word Default_Handler    /* WWDG */
    .word Default_Handler    /* PVD */
    .word Default_Handler    /* TAMPER */
    .word Default_Handler    /* RTC */
    .word Default_Handler    /* FLASH */
    .word Default_Handler    /* RCC */
    .word Default_Handler    /* EXTI0 */
    .word Default_Handler    /* EXTI1 */
    .word Default_Handler    /* EXTI2 */
    .word Default_Handler    /* EXTI3 */
    .word Default_Handler    /* EXTI4 */
    .word Default_Handler    /* DMA1 ch1 */
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler    /* DMA1 ch9 */
    .word Default_Handler    /* DMA1 ch10 */
    .word Default_Handler    /* DMA2 ch1 */
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word USART1_IRQHandler  /* position 27 */
    .fill 100, 4, 0           /* rest default */

.section .text
.global Reset_Handler
Reset_Handler:
    /* Set stack pointer */
    ldr r0, =_estack
    mov sp, r0
    /* Copy .data from flash to SRAM */
    ldr r0, =_sidata
    ldr r1, =_sdata
    ldr r2, =_edata
copy_data:
    cmp r1, r2
    bcs copy_done
    ldr r3, [r0], #4
    str r3, [r1], #4
    b copy_data
copy_done:
    /* Zero .bss */
    ldr r0, =_sbss
    ldr r1, =_ebss
zero_bss:
    cmp r0, r1
    bcs zero_done
    mov r3, #0
    str r3, [r0], #4
    b zero_bss
zero_done:
    bl SystemInit
    bl main
hang:
    b hang

.section .text
.weak NMI_Handler
NMI_Handler: b .
.weak HardFault_Handler
HardFault_Handler: b .
.weak MemManage_Handler
MemManage_Handler: b .
.weak BusFault_Handler
BusFault_Handler: b .
.weak UsageFault_Handler
UsageFault_Handler: b .
.weak SVC_Handler
SVC_Handler: b .
.weak DebugMon_Handler
DebugMon_Handler: b .
.weak PendSV_Handler
PendSV_Handler: b .
.weak Default_Handler
Default_Handler: b .

.section .link_symbols
_estack = 0x2001FFFF