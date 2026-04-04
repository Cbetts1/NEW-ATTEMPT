; =============================================================================
; AI Aura OS — Kernel Entry Point (32-bit)
; File   : kernel/entry.asm
; Target : x86 (i386), NASM syntax
; Role   : First code executed after the bootloader jumps to 0x10000.
;          The bootloader has already established 32-bit protected mode
;          with a flat GDT, so this stub only reloads segment registers,
;          sets up the kernel stack, and calls kernel_main().
;          Never returns.
; =============================================================================

[BITS 32]

global _start
extern kernel_main

_start:
    ; ── Reload segment registers with the flat data selector (index 2) ───────
    mov  ax, 0x10
    mov  ds, ax
    mov  es, ax
    mov  fs, ax
    mov  gs, ax
    mov  ss, ax

    ; ── Kernel stack: grows down from 2 MB (heap base starts at 2 MB, grows up)
    mov  esp, 0x00200000

    ; ── Call the C kernel — never returns ────────────────────────────────────
    call kernel_main

.halt:
    cli
    hlt
    jmp  .halt

; Mark the stack as non-executable so the linker does not warn.
section .note.GNU-stack noalloc noexec nowrite progbits
