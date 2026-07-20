/*
 * hart-bleeder — startup_stm32g474.s
 * Minimal Cortex-M4 startup code and vector table for the
 * HART Fieldbus Covert In-Line Attack Implant.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

    .syntax unified
    .cpu cortex-m4
    .thumb

.extern main
.extern _estack
.extern _sidata
.extern _sdata
.extern _edata
.extern _sbss
.extern _ebss

/* Default handler */
.section .text.Default_Handler
.global Default_Handler
.type Default_Handler, %function
Default_Handler:
    b Default_Handler
.size Default_Handler, . - Default_Handler

/* Reset handler */
.section .text.Reset_Handler
.global Reset_Handler
.type Reset_Handler, %function
Reset_Handler:
    ldr   r0, =_estack
    mov   sp, r0
    /* Copy .data from flash to RAM */
    ldr   r0, =_sidata
    ldr   r1, =_sdata
    ldr   r2, =_edata
1:  cmp   r1, r2
    bcc   2f
    b     3f
2:  ldr   r3, [r0], #4
    str   r3, [r1], #4
    b     1b
3:  /* Zero .bss */
    ldr   r1, =_sbss
    ldr   r2, =_ebss
4:  cmp   r1, r2
    bcc   5f
    b     6f
5:  movs  r3, #0
    str   r3, [r1], #4
    b     4b
6:  /* Enable FPU */
    ldr   r0, =0xE000ED88
    ldr   r1, [r0]
    orr   r1, r1, #0xF00000
    str   r1, [r0]
    dsb
    isb
    bl    main
    b     Default_Handler
.size Reset_Handler, . - Reset_Handler

/* Vector table */
.section .isr_vector
.global __isr_vector
.type __isr_vector, %object
__isr_vector:
    .word _estack
    .word Reset_Handler
    .word Default_Handler          /* NMI */
    .word Default_Handler          /* HardFault */
    .word Default_Handler          /* MemManage */
    .word Default_Handler          /* BusFault */
    .word Default_Handler          /* UsageFault */
    .word 0
    .word 0
    .word 0
    .word 0
    .word Default_Handler          /* SVC */
    .word Default_Handler          /* DebugMon */
    .word 0
    .word Default_Handler          /* PendSV */
    .word SysTick_Handler
    /* External IRQs (STM32G4) */
    .word Default_Handler          /* WWDG */
    .word Default_Handler          /* PVD */
    .word Default_Handler          /* TAMP */
    .word Default_Handler          /* RTC */
    .word Default_Handler          /* FLASH */
    .word Default_Handler          /* RCC */
    .word Default_Handler          /* EXTI0 */
    .word Default_Handler          /* EXTI1 */
    .word Default_Handler          /* EXTI2 */
    .word Default_Handler          /* EXTI3 */
    .word Default_Handler          /* EXTI4 */
    .word Default_Handler          /* DMA1 ch1 */
    .word Default_Handler          /* DMA1 ch2 */
    .word Default_Handler          /* DMA1 ch3 */
    .word Default_Handler          /* DMA1 ch4 */
    .word Default_Handler          /* DMA1 ch5 */
    .word Default_Handler          /* DMA1 ch6 */
    .word Default_Handler          /* DMA1 ch7 */
    .word Default_Handler          /* ADC1_2 */
    .word Default_Handler          /* USB_HP */
    .word Default_Handler          /* USB_LP */
    .word Default_Handler          /* FDCAN1_IT0 */
    .word Default_Handler          /* FDCAN1_IT1 */
    .word Default_Handler          /* EXTI9_5 */
    .word Default_Handler          /* TIM1_BRK */
    .word Default_Handler          /* TIM1_UP */
    .word Default_Handler          /* TIM1_TRG_COM */
    .word Default_Handler          /* TIM1_CC */
    .word Default_Handler          /* TIM2 */
    .word Default_Handler          /* TIM3 */
    .word Default_Handler          /* TIM4 */
    .word Default_Handler          /* I2C1_EV */
    .word Default_Handler          /* I2C1_ER */
    .word Default_Handler          /* I2C2_EV */
    .word Default_Handler          /* I2C2_ER */
    .word Default_Handler          /* SPI1 */
    .word Default_Handler          /* SPI2 */
    .word Default_Handler          /* USART1 */
    .word Default_Handler          /* USART2 */
    .word Default_Handler          /* USART3 */
    .word Default_Handler          /* UART4 */
    .word Default_Handler          /* UART4 */
    .word Default_Handler          /* TIM6_DAC */
    .word TIM7_DAC_IRQHandler      /* TIM7 */
    .word Default_Handler          /* DMA2 */
    .word Default_Handler          /* DMA2 */
    .word Default_Handler          /* COMP */
    .word Default_Handler          /* EXTI15_10 */
    .word Default_Handler          /* RTC wakeup */
    .word Default_Handler          /* reserved */
    .word Default_Handler          /* SPI3 */
    .word Default_Handler          /* TIM8 */
.size __isr_vector, . - __isr_vector