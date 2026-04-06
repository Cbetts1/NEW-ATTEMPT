# AI Aura OS — Architecture Document

## Overview

AI Aura OS is a fully autonomous, self-contained x86 operating system designed to boot directly from hardware (or QEMU) without any dependency on Android, Linux, or Termux at runtime. Termux is used **only** as a build forge.

---

## Directory Structure

```
NEW-ATTEMPT/
├── bootloader/
│   └── boot.asm          # Stage 1 MBR bootloader (NASM, 512 bytes)
├── kernel/
│   ├── kernel.c          # Kernel entry + heartbeat loop
│   ├── kernel.h          # Core types, status codes, global state
│   ├── vga.c / vga.h     # VGA text-mode driver (0xB8000); vga_puts/vga_printf
│   ├── io.h              # x86 I/O port inline helpers
│   ├── memory.c / .h     # Free-list heap allocator (2 MB, 512 KB)
│   ├── paging.c / .h     # 4 MB identity-map, heap guard pages, CR0.PG
│   ├── idt.c / .h        # IDT (256 entries), 8259A PIC remap, ISR/IRQ dispatch
│   ├── pit.c / .h        # PIT ~100 Hz (IRQ0) → g_tick_count
│   ├── eventbus.c / .h   # Publish-subscribe event bus (32 topics, 64-deep queue)
│   ├── plugin.c / .h     # Plugin/adapter lifecycle manager
│   ├── mirror.c / .h     # System mirroring engine (XOR-checksummed snapshots)
│   ├── scheduler.c / .h  # Cooperative round-robin scheduler (16 slots)
│   ├── menu.c / .h       # Interactive text menu (13 options)
│   ├── keyboard.c / .h   # PS/2 keyboard driver (IRQ1 + polling, scancode→ASCII)
│   ├── kstring.c / .h    # Shared kernel string utilities (strncpy_k etc.)
│   └── kernel.ld         # Linker script (ELF32, base 0x10000)
├── env/
│   ├── env.c / env.h     # Key-value environment registry (save/load via FAT12)
│   ├── fs.c / fs.h       # In-memory virtual filesystem
│   └── fat12.c / fat12.h # FAT12 read/write for disk persistence
├── modules/
│   ├── loader.c / .h     # Module loader (static registry)
│   ├── mod_hello.c       # Example service module
│   └── aura_core.c       # Built-in core diagnostic module
├── adapters/
│   ├── adapter.h         # Adapter interface definition
│   ├── adapter_serial.c  # COM1 serial port driver (VGA mirror, serial→keyboard)
│   ├── aura_net.c        # Virtual network adapter stub
│   └── ata.c / ata.h    # ATA PIO primary master sector read/write
├── build/                # Build artifacts (auto-created by make)
├── image/
│   └── AIOS.img          # Final bootable disk image (1.44 MB floppy)
├── docs/
│   ├── architecture.md   # This file
│   ├── build.md          # Build instructions
│   ├── plugin_api.md     # Plugin API reference
│   └── qemu.md           # QEMU run instructions
├── Makefile
└── README.md
```

---

## Subsystem Map

```
┌──────────────────────────────────────────────────────────┐
│                    AI Aura OS Boot                       │
│  BIOS → MBR (boot.asm) → Protected Mode → kernel_main() │
└────────────────────────┬─────────────────────────────────┘
                         │
         ┌───────────────▼───────────────┐
         │          kernel.c             │
         │   kernel_main() bootstrap     │
         └──┬──┬──┬──┬──┬──┬──┬──┬──┬──┘
            │  │  │  │  │  │  │  │  │
        ┌───▼─┐│┌─▼─┐│┌─▼─┐│┌─▼─┐ │┌──▼──┐
        │ VGA ││ Mem ││Pag ││ IDT│ ││PIT  │
        │ drv ││ Mgr ││ ing││+PIC│ ││~100 │
        └─────┘│└───┘│└───┘│└─┬─┘ ││ Hz  │
               │     │     │  │   │└──────┘
          ┌────▼─┐ ┌──▼─┐  │  │IRQ1│
          │Event │ │ Env│  │  ▼    │
          │ Bus  │ │ FS │ ┌─▼──────▼──┐
          └──────┘ └────┘ │ Keyboard  │
               │          │ IRQ+poll  │
         ┌─────▼──┐       └───────────┘
         │ Plugin │
         │  Mgr   │
         └──┬─────┘
            │
   ┌────────┴────────────┐
   │                     │
┌──▼──┐  ┌──────┐  ┌─────▼────────┐
│Sched│  │Mirror│  │  Adapters    │
│ 16  │  │ sync │  │serial/net/ATA│
│slots│  └──────┘  └──────────────┘
└──┬──┘
   │
┌──▼───────────────────────┐
│  Modules (loader)        │
│  hello / aura_core       │
└──────────────────────────┘

Env layer: env (key-value + FAT12 save/load) + fs (VFS)
```

---

## Scheduler Tasks (registered at boot)

| Task name      | Function          | Period | Purpose                                       |
|----------------|-------------------|--------|-----------------------------------------------|
| `heartbeat`    | kernel_heartbeat  | 1      | Increment global tick counter (sw fallback)   |
| `event_drain`  | eventbus_process  | 1      | Dispatch pending events                       |
| `mirror_sync`  | mirror_sync       | 10     | Update live system snapshot (incl. proc_count)|
| `plugin_tick`  | plugin_tick_all   | 5      | Tick all active plugins                       |
| `keyboard_poll`| keyboard_poll     | 1      | Poll PS/2 fallback (IRQ1 handler is primary)  |
| `menu_tick`    | menu_tick         | 2      | Handle interactive menu input                 |
| `serial_poll`  | serial_poll       | 5      | Drain COM1 RX → keyboard ring buffer          |

---

## Memory Layout

| Region             | Physical Address | Size    | Use                     |
|--------------------|-----------------|---------|-------------------------|
| BIOS ROM           | 0x000F0000      | 64 KB   | BIOS                    |
| VGA Buffer         | 0x000B8000      | 4 KB    | Text display            |
| Kernel stack       | 0x00090000      | ~32 KB  | Boot-time stack         |
| Bootloader (MBR)   | 0x00007C00      | 512 B   | Loaded by BIOS          |
| Kernel image       | 0x00010000      | ≤32 KB  | Loaded by bootloader    |
| Kernel heap        | 0x00200000      | 512 KB  | kmalloc / kfree         |
| Heap guard (lo)    | 0x001FF000      | 4 KB    | Not-present guard page  |
| Heap guard (hi)    | 0x00280000      | 4 KB    | Not-present guard page  |
| Page directory     | 0x00280000      | 4 KB    | CR3 / 1024 PDEs         |
| Page table 0       | 0x00281000      | 4 KB    | 1024 PTEs (0–4 MB map)  |

---

## API Design (Generation 1 — canonical)

All subsystems use the **Generation 1** API:

- Return type: `aura_status_t` (`AURA_OK`, `AURA_ERR`, `AURA_NOMEM`, …)
- Plugin descriptor: `plugin_desc_t` with `char version[]` and `aura_status_t` callbacks
- Event bus: `eventbus_*` family with `uint8_t topic` and `uint32_t data`
- String utilities: `strncpy_k` / `strncmp_k` / `strlen_k` from `kernel/kstring.h`
- VGA output: `vga_print`, `vga_println`, `vga_puts`, `vga_printf`
- IRQ handling: `irq_register_handler(irq, fn)` / `irq_unmask(irq)` from `kernel/idt.h`

---

## Key Design Principles

1. **Zero host-OS dependencies at runtime** — the OS image is 100% self-contained.
2. **Single heartbeat loop** — `scheduler_tick()` drives everything; tick timing is maintained by the ~100 Hz PIT interrupt (IRQ0).
3. **Event-driven subsystems** — subsystems communicate via the event bus, not direct calls.
4. **Pluggable architecture** — every service is a plugin registered with `plugin_manager`.
5. **Mirroring** — the system mirror captures XOR-checksummed OS state snapshots and restores them (env re-init + VGA reset + menu redisplay).
6. **Protected memory** — paging enabled with guard pages around the kernel heap to trap overflows via #PF.
7. **Interactive menu** — PS/2 keyboard via IRQ1 feeds a cooperative `menu_tick` task; COM1 serial input is forwarded to the same ring buffer for headless operation.
8. **Disk persistence** — the env registry is saved to `AIOS.ENV` on the FAT12 volume via `[Q]` clean shutdown or on kernel panic.
