# =============================================================================
# AI Aura OS — Master Makefile
# Builds the complete OS: bootloader + kernel → AIOS.img
#
# Toolchain: nasm + i686-elf-gcc (cross-compiler)  or  nasm + gcc -m32
# Termux install: pkg install nasm gcc binutils
#
# Usage:
#   make            - Build AIOS.img
#   make clean      - Remove build artifacts
#   make run        - Boot with QEMU
#   make run-serial - Boot with QEMU + serial output to stdout
# =============================================================================

# ---------------------------------------------------------------------------
# Toolchain detection
# Prefer i686-elf cross-compiler; fall back to host gcc -m32
# ---------------------------------------------------------------------------
ifneq ($(shell which i686-elf-gcc 2>/dev/null),)
    CC      := i686-elf-gcc
    LD      := i686-elf-ld
    OBJCOPY := i686-elf-objcopy
    CROSS   := 1
else
    CC      := gcc
    LD      := ld
    OBJCOPY := objcopy
    CROSS   := 0
endif

AS      := nasm
QEMU    := qemu-system-i386

# ---------------------------------------------------------------------------
# Flags
# ---------------------------------------------------------------------------
ASFLAGS  := -f bin

ifeq ($(CROSS),1)
CFLAGS   := -std=c99 -ffreestanding -O2 -Wall -Wextra \
            -fno-stack-protector -fno-builtin \
            -I kernel -I env -I modules -I adapters
LDFLAGS  := -T kernel/kernel.ld -nostdlib
else
CFLAGS   := -std=c99 -m32 -ffreestanding -O2 -Wall -Wextra \
            -fno-stack-protector -fno-builtin \
            -I kernel -I env -I modules -I adapters
LDFLAGS  := -T kernel/kernel.ld -melf_i386 -nostdlib
endif

# ---------------------------------------------------------------------------
# Directories
# ---------------------------------------------------------------------------
BUILD_DIR := build
IMAGE_DIR := image
IMAGE     := $(IMAGE_DIR)/AIOS.img

# ---------------------------------------------------------------------------
# Source files
# ---------------------------------------------------------------------------
KERNEL_SRCS := \
    kernel/kernel.c    \
    kernel/vga.c       \
    kernel/memory.c    \
    kernel/eventbus.c  \
    kernel/plugin.c    \
    kernel/mirror.c    \
    kernel/scheduler.c \
    kernel/menu.c

ENV_SRCS := \
    env/env.c  \
    env/fs.c

MODULE_SRCS := \
    modules/loader.c   \
    modules/mod_hello.c

ADAPTER_SRCS := \
    adapters/adapter_serial.c

ALL_C_SRCS := $(KERNEL_SRCS) $(ENV_SRCS) $(MODULE_SRCS) $(ADAPTER_SRCS)

KERNEL_OBJS := $(patsubst %.c, $(BUILD_DIR)/%.o, $(ALL_C_SRCS))

BOOT_BIN   := $(BUILD_DIR)/bootloader/boot.bin
KERNEL_BIN := $(BUILD_DIR)/kernel.bin

# ---------------------------------------------------------------------------
# Phony targets
# ---------------------------------------------------------------------------
.PHONY: all clean run run-serial directories info test test-dirs

all: directories $(IMAGE)
	@echo ""
	@echo "  ================================================="
	@echo "   AI Aura OS build complete: $(IMAGE)"
	@echo "  ================================================="

# ---------------------------------------------------------------------------
# Create output directories
# ---------------------------------------------------------------------------
directories:
	@mkdir -p $(BUILD_DIR)/bootloader
	@mkdir -p $(BUILD_DIR)/kernel
	@mkdir -p $(BUILD_DIR)/env
	@mkdir -p $(BUILD_DIR)/modules
	@mkdir -p $(BUILD_DIR)/adapters
	@mkdir -p $(IMAGE_DIR)

# ---------------------------------------------------------------------------
# Assemble bootloader → flat binary (512 bytes, MBR)
# ---------------------------------------------------------------------------
$(BOOT_BIN): bootloader/boot.asm
	@echo "[AS]  $<"
	$(AS) $(ASFLAGS) -o $@ $<

# ---------------------------------------------------------------------------
# Compile kernel C sources → object files
# ---------------------------------------------------------------------------
$(BUILD_DIR)/kernel/%.o: kernel/%.c
	@echo "[CC]  $<"
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/env/%.o: env/%.c
	@echo "[CC]  $<"
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/modules/%.o: modules/%.c
	@echo "[CC]  $<"
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/adapters/%.o: adapters/%.c
	@echo "[CC]  $<"
	$(CC) $(CFLAGS) -c -o $@ $<

# ---------------------------------------------------------------------------
# Link kernel objects → ELF → strip to flat binary
# ---------------------------------------------------------------------------
$(KERNEL_BIN): $(KERNEL_OBJS) kernel/kernel.ld
	@echo "[LD]  kernel ELF"
	$(LD) $(LDFLAGS) -o $(BUILD_DIR)/kernel.elf $(KERNEL_OBJS)
	@echo "[BIN] stripping to flat binary"
	$(OBJCOPY) -O binary $(BUILD_DIR)/kernel.elf $@

# ---------------------------------------------------------------------------
# Pack bootloader + kernel binary into a disk image
# Bootloader  = sector 1 (512 bytes)
# Kernel      = sectors 2..N (padded to 512-byte boundary)
# Total image = 1.44 MB floppy image
# ---------------------------------------------------------------------------
$(IMAGE): $(BOOT_BIN) $(KERNEL_BIN)
	@echo "[IMG] Building $(IMAGE)"
	@# Create a blank 1.44 MB floppy image
	dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	@# Write bootloader to sector 1
	dd if=$(BOOT_BIN) of=$@ bs=512 count=1 conv=notrunc status=none
	@# Write kernel starting at sector 2 (offset 512 bytes)
	dd if=$(KERNEL_BIN) of=$@ bs=512 seek=1 conv=notrunc status=none
	@echo "[IMG] Image size: $$(wc -c < $@) bytes"

# ---------------------------------------------------------------------------
# Run in QEMU
# ---------------------------------------------------------------------------
run: all
	$(QEMU) -drive format=raw,file=$(IMAGE) -m 32M

run-serial: all
	$(QEMU) -drive format=raw,file=$(IMAGE) -m 32M \
	        -serial stdio -display none

# ---------------------------------------------------------------------------
# Clean
# ---------------------------------------------------------------------------
clean:
	@echo "[CLEAN] Removing build artifacts"
	rm -rf $(BUILD_DIR) $(IMAGE_DIR)

# ---------------------------------------------------------------------------
# Test suite — host-side unit tests (no cross-compiler, no -m32)
# ---------------------------------------------------------------------------
TEST_CC     := gcc
TEST_CFLAGS := -std=c99 -Wall -Wextra -Wno-unused-parameter \
               -I . -I kernel -I modules -I adapters \
               -g

TEST_STUBS  := tests/stubs/vga_stub.c tests/stubs/globals.c

TEST_BINS   := \
    build/tests/test_memory    \
    build/tests/test_eventbus  \
    build/tests/test_scheduler \
    build/tests/test_plugin    \
    build/tests/test_mirror

test-dirs:
	@mkdir -p build/tests

test: test-dirs $(TEST_BINS)
	@echo ""
	@echo "  ================================================="
	@echo "   AI Aura OS — running test suite"
	@echo "  ================================================="
	@failed=0; \
	for t in $(TEST_BINS); do \
	    if $$t; then \
	        echo "  PASS  $$t"; \
	    else \
	        echo "  FAIL  $$t"; \
	        failed=1; \
	    fi; \
	done; \
	echo "  ================================================="; \
	if [ $$failed -eq 0 ]; then \
	    echo "  All tests passed."; \
	else \
	    echo "  Some tests FAILED."; \
	    exit 1; \
	fi

build/tests/test_memory: tests/test_memory.c kernel/memory.c $(TEST_STUBS)
	$(TEST_CC) $(TEST_CFLAGS) -o $@ $^

build/tests/test_eventbus: tests/test_eventbus.c kernel/eventbus.c $(TEST_STUBS)
	$(TEST_CC) $(TEST_CFLAGS) -o $@ $^

build/tests/test_scheduler: tests/test_scheduler.c kernel/scheduler.c \
                             kernel/eventbus.c $(TEST_STUBS)
	$(TEST_CC) $(TEST_CFLAGS) -o $@ $^

build/tests/test_plugin: tests/test_plugin.c kernel/plugin.c \
                         kernel/eventbus.c $(TEST_STUBS)
	$(TEST_CC) $(TEST_CFLAGS) -o $@ $^

build/tests/test_mirror: tests/test_mirror.c kernel/mirror.c \
                         kernel/eventbus.c $(TEST_STUBS) \
                         tests/stubs/mirror_deps.c
	$(TEST_CC) $(TEST_CFLAGS) -o $@ $^

# ---------------------------------------------------------------------------
# Build info
# ---------------------------------------------------------------------------
info:
	@echo "Compiler : $(CC)"
	@echo "Assembler: $(AS)"
	@echo "Linker   : $(LD)"
	@echo "Image    : $(IMAGE)"
	@echo "C sources: $(ALL_C_SRCS)"
