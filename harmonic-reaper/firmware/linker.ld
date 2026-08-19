/*
 * linker.ld — GNU linker script for nRF52840 + S140 SoftDevice
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The S140 SoftDevice occupies flash 0x0000..0x27000 and RAM 0x20000000..0x20008000.
 * The application starts at 0x27000 (flash) and uses RAM from 0x20008000.
 */
MEMORY
{
    FLASH (rx) : ORIGIN = 0x27000, LENGTH = 256K
    RAM   (rwx): ORIGIN = 0x20008000, LENGTH = 0x20008000 - 0x20008000  /* fix below */
}

/* Correct RAM layout: SoftDevice uses 0x20000000..0x20008000 (32 KB),
 * leaving 224 KB for the app (0x20008000..0x20040000). */
FLASH = 0x00027000;
FLASH_LENGTH = 0x100000 - 0x27000;  /* 256 KB reserved for app + padding */
RAM = 0x20008000;
RAM_LENGTH = 0x20040000 - 0x20008000;  /* 224 KB */

ENTRY(Reset_Handler)

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
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    .ARM.exidx :
    {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > FLASH

    _sidata = LOADADDR(.data);

    .data : AT (ADDR(.text) + SIZEOF(.text))
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        _estack = .;
        . = . + 0x2000;   /* 8 KB stack */
        . = ALIGN(8);
        _end = .;
    } > RAM

    PROVIDE(__stack_top = ORIGIN(RAM) + LENGTH(RAM));
}

/* EOF — linker.ld — jayis1 */