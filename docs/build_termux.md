# AI Aura OS — Termux Build Guide
# File: docs/build_termux.md

## Prerequisites

Install the required packages in Termux:

```bash
pkg update && pkg upgrade -y
pkg install -y nasm gcc binutils qemu-system-x86-64
```

> **Note:** Termux's `gcc` targets AArch64 (your phone's CPU).  To produce
> i386 binaries you must use the `USE_SYSTEM_GCC=1` flag, which tells the
> Makefile to pass `-m32` to gcc.  This requires the 32-bit multilib support
> that Termux's gcc includes.

## Build Steps

```bash
# 1. Clone or navigate to the repository
cd ~/NEW-ATTEMPT

# 2. Build the OS image
make USE_SYSTEM_GCC=1

# Output: image/AIOS.img
```

## Expected Output

```
[ASM] Assembling bootloader → build/bootloader/boot.bin
[ASM] Assembling kernel/entry.asm → build/kernel/entry.o
[CC ] Compiling kernel/kernel.c ...
[CC ] Compiling kernel/vga.c ...
[CC ] Compiling kernel/memory.c ...
[CC ] Compiling kernel/event_bus.c ...
[CC ] Compiling kernel/plugin.c ...
[CC ] Compiling kernel/mirror.c ...
[CC ] Compiling kernel/menu.c ...
[CC ] Compiling modules/aura_core.c ...
[CC ] Compiling adapters/aura_fs.c ...
[CC ] Compiling adapters/aura_net.c ...
[LD ] Linking kernel → build/kernel.bin
[IMG] Creating disk image: image/AIOS.img

  ╔══════════════════════════════════════╗
  ║   AI Aura OS image built: AIOS.img   ║
  ╚══════════════════════════════════════╝
```

## Troubleshooting

| Problem | Solution |
|---------|----------|
| `nasm: command not found` | `pkg install nasm` |
| `gcc: error: unrecognized -m32` | Try `pkg install clang` and `USE_SYSTEM_GCC=1 CC=clang` |
| `ld: cannot find -lgcc` | Pass `-fno-stack-protector` (already in Makefile) |
| Linker script path error | Ensure you run `make` from repo root |

## Clean Build

```bash
make clean
make USE_SYSTEM_GCC=1
```

## Using a Cross-Compiler (PC / preferred)

```bash
# Install i686-elf toolchain (Debian/Ubuntu example)
sudo apt install nasm gcc-multilib binutils qemu-system-x86

# Build (no special flag needed if i686-elf-gcc is in PATH)
make
```
