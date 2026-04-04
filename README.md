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
    ├── Memory manager    — free-list heap at 2 MB
    ├── Event bus         — publish/subscribe, 32 topics, 64-event queue
    ├── Plugin manager    — lifecycle (load / activate / tick / shutdown)
    ├── System mirror     — OS state snapshots (memory, processes, plugins)
    ├── Scheduler         — cooperative round-robin task runner
    └── Main menu         — text UI, system status, snapshot management

Environment
    ├── env               — key-value OS environment registry
    └── fs                — in-memory virtual filesystem

Modules
    ├── loader            — static module registry + loader
    └── mod_hello         — example service module

Adapters
    ├── adapter.h         — adapter interface
    └── adapter_serial    — COM1 serial port driver
```

---

## Repository Structure

| Path | Description |
|------|-------------|
| `bootloader/boot.asm` | Stage 1 MBR bootloader (NASM) |
| `kernel/` | Kernel C sources + headers |
| `kernel/kernel.ld` | Linker script (base `0x10000`) |
| `env/` | Environment registry + virtual FS |
| `modules/` | Module loader + built-in modules |
| `adapters/` | Hardware/virtual device adapters |
| `build/` | Compiled objects (auto-created) |
| `image/AIOS.img` | Final bootable disk image |
| `docs/` | Architecture, build, and QEMU guides |
| `Makefile` | Top-level build system |

---

## Docs

- [Architecture](docs/architecture.md) — subsystem design, memory map
- [Build guide](docs/build.md) — Termux, Linux, macOS instructions
- [QEMU guide](docs/qemu.md) — running and debugging the OS

---

## Design Principles

1. **Zero runtime host-OS dependencies** — the image is 100% self-contained.
2. **Single heartbeat loop** — `scheduler_tick()` drives everything.
3. **Event-driven** — subsystems communicate via the internal event bus.
4. **Pluggable** — every service is a plugin with a standard lifecycle.
5. **Mirrored** — the system mirror engine captures and restores OS state.
6. **Phone-buildable** — the entire OS compiles on Android/Termux.

---

## License

See [LICENSE](LICENSE).
