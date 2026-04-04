; =============================================================================
; AI Aura OS — Kernel Entry Point
; File   : kernel/entry.asm
; Target : x86 (i386), NASM syntax
; Role   : Entered from bootloader in real mode (16-bit).
;          Sets up the GDT, switches to 32-bit protected mode, establishes
;          segment registers and a proper C-compatible stack, then calls
;          kernel_main().  Never returns.
; =============================================================================

[BITS 16]
[ORG 0x0000]                    ; loaded at 0x1000:0x0000

global _start
extern kernel_main

_start:
    cli

    ; ── Load GDT ─────────────────────────────────────────────────────────────
    lgdt [gdt_descriptor]

    ; ── Enable protected mode (set PE bit in CR0) ────────────────────────────
    mov  eax, cr0
    or   eax, 0x1
    mov  cr0, eax

    ; ── Far-jump to flush prefetch queue and enter 32-bit code segment ───────
    jmp  0x08:protected_mode_entry

; =============================================================================
[BITS 32]
protected_mode_entry:
    ; set all data segments to the flat data descriptor (index 2, RPL 0)
    mov  ax, 0x10
    mov  ds, ax
    mov  es, ax
    mov  fs, ax
    mov  gs, ax
    mov  ss, ax

    ; set up kernel stack (grows down from 2 MB mark)
    mov  esp, 0x200000

    ; call the C kernel
    call kernel_main

    ; should never return, but halt just in case
.halt:
    cli
    hlt
    jmp  .halt

; =============================================================================
; GDT — three entries: null, code (ring-0 flat), data (ring-0 flat)
; =============================================================================
gdt_start:

; entry 0 — null descriptor
    dd 0x00000000
    dd 0x00000000

; entry 1 — code segment: base=0, limit=4GB, 32-bit, ring-0, execute/read
    dw 0xFFFF           ; limit[15:0]
    dw 0x0000           ; base[15:0]
    db 0x00             ; base[23:16]
    db 10011010b        ; P=1 DPL=00 S=1 Type=1010 (exec/read)
    db 11001111b        ; G=1 D/B=1 L=0 AVL=0 limit[19:16]=1111
    db 0x00             ; base[31:24]

; entry 2 — data segment: base=0, limit=4GB, 32-bit, ring-0, read/write
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b        ; P=1 DPL=00 S=1 Type=0010 (read/write)
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; limit (size - 1)
    dd gdt_start                ; base address
