/**
 * photonstrike.ld — linker script for STM32H757 M7 image
 * Author: jayis1
 * License: GPL-2.0
 *
 * The M7 image occupies the first 896 kB of flash (0x08000000).
 * The M4 image is appended at 0x080E0000 (last 128 kB) by the Makefile.
 */

ENTRY(Reset_Handler)

MEMORY
{
    ITCM  (rx) : ORIGIN = 0x00000000, LENGTH = 64K
    DTCM  (rw) : ORIGIN = 0x20000000, LENGTH = 128K
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 896K
    RAM   (rwx): ORIGIN = 0x24000000, LENGTH = 512K   /* AXI SRAM */
    D3SRAM(rw) : ORIGIN = 0x38000000, LENGTH = 64K    /* M4 mailbox */
}

_estack = ORIGIN(DTCM) + LENGTH(DTCM);

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
        . = ALIGN(4);
        _etext = .;
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

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    _sidata = LOADADDR(.data);

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = .;
    } > RAM

    .d3sram (NOLOAD) :
    {
        . = ALIGN(4);
        *(.d3sram)
        . = ALIGN(4);
    } > D3SRAM

    _end = .;
    PROVIDE(end = .);
}