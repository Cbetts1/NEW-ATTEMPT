# AI Aura OS — Build Instructions (Termux / Linux)

## Prerequisites

### On Termux (Android phone)

```bash
# Update package list
pkg update && pkg upgrade

# Required tools
pkg install nasm gcc binutils make

# Verify
nasm --version
gcc --version
ld --version
```

### On Linux / WSL / macOS

```bash
# Debian / Ubuntu
sudo apt install nasm gcc gcc-multilib binutils make

# macOS (Homebrew + cross-compiler tap)
brew install nasm make
brew tap nativeos/i386-elf-toolchain
brew install i386-elf-gcc i386-elf-binutils
```

---

## Build

```bash
# Clone the repository
git clone https://github.com/Cbetts1/NEW-ATTEMPT.git
cd NEW-ATTEMPT

# Build the OS image
make

# The output is:
#   image/AIOS.img
```

### What `make` does

1. Assembles `bootloader/boot.asm` → `build/bootloader/boot.bin` (512-byte MBR)
2. Compiles all C sources from `kernel/`, `env/`, `modules/`, `adapters/`
3. Links objects using `kernel/kernel.ld` → `build/kernel.elf` → `build/kernel.bin`
4. Packs: `boot.bin` (sector 1) + `kernel.bin` (sectors 2–N) → `image/AIOS.img`

---

## Toolchain Notes

The Makefile automatically detects whether `i686-elf-gcc` (cross-compiler) is available. If not, it falls back to `gcc -m32` with freestanding flags. Both produce a valid flat binary kernel.

### Installing an i686 cross-compiler on Termux

If `gcc -m32` fails (common on ARM Termux builds), install a pre-built cross-compiler:

```bash
pkg install i686-elf-gcc   # if available in your Termux channel
```

Or build one manually:

```bash
# Download binutils + gcc source, configure with --target=i686-elf
# This takes 30–60 min on a phone — only needed once
```

---

## Makefile targets

| Target        | Description                            |
|---------------|----------------------------------------|
| `make`        | Build `image/AIOS.img`                 |
| `make clean`  | Remove all build artifacts             |
| `make run`    | Build and boot in QEMU (GUI)           |
| `make run-serial` | Boot in QEMU, serial → stdout      |
| `make info`   | Show detected toolchain info           |

---

## Troubleshooting

### `nasm: command not found`
```bash
pkg install nasm   # Termux
sudo apt install nasm   # Debian/Ubuntu
```

### `ld: unrecognized -melf_i386`
You are using a non-cross-compiler ld. Install `gcc-multilib`:
```bash
sudo apt install gcc-multilib
```
Or switch to the cross-compiler `i686-elf-ld`.

### Kernel binary too large for 64-sector slot
Increase `KERNEL_SECTORS` in `bootloader/boot.asm` and the `dd seek=` count in the Makefile to match.
