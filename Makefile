# =============================================================================
# AI Aura OS — Master Makefile
# =============================================================================
# Builds the complete OS image (AIOS.img) from source.
#
# Host requirements:
#   nasm              — assembles bootloader and kernel entry
#   i686-elf-gcc      — bare-metal C cross-compiler  (preferred)
#   i686-elf-ld       — bare-metal linker            (preferred)
#
# Termux fallback (if i686-elf toolchain is unavailable):
#   apt install nasm gcc binutils qemu-system-x86   (in Termux pkg)
#   Set USE_SYSTEM_GCC=1 on the make command line.
#
# Usage:
#   make              — build everything → image/AIOS.img
#   make run          — build and launch in QEMU
#   make clean        — remove all build artefacts
#   make USE_SYSTEM_GCC=1        — use system gcc instead of cross-compiler
# =============================================================================

# ── Toolchain ─────────────────────────────────────────────────────────────────
NASM := nasm

ifdef USE_SYSTEM_GCC
    CC  := gcc
    LD  := ld
    CFLAGS_ARCH  := -m32
    LDFLAGS_ARCH := -m elf_i386
else
    CC  := i686-elf-gcc
    LD  := i686-elf-ld
    CFLAGS_ARCH  :=
    LDFLAGS_ARCH :=
endif

CFLAGS := $(CFLAGS_ARCH) \
          -std=c99       \
          -ffreestanding \
          -fno-builtin   \
          -fno-stack-protector \
          -nostdlib      \
          -Wall          \
          -Wextra        \
          -O2            \
          -Ikernel

LDFLAGS := $(LDFLAGS_ARCH) \
           -nostdlib        \
           -T kernel/link.ld

# ── Directories ───────────────────────────────────────────────────────────────
BUILD := build
IMAGE := image

# ── Sources ───────────────────────────────────────────────────────────────────
KERNEL_C_SRCS := \
    kernel/kernel.c   \
    kernel/vga.c      \
    kernel/memory.c   \
    kernel/event_bus.c \
    kernel/plugin.c   \
    kernel/mirror.c   \
    kernel/menu.c     \
    modules/aura_core.c \
    adapters/aura_fs.c  \
    adapters/aura_net.c

KERNEL_ASM_SRCS := kernel/entry.asm
BOOT_ASM_SRC    := bootloader/boot.asm

# ── Object files ──────────────────────────────────────────────────────────────
KERNEL_C_OBJS   := $(patsubst %.c,   $(BUILD)/%.o, $(KERNEL_C_SRCS))
KERNEL_ASM_OBJS := $(patsubst %.asm, $(BUILD)/%.o, $(KERNEL_ASM_SRCS))
BOOT_OBJ        := $(BUILD)/bootloader/boot.bin

KERNEL_BIN      := $(BUILD)/kernel.bin
AIOS_IMG        := $(IMAGE)/AIOS.img

# ── Default target ────────────────────────────────────────────────────────────
.PHONY: all
all: $(AIOS_IMG)
	@echo ""
	@echo "  ╔══════════════════════════════════════╗"
	@echo "  ║   AI Aura OS image built: AIOS.img   ║"
	@echo "  ╚══════════════════════════════════════╝"
	@echo ""

# ── Final disk image ──────────────────────────────────────────────────────────
$(AIOS_IMG): $(BOOT_OBJ) $(KERNEL_BIN) | $(IMAGE)
	@echo "[IMG] Creating disk image: $(AIOS_IMG)"
	dd if=/dev/zero    of=$(AIOS_IMG)  bs=512  count=2880  status=none
	dd if=$(BOOT_OBJ)  of=$(AIOS_IMG)  bs=512  count=1     conv=notrunc status=none
	dd if=$(KERNEL_BIN) of=$(AIOS_IMG) bs=512  seek=1      conv=notrunc status=none

# ── Kernel binary (linked) ────────────────────────────────────────────────────
$(KERNEL_BIN): $(KERNEL_ASM_OBJS) $(KERNEL_C_OBJS) | $(BUILD)
	@echo "[LD ] Linking kernel → $(KERNEL_BIN)"
	$(LD) $(LDFLAGS) -o $(KERNEL_BIN) $(KERNEL_ASM_OBJS) $(KERNEL_C_OBJS)

# ── Bootloader binary ─────────────────────────────────────────────────────────
$(BOOT_OBJ): $(BOOT_ASM_SRC) | $(BUILD)/bootloader
	@echo "[ASM] Assembling bootloader → $(BOOT_OBJ)"
	$(NASM) -f bin -o $(BOOT_OBJ) $(BOOT_ASM_SRC)

# ── Kernel entry (ASM → ELF object) ──────────────────────────────────────────
$(BUILD)/kernel/entry.o: kernel/entry.asm | $(BUILD)/kernel
	@echo "[ASM] Assembling $< → $@"
	$(NASM) -f elf32 -o $@ $<

# ── C sources → ELF objects ───────────────────────────────────────────────────
$(BUILD)/kernel/%.o: kernel/%.c | $(BUILD)/kernel
	@echo "[CC ] Compiling $< → $@"
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/modules/%.o: modules/%.c | $(BUILD)/modules
	@echo "[CC ] Compiling $< → $@"
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/adapters/%.o: adapters/%.c | $(BUILD)/adapters
	@echo "[CC ] Compiling $< → $@"
	$(CC) $(CFLAGS) -c -o $@ $<

# ── Directory rules ───────────────────────────────────────────────────────────
$(BUILD) $(IMAGE) $(BUILD)/bootloader $(BUILD)/kernel $(BUILD)/modules $(BUILD)/adapters:
	@mkdir -p $@

# ── Run in QEMU ───────────────────────────────────────────────────────────────
.PHONY: run
run: $(AIOS_IMG)
	@echo "[RUN] Launching AI Aura OS in QEMU..."
	qemu-system-i386 \
	    -drive file=$(AIOS_IMG),format=raw,if=ide \
	    -display sdl \
	    -m 64M \
	    -no-reboot

# ── Run headless (for Termux / CI) ───────────────────────────────────────────
.PHONY: run-nographic
run-nographic: $(AIOS_IMG)
	qemu-system-i386 \
	    -drive file=$(AIOS_IMG),format=raw,if=ide \
	    -nographic \
	    -m 64M \
	    -no-reboot \
	    -serial stdio

# ── Clean ─────────────────────────────────────────────────────────────────────
.PHONY: clean
clean:
	@echo "[CLN] Removing build artefacts"
	rm -rf $(BUILD) $(IMAGE)

# ── Info ──────────────────────────────────────────────────────────────────────
.PHONY: info
info:
	@echo "Toolchain:"
	@echo "  CC   = $(CC)"
	@echo "  LD   = $(LD)"
	@echo "  NASM = $(NASM)"
	@echo "Sources:"
	@echo "  ASM  = $(BOOT_ASM_SRC) $(KERNEL_ASM_SRCS)"
	@echo "  C    = $(KERNEL_C_SRCS)"
