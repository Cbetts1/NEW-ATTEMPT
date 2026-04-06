# AI Aura OS

**AI Aura OS** is a fully autonomous, self-contained x86 operating system built from scratch. It boots directly into itself — no Linux, no Android, no Termux at runtime. Termux (or any Linux/macOS machine) is used only as a **build forge**.

The output is a bootable raw disk image: `image/AIOS.img`.

---

## Quick Start

### Build (Termux / Linux)
```bash
# Install dependencies (Termux)
pkg install nasm gcc binutils make

# Build
make

# Output: image/AIOS.img
```

### Run in QEMU
```bash
# Graphical
make run

# Headless (serial output to stdout — works on Termux)
make run-serial
```

---

## Architecture

```
Bootloader (boot.asm / NASM / 512-byte MBR)
    └── loads kernel at 0x10000 → enters 32-bit protected mode

Kernel (kernel.c + subsystems)
    ├── VGA driver        — direct write to 0xB8000
    ├── Memory manager    — free-list heap at 2 MB (512 KB)
    ├── Paging            — 4 MB identity-mapped page directory, heap guard pages
    ├── IDT + PIC         — 256-entry IDT, 8259A remapped to vector 0x20
    ├── PIT timer         — ~100 Hz IRQ0, drives g_tick_count via interrupt
    ├── Keyboard driver   — IRQ1 + polling fallback, ring buffer, scancode→ASCII
    ├── Event bus         — publish/subscribe, 32 topics, 64-event queue
    ├── Plugin manager    — lifecycle (load / activate / tick / shutdown)
    ├── System mirror     — OS state snapshots (memory, tasks, plugins)
    ├── Scheduler         — cooperative round-robin, 16 task slots
    └── Main menu         — text UI (13 options), system status, snapshots

Environment
    ├── env               — key-value OS environment registry (load/save to disk)
    ├── fs                — in-memory virtual filesystem
    └── fat12             — FAT12 read/write for disk persistence

Modules
    ├── loader            — static module registry + loader
    ├── mod_hello         — example service module
    └── aura_core         — core health-check + heartbeat events

Adapters
    ├── adapter.h         — adapter interface
    ├── adapter_serial    — COM1 serial port (VGA mirror + serial input → keyboard)
    ├── aura_net          — virtual network adapter stub
    └── ata               — ATA PIO sector read/write (primary master)
```

---

## Repository Structure

| Path | Description |
|------|-------------|
| `bootloader/boot.asm` | Stage 1 MBR bootloader (NASM) |
| `kernel/` | Kernel C sources + headers |
| `kernel/idt.c` | IDT + 8259A PIC remapping, exception/IRQ dispatch |
| `kernel/pit.c` | PIT ~100 Hz timer (IRQ0 → `g_tick_count`) |
| `kernel/paging.c` | 4 MB identity-map, heap guard pages, CR0.PG |
| `kernel/kstring.c` | Shared string utilities (`strncpy_k`, `strncmp_k`, `strlen_k`) |
| `kernel/keyboard.c` | PS/2 keyboard driver (IRQ1 + polling, scancode→ASCII) |
| `kernel/kernel.ld` | Linker script (ELF32, base `0x10000`) |
| `env/` | Environment registry + virtual FS + FAT12 driver |
| `env/fat12.c` | FAT12 read/write for disk persistence (`AIOS.ENV`) |
| `modules/` | Module loader + built-in modules (`mod_hello`, `aura_core`) |
| `adapters/` | Hardware/virtual device adapters (`serial`, `net stub`, `ata`) |
| `adapters/ata.c` | ATA PIO primary master sector read/write |
| `build/` | Compiled objects (auto-created) |
| `image/AIOS.img` | Final bootable disk image |
| `docs/` | Architecture, build, QEMU, and plugin API guides |
| `Makefile` | Top-level build system |

---

## Docs

- [Architecture](docs/architecture.md) — subsystem design, memory map
- [Build guide](docs/build.md) — Termux, Linux, macOS instructions
- [QEMU guide](docs/qemu.md) — running and debugging the OS

---

## Design Principles

1. **Zero runtime host-OS dependencies** — the image is 100% self-contained.
2. **Single heartbeat loop** — `scheduler_tick()` drives everything; tick timing is now maintained by a ~100 Hz PIT interrupt (IRQ0).
3. **Event-driven** — subsystems communicate via the internal event bus.
4. **Pluggable** — every service is a plugin with a standard lifecycle.
5. **Mirrored** — the system mirror engine captures and restores OS state (env re-init + VGA reset + menu redisplay).
6. **Phone-buildable** — the entire OS compiles on Android/Termux.
7. **Protected** — paging enabled with guard pages around the kernel heap to trap overflows via #PF.
8. **Persistent** — env registry saved to `AIOS.ENV` on the FAT12 volume via `[Q]` shutdown or kernel panic.

---

## License

See [LICENSE](LICENSE).
