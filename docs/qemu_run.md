# AI Aura OS — QEMU Run Instructions
# File: docs/qemu_run.md

## Quick Start

```bash
# Build the image first
make USE_SYSTEM_GCC=1

# Launch with SDL display (requires display server)
make run

# Launch headless / in terminal (Termux-friendly)
make run-nographic
```

## Manual QEMU Command

```bash
qemu-system-i386 \
    -drive file=image/AIOS.img,format=raw,if=ide \
    -display sdl \
    -m 64M \
    -no-reboot
```

## Headless / Serial Console

```bash
qemu-system-i386 \
    -drive file=image/AIOS.img,format=raw,if=ide \
    -nographic \
    -m 64M \
    -no-reboot \
    -serial stdio
```

> The OS currently uses the VGA text buffer (0xB8000), not a UART serial port.
> In headless mode the screen output will not be visible via `-serial stdio`
> unless a COM1 driver is added.  Use the SDL display for full output.

## What You Should See

1. **Boot screen** (bootloader real-mode output):
   ```
   AI Aura OS Booting...
   Kernel loaded. Transferring control.
   ```

2. **Kernel startup** (VGA text, green on black):
   ```
   ##################################################
   #          A I   A U R A   O S   v1.0           #
   #        Self-Contained Autonomous System        #
   ##################################################

   [MEM] Memory manager initialised. Heap: 4096 KB
   [EVT] Event bus initialised.
   [PLG] Plugin manager initialised.
   [PLG] Registered: aura.core (v100)
   [PLG] Registered: aura.fs   (v100)
   [PLG] Registered: aura.net  (v100)
   [PLG] Loaded: aura.core
   [CORE] Aura core plugin online.
   ...
   [MIR] System mirroring engine initialised.
   [MNU] Main menu initialised.
   [KERN] All subsystems online. Entering heartbeat loop.
   [KERN] Press any key to open the main menu.
   ```

3. **Press any key** to open the interactive main menu:
   ```
   ============================================
     AI AURA OS  —  Main Menu
   ============================================
     1. List plugins / adapters
     2. System mirror dump
     3. Memory heap dump
     4. Capture snapshot now
     5. Shutdown

   Press key [1-5]:
   ```

## QEMU Debug Tips

```bash
# Enable QEMU monitor
qemu-system-i386 ... -monitor stdio

# Dump physical memory at kernel load address
(qemu) xp/32x 0x10000

# Single-step with GDB
qemu-system-i386 ... -s -S &
gdb -ex "target remote :1234" -ex "set arch i386"
```
