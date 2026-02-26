KERNEL = target/aarch64-unknown-none/debug/aethernxt
BIN = kernel8.img

build:
	cargo build
	llvm-objcopy -O binary $(KERNEL) $(BIN)

run: build
	qemu-system-aarch64 \
		-M virt,gic-version=3,highmem=off \
		-cpu max \
		-m 1G \
		-serial stdio \
		-display none \
		-machine virtualization=off \
		-kernel $(BIN)

clean:
	cargo clean
	rm -f kernel8.img