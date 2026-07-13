/*
 * eddy-phantom — link.ld
 * Linker script for STM32H743VIT6
 * Flash: 2 MB at 0x08000000, RAM: 1 MB at 0x20000000,
 * SDRAM: 32 MB at 0xC0000000 (FMC), QSPI: 16 MB at 0x90000000
 *
 * Author: jayis1
 * License: GPL-2.0
 */

ENTRY(Reset_Handler)

MEMORY
{
    ITCM  (rx)  : ORIGIN = 0x00000000, LENGTH = 64K
    DTCM  (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
    RAM   (rwx) : ORIGIN = 0x24000000, LENGTH = 896K
    SRAM2 (rwx) : ORIGIN = 0x30000000, LENGTH = 32K
    SRAM3 (rwx) : ORIGIN = 0x30020000, LENGTH = 16K
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
    SDRAM (rwx) : ORIGIN = 0xC0000000, LENGTH = 32M
    QSPI  (rx)  : ORIGIN = 0x90000000, LENGTH = 16M
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
        KEEP(*(.init))
        KEEP(*(.fini))
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

    .heap_stack :
    {
        . = ALIGN(8);
        . = . + 0x4000;     /* 16 KB heap */
        . = ALIGN(8);
        . = . + 0x2000;     /* 8 KB stack overflow guard */
    } > RAM

    _sidata = LOADADDR(.data);

    .sdram (NOLOAD) :
    {
        . = ALIGN(4);
        _ssdram = .;
        *(.sdram)
        *(.sdram*)
        . = ALIGN(4);
        _esdram = .;
    } > SDRAM

    .qspi (NOLOAD) :
    {
        . = ALIGN(4);
        *(.qspi)
        *(.qspi*)
        . = ALIGN(4);
    } > QSPI
}