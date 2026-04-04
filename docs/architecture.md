# AI Aura OS — Architecture Overview
# File: docs/architecture.md

## System Design

```
┌─────────────────────────────────────────────────────────────┐
│                        DISK IMAGE (AIOS.img)                │
│                                                             │
│  Sector 0     │  Sectors 1–64            │  (unused)       │
│  Bootloader   │  Kernel binary           │                  │
│  (512 bytes)  │  (≤ 32 KB)               │                  │
└─────────────────────────────────────────────────────────────┘
```

## Boot Sequence

```
BIOS
 └─► Bootloader (0x7C00)
      ├─ Print "AI Aura OS Booting..."
      ├─ Load kernel sectors from disk → 0x1000:0x0000
      └─► Kernel Entry (0x10000)
           ├─ Set up GDT
           ├─ Switch to 32-bit protected mode
           └─► kernel_main()
                ├─ vga_init()
                ├─ memory_init()
                ├─ event_bus_init()
                ├─ plugin_manager_init()
                ├─ mirror_init()
                ├─ menu_init()
                └─► Heartbeat Loop (infinite)
```

## Subsystem Map

```
kernel_main()
│
├── VGA Driver          kernel/vga.c
│    └─ 80×25 text mode, colour, scroll, printf
│
├── Memory Manager      kernel/memory.c
│    └─ Free-list allocator, 4 MB heap, no libc
│
├── Event Bus           kernel/event_bus.c
│    └─ Pub/sub ring buffer, 64-event queue
│
├── Plugin Manager      kernel/plugin.c
│    ├─ aura.core       modules/aura_core.c
│    ├─ aura.fs         adapters/aura_fs.c
│    └─ aura.net        adapters/aura_net.c
│
├── Mirror Engine       kernel/mirror.c
│    └─ 8-slot snapshot ring, XOR checksum, auto-capture
│
└── Main Menu           kernel/menu.c
     └─ PS/2 keyboard polling, 5-entry interactive menu
```

## Memory Map

| Region          | Address           | Size     |
|-----------------|-------------------|----------|
| BIOS / real-mode | 0x00000–0x007FFF  | 32 KB    |
| Bootloader      | 0x007C00–0x007DFF | 512 B    |
| Kernel          | 0x010000–0x017FFF | ≤ 32 KB  |
| Kernel stack    | grows ↓ 0x200000  | ~1 MB    |
| Kernel heap     | 0x300000–0x6FFFFF | 4 MB     |
| VGA text buffer | 0x0B8000–0x0BFFFF | 32 KB    |

## Key Design Principles

1. **Zero external dependencies at runtime** — the OS never calls back to the host.
2. **Event-driven core** — subsystems communicate via the event bus, not direct calls.
3. **Plugin architecture** — all capabilities are loaded through the plugin manager.
4. **Self-healing via mirroring** — periodic snapshots allow state restoration.
5. **Build anywhere** — entire OS compiles with standard toolchain on phone or PC.
