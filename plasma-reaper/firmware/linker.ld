/*
 * linker.ld — Linker script for PlasmaReaper (STM32H723ZGT6)
 * Author: jayis1
 * License: GPL-2.0
 *
 * STM32H723 memory map:
 *   FLASH: 1 MB at 0x08000000
 *   ITCM:  64 KB at 0x00000000
 *   DTCM:  128 KB at 0x20000000
 *   SRAM:  320 KB at 0x24000000 (AXI SRAM)
 *   SRAM1: 16 KB at 0x30000000
 *   SRAM2: 16 KB at 0x30020000
 *   SRAM4: 16 KB at 0x38000000
 */

ENTRY(Reset_Handler)

/* Stack starts at top of DTCM */
_stack_top = ORIGIN(DTCM) + LENGTH(DTCM);

MEMORY
{
    ITCM  (rx)  : ORIGIN = 0x00000000, LENGTH = 64K
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
    DTCM  (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
    SRAM  (rwx) : ORIGIN = 0x24000000, LENGTH = 320K
}

SECTIONS
{
    /* Reset vector and ISR table at the start of FLASH */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* Code — copy to ITCM for zero-wait execution */
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

    /* ARM exception unwinding tables */
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

    /* Data — copied from FLASH to DTCM at boot */
    _sidata = LOADADDR(.data);
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > DTCM AT > FLASH

    /* BSS — zero-initialized data in DTCM */
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

    /* Heap in SRAM (for dynamic allocation if needed) */
    .heap :
    {
        . = ALIGN(8);
        _sheap = .;
        . = . + 0x10000; /* 64 KB heap */
        . = ALIGN(8);
        _eheap = .;
    } > SRAM

    /* Stack in DTCM */
    .stack :
    {
        . = ALIGN(8);
        _sstack = .;
        . = . + 0x2000; /* 8 KB stack */
        . = ALIGN(8);
        _estack = .;
    } > DTCM

    /* DMA buffers in SRAM (non-cacheable region) */
    .dma (NOLOAD) :
    {
        . = ALIGN(4);
        *(.dma)
        *(.dma*)
        . = ALIGN(4);
    } > SRAM

    /* Drop debugging info that we don't need in the binary */
    /DISCARD/ :
    {
        *(.ARM.attributes)
        *(.comment)
    }
}

/* Author: jayis1 */