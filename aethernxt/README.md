# AetherNxt

**AetherNxt** is a bare-metal AArch64 operating system kernel written in Rust.  
It is the Rust-based evolution of the Aether WebOS project, focused on building a clean, modular, and future-proof OS architecture from the ground up.

This project targets the **QEMU `virt` machine** and runs in a completely freestanding (`no_std`) environment — no Linux, no POSIX, no libc, and no runtime.

---

## Project Goals

AetherNxt is designed to:

- Run on AArch64 (ARMv8) hardware
- Operate without `std`
- Use a custom linker script
- Maintain full control over memory layout
- Separate architecture, drivers, kernel, memory, and networking layers
- Eventually support:
  - Interrupt handling
  - GIC
  - PCI
  - VirtIO
  - Ethernet
  - ARP / IPv4 / TCP stack

Current stage: **Minimal boot + UART output**

---

## Implemented Features (Current State)

### Boot & Architecture

- Custom linker script
- Entry point `_start`
- Stack initialization in AArch64 assembly
- Rust entry function `rust_main`
- Panic handler (abort-based)
- Runs on QEMU `virt` machine
- Target: `aarch64-unknown-none`

### Rust Configuration

- `#![no_std]`
- `#![no_main]`
- `panic = "abort"`
- Custom `.cargo/config.toml` for target and linker
- Built with `rustup` toolchain
- Cross-compiled via LLVM backend

### UART (PL011)

- Memory-mapped IO driver
- Volatile register access
- Clean unsafe boundary isolation
- `putc()` and `puts()` implementation
- UART initialization logic included
- Intended base address: `0x09000000`

### Kernel Structure

Fully modular, future-proof layout:

src/
│
├── main.rs
│
├── arch/
│ ├── mod.rs
│ └── aarch64/
│ ├── mod.rs
│ └── boot.rs
│
├── drivers/
│ ├── mod.rs
│ └── uart.rs
│
├── kernel/
│ ├── mod.rs
│ └── init.rs
│
├── memory/
│ └── mod.rs
│
└── net/
└── mod.rs


### Layer Separation

- `arch/` → CPU-specific code and boot assembly
- `drivers/` → Hardware drivers (UART currently)
- `kernel/` → Core initialization logic
- `memory/` → Future memory management subsystem
- `net/` → Future networking stack

Unsafe code is confined to hardware access layers.

---

## Toolchain

- Rust (via `rustup`)
- Target: `aarch64-unknown-none`
- QEMU (AArch64)
- LLVM tools (for object inspection)

---

## Build Instructions

### Build

```bash
cargo build
```
### Run in QEMU

```bash
qemu-system-aarch64 \
  -machine virt \
  -cpu cortex-a72 \
  -nographic \
  -kernel target/aarch64-unknown-none/debug/aethernxt
```
or use

```bash
make run
```

## Architecture Overview
Boot Flow

- QEMU loads ELF at 0x40080000
- _start executes (assembly)
- Stack pointer initialized
- Branch to rust_main
- kernel_init() executes
- UART prints boot message
- Kernel loops forever

## Memory Layout

Defined in linker.ld:

Load address: 0x40080000

Sections:

.text

.rodata

.data

.bss

Stack reserved via _stack_top

## Design Principles

- Strict module boundaries
- Unsafe isolated to hardware layer
- Architecture independence at kernel level
- Networking logic separated from drivers
- Future SMP compatibility in mind
- Designed to scale into a full TCP/IP-enabled OS

## Roadmap

After stable UART output:

- Zero .bss
- Exception vector table
- EL management
- Timer support
- GIC interrupt controller
- PCI enumeration
- VirtIO driver
- VirtIO-Net
- ARP implementation
- IPv4 stack
- TCP state machine
- Heap allocator

## Why Rust?

AetherNxt explores Rust as a systems language alternative to C:

- Strong compile-time guarantees
- Safer abstractions
- Explicit unsafe boundaries
- Zero-cost abstractions
- LLVM-powered cross-compilation

The goal is not to avoid unsafe code —
but to contain it precisely.

## Current Status

- Boots successfully in QEMU
- Stack initialization verified
- Entry point confirmed via symbol inspection
- UART driver implemented (debugging in progress)

This is the foundation stage of AetherNxt.

## Long-Term Vision

AetherNxt aims to evolve into:

- A network-capable Rust-based OS
- Supporting VirtIO networking
- Custom TCP/IP stack
- Clean modular design
- Potential deployment on ARM SBCs

## Author

Aritrash Sarkar
Asta Epsilon Group

AetherNxt is the next evolution of low-level system design —
where architecture discipline meets modern language safety.