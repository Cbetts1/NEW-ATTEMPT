; =============================================================================
; AI Aura OS — Stage 1 Bootloader (MBR)
; Assembler: NASM
; Target: x86 real mode, loaded by BIOS at 0x7C00
; Responsibility: Print banner, load kernel, enter protected mode, jump to kernel
; =============================================================================

[BITS 16]
[ORG 0x7C00]

KERNEL_LOAD_SEG  equ 0x1000      ; Physical 0x10000
KERNEL_SECTORS   equ 64          ; 32 KB of kernel
STACK_TOP        equ 0x7C00

start:
    cli
    xor     ax, ax
    mov     ds, ax
    mov     es, ax
    mov     ss, ax
    mov     sp, STACK_TOP
    sti

    ; Save boot drive
    mov     [boot_drive], dl

    ; Print banner
    mov     si, msg_boot
    call    print_str

    ; Load kernel from disk into KERNEL_LOAD_SEG:0
    mov     ax, KERNEL_LOAD_SEG
    mov     es, ax
    xor     bx, bx

    mov     ah, 0x02            ; BIOS: read sectors
    mov     al, KERNEL_SECTORS
    mov     ch, 0               ; Cylinder 0
    mov     cl, 2               ; Start at LBA sector 2 (sector index 1-based)
    mov     dh, 0               ; Head 0
    mov     dl, [boot_drive]
    int     0x13
    jc      .disk_error

    mov     si, msg_loaded
    call    print_str

    ; Disable interrupts before mode switch
    cli

    ; Load the Global Descriptor Table
    lgdt    [gdt_descriptor]

    ; Set PE bit in CR0
    mov     eax, cr0
    or      eax, 0x1
    mov     cr0, eax

    ; Far jump to flush pipeline and enter 32-bit protected mode
    jmp     0x08:protected_mode_entry

.disk_error:
    mov     si, msg_disk_err
    call    print_str
    hlt

; -----------------------------------------------------------------------------
; print_str: Print a null-terminated string via BIOS INT 0x10 (TTY mode)
; SI = pointer to string
; -----------------------------------------------------------------------------
print_str:
    pusha
    mov     ah, 0x0E
    mov     bh, 0x00
    mov     bl, 0x07
.loop:
    lodsb
    test    al, al
    jz      .done
    int     0x10
    jmp     .loop
.done:
    popa
    ret

; -----------------------------------------------------------------------------
; Data
; -----------------------------------------------------------------------------
boot_drive    db  0
msg_boot      db  "AI Aura OS Booting...", 0x0D, 0x0A, 0
msg_loaded    db  "Kernel loaded. Entering protected mode...", 0x0D, 0x0A, 0
msg_disk_err  db  "FATAL: Disk read error. System halted.", 0x0D, 0x0A, 0

; -----------------------------------------------------------------------------
; GDT — Flat 32-bit model
; -----------------------------------------------------------------------------
align 8
gdt_start:
    ; Null descriptor (required)
    dq  0x0000000000000000

    ; Code segment: base=0, limit=4GB, DPL=0, 32-bit, executable, readable
    dw  0xFFFF              ; Limit[15:0]
    dw  0x0000              ; Base[15:0]
    db  0x00                ; Base[23:16]
    db  0x9A                ; Access: present, ring0, code, exec/read
    db  0xCF                ; Flags: 4KB granularity, 32-bit; Limit[19:16]=0xF
    db  0x00                ; Base[31:24]

    ; Data segment: base=0, limit=4GB, DPL=0, 32-bit, writable
    dw  0xFFFF
    dw  0x0000
    db  0x00
    db  0x92                ; Access: present, ring0, data, read/write
    db  0xCF
    db  0x00
gdt_end:

gdt_descriptor:
    dw  gdt_end - gdt_start - 1
    dd  gdt_start

; -----------------------------------------------------------------------------
; Protected mode entry — 32-bit from here
; -----------------------------------------------------------------------------
[BITS 32]
protected_mode_entry:
    ; Reload all data segment registers with data selector (index 2, offset 0x10)
    mov     ax, 0x10
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax
    mov     esp, 0x00090000     ; Stack below kernel load address

    ; Jump to kernel entry point at physical address 0x10000
    jmp     (KERNEL_LOAD_SEG << 4)

; -----------------------------------------------------------------------------
; MBR padding + boot signature
; -----------------------------------------------------------------------------
times 510 - ($ - $$) db 0
dw 0xAA55
