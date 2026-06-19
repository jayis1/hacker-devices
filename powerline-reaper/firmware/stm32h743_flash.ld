/*
 * stm32h743_flash.ld — linker script for STM32H743VIT6
 * Flash at 0x08000000 (1 MB), DTCM 128 KB, SRAM 512 KB + BKPSRAM 4 KB
 *
 * Author: jayis1
 * License: GPL-2.0
 */

ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
    DTCM  (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
    SRAM  (rwx) : ORIGIN = 0x24000000, LENGTH = 512K
    BKPSRAM (rw): ORIGIN = 0x38800000, LENGTH = 4K
}

_stack_top = ORIGIN(DTCM) + LENGTH(DTCM);
_stack_size = 0x2000;

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
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .ARM.extab   : { *(.ARM.extab* .gnu*) } > FLASH
    .ARM.exidx   : { *(.ARM.exidx*) } > FLASH

    .data : AT (_etext)
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data.*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > SRAM

    .bkpsram (NOLOAD) :
    {
        . = ALIGN(4);
        *(.bkpsram)
        . = ALIGN(4);
    } > BKPSRAM

    _end = .;
    PROVIDE(end = .);
}

PROVIDE(_stack_top = _stack_top);