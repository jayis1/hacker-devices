/*
 * SILENT-SYMPHONY — Ultrasonic Covert Channel Communicator
 * STM32H743VIT6 Linker Script
 *
 * Memory map for the STM32H743 (2 MB Flash, 1 MB SRAM).
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

MEMORY
{
    FLASH   (rx)  : ORIGIN = 0x08000000, LENGTH = 2M
    RAM_D1  (rwx) : ORIGIN = 0x24000000, LENGTH = 512K  /* SRAM1 + SRAM2 (AXI domain) */
    RAM_D2  (rwx) : ORIGIN = 0x30000000, LENGTH = 288K  /* SRAM3 (AHB domain) */
    RAM_D3  (rwx) : ORIGIN = 0x38000000, LENGTH = 64K   /* SRAM4 (backup domain) */
    ITCM    (rwx) : ORIGIN = 0x00000000, LENGTH = 64K   /* ITCM RAM */
    DTCM    (rwx) : ORIGIN = 0x20000000, LENGTH = 128K  /* DTCM RAM */
}

/* Entry point */
ENTRY(_start)

/* Stack and heap sizes */
__stack_size   = 0x4000;  /* 16 KB stack */
__heap_size    = 0x8000;  /* 32 KB heap */

SECTIONS
{
    /* ====== ISR Vector Table ====== */
    .isr_vector : ALIGN(4)
    {
        __vectors_start = .;
        KEEP(*(.isr_vector))
        __vectors_end = .;
    } > FLASH

    /* ====== Program Code ====== */
    .text : ALIGN(4)
    {
        . = ALIGN(4);
        _stext = .;
        
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
        *(.glue_7)
        *(.glue_7t)
        
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* ====== Constructor/Destructor Tables ====== */
    .preinit_array : ALIGN(4)
    {
        __preinit_array_start = .;
        KEEP(*(.preinit_array))
        __preinit_array_end = .;
    } > FLASH

    .init_array : ALIGN(4)
    {
        __init_array_start = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = .;
    } > FLASH

    .fini_array : ALIGN(4)
    {
        __fini_array_start = .;
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        __fini_array_end = .;
    } > FLASH

    /* ====== Initialized Data (LMA in Flash, VMA in RAM) ====== */
    _sidata = .;

    .data : ALIGN(4)
    {
        . = ALIGN(4);
        _sdata = .;
        
        *(.data)
        *(.data.*)
        
        . = ALIGN(4);
        _edata = .;
    } > RAM_D1 AT > FLASH

    /* ====== Uninitialized Data (BSS) ====== */
    .bss (NOLOAD) : ALIGN(4)
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = .;
        
        *(.bss)
        *(.bss.*)
        *(COMMON)
        
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = .;
    } > RAM_D1

    /* ====== Heap ====== */
    .heap (NOLOAD) : ALIGN(8)
    {
        . = ALIGN(8);
        __heap_start__ = .;
        . = . + __heap_size;
        __heap_end__ = .;
    } > RAM_D1

    /* ====== Stack ====== */
    .stack (NOLOAD) : ALIGN(8)
    {
        . = ALIGN(8);
        __stack_start__ = .;
        . = . + __stack_size;
        __stack_end__ = .;
    } > RAM_D1

    /* ====== I²S Audio Buffers in DTCM for Fast Access ====== */
    .audio_dtcm (NOLOAD) : ALIGN(32)
    {
        *(.audio_buffers)
        *(.audio_buffers.*)
    } > DTCM

    /* ====== DMA-safe buffers in SRAM3 (D2 domain) ====== */
    .dma_ram (NOLOAD) : ALIGN(32)
    {
        *(.dma_ram)
        *(.dma_ram.*)
    } > RAM_D2

    /* ====== Discard unused sections ====== */
    /DISCARD/ :
    {
        *(.ARM.exidx*)
        *(.ARM.attributes*)
        *(.comment)
        *(.debug*)
    }
}

/* Top-of-stack for initial SP */
PROVIDE(_estack = ORIGIN(RAM_D1) + LENGTH(RAM_D1));