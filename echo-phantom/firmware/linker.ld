/*
 * linker.ld — Linker Script for ECHO-Phantom (STM32H743VIT6)
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Memory layout:
 *   FLASH (Bank 1): 1 MB at 0x08000000 — firmware
 *   FLASH (Bank 2): 1 MB at 0x08080000 — inject clips
 *   DTCM:  128 KB at 0x20000000 — data, heap
 *   ITCM:  64 KB at 0x00000000 — time-critical code (optional)
 *   SRAM1: 368 KB at 0x30000000 — audio buffers
 *   SRAM2: 64 KB at 0x30040000 — DMA buffers
 *   SDRAM: 128 MB at 0xD0000000 — capture ring buffer
 */

ENTRY(Reset_Handler)

MEMORY
{
    ITCM  (rx)  : ORIGIN = 0x00000000, LENGTH = 64K
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
    DTCM  (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
    SRAM1 (xrw) : ORIGIN = 0x30000000, LENGTH = 368K
    SRAM2 (xrw) : ORIGIN = 0x30040000, LENGTH = 64K
    SDRAM (xrw) : ORIGIN = 0xD0000000, LENGTH = 128M
}

_estack = ORIGIN(DTCM) + LENGTH(DTCM);

SECTIONS
{
    /* Vector table — must be at the start of flash */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* Code */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)

        KEEP(*(.init))
        KEEP(*(.fini))

        . = ALIGN(4);
        _etext = .;
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

    /* Used by startup to initialize data */
    _sidata = LOADADDR(.data);

    /* Initialized data in DTCM */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > DTCM AT > FLASH

    /* Uninitialized data in DTCM */
    .bss :
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
    } > DTCM

    /* Audio buffers in SRAM1 (for DMA-accessible memory) */
    .sram1 (NOLOAD) :
    {
        . = ALIGN(32);
        _ssram1 = .;
        *(.sram1)
        *(.sram1*)
        . = ALIGN(32);
        _esram1 = .;
    } > SRAM1

    /* DMA buffers in SRAM2 */
    .sram2 (NOLOAD) :
    {
        . = ALIGN(32);
        _ssram2 = .;
        *(.sram2)
        *(.sram2*)
        . = ALIGN(32);
        _esram2 = .;
    } > SRAM2

    /* Heap grows upward */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        . = . + 0x4000;  /* 16 KB heap */
        . = ALIGN(8);
    } > DTCM

    /* Discard */
    /DISCARD/ :
    {
        *(.ARM.exidx*)
        *(.comment)
        *(.ARM.attributes)
    }
}