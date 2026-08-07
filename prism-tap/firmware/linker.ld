/*
 * linker.ld — Linker Script for Prism-Tap
 * STM32H730VB — Cortex-M7 @ 550 MHz
 *
 * Author: jayis1
 * License: GPL-2.0
 */

ENTRY(main)

/* ---- Memory layout ---- */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
    DTCM  (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
    SRAM1 (rwx) : ORIGIN = 0x24000000, LENGTH = 128K
    SRAM2 (rwx) : ORIGIN = 0x24020000, LENGTH = 16K
    SRAM3 (rwx) : ORIGIN = 0x24024000, LENGTH = 16K
}

/* ---- Stack configuration ---- */
_stack_top = ORIGIN(DTCM) + LENGTH(DTCM);
_heap_base = ORIGIN(SRAM2);

/* ---- Sections ---- */
SECTIONS
{
    /* ---- Vector table (at flash start) ---- */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* ---- Code ---- */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        . = ALIGN(4);
    } > FLASH

    /* ---- Read-only data ---- */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* ---- ARM attributes ---- */
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

    /* ---- Preinit array ---- */
    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH

    /* ---- Init array ---- */
    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH

    /* ---- Fini array ---- */
    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    /* ---- SIDATA (init data source in flash) ---- */
    _sidata = LOADADDR(.data);

    /* ---- Initialized data (in DTCM) ---- */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > DTCM AT > FLASH

    /* ---- BSS (uninitialized data in DTCM) ---- */
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
    } > DTCM

    /* ---- Large frame buffers in SRAM1 ---- */
    .sram1_data (NOLOAD) :
    {
        . = ALIGN(32);
        *(.sram1_data)
        *(.sram1_data*)
        . = ALIGN(32);
    } > SRAM1

    /* ---- Heap in SRAM2 ---- */
    .heap (NOLOAD) :
    {
        . = ALIGN(8);
        _sheap = .;
        . = . + 0x2000;
        . = ALIGN(8);
        _eheap = .;
    } > SRAM2

    /* ---- Stack ---- */
    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        _sstack = .;
        . = . + 0x4000;
        . = ALIGN(8);
        _estack = .;
    } > DTCM

    /* ---- Discard debug info (optional) ---- */
    /DISCARD/ :
    {
        *(.ARM.attributes)
        *(.comment)
    }
}

/* ---- End of linker.ld ----
 * Author: jayis1
 */