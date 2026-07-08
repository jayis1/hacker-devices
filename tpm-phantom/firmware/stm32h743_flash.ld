/*
 * tpm-phantom — stm32h743_flash.ld
 * Linker script for STM32H743VIT6 (Cortex-M7, 2 MB Flash, 1 MB SRAM).
 *
 * Author: jayis1
 * License: GPL-2.0
 */

ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
    DTCM  (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
    SRAM  (rwx) : ORIGIN = 0x24000000, LENGTH = 512K
    SRAM2 (rwx) : ORIGIN = 0x30000000, LENGTH = 288K
    ITCM  (rwx) : ORIGIN = 0x00000000, LENGTH = 16K
}

_min_stack     = 0x1000;
_min_heap      = 0x1000;

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
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
    } > FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM AT > FLASH

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > SRAM

    .dtcm :
    {
        . = ALIGN(4);
        *(.dtcm)
        *(.dtcm*)
        . = ALIGN(4);
    } > DTCM

    .itcm :
    {
        . = ALIGN(4);
        *(.itcm)
        *(.itcm*)
        . = ALIGN(4);
    } > ITCM

    _heap_start = .;
    _heap_end = ORIGIN(SRAM) + LENGTH(SRAM) - _min_stack;
    _stack_start = _heap_end;
    _stack_end = ORIGIN(SRAM) + LENGTH(SRAM);

    PROVIDE(_heap_start = _heap_start);
    PROVIDE(_heap_end = _heap_end);
}