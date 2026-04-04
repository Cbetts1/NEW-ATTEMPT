/*
 * AI Aura OS — Kernel Linker Script
 * Places kernel at physical 0x10000 (segment 0x1000, offset 0x0000)
 * in a flat binary with .text first, then .rodata, .data, .bss.
 */

OUTPUT_FORMAT("binary")
OUTPUT_ARCH(i386)
ENTRY(kernel_main)

SECTIONS
{
    . = 0x10000;

    .text :
    {
        *(.text)
    }

    .rodata :
    {
        *(.rodata)
        *(.rodata.*)
    }

    .data :
    {
        *(.data)
    }

    .bss :
    {
        *(COMMON)
        *(.bss)
    }
}
