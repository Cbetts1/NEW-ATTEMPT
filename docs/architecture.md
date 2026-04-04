# AI Aura OS — Architecture Document

## Overview

AI Aura OS is a fully autonomous, self-contained x86 operating system designed to boot directly from hardware (or QEMU) without any dependency on Android, Linux, or Termux at runtime. Termux is used **only** as a build forge.

---

## Directory Structure

```
NEW-ATTEMPT/
├── bootloader/
│   ├── boot.asm          # Stage 1 MBR bootloader (NASM, 512 bytes)
│   └── (boot.bin → build/) # Assembled output
├── kernel/
│   ├── kernel.c          # Kernel entry + heartbeat loop
│   ├── kernel.h          # Core types, status codes, global state
│   ├── vga.c / vga.h     # VGA text-mode driver (0xB8000)
│   ├── io.h              # x86 I/O port inline helpers
│   ├── memory.c / .h     # Free-list heap allocator
│   ├── eventbus.c / .h   # Publish-subscribe event bus
│   ├── plugin.c / .h     # Plugin/adapter lifecycle manager
│   ├── mirror.c / .h     # System mirroring engine
│   ├── scheduler.c / .h  # Cooperative round-robin scheduler
│   ├── menu.c / .h       # Main menu interface
│   └── kernel.ld         # Linker script (base 0x10000)
├── env/
│   ├── env.c / env.h     # Key-value environment registry
│   └── fs.c / fs.h       # In-memory virtual filesystem
├── modules/
│   ├── loader.c / .h     # Module loader (static registry)
│   └── mod_hello.c       # Example module (hello service)
├── adapters/
│   ├── adapter.h         # Adapter interface definition
│   └── adapter_serial.c  # COM1 serial port adapter
├── build/                # Build artifacts (auto-created by make)
├── image/
│   └── AIOS.img          # Final bootable disk image
├── docs/
│   ├── architecture.md   # This file
│   ├── build.md          # Build instructions
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
         └───┬───┬───┬───┬───┬───┬───┬──┘
             │   │   │   │   │   │   │
         ┌───▼─┐ │ ┌─▼─┐ │ ┌─▼─┐ │ ┌▼─────┐
         │ VGA │ │ │Mem│ │ │Evt│ │ │Sched │
         │ drv │ │ │Mgr│ │ │Bus│ │ │      │
         └─────┘ │ └───┘ │ └───┘ │ └──────┘
                 │       │       │
              ┌──▼──┐ ┌──▼──┐ ┌──▼──┐
              │Plugn│ │Mirro│ │ Menu│
              │ Mgr │ │  r  │ │     │
              └──┬──┘ └─────┘ └─────┘
                 │
        ┌────────┴────────┐
        │                 │
   ┌────▼────┐      ┌─────▼─────┐
   │ Modules │      │ Adapters  │
   │ (hello) │      │ (serial)  │
   └─────────┘      └───────────┘
```

---

## Memory Layout

| Region             | Physical Address | Size    | Use                  |
|--------------------|-----------------|---------|----------------------|
| BIOS ROM           | 0x000F0000      | 64 KB   | BIOS                 |
| VGA Buffer         | 0x000B8000      | 4 KB    | Text display         |
| Kernel stack       | 0x00090000      | ~32 KB  | Boot-time stack      |
| Bootloader (MBR)   | 0x00007C00      | 512 B   | Loaded by BIOS       |
| Kernel image       | 0x00010000      | ≤32 KB  | Loaded by bootloader |
| Kernel heap        | 0x00200000      | 512 KB  | kmalloc / kfree      |

---

## Key Design Principles

1. **Zero host-OS dependencies at runtime** — the OS image is 100% self-contained.
2. **Single heartbeat loop** — `scheduler_tick()` drives everything; no busy timers.
3. **Event-driven subsystems** — subsystems communicate via the event bus, not direct calls.
4. **Pluggable architecture** — every service is a plugin registered with `plugin_manager`.
5. **Mirroring** — the system mirror captures and can restore OS state snapshots.

---

## Boot Flow

```
BIOS loads MBR (boot.asm) at 0x7C00 (real mode, 16-bit)
  │  prints banner, loads kernel sectors into 0x10000 via INT 0x13
  │  sets up GDT, enters 32-bit protected mode
  └─ far-jumps to 0x10000 (kernel/entry.asm :: _start)

entry.asm :: _start  [BITS 32, already in protected mode]
  │  reloads segment registers (0x10), sets ESP = 0x200000
  └─ calls kernel_main()

kernel_main() (kernel/kernel.c)
  │  vga_init → memory_init → eventbus_init → plugin_manager_init
  │  mirror_init → scheduler_init → env_init → fs_init
  │  loader_init + loader_load_all  (registers mod_hello plugin)
  │  adapter_serial_register        (COM1 serial adapter)
  │  scheduler_add_task × 4
  │  menu_run()   ← displays boot banner + system status
  └─ while(1) { scheduler_tick(); }  ← perpetual heartbeat loop
```
