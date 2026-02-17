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
OBJS := $(SRCS_C:%.c=$(BUILD_DIR)/%.o) $(SRCS_S:%.S=$(BUILD_DIR)/%.o) $(BUILD_DIR)/banner.o

all: kernel8.img

# --- Pattern Rules ---
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
    QEMU_CPU = max
    QEMU_RAM = 1G
    # Standard Serial + TCP Bridge for WebUI
    QEMU_SERIAL = -serial stdio -serial tcp:127.0.0.1:1234,server,nowait
    QEMU_EXTRA = -display none -machine virtualization=off
else
    KERNEL_ADDR = 0x80000
    QEMU_MACHINE = raspi4b
    QEMU_CPU = cortex-a72
    QEMU_RAM = 2G
    QEMU_SERIAL = -serial stdio
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
	qemu-system-aarch64 -M $(QEMU_MACHINE) \
	-cpu $(QEMU_CPU) -m $(QEMU_RAM) \
	$(QEMU_SERIAL) $(QEMU_EXTRA) \
	-device qemu-xhci,p2=15,p3=15 \
	-device usb-tablet \
	-netdev user,id=net0,hostfwd=tcp::8080-:80 \
	-device virtio-net-pci,netdev=net0 \
	-kernel kernel8.img