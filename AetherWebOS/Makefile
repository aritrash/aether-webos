# Cross-compiler prefix
CC = aarch64-none-elf-gcc
LD = aarch64-none-elf-ld
OBJCOPY = aarch64-none-elf-objcopy

# --- Platform Toggle Logic ---
# Usage: 'make BOARD=RPI4' or 'make BOARD=VIRT' (Default is VIRT)
BOARD ?= VIRT
CFLAGS += -DBOARD_$(BOARD)

# --- Flags ---
COMPILER_HEADERS = $(shell $(CC) -print-file-name=include)
COMMON_FLAGS = -mcpu=cortex-a72 -Iinclude -I"$(COMPILER_HEADERS)"
CFLAGS += $(COMMON_FLAGS) -Wall -O2 -ffreestanding -nostdinc -nostdlib
ASMFLAGS = $(COMMON_FLAGS)

# --- Directories ---
SRC_DIR = src
ARCH_DIR = arch
BUILD_DIR = build
ASSETS_DIR = assets

# --- Files ---
OBJ_FILES = $(BUILD_DIR)/boot.o \
            $(BUILD_DIR)/kernel.o \
            $(BUILD_DIR)/uart.o \
            $(BUILD_DIR)/mmu.o \
            $(BUILD_DIR)/vectors.o \
            $(BUILD_DIR)/exception.o \
            $(BUILD_DIR)/utils.o \
            $(BUILD_DIR)/asm_utils.o \
            $(BUILD_DIR)/timer.o \
            $(BUILD_DIR)/pcie.o \
            $(BUILD_DIR)/gic.o \
            $(BUILD_DIR)/memory.o \
            $(BUILD_DIR)/banner.o

all: kernel8.img

# --- Pattern Rules (Cleans up the file) ---

# Compile C files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble .S files from arch directory
$(BUILD_DIR)/boot.o: $(ARCH_DIR)/boot.S
	@mkdir -p $(BUILD_DIR)
	$(CC) $(ASMFLAGS) -c $< -o $@

# Assemble .S files from src directory (vectors, asm_utils)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.S
	@mkdir -p $(BUILD_DIR)
	$(CC) $(ASMFLAGS) -c $< -o $@

# Handle Binary Assets
$(BUILD_DIR)/banner.o: $(ASSETS_DIR)/banner.txt
	@mkdir -p $(BUILD_DIR)
	$(OBJCOPY) -I binary -O elf64-littleaarch64 -B aarch64 $< $@

# --- Linking ---

# Define the kernel load address for each platform
ifeq ($(BOARD), VIRT)
    KERNEL_ADDR = 0x40080000
    QEMU_MACHINE = virt,gic-version=3
    QEMU_EXTRA = -cpu max -m 1G -display none 
else
    KERNEL_ADDR = 0x80000
    QEMU_MACHINE = raspi4b
    QEMU_EXTRA = 
endif

# Pass the KERNEL_START symbol to the linker
LDFLAGS = -T linker.ld --defsym KERNEL_START=$(KERNEL_ADDR)

kernel8.img: $(OBJ_FILES)
	$(LD) $(LDFLAGS) -o $(BUILD_DIR)/aether.elf $(OBJ_FILES)
	$(OBJCOPY) $(BUILD_DIR)/aether.elf -O binary kernel8.img

# --- Maintenance ---

clean:
	rm -rf $(BUILD_DIR) *.img

# --- Execution ---

.PHONY: run debug clean all

# Logic to pick QEMU flags based on board
ifeq ($(BOARD), VIRT)
    QEMU_MACHINE = virt,gic-version=3
    QEMU_EXTRA = -cpu max -m 1G -display none 
else
    QEMU_MACHINE = raspi4b
    QEMU_EXTRA = 
endif

run: all
	@echo "Launching Aether OS ($(BOARD) Mode) in QEMU..."
	qemu-system-aarch64 -M $(QEMU_MACHINE) $(QEMU_EXTRA) -serial stdio -kernel kernel8.img

debug: all
	@echo "Launching $(BOARD) in Debug Mode (Waiting for GDB)..."
	qemu-system-aarch64 -M $(QEMU_MACHINE) $(QEMU_EXTRA) -serial stdio -S -s -kernel kernel8.img