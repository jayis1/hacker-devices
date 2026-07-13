/*
 * eddy-phantom — startup_stm32h743.s
 * Startup code and interrupt vector table for STM32H743
 *
 * Author: jayis1
 * License: GPL-2.0
 */

    .syntax unified
    .cpu cortex-m7
    .fpu fpv5-d16
    .thumb

    .section .isr_vector, "a", %progbits
    .align 2
    .global g_pfnVectors
    .type g_pfnVectors, %object

g_pfnVectors:
    .word _estack                 /* 0: Initial stack pointer */
    .word Reset_Handler           /* 1: Reset */
    .word NMI_Handler             /* 2: NMI */
    .word HardFault_Handler       /* 3: Hard fault */
    .word MemManage_Handler       /* 4: MemManage fault */
    .word BusFault_Handler        /* 5: Bus fault */
    .word UsageFault_Handler     /* 6: Usage fault */
    .word 0                       /* 7: Reserved */
    .word 0                       /* 8: Reserved */
    .word 0                       /* 9: Reserved */
    .word 0                       /* 10: Reserved */
    .word SVC_Handler             /* 11: SVCall */
    .word DebugMon_Handler       /* 12: Debug monitor */
    .word 0                       /* 13: Reserved */
    .word PendSV_Handler         /* 14: PendSV */
    .word SysTick_Handler        /* 15: SysTick */

    /* External interrupts — STM32H743 */
    .word WWDG_IRQHandler         /* 16  */
    .word PVD_AVD_IRQHandler      /* 17  */
    .word TAMP_STAMP_IRQHandler   /* 18  */
    .word RTC_WKUP_IRQHandler     /* 19  */
    .word FLASH_IRQHandler        /* 20  */
    .word RCC_IRQHandler          /* 21  */
    .word EXTI0_IRQHandler        /* 22  */
    .word EXTI1_IRQHandler        /* 23  */
    .word EXTI2_IRQHandler        /* 24  */
    .word EXTI3_IRQHandler        /* 25  */
    .word EXTI4_IRQHandler        /* 26  */
    .word DMA1_Stream0_IRQHandler /* 27  */
    .word DMA1_Stream1_IRQHandler /* 28  */
    .word DMA1_Stream2_IRQHandler /* 29  */
    .word DMA1_Stream3_IRQHandler /* 30  */
    .word DMA1_Stream4_IRQHandler /* 31  */
    .word DMA1_Stream5_IRQHandler /* 32  */
    .word DMA1_Stream6_IRQHandler /* 33  */
    .word ADC_IRQHandler          /* 34  */
    .word FDCAN1_IT0_IRQHandler   /* 35  */
    .word FDCAN1_IT1_IRQHandler   /* 36  */
    .word FDCAN2_IT0_IRQHandler   /* 37  */
    .word FDCAN2_IT1_IRQHandler   /* 38  */
    .word FDCAN3_IT0_IRQHandler   /* 39  */
    .word FDCAN3_IT1_IRQHandler   /* 40  */
    .word ETH_IRQHandler          /* 41  */
    .word ETH_WKUP_IRQHandler     /* 42  */
    .word FDCAN_CAL_IRQHandler    /* 43  */
    .word 0                       /* 44  Reserved */
    .word 0                       /* 45  Reserved */
    .word 0                       /* 46  Reserved */
    .word 0                       /* 47  Reserved */
    .word DMA2_Stream0_IRQHandler /* 48  */
    .word DMA2_Stream1_IRQHandler /* 49  */
    .word DMA2_Stream2_IRQHandler /* 50  */
    .word DMA2_Stream3_IRQHandler /* 51  */
    .word DMA2_Stream4_IRQHandler /* 52  */
    .word DMA2_Stream5_IRQHandler /* 53  */
    .word DMA2_Stream6_IRQHandler /* 54  */
    .word DMA2_Stream7_IRQHandler /* 55  */
    .word USART6_IRQHandler       /* 56  */
    .word I2C1_EV_IRQHandler      /* 57  */
    .word I2C1_ER_IRQHandler      /* 58  */
    .word I2C2_EV_IRQHandler      /* 59  */
    .word I2C2_ER_IRQHandler      /* 60  */
    .word SPI3_IRQHandler         /* 61  */
    .word UART4_IRQHandler        /* 62  */
    .word UART5_IRQHandler        /* 63  */
    .word USART2_IRQHandler       /* 64  */
    .word USART3_IRQHandler       /* 65  */
    .word EXTI9_5_IRQHandler      /* 66  */
    .word SPI1_IRQHandler         /* 67  */
    .word SPI2_IRQHandler         /* 68  */
    .word SPI4_IRQHandler         /* 69  */
    .word 0                       /* 70  Reserved */
    .word 0                       /* 71  Reserved */
    .word TIM12_IRQHandler        /* 72  */
    .word TIM13_IRQHandler        /* 73  */
    .word TIM14_IRQHandler        /* 74  */
    .word TIM8_UP_IRQHandler      /* 75  */
    .word TIM8_TRG_COM_IRQHandler /* 76  */
    .word TIM8_CC_IRQHandler      /* 77  */
    .word DMA1_Stream7_IRQHandler /* 78  */
    .word FMC_IRQHandler          /* 79  */
    .word SDMMC1_IRQHandler       /* 80  */
    .word TIM5_IRQHandler         /* 81  */
    .word SPI3_IRQHandler         /* 82  */
    .word UART4_IRQHandler        /* 83  */
    .word UART5_IRQHandler        /* 84  */
    .word TIM6_DAC_IRQHandler     /* 85  */
    .word TIM7_IRQHandler         /* 86  */
    .word DMA2_Stream0_IRQHandler /* 87  */
    .word 0                       /* 88  */
    .word 0                       /* 89  */
    .word 0                       /* 90  */
    .word 0                       /* 91  */
    .word 0                       /* 92  */
    .word 0                       /* 93  */
    .word 0                       /* 94  */
    .word 0                       /* 95  */
    .word 0                       /* 96  */
    .word 0                       /* 97  */
    .word 0                       /* 98  */
    .word 0                       /* 99  */
    .word 0                       /*100  */
    .word 0                       /*101  */
    .word 0                       /*102  */
    .word 0                       /*103  */
    .word 0                       /*104  */
    .word 0                       /*105  */
    .word 0                       /*106  */
    .word 0                       /*107  */
    .word 0                       /*108  */
    .word 0                       /*109  */
    .word 0                       /*110  */
    .word 0                       /*111  */
    .word 0                       /*112  */
    .word 0                       /*113  */
    .word 0                       /*114  */
    .word 0                       /*115  */
    .word 0                       /*116  */
    .word 0                       /*117  */
    .word 0                       /*118  */
    .word 0                       /*119  */
    .word 0                       /*120  */
    .word 0                       /*121  */
    .word 0                       /*122  */
    .word 0                       /*123  */
    .word 0                       /*124  */
    .word 0                       /*125  */
    .word 0                       /*126  */
    .word 0                       /*127  */
    .word 0                       /*128  */
    .word 0                       /*129  */
    .word 0                       /*130  */
    .word 0                       /*131  */
    .word 0                       /*132  */
    .word 0                       /*133  */
    .word 0                       /*134  */
    .word 0                       /*135  */
    .word 0                       /*136  */
    .word 0                       /*137  */
    .word 0                       /*138  */
    .word 0                       /*139  */
    .word 0                       /*140  */
    .word 0                       /*141  */
    .word 0                       /*142  */
    .word 0                       /*143  */
    .word EXTI15_10_IRQHandler    /*144  */

    .size g_pfnVectors, . - g_pfnVectors

    .section .text.Reset_Handler, "ax", %progbits
    .weak Reset_Handler
    .type Reset_Handler, %function
Reset_Handler:
    /* Disable interrupts */
    cpsid i

    /* Copy .data from flash to RAM */
    ldr r0, =_sidata
    ldr r1, =_sdata
    ldr r2, =_edata
copy_data:
    cmp r1, r2
    bcs copy_data_done
    ldr r3, [r0], #4
    str r3, [r1], #4
    b copy_data
copy_data_done:

    /* Zero .bss */
    ldr r1, =_sbss
    ldr r2, =_ebss
    movs r3, #0
zero_bss:
    cmp r1, r2
    bcs zero_bss_done
    str r3, [r1], #4
    b zero_bss
zero_bss_done:

    /* Enable FPU before main */
    ldr r0, =0xE000ED88
    ldr r1, [r0]
    orr r1, r1, #(0xF << 20)
    str r1, [r0]
    dsb
    isb

    /* Enable interrupts */
    cpsie i

    /* Branch to main */
    bl main

    /* If main returns, loop forever */
infinite_loop:
    b infinite_loop
    .size Reset_Handler, . - Reset_Handler

    /* Default handlers */
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
    .weak SysTick_Handler
    .thumb_set SysTick_Handler, Default_Handler

    .weak WWDG_IRQHandler
    .thumb_set WWDG_IRQHandler, Default_Handler
    .weak PVD_AVD_IRQHandler
    .thumb_set PVD_AVD_IRQHandler, Default_Handler
    .weak TAMP_STAMP_IRQHandler
    .thumb_set TAMP_STAMP_IRQHandler, Default_Handler
    .weak RTC_WKUP_IRQHandler
    .thumb_set RTC_WKUP_IRQHandler, Default_Handler
    .weak FLASH_IRQHandler
    .thumb_set FLASH_IRQHandler, Default_Handler
    .weak RCC_IRQHandler
    .thumb_set RCC_IRQHandler, Default_Handler
    .weak EXTI0_IRQHandler
    .thumb_set EXTI0_IRQHandler, Default_Handler
    .weak EXTI1_IRQHandler
    .thumb_set EXTI1_IRQHandler, Default_Handler
    .weak EXTI2_IRQHandler
    .thumb_set EXTI2_IRQHandler, Default_Handler
    .weak EXTI3_IRQHandler
    .thumb_set EXTI3_IRQHandler, Default_Handler
    .weak EXTI4_IRQHandler
    .thumb_set EXTI4_IRQHandler, Default_Handler
    .weak EXTI9_5_IRQHandler
    .thumb_set EXTI9_5_IRQHandler, Default_Handler
    .weak EXTI15_10_IRQHandler
    .thumb_set EXTI15_10_IRQHandler, Default_Handler
    .weak DMA1_Stream0_IRQHandler
    .thumb_set DMA1_Stream0_IRQHandler, Default_Handler
    .weak DMA1_Stream1_IRQHandler
    .thumb_set DMA1_Stream1_IRQHandler, Default_Handler
    .weak DMA1_Stream2_IRQHandler
    .thumb_set DMA1_Stream2_IRQHandler, Default_Handler
    .weak DMA1_Stream3_IRQHandler
    .thumb_set DMA1_Stream3_IRQHandler, Default_Handler
    .weak DMA1_Stream4_IRQHandler
    .thumb_set DMA1_Stream4_IRQHandler, Default_Handler
    .weak SPI1_IRQHandler
    .thumb_set SPI1_IRQHandler, Default_Handler
    .weak SPI2_IRQHandler
    .thumb_set SPI2_IRQHandler, Default_Handler
    .weak SPI4_IRQHandler
    .thumb_set SPI4_IRQHandler, Default_Handler
    .weak USART2_IRQHandler
    .thumb_set USART2_IRQHandler, Default_Handler
    .weak USART3_IRQHandler
    .thumb_set USART3_IRQHandler, Default_Handler
    .weak FMC_IRQHandler
    .thumb_set FMC_IRQHandler, Default_Handler

    .section .text.Default_Handler, "ax", %progbits
Default_Handler:
    b Default_Handler
    .size Default_Handler, . - Default_Handler

    .end