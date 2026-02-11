# Cross-compiler prefix
CC = aarch64-none-elf-gcc
LD = aarch64-none-elf-ld
OBJCOPY = aarch64-none-elf-objcopy

# Flags
# -ffreestanding: No standard libraries
# -nostdlib: Don't link system startup files
# -mcpu=cortex-a72: Target Raspberry Pi 4 CPU
# -Iinclude tells the compiler to add the 'include' folder to its search path

COMPILER_HEADERS = $(shell $(CC) -print-file-name=include)
CFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -mcpu=cortex-a72 -Iinclude -I"$(COMPILER_HEADERS)"
ASMFLAGS = -mcpu=cortex-a72

# Directories
SRC_DIR = src
ARCH_DIR = arch
BUILD_DIR = build

# Files
# Add build/utils.o to your object list
OBJ_FILES = $(BUILD_DIR)/boot.o \
            $(BUILD_DIR)/kernel.o \
            $(BUILD_DIR)/uart.o \
			$(BUILD_DIR)/mmu.o \
            $(BUILD_DIR)/vectors.o \
            $(BUILD_DIR)/exception.o \
            $(BUILD_DIR)/utils.o \
			$(BUILD_DIR)/asm_utils.o \
			$(BUILD_DIR)/timer.o \
			$(BUILD_DIR)/gic.o \
			$(BUILD_DIR)/memory.o \
            $(BUILD_DIR)/banner.o

all: kernel8.img

$(BUILD_DIR)/boot.o: $(ARCH_DIR)/boot.S
	@mkdir -p $(BUILD_DIR)
	$(CC) $(ASMFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel.o: $(SRC_DIR)/kernel.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/uart.o: $(SRC_DIR)/uart.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/utils.o: $(SRC_DIR)/utils.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/vectors.o: $(SRC_DIR)/vectors.S
	@mkdir -p $(BUILD_DIR)
	$(CC) $(ASMFLAGS) -c $< -o $@

$(BUILD_DIR)/exception.o: $(SRC_DIR)/exception.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/timer.o: $(SRC_DIR)/timer.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/gic.o: $(SRC_DIR)/gic.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/memory.o: $(SRC_DIR)/memory.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mmu.o: $(SRC_DIR)/mmu.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_utils.o: $(SRC_DIR)/asm_utils.S
	@mkdir -p $(BUILD_DIR)
	$(CC) $(ASMFLAGS) -c $< -o $@

$(BUILD_DIR)/banner.o: assets/banner.txt
	@mkdir -p $(BUILD_DIR)
	$(OBJCOPY) -I binary -O elf64-littleaarch64 -B aarch64 $< $@

kernel8.img: $(OBJ_FILES)
	$(LD) -T linker.ld -o $(BUILD_DIR)/aether.elf $(OBJ_FILES)
	$(OBJCOPY) $(BUILD_DIR)/aether.elf -O binary kernel8.img

clean:
	rm -rf $(BUILD_DIR) *.img

.PHONY: run
run: all
	@echo "Launching Aether OS in QEMU..."
	qemu-system-aarch64 -M raspi4b -serial stdio -kernel kernel8.img

.PHONY: debug
debug: all
	@echo "Launching in Debug Mode (Waiting for GDB)..."
	qemu-system-aarch64 -M raspi4b -serial stdio -S -s -kernel kernel8.img