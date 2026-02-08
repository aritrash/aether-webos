# Cross-compiler prefix
CC = aarch64-none-elf-gcc
LD = aarch64-none-elf-ld
OBJCOPY = aarch64-none-elf-objcopy

# Flags
# -ffreestanding: No standard libraries
# -nostdlib: Don't link system startup files
# -mcpu=cortex-a72: Target Raspberry Pi 4 CPU
CFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -mcpu=cortex-a72
ASMFLAGS = -mcpu=cortex-a72

# Directories
SRC_DIR = src
ARCH_DIR = arch
BUILD_DIR = build

# Files
OBJ_FILES = $(BUILD_DIR)/boot.o $(BUILD_DIR)/kernel.o

all: kernel8.img

$(BUILD_DIR)/boot.o: $(ARCH_DIR)/boot.S
	@mkdir -p $(BUILD_DIR)
	$(CC) $(ASMFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel.o: $(SRC_DIR)/kernel.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

kernel8.img: $(OBJ_FILES)
	$(LD) -T linker.ld -o $(BUILD_DIR)/aether.elf $(OBJ_FILES)
	$(OBJCOPY) $(BUILD_DIR)/aether.elf -O binary kernel8.img

clean:
	rm -rf $(BUILD_DIR) *.img