# AETHER OS Restoration Makefile - Windows-Friendly Edition
CC = aarch64-none-elf-gcc
LD = aarch64-none-elf-ld
OBJCOPY = aarch64-none-elf-objcopy

BOARD ?= VIRT
CFLAGS += -DBOARD_$(BOARD)

# --- Directories ---
SRC_DIR   = src
ARCH_DIR  = arch
BUILD_DIR = build
ASSETS_DIR = assets
INC_DIR   = include

# --- Flags ---
# Use := to expand once. We wrap in quotes and use subst to flip backslashes to forward slashes
# just in case the toolchain returns Windows-style paths.
INTERNAL_INC := "$(subst \,/,$(shell $(CC) -print-file-name=include))"

INCLUDES = -I$(INC_DIR) \
           -I$(INC_DIR)/common \
           -I$(INC_DIR)/drivers \
           -I$(INC_DIR)/kernel \
           -I$(INTERNAL_INC)

COMMON_FLAGS = -mcpu=cortex-a72 $(INCLUDES) -mgeneral-regs-only -fno-tree-loop-distribute-patterns
CFLAGS += $(COMMON_FLAGS) -Wall -O2 -ffreestanding -nostdlib -nostdinc
ASMFLAGS = $(COMMON_FLAGS)

# --- Discovery ---
SRCS_C := $(shell find $(SRC_DIR) $(ARCH_DIR) -name "*.c")
SRCS_S := $(shell find $(SRC_DIR) $(ARCH_DIR) -name "*.S")
# Wrap objects in quotes to be safe, though not strictly necessary if paths have no spaces
OBJS := $(SRCS_C:%.c=$(BUILD_DIR)/%.o) $(SRCS_S:%.S=$(BUILD_DIR)/%.o) $(BUILD_DIR)/banner.o

all: kernel8.img

# --- Pattern Rules ---
# Added quotes around directory targets and file paths
$(BUILD_DIR)/%.o: %.c
	@mkdir -p "$(dir $@)"
	@$(CC) $(CFLAGS) -c "$<" -o "$@"

$(BUILD_DIR)/%.o: %.S
	@mkdir -p "$(dir $@)"
	@$(CC) $(ASMFLAGS) -c "$<" -o "$@"

$(BUILD_DIR)/banner.o: $(ASSETS_DIR)/banner.txt
	@mkdir -p "$(BUILD_DIR)"
	@$(OBJCOPY) -I binary -O elf64-littleaarch64 -B aarch64 "$<" "$@"

# --- Platform Configuration ---
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
	@$(LD) $(LDFLAGS) -o "$(BUILD_DIR)/aether.elf" $(OBJS)
	@$(OBJCOPY) "$(BUILD_DIR)/aether.elf" -O binary kernel8.img
	@echo "[OK] Aether OS Image Generated for $(BOARD)"

clean:
	rm -rf $(BUILD_DIR) *.img

run: all
	qemu-system-aarch64 -M $(QEMU_MACHINE) $(QEMU_EXTRA) \
	-serial stdio \
	-monitor telnet:127.0.0.1:4444,server,nowait \
	-kernel kernel8.img