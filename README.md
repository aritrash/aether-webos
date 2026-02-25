# Aether Project

> Aether is a long-term systems research initiative focused on building a fully independent operating system stack — from boot to TCP — across two parallel implementations:
>
> - **AetherWebOS/** → C-based bare-metal AArch64 kernel  
> - **AetherNxt/** → Rust-based next-generation experimental kernel  
> - **buildfiles/** → Archived build snapshots and backups  

This repository is not a toy OS.  
It is a structured attempt to understand and engineer a complete, hardware-aware, extensible operating system architecture from first principles.

---

# Repository Layout

```
Aether/
│
├── aetherwebos/   # C implementation (primary research kernel)
├── aethernxt/     # Rust implementation (experimental next-gen kernel)
├── buildfiles/    # tar.gz archived builds and snapshots
└── README.md
```

---

# Project Vision

The Aether Project exists to explore:

- Bare-metal AArch64 systems programming
- Custom TCP/IP stack implementation
- VirtIO and PCIe driver development
- Memory management without libc
- Long-term ARM server experimentation
- Cross-language kernel architecture (C + Rust)

The goal is not “make it boot”.

The goal is:

> Build a clean, correct, extensible systems foundation.

---

# aetherwebos/ (C Edition)

### Language
C (freestanding, no POSIX, no libc)

### Target
- AArch64
- QEMU `virt` machine
- Future: Raspberry Pi / ARM hardware

---

## Implemented Features

### Kernel Core
- AArch64 exception vectors
- MMU initialization
- Page table setup
- Interrupt enabling
- PSCI system shutdown
- Custom linker script
- Freestanding build flags

---

### Memory Management
- Custom `kmalloc` / `kfree`
- Kernel heap initialization
- Memory tracking
- No libc dependency

---

### Timer & Interrupt System
- GIC initialization
- System timer
- Uptime tracking
- Optional low-power `wfi`

---

### UART Console
- PL011 driver
- ANSI terminal control
- Boot banner (linked binary asset)
- Portal UI system
- Function-key navigation

---

### PCIe Subsystem
- PCIe enumeration
- Device discovery
- VirtIO device detection

---

## Custom Networking Stack

AetherWebOS includes a fully custom TCP/IP stack:

### VirtIO-Net Driver
- PCI-based VirtIO-Net
- RX/TX descriptor ring handling
- Poll-driven packet processing
- QEMU user-mode networking compatible

---

### Ethernet Layer
- Frame parsing
- MAC filtering
- EtherType demux
- Frame construction

---

### ARP
- ARP request handling
- ARP reply generation
- QEMU gateway compatibility
- Static MAC resolution model

---

### IPv4
- Header validation
- Checksum verification
- Network ↔ host byte order handling
- Protocol demux:
  - TCP
  - UDP (stub)
  - ICMP (stub)
- No fragmentation support

---

### TCP Stack

Structured minimal TCP implementation:

- TCB (Transmission Control Block)
- 4-tuple connection lookup
- TCP states:
  - LISTEN
  - SYN_RECEIVED
  - ESTABLISHED
  - FIN handling states
- Sequence tracking:
  - snd_una
  - snd_nxt
  - rcv_nxt
- ACK validation
- Sequence validation
- RST handling
- FIN handling
- Chrome-compatible handshake
- Multiple parallel connections
- Port 80 HTTP listener

---

### HTTP Server

- Static HTML serving
- HTTP GET detection
- Content-Length generation
- HTTP/1.0 compatible responses
- Proper CRLF formatting
- Connection close handling

---

## Running (C Kernel)

Example QEMU command:

```bash
qemu-system-aarch64 \
  -M virt,gic-version=3,highmem=off \
  -cpu max \
  -m 1G \
  -serial stdio \
  -netdev user,id=net0,hostfwd=tcp::9090-:80 \
  -device virtio-net-pci,netdev=net0,mac=52:54:00:12:34:56 \
  -display none \
  -machine virtualization=off \
  -kernel kernel8.img
```

Open in browser:

```
http://127.0.0.1:9090
```

---

# aethernxt/ (Rust Edition)

### Language
Rust (`#![no_std]`, bare-metal)

### Purpose
AetherNxt is a clean-room experimental kernel designed to:

- Explore Rust in systems programming
- Build a safer TCP/IP architecture
- Re-architect networking with ownership safety
- Compare C vs Rust kernel design decisions

This is not a port.  
It is a parallel evolution.

---

## Goals of AetherNxt

- Bare-metal AArch64 Rust kernel
- UART driver in Rust
- Custom memory allocator
- VirtIO driver in Rust
- Clean TCP stack designed with Rust ownership
- Safer TCB lifetime management
- Structured layering from day one

---

## Why Two Kernels?

| Aspect | AetherWebOS (C) | AetherNxt (Rust) |
|---------|-----------------|------------------|
| Maturity | Primary | Experimental |
| Stability | Higher | Early-stage |
| Safety | Manual | Ownership enforced |
| Goal | Production-style architecture | Modern research kernel |
| Risk | Memory bugs possible | Borrow-checked |

The two kernels inform each other.

Lessons from C improve Rust architecture.  
Rust experiments influence C refactoring.

---

# buildfiles/

Contains:

- Archived `.tar.gz` snapshots
- Known-good build images
- Experimental branch exports
- Recovery checkpoints

This directory exists to prevent architectural loss during major rewrites.

---

# Long-Term Roadmap

### Kernel
- SMP support
- Preemptive scheduling
- Virtual memory refinement

### Networking
- TCP retransmission timers
- Basic congestion control
- UDP full implementation
- ICMP echo reply
- ARP cache improvements

### Filesystem
- In-memory FS
- SD/eMMC support
- FAT/ext-like experimental FS

### Hardware
- Raspberry Pi deployment
- ARM server experimentation
- Clustered node research

### Research Goals
- ARM-based distributed compute nodes
- Low-level infrastructure for AI workloads
- Sustainable ARM server design exploration

---

# Design Philosophy

- No lwIP
- No Linux reuse
- No shortcut stacks
- Full control over every layer
- Clean layering discipline
- Research-grade architecture

Aether is built to understand systems — not just to run them.

---

# Toolchains

## C Kernel
- `aarch64-none-elf-gcc`
- `aarch64-none-elf-ld`
- `aarch64-none-elf-objcopy`

## Rust Kernel
- `rustup`
- `cargo`
- `aarch64-unknown-none` target
- `#![no_std]` environment

---

# License

Apache License

---

# Maintainer

Developed as part of a long-term systems engineering research initiative focused on:

- Architecture
- Networking
- Hardware-software integration
- Modern systems language comparison

---

# Aether

Two kernels.  
One architecture philosophy.  
Built from first principles.