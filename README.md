# AI Aura OS

```
##################################################
#                                                #
#          A I   A U R A   O S   v1.0           #
#        Self-Contained Autonomous System        #
#                                                #
##################################################
```

A fully autonomous, self-contained operating system built from scratch.
AI Aura OS boots directly into itself — not into Android, Linux, or Termux.
Termux is used only as a build forge; the OS has zero runtime dependencies.

---

## Table of Contents

1. [Project Vision](#project-vision)
2. [Architecture](#architecture)
3. [Repository Structure](#repository-structure)
4. [File Inventory](#file-inventory)
5. [Build Instructions (Termux)](#build-instructions-termux)
6. [QEMU Run Instructions](#qemu-run-instructions)
7. [Subsystem Reference](#subsystem-reference)
8. [Extending the OS](#extending-the-os)

---

## Project Vision

| Property            | Value                                     |
|---------------------|-------------------------------------------|
| Name                | AI Aura OS                                |
| Version             | 1.0                                       |
| Target Architecture | i386 (x86 32-bit)                         |
| Output Artefact     | `image/AIOS.img` (raw disk image)         |
| Boot Medium         | QEMU virtual disk / USB / SD card         |
| Runtime Deps        | **None** — fully self-contained           |
| Build Host          | Termux (Android) or any Linux/macOS host  |
| Build Tools         | `nasm`, `gcc` (or `i686-elf-gcc`), `ld`  |

---

## Architecture

### Boot Sequence

```
BIOS
 └─► Bootloader @ 0x7C00  (bootloader/boot.asm)
      ├─ Prints "AI Aura OS Booting..."
      ├─ Loads kernel sectors from disk → 0x10000
      └─► Kernel Entry @ 0x10000  (kernel/entry.asm)
           ├─ Sets up GDT (flat 32-bit segments)
           ├─ Switches to protected mode
           └─► kernel_main()  (kernel/kernel.c)
                ├─ VGA driver
                ├─ Memory manager (4 MB heap)
                ├─ Event bus
                ├─ Plugin manager
                │    ├─ aura.core   (health module)
                │    ├─ aura.fs     (virtual FS adapter)
                │    └─ aura.net    (network stub)
                ├─ System mirroring engine
                ├─ Main menu
                └─► Heartbeat Loop ∞
```

### Memory Map

| Region           | Address                   | Size   |
|------------------|---------------------------|--------|
| Bootloader       | `0x007C00`–`0x007DFF`     | 512 B  |
| Kernel binary    | `0x010000`–`~0x017FFF`    | ≤32 KB |
| Kernel stack     | grows ↓ `0x200000`        | ~1 MB  |
| Kernel heap      | `0x300000`–`0x6FFFFF`     | 4 MB   |
| VGA text buffer  | `0x0B8000`–`0x0BFFFF`     | 32 KB  |

---

## Repository Structure

```
NEW-ATTEMPT/
├── Makefile                    ← Master build system
├── README.md                   ← This file
│
├── bootloader/
│   └── boot.asm                ← Stage-1 MBR bootloader (NASM, 512 bytes)
│
├── kernel/
│   ├── entry.asm               ← Kernel entry: GDT, protected mode, → C
│   ├── link.ld                 ← Linker script (entry.o first at 0x10000)
│   ├── kernel.c                ← kernel_main(), heartbeat loop
│   ├── vga.c                   ← VGA 80×25 text-mode driver + printf
│   ├── memory.c                ← Free-list heap allocator (kmalloc/kfree)
│   ├── event_bus.c             ← Pub/sub ring-buffer event bus
│   ├── plugin.c                ← Plugin/adapter registry & lifecycle
│   ├── mirror.c                ← System mirroring & snapshot engine
│   ├── menu.c                  ← Interactive main menu (PS/2 keyboard)
│   └── include/
│       ├── vga.h
│       ├── memory.h
│       ├── event_bus.h
│       ├── plugin.h
│       ├── mirror.h
│       └── menu.h
│
├── modules/
│   ├── aura_core.c             ← Built-in core health module
│   └── README.md
│
├── adapters/
│   ├── aura_fs.c               ← Virtual in-memory filesystem adapter
│   ├── aura_net.c              ← Virtual network adapter (stub)
│   └── README.md
│
├── env/
│   ├── aios.env                ← Environment constants reference
│   └── README.md
│
├── docs/
│   ├── README.md               ← Docs index
│   ├── architecture.md         ← Full architecture diagram
│   ├── build_termux.md         ← Termux build guide
│   ├── qemu_run.md             ← QEMU launch guide
│   └── plugin_api.md           ← Plugin API contract
│
├── build/                      ← Generated: object files & kernel.bin
└── image/                      ← Generated: AIOS.img
```

---

## File Inventory

| File                        | Role                                                 |
|-----------------------------|------------------------------------------------------|
| `bootloader/boot.asm`       | MBR bootloader, prints banner, loads kernel          |
| `kernel/entry.asm`          | GDT setup, protected-mode switch, calls kernel_main  |
| `kernel/link.ld`            | Places entry.o first at 0x10000, defines sections    |
| `kernel/kernel.c`           | `kernel_main()`, heartbeat loop, subsystem init      |
| `kernel/vga.c`              | 80×25 VGA text driver, colour, scroll, printf        |
| `kernel/memory.c`           | Boundary-tag allocator, 4 MB heap, no libc           |
| `kernel/event_bus.c`        | 64-event ring queue, subscribe/publish/dispatch      |
| `kernel/plugin.c`           | Plugin registry, load/unload, tick dispatch          |
| `kernel/mirror.c`           | 8-slot snapshot ring, XOR checksum, auto-restore     |
| `kernel/menu.c`             | PS/2 keyboard poll, 5-entry interactive menu         |
| `modules/aura_core.c`       | Core diagnostic module (PLUGIN_TYPE_MODULE)          |
| `adapters/aura_fs.c`        | In-memory VFS (PLUGIN_TYPE_ADAPTER)                  |
| `adapters/aura_net.c`       | Network stub (PLUGIN_TYPE_ADAPTER)                   |
| `Makefile`                  | Compiles all sources → links → packs AIOS.img        |

---

## Build Instructions (Termux)

### Install dependencies

```bash
pkg update && pkg upgrade -y
pkg install -y nasm gcc binutils qemu-system-x86-64
```

### Build

```bash
cd ~/storage/shared/NEW-ATTEMPT   # or wherever you cloned it
make USE_SYSTEM_GCC=1
```

The image is written to `image/AIOS.img`.

### Clean rebuild

```bash
make clean && make USE_SYSTEM_GCC=1
```

### Using a proper cross-compiler (Linux/macOS, recommended)

```bash
# Ubuntu/Debian
sudo apt install nasm gcc-multilib binutils qemu-system-x86

make        # uses i686-elf-gcc if available, else falls back automatically
```

---

## QEMU Run Instructions

### Graphical (SDL window)

```bash
make run
```

### Headless / Termux

```bash
make run-nographic
```

### Manual

```bash
qemu-system-i386 \
    -drive file=image/AIOS.img,format=raw,if=ide \
    -display sdl \
    -m 64M \
    -no-reboot
```

---

## Subsystem Reference

### VGA Driver (`kernel/vga.c`)
80×25 colour text mode, hardware scroll, `vga_printf()` supporting `%s %d %u %x %c`.

### Memory Manager (`kernel/memory.c`)
Boundary-tag free-list allocator over a 4 MB static heap.  `kmalloc()`/`kfree()`,
merge-on-free coalescing, corruption detection via magic numbers.

### Event Bus (`kernel/event_bus.c`)
Lightweight publish/subscribe over a 64-event ring buffer.  Handlers registered
per event ID, dispatched synchronously from the heartbeat loop.

### Plugin Manager (`kernel/plugin.c`)
Central registry for up to 32 plugins.  Each plugin provides `init()`, `tick()`,
`shutdown()` callbacks.  `plugin_tick_all()` is called every heartbeat.

### System Mirror Engine (`kernel/mirror.c`)
Captures periodic snapshots of memory, process, and device state into an 8-slot
ring buffer.  Each snapshot carries an XOR checksum.  Call `mirror_restore(slot)`
to roll back to any captured state.

### Main Menu (`kernel/menu.c`)
PS/2 keyboard polling (port 0x60/0x64).  Interactive 5-option menu: list plugins,
dump mirror, dump heap, capture snapshot, shutdown.

---

## Extending the OS

See [`docs/plugin_api.md`](docs/plugin_api.md) for the full plugin API contract.

**Quick summary:** create a new `.c` file in `modules/` or `adapters/`, export a
`plugin_descriptor_t`, add it to `KERNEL_C_SRCS` in the Makefile, and register
it in `kernel/kernel.c`.

---

*AI Aura OS — Built to run itself.*
