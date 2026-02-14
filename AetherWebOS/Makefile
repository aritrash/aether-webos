# Cross-compiler prefix
CC = aarch64-none-elf-gcc
LD = aarch64-none-elf-ld
OBJCOPY = aarch64-none-elf-objcopy

# --- Platform Toggle Logic ---
BOARD ?= VIRT
CFLAGS += -DBOARD_$(BOARD)

# --- Directories ---
SRC_DIR   = src
ARCH_DIR  = arch
BUILD_DIR = build
ASSETS_DIR = assets
INC_DIR   = include

# --- Flags ---
COMPILER_HEADERS = $(shell $(CC) -print-file-name=include)
# Include all subdirectories in include/
INCLUDES = -I$(INC_DIR) \
           -I$(INC_DIR)/common \
           -I$(INC_DIR)/drivers \
           -I$(INC_DIR)/drivers/virtio \
           -I$(INC_DIR)/kernel \
           -I"$(COMPILER_HEADERS)"

COMMON_FLAGS = -mcpu=cortex-a72 $(INCLUDES)
CFLAGS += $(COMMON_FLAGS) -Wall -O2 -ffreestanding -nostdinc -nostdlib
ASMFLAGS = $(COMMON_FLAGS)

# --- Automatic File Discovery ---
# Find all .c and .S files in src and arch
SRCS_C := $(shell find $(SRC_DIR) $(ARCH_DIR) -name "*.c")
SRCS_S := $(shell find $(SRC_DIR) $(ARCH_DIR) -name "*.S")

# Convert source file paths to object file paths in the build directory
# Example: src/kernel/main.c -> build/src/kernel/main.o
OBJS := $(SRCS_C:%.c=$(BUILD_DIR)/%.o)
OBJS += $(SRCS_S:%.S=$(BUILD_DIR)/%.o)
OBJS += $(BUILD_DIR)/banner.o

all: kernel8.img

# --- Pattern Rules ---

# Compile C files (handles nested directories)
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Assemble .S files (handles nested directories)
$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	@echo "AS  $<"
	@$(CC) $(ASMFLAGS) -c $< -o $@

# Handle Binary Assets
$(BUILD_DIR)/banner.o: $(ASSETS_DIR)/banner.txt
	@mkdir -p $(BUILD_DIR)
	@$(OBJCOPY) -I binary -O elf64-littleaarch64 -B aarch64 $< $@

# --- Linking Logic ---

ifeq ($(BOARD), VIRT)
    KERNEL_ADDR = 0x40080000
    QEMU_MACHINE = virt,gic-version=3,highmem=off 
    QEMU_EXTRA = -cpu max -m 1G -display none -machine virtualization=off
else
    KERNEL_ADDR = 0x80000
    QEMU_MACHINE = raspi4b
    QEMU_EXTRA = 
endif

LDFLAGS = -T linker.ld --defsym KERNEL_START=$(KERNEL_ADDR)

kernel8.img: $(OBJS)
	@echo "LD  aether.elf"
	@$(LD) $(LDFLAGS) -o $(BUILD_DIR)/aether.elf $(OBJS)
	@$(OBJCOPY) $(BUILD_DIR)/aether.elf -O binary kernel8.img
	@echo "[OK] Aether OS Image Generated: kernel8.img"

# --- Maintenance ---

clean:
	rm -rf $(BUILD_DIR) *.img

# --- Execution ---

.PHONY: run debug clean all

run: all
	@echo "Launching Aether OS ($(BOARD) Mode) in QEMU..."
	qemu-system-aarch64 -M $(QEMU_MACHINE) $(QEMU_EXTRA) \
	-serial stdio \
	-device virtio-net-pci,bus=pcie.0,disable-legacy=on,disable-modern=off \
	-kernel kernel8.img

debug: all
	@echo "Launching $(BOARD) in Debug Mode (Waiting for GDB)..."
	qemu-system-aarch64 -M $(QEMU_MACHINE) $(QEMU_EXTRA) -serial stdio -device virtio-net-pci -S -s -kernel kernel8.img