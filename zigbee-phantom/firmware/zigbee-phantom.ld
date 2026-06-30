/*
 * zigbee-phantom.ld — Linker script for CC2652R1F
 * Author: jayis1
 * License: GPL-2.0
 *
 * CC2652R1F memory map:
 *   FLASH: 512 KB at 0x00000000
 *   SRAM:  128 KB at 0x20000000
 *   The first 0x100 bytes are the interrupt vector table.
 */

ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 512K
    SRAM  (rwx): ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS
{
    .isr_vector :
    {
        KEEP(*(.isr_vector))
    } > FLASH

    .text :
    {
        *(.text*)
        *(.rodata*)
        _etext = .;
    } > FLASH

    .ARM.exidx :
    {
        *(.ARM.exidx*)
    } > FLASH

    .data :
    {
        _sdata = .;
        *(.data*)
        _edata = .;
    } > SRAM AT > FLASH

    .bss (NOLOAD) :
    {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > SRAM

    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        _estack = ORIGIN(SRAM) + LENGTH(SRAM);
    } > SRAM

    PROVIDE(_estack = ORIGIN(SRAM) + LENGTH(SRAM));
}