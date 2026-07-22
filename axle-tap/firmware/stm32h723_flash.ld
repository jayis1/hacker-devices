/*
 * stm32h723_flash.ld — AxleTap linker script
 * Author: jayis1
 * License: GPL-2.0
 * Date:   2026-07-22
 *
 * STM32H723ZGT6 memory map:
 *   Flash  2 MB @ 0x08000000
 *   ITCM   64 KB @ 0x00000000 (bridge + gPTP code)
 *   DTCM   128 KB @ 0x20000000 (DMA descriptors, frame rings)
 *   AXI SRAM 1 MB @ 0x24000000 (frame buffers, PCAP, service map)
 *   SRAM1..3 @ 0x30000000 (backup, BLE buffers)
 */

ENTRY(Reset_Handler)

MEMORY
{
    ITCM   (rx)  : ORIGIN = 0x00000000, LENGTH = 64K
    DTCM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
    FLASH  (rx)  : ORIGIN = 0x08000000, LENGTH = 2M
    RAM    (rwx) : ORIGIN = 0x24000000, LENGTH = 1M
    SRAM2  (rwx) : ORIGIN = 0x30020000, LENGTH = 32K
}

/* Stack at top of DTCM */
_estack = ORIGIN(DTCM) + LENGTH(DTCM);

/* Sections placed in ITCM */
SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .itcm_text :
    {
        . = ALIGN(4);
        *(.itcm)
        . = ALIGN(4);
    } > ITCM AT> FLASH

    .dtcm_data :
    {
        . = ALIGN(4);
        *(.dtcm)
        . = ALIGN(4);
    } > DTCM AT> FLASH

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

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT> FLASH

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

    /* Frame buffers in AXI SRAM (1 MB) */
    .axi_sram (NOLOAD) :
    {
        . = ALIGN(32);
        *(.axi_sram)
        *(.axi_sram*)
    } > RAM
}

_sidata = LOADADDR(.data);
_sitcm  = LOADADDR(.itcm_text);
_sdtcm  = LOADADDR(.dtcm_data);

PROVIDE(end = .);
PROVIDE(_end = .);