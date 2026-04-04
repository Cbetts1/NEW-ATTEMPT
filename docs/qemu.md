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

### 1. Kernel init (brief green-text phase)

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
[OK] Keyboard Driver initialized
[OK] User Registry initialized
[OK] Built-in tasks registered
==================================
```

### 2. Boot splash (~5 seconds animated)

Full-screen blue background with the AI Aura OS ASCII logo and an
animated progress bar that fills green left-to-right.

### 3. Win95-themed login dialog

Username and password fields (default: admin / admin).
Type each value and press **Enter** to advance.

### 4. Interactive desktop

Gray 80×25 desktop with a blue header bar at top, a taskbar with
`[Start]` button at the bottom. Press **S** to open the Start Menu.

### 5. Start Menu (arrow-key navigation)

Nine items accessible with ↑/↓ and Enter:
System Status, Plugin Manager, Memory Stats, File System,
Scheduler Tasks, Mirror Snapshots, About, Shut Down.

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
