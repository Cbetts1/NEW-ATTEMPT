# AI Aura OS — QEMU Run Instructions

## Install QEMU

### Termux (Android)
```bash
pkg install qemu-system-x86-64
# Use qemu-system-i386 or qemu-system-x86_64
```

### Linux
```bash
sudo apt install qemu-system-x86
```

### macOS
```bash
brew install qemu
```

---

## Run AI Aura OS

### Basic (graphical window)
```bash
make run
# or manually:
qemu-system-i386 -drive format=raw,file=image/AIOS.img -m 32M
```

### Serial-only (headless — useful on Termux)
```bash
make run-serial
# or manually:
qemu-system-i386 -drive format=raw,file=image/AIOS.img -m 32M \
    -serial stdio -display none
```

The kernel serial adapter (COM1) outputs to stdout when run this way.

### With more RAM
```bash
qemu-system-i386 -drive format=raw,file=image/AIOS.img -m 128M
```

### Debug mode (GDB stub)
```bash
qemu-system-i386 -drive format=raw,file=image/AIOS.img -m 32M \
    -s -S
# Then in another terminal:
gdb
(gdb) target remote :1234
(gdb) set architecture i386
(gdb) continue
```

---

## Expected Boot Sequence

```
AI Aura OS Booting...
Kernel loaded. Entering protected mode...
AI Aura OS — Kernel Initializing
==================================
[OK] Memory Manager initialized
[OK] Event Bus initialized
[OK] Plugin Manager initialized
[OK] System Mirror initialized
[OK] Scheduler initialized
[OK] Environment Registry initialized
[OK] Virtual Filesystem initialized
[OK] Module Loader initialized
[OK] Serial Adapter registered
[OK] Built-in tasks registered
==================================
------------------------------------------------------------
       AI AURA OS  v1.0.0   --  Aura Kernel
           Autonomous Intelligence Platform
------------------------------------------------------------

=== System Status ===
  Memory Used : 0 bytes
  Memory Free : 524224 bytes
  Plugins     : 1
  Tasks       : 4
  Kernel Tick : 0

> **Note:** The `mod_hello` service plugin is registered automatically by
> `loader_load_all()` during boot, so the plugin count starts at 1.

=== Main Menu ===
  [1] Show system status
  [2] List plugins
  [3] List scheduler tasks
  [4] Capture system snapshot
  [5] Dump mirror snapshots
  [6] Memory stats
  [H] Heartbeat info
  [K] Kernel panic (test)

  (System running — kernel heartbeat active)

[Aura] OS ready. Entering autonomous kernel loop...
```

---

## QEMU Disk Image Notes

- The disk image is a raw 1.44 MB floppy image (2880 × 512-byte sectors).
- Sector 1 = MBR bootloader.
- Sectors 2–65 = kernel binary.
- QEMU loads it as a bootable floppy or hard disk (`format=raw`).

To inspect the image:
```bash
xxd image/AIOS.img | head -32          # View MBR
xxd image/AIOS.img | tail -n +2 | head # View kernel start
```
