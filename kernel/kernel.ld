/*
 * AI Aura OS — Kernel Linker Script
 * Places kernel at physical 0x10000 (segment 0x1000, offset 0x0000).
 * Outputs ELF so that objcopy can strip it to a flat binary.
 */

OUTPUT_FORMAT("elf32-i386")
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
