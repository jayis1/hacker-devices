/*
 * startup_stm32h743.s — Startup code for ECHO-Phantom (STM32H743VIT6)
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Vector table and startup code for the Cortex-M7 core.
 * Initializes the stack pointer, copies .data from flash to DTCM,
 * zeros .bss, and calls main().
 */

    .syntax unified
    .cpu cortex-m7
    .fpu softvfp
    .thumb

.word _estack           /* Initial stack pointer */

.section .isr_vector, "a", %progbits
.global g_pfnVectors
.type g_pfnVectors, %object

g_pfnVectors:
    .word _estack                   /*  0: Initial Stack Pointer */
    .word Reset_Handler             /*  1: Reset */
    .word NMI_Handler              /*  2: NMI */
    .word HardFault_Handler        /*  3: Hard Fault */
    .word MemManage_Handler        /*  4: MemManage */
    .word BusFault_Handler         /*  5: BusFault */
    .word UsageFault_Handler       /*  6: UsageFault */
    .word 0                        /*  7: Reserved */
    .word 0                        /*  8: Reserved */
    .word 0                        /*  9: Reserved */
    .word 0                        /* 10: Reserved */
    .word SVC_Handler              /* 11: SVCall */
    .word DebugMon_Handler         /* 12: Debug Monitor */
    .word 0                        /* 13: Reserved */
    .word PendSV_Handler           /* 14: PendSV */
    .word SysTick_Handler          /* 15: SysTick */

    /* External interrupts (simplified — only the ones we use) */
    .word 0                        /* 16: WWDG */
    .word 0                        /* 17: PVD */
    .word 0                        /* 18: TAMPER */
    .word 0                        /* 19: RTC */
    .word 0                        /* 20: FLASH */
    .word 0                        /* 21: RCC */
    .word 0                        /* 22: EXTI0 */
    .word 0                        /* 23: EXTI1 */
    .word 0                        /* 24: EXTI2 */
    .word 0                        /* 25: EXTI3 */
    .word 0                        /* 26: EXTI4 */
    .word DMA1_Stream0_IRQHandler  /* 27: DMA1 Stream 0 (SAI1_A RX) */
    .word DMA1_Stream1_IRQHandler  /* 28: DMA1 Stream 1 (SAI1_B TX) */
    .word 0                        /* 29: DMA1 Stream 2 */
    .word 0                        /* 30: DMA1 Stream 3 */
    .word 0                        /* 31: DMA1 Stream 4 */
    .word 0                        /* 32: DMA1 Stream 5 */
    .word 0                        /* 33: DMA1 Stream 6 */
    .word 0                        /* 34: DMA1 Stream 7 */
    .word 0                        /* 35: ADC1_2 */
    .word 0                        /* 36: FMC */
    .word USART1_IRQHandler        /* 37: USART1 (BLE C2) */
    .word 0                        /* 38: USART2 */
    .word 0                        /* 39: USART3 */
    .word 0                        /* 40: UART4 */
    .word 0                        /* 41: UART5 */
    .word 0                        /* 42: LPUART1 */
    .word 0                        /* 43: TIM15 */
    .word 0                        /* 44: TIM16 */
    .word 0                        /* 45: TIM17 */
    .word 0                        /* 46: TIM12 */
    .word 0                        /* 47: TIM13 */
    .word 0                        /* 48: TIM14 */
    .word 0                        /* 49: TIM1_BRK */
    .word 0                        /* 50: TIM1_UP */
    .word 0                        /* 51: TIM1_TRG_COM */
    .word 0                        /* 52: TIM1_CC */
    .word 0                        /* 53: TIM2 */
    .word 0                        /* 54: TIM3 */
    .word 0                        /* 55: TIM4 */
    .word 0                        /* 56: I2C1_EV */
    .word 0                        /* 57: I2C1_ER */
    .word 0                        /* 58: I2C2_EV */
    .word 0                        /* 59: I2C2_ER */
    .word 0                        /* 60: SPI1 */
    .word 0                        /* 61: SPI2 */
    .word 0                        /* 62: SPI3 */
    .word 0                        /* 63: Reserved */
    .word 0                        /* 64: TIM8_BRK */
    .word 0                        /* 65: TIM8_UP */
    .word 0                        /* 66: TIM8_TRG_COM */
    .word 0                        /* 67: TIM8_CC */
    .word 0                        /* 68: ADC3 */
    .word 0                        /* 69: FMC */
    .word 0                        /* 70: SDMMC1 */
    .word 0                        /* 71: TIM5 */
    .word 0                        /* 72: SPI3 */
    .word 0                        /* 73: UART4 */
    .word 0                        /* 74: UART5 */
    .word 0                        /* 75: TIM6 */
    .word 0                        /* 76: TIM7 */
    .word 0                        /* 77: DMA2 Stream 0 */
    .word 0                        /* 78: DMA2 Stream 1 */
    .word 0                        /* 79: DMA2 Stream 2 */
    .word 0                        /* 80: DMA2 Stream 3 */
    .word 0                        /* 81: DMA2 Stream 4 */
    .word 0                        /* 82: ETH */
    .word 0                        /* 83: ETH_WKUP */
    .word 0                        /* 84: FDCAN1_IT0 */
    .word SAI1_IRQHandler          /* 85: SAI1 */
    .word 0                        /* 86: TIM15 */
    .word 0                        /* 87: TIM16 */
    .word 0                        /* 88: TIM17 */
    .word 0                        /* 89: MDIOS */
    .word 0                        /* 90: MDMA */
    .word 0                        /* 91: Reserved */
    .word 0                        /* 92: SDMMC2 */
    .word 0                        /* 93: HSEM1 */
    .word 0                        /* 94: HSEM2 */
    .word 0                        /* 95: ADC3 */
    .word 0                        /* 96: Reserved */
    .word 0                        /* 97: Reserved */
    .word 0                        /* 98: SAI2 */
    .word 0                        /* 99: QUADSPI */
    .word 0                        /* 100: VREFBUF */
    .word 0                        /* 101: Reserved */
    .word 0                        /* 102: LPTIM1 */
    .word 0                        /* 103: LPTIM2 */
    .word 0                        /* 104: OTG1_IRQHandler */
    .word 0                        /* 105: Reserved */

.size g_pfnVectors, .-g_pfnVectors

/* ========================================================================
 *  Reset handler — entry point
 * ======================================================================== */

.section .text.Reset_Handler, "ax", %progbits
.global Reset_Handler
.type Reset_Handler, %function
Reset_Handler:
    ldr   sp, =_estack       /* Set stack pointer */

    /* Copy .data from flash to DTCM */
    ldr   r0, =_sdata
    ldr   r1, =_edata
    ldr   r2, =_sidata
copy_data:
    cmp   r0, r1
    bcc   copy_loop
    b     zero_bss
copy_loop:
    ldr   r3, [r2], #4
    str   r3, [r0], #4
    b     copy_data

    /* Zero .bss */
zero_bss:
    ldr   r0, =_sbss
    ldr   r1, =_ebss
    movs  r2, #0
zero_loop:
    cmp   r0, r1
    bcc   zero_write
    b     call_main
zero_write:
    str   r2, [r0], #4
    b     zero_loop

call_main:
    bl    main
    b     .                       /* Should never get here */

.size Reset_Handler, .-Reset_Handler

/* ========================================================================
 *  Default handlers (weak)
 * ======================================================================== */

.weak NMI_Handler
.thumb_set NMI_Handler, Default_Handler
.weak HardFault_Handler
.thumb_set HardFault_Handler, Default_Handler
.weak MemManage_Handler
.thumb_set MemManage_Handler, Default_Handler
.weak BusFault_Handler
.thumb_set BusFault_Handler, Default_Handler
.weak UsageFault_Handler
.thumb_set UsageFault_Handler, Default_Handler
.weak SVC_Handler
.thumb_set SVC_Handler, Default_Handler
.weak DebugMon_Handler
.thumb_set DebugMon_Handler, Default_Handler
.weak PendSV_Handler
.thumb_set PendSV_Handler, Default_Handler

.section .text.Default_Handler, "ax", %progbits
.global Default_Handler
.type Default_Handler, %function
Default_Handler:
    b Default_Handler
.size Default_Handler, .-Default_Handler