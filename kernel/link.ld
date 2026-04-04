/* =============================================================================
 * AI Aura OS — Linker Script
 * File   : kernel/link.ld
 * Role   : Places entry.o first so the CPU executes the ASM entry stub before
 *          any C code.  All sections are mapped to a flat 32-bit address space
 *          starting at the kernel load address (0x10000 = 0x1000:0x0000).
 * =========================================================================== */

ENTRY(_start)

SECTIONS
{
    /* Kernel is loaded by the bootloader at linear address 0x10000 */
    . = 0x10000;

    /* ── Code ─────────────────────────────────────────────────────────────── */
    .text :
    {
        /* entry.o MUST be first so _start is at exactly 0x10000 */
        build/entry.o(.text)
        *(.text*)
    }

    /* ── Read-only data ───────────────────────────────────────────────────── */
    .rodata :
    {
        *(.rodata*)
    }

    /* ── Initialized data ─────────────────────────────────────────────────── */
    .data :
    {
        *(.data*)
    }

    /* ── BSS (zero-initialised) ───────────────────────────────────────────── */
    .bss :
    {
        *(COMMON)
        *(.bss*)
    }
}
