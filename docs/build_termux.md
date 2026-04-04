# AI Aura OS — Termux Build Guide
# File: docs/build_termux.md

## Prerequisites

Install the required packages in Termux.

### x86/x86_64 machine
```bash
pkg update && pkg upgrade -y
pkg install -y nasm gcc-multilib binutils make qemu-system-x86-64
make
```

### Android phone (ARM / AArch64) — Termux
```bash
pkg update && pkg upgrade -y
# clang + lld are the cross-compilation toolchain for i386 on ARM
pkg install -y nasm clang lld llvm binutils make
make
```

The Makefile auto-detects AArch64/ARM and switches to the `clang --target=i386-pc-none-elf` path automatically.  No extra flags are needed.

---

## Build Steps

```bash
# Navigate to the repository
cd ~/NEW-ATTEMPT

# Build the OS image (toolchain is auto-detected)
make

# Output: image/AIOS.img
```

## Expected Output (Termux ARM)

```
[AS]  bootloader/boot.asm
[AS]  kernel/entry.asm
[CC]  kernel/kernel.c            (clang --target=i386-pc-none-elf)
[CC]  kernel/vga.c
[CC]  kernel/memory.c
[CC]  kernel/eventbus.c
[CC]  kernel/plugin.c
[CC]  kernel/mirror.c
[CC]  kernel/scheduler.c
[CC]  kernel/keyboard.c
[CC]  kernel/user.c
[CC]  kernel/ui95.c
[CC]  kernel/menu.c
[CC]  env/env.c
[CC]  env/fs.c
[CC]  modules/loader.c
[CC]  modules/mod_hello.c
[CC]  adapters/adapter_serial.c
[LD]  kernel ELF                 (ld.lld -m elf_i386)
[BIN] stripping to flat binary   (llvm-objcopy / objcopy -O binary)
[IMG] Building image/AIOS.img

  =================================================
   AI Aura OS build complete: image/AIOS.img
  =================================================
```

## Toolchain Matrix

| Host platform      | CC                                 | LD               | OBJCOPY          |
|--------------------|------------------------------------|------------------|------------------|
| i686-elf-gcc present (any host) | `i686-elf-gcc`       | `i686-elf-ld`    | `i686-elf-objcopy` |
| AArch64 / ARM (Termux)          | `clang --target=i386-pc-none-elf` | `ld.lld -m elf_i386` | `llvm-objcopy` |
| x86 / x86_64 Linux / macOS      | `gcc -m32`            | `ld -melf_i386`  | `objcopy`        |

Run `make info` to see which toolchain was detected.

## Troubleshooting

| Problem | Solution |
|---------|----------|
| `nasm: command not found` | `pkg install nasm` |
| `clang: command not found` | `pkg install clang` |
| `ld.lld: command not found` | `pkg install lld` |
| `llvm-objcopy not found` | `pkg install llvm` (or ignore — plain `objcopy` also works) |
| `error: ARM host ... clang not found` | Install clang: `pkg install clang lld` |
| linker script error | Run `make` from the repo root directory |

## Clean Build

```bash
make clean
make
```

## Running in QEMU (Termux)

```bash
pkg install qemu-system-x86-64

# Headless (serial console — Termux-friendly, no display needed)
make run-serial

# With QEMU display (requires VNC or X11)
make run
```

