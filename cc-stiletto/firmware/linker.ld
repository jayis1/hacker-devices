/*
 * linker.ld — GNU linker script for CC-Stiletto on STM32G474CEU6 (512 KB flash,
 * 128 KB SRAM). Places .text at 0x08000000, vectors first, and splits the
 * 128 KB SRAM between .data/.bss and a 16 KB stack at the top.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text.Reset_Handler)
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .data : AT (_etext)
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    .stack :
    {
        . = ALIGN(8);
        . = . + 0x4000;   /* 16 KB stack */
        . = ALIGN(8);
        _estack_bottom = .;
    } > RAM

    /DISCARD/ :
    {
        *(.ARM.exidx*)
        *(.comment)
    }
}