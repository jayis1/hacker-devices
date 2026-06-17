/**
 * stm32h743ii_flash.ld — STM32H743IIK6 Linker Script
 * Author: jayis1
 * License: Proprietary — Authorized Security Research Use Only
 *
 * Memory map for STM32H743IIK6 (BGA176, 2 MB flash, 1 MB SRAM)
 */

MEMORY
{
    FLASH   (rx)  : ORIGIN = 0x08000000, LENGTH = 2M
    DTCM    (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
    SRAM1   (rwx) : ORIGIN = 0x24000000, LENGTH = 512K
    SRAM2   (rwx) : ORIGIN = 0x30000000, LENGTH = 288K
    SRAM3   (rwx) : ORIGIN = 0x38000000, LENGTH = 64K
    SRAM4   (rwx) : ORIGIN = 0x3C000000, LENGTH = 64K
    BKPSRAM (rwx) : ORIGIN = 0x38800000, LENGTH = 4K
    ITCM    (rwx) : ORIGIN = 0x00000000, LENGTH = 64K
}

/* Stack and heap sizes */
__stack_size__  = 0x4000;   /* 16 KB stack */
__heap_size__   = 0x8000;   /* 32 KB heap */

/* Section definitions */
SECTIONS
{
    /* Vector table and code in flash */
    .vectors :
    {
        . = ALIGN(4);
        KEEP(*(.vectors))
        __Vectors_End = .;
        __Vectors_Size = __Vectors_End - __Vectors;
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

    _sidata = .;

    /* Initialized data in DTCM */
    .data : AT (_sidata)
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > DTCM

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

    /* SRAM1 for large buffers */
    .sram1_buffers (NOLOAD) :
    {
        . = ALIGN(4);
        *(.sram1_buffers)
        *(.sram1_buffers*)
        . = ALIGN(4);
    } > SRAM1

    /* SRAM4 for backup data */
    .sram4_bss (NOLOAD) :
    {
        . = ALIGN(4);
        *(.sram4_bss)
        *(.sram4_bss*)
        . = ALIGN(4);
    } > SRAM4

    /* Stack */
    .stack (COPY) :
    {
        . = ALIGN(8);
        . = . + __stack_size__;
        . = ALIGN(8);
        _estack = .;
    } > DTCM

    /* Heap for malloc */
    .heap (COPY) :
    {
        . = ALIGN(8);
        __end__ = .;
        PROVIDE(end = .);
        . = . + __heap_size__;
        . = ALIGN(8);
        _eheap = .;
    } > DTCM
}