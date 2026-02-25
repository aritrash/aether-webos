TARGET = aarch64-unknown-none
BUILD = debug
KERNEL = target/$(TARGET)/$(BUILD)/aethernxt

QEMU = qemu-system-aarch64

all: build

build:
	cargo build

release:
	cargo build --release

run: build
	$(QEMU) \
		-machine virt \
		-cpu cortex-a72 \
		-nographic \
		-kernel $(KERNEL)

run-release: release
	$(QEMU) \
		-machine virt \
		-cpu cortex-a72 \
		-nographic \
		-kernel target/$(TARGET)/release/aethernxt

clean:
	cargo clean

.PHONY: all build release run run-release clean