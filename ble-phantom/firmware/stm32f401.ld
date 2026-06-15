/* STM32F401 Linker Script for BLE Phantom */

MEMORY
{
    FLASH (rx)   : ORIGIN = 0x08000000, LENGTH = 256K
    SRAM (rwx)   : ORIGIN = 0x20000000, LENGTH = 64K
    CCM  (rwx)   : ORIGIN = 0x10000000, LENGTH = 16K
}

/* Stack at end of SRAM */
_estack = ORIGIN(SRAM) + LENGTH(SRAM);

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
        . = ALIGN(4);
    } > FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM AT > FLASH

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > SRAM

    .ccm_buffer (NOLOAD) :
    {
        . = ALIGN(4);
        _sccm = .;
        *(.ccm_buffer)
        . = ALIGN(4);
        _eccm = .;
    } > CCM

    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        . = . + 0x400;  /* Heap: 1 KB */
        . = . + 0x400;  /* Stack: 1 KB (minimum, stack grows downward from _estack) */
        . = ALIGN(8);
    } > SRAM
}

/* Entry point */
ENTRY(Reset_Handler)