; =============================================================================
; AI Aura OS — Bootloader
; File   : bootloader/boot.asm
; Target : x86 (i386), MBR (512 bytes), NASM syntax
; Role   : Stage-1 boot.  Prints startup banner, sets up a minimal real-mode
;          stack, then loads the kernel from the sectors immediately following
;          the MBR on the disk and transfers control to it.
; =============================================================================

[BITS 16]
[ORG 0x7C00]

; ── Startup ──────────────────────────────────────────────────────────────────
start:
    cli                         ; disable interrupts during init
    xor  ax, ax
    mov  ds, ax
    mov  es, ax
    mov  ss, ax
    mov  sp, 0x7C00             ; stack grows downward from 0x7C00
    sti                         ; re-enable interrupts

    ; print banner
    mov  si, msg_boot
    call print_str

    ; ── Load kernel sectors from disk ────────────────────────────────────────
    ; BIOS loads kernel at 0x1000:0x0000 (linear 0x10000)
    mov  ax, 0x1000
    mov  es, ax
    xor  bx, bx                 ; ES:BX = 0x1000:0x0000

    mov  ah, 0x02               ; BIOS read sectors
    mov  al, KERNEL_SECTORS     ; number of sectors to read
    mov  ch, 0                  ; cylinder 0
    mov  cl, 2                  ; starting sector 2 (sector 1 = MBR)
    mov  dh, 0                  ; head 0
    ; DL = boot drive (preserved from BIOS)
    int  0x13
    jc   disk_error

    mov  si, msg_loaded
    call print_str

    ; ── Jump to kernel entry ──────────────────────────────────────────────────
    jmp  0x1000:0x0000

; ── Disk error handler ────────────────────────────────────────────────────────
disk_error:
    mov  si, msg_error
    call print_str
    cli
    hlt

; ── Subroutine: print null-terminated string via BIOS TTY ────────────────────
print_str:
    lodsb
    or   al, al
    jz   .done
    mov  ah, 0x0E
    mov  bh, 0x00
    int  0x10
    jmp  print_str
.done:
    ret

; ── Data ─────────────────────────────────────────────────────────────────────
msg_boot   db  13, 10, "AI Aura OS Booting...", 13, 10, 0
msg_loaded db  "Kernel loaded. Transferring control.", 13, 10, 0
msg_error  db  "DISK ERROR - halted.", 13, 10, 0

KERNEL_SECTORS equ 64           ; load up to 64 x 512 = 32 KB of kernel

; ── MBR padding + boot signature ─────────────────────────────────────────────
times 510-($-$$) db 0
dw    0xAA55
