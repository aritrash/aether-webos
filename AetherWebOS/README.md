# Aether WebOS (C Edition)

> Aether WebOS is a bare-metal AArch64 operating system built from scratch in C, designed for deep systems research, custom networking stacks, and full hardware control without POSIX, Linux, or external TCP/IP libraries.

---

## Overview

Aether WebOS is a clean-room, single-core, bare-metal OS targeting:

- **Architecture:** AArch64 (ARMv8)
- **Platform:** QEMU `virt` machine (primary), Raspberry Pi (planned)
- **Language:** C (freestanding, no libc)
- **Networking:** Custom VirtIO-Net + ARP + IPv4 + TCP stack
- **Goal:** Build a production-grade network-capable OS kernel from first principles

This is not a hobby bootloader.  
This is a structured operating system architecture project.

---

## Core Architecture

### Boot & Low-Level Initialization

- AArch64 exception vectors
- MMU initialization
- Page table setup
- Interrupt enabling
- PSCI shutdown support
- Custom linker script
- Freestanding compilation (`-ffreestanding -nostdlib`)

---

### Memory Management

- Custom `kmalloc` / `kfree`
- Kernel heap initialization
- Memory tracking
- No libc allocator
- No dynamic runtime dependencies

---

### Interrupt & Timer Subsystem

- GIC initialization
- System timer setup
- Uptime tracking (`get_system_uptime_ms`)
- Interrupt enabling
- Optional `wfi` low-power idle support

---

### UART Console

- PL011 UART driver
- `uart_putc`
- `uart_puts`
- CR/LF handling
- ANSI screen clear support
- Boot banner rendering (linked binary asset)

---

### PCIe Subsystem

- PCIe initialization
- Device enumeration
- VirtIO device detection
- MMIO register interaction

---

## Networking Stack (Fully Custom)

Aether WebOS includes a from-scratch TCP/IP stack designed for research and long-term extensibility.

### VirtIO-Net Driver

- PCI-based VirtIO-Net device
- RX/TX descriptor ring management
- Poll-based packet reception
- Memory buffer recycling
- QEMU user-mode networking compatible

---

### Ethernet Layer

- Ethernet frame parsing
- MAC filtering
- EtherType demultiplexing
- Broadcast handling
- Frame construction for transmission

---

### ARP (Address Resolution Protocol)

- ARP packet parsing
- ARP reply generation
- ARP request handling
- Static gateway resolution for QEMU
- Basic ARP cache logic
- Host-order IP handling policy

---

### IPv4 Layer

- IPv4 header parsing
- Version/IHL validation
- Header checksum verification
- Network-to-host byte order conversion
- Local delivery filtering
- Protocol demultiplexing:
  - TCP
  - UDP (stub)
  - ICMP (stub)
- IPv4 packet construction
- No fragmentation support (drops fragments)

---

### TCP Stack (Custom Implementation)

Aether WebOS implements a minimal but structured TCP stack:

#### Implemented Components

- TCB (Transmission Control Block) structure
- 4-tuple connection identification
- TCP state tracking:
  - LISTEN
  - SYN_RECEIVED
  - ESTABLISHED
  - FIN handling
- Sequence number tracking:
  - `snd_una`
  - `snd_nxt`
  - `rcv_nxt`
- ACK validation
- Sequence validation
- Payload delivery
- RST generation
- FIN/ACK handling
- Chrome-compatible handshake behavior
- Support for multiple parallel connections
- Port 80 HTTP listener
- QEMU hostfwd integration (`9090 -> 80`)

#### TCP Processing Pipeline

1. Segment parsing
2. Checksum validation
3. TCB lookup
4. State transition
5. ACK update
6. Payload extraction
7. Socket dispatch
8. Segment transmission
9. Close handling

---

### HTTP Layer (Minimal)

- Static HTML page serving
- HTTP GET detection
- HTTP/1.0 compliant response
- Content-Length header generation
- Proper CRLF termination
- Connection close after response
- Chrome compatibility

---

## Kernel UI System

Aether includes a lightweight UART-driven interface:

### Modes

- `MODE_PORTAL`
- `MODE_NET_STATS`
- `MODE_CONFIRM_SHUTDOWN`

### Features

- Live system status
- Memory usage
- Network state
- Traffic counters
- TCP listener status
- Active connection count
- ANSI-based screen rendering
- Function-key navigation (F7 shutdown, F10 network dashboard)

---

## Network Dashboard

Displays:

- Interface status (VirtIO-Net-PCI)
- MAC address
- IPv4 address (10.0.2.15 default)
- IPv6 link-local
- RX/TX frame counters
- Collision/drop counters
- TCP listener state
- Active connections

---

## Build System

### Toolchain

- `aarch64-none-elf-gcc`
- `aarch64-none-elf-ld`
- `aarch64-none-elf-objcopy`

### Compilation

- `-mcpu=cortex-a72`
- `-ffreestanding`
- `-nostdlib`
- `-nostdinc`
- Custom include path handling
- Automatic source discovery (excluding backups)

---

## Running (QEMU)

Example:

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

## Research Goals

- Build a fully independent TCP/IP stack
- Understand network state machines deeply
- Design long-term extensible kernel architecture
- Prepare for ARM-based physical hardware deployment
- Explore distributed compute nodes
- Eventually support AI workloads on ARM clusters

---

## Design Philosophy

- No shortcuts.
- No lwIP.
- No borrowed kernel code.
- Clean layering.
- Hardware-aware.
- Architecturally sound.
- Long-term maintainability.

Aether WebOS is designed as a research-grade foundation, not a quick demo OS.

---

## Project Structure (High-Level)

```
arch/
drivers/
    ethernet/
        arp/
        ipv4/
        tcp/
    virtio/
    uart/
kernel/
portal/
common/
assets/
include/
build/
```

---

## Future Roadmap

- Retransmission timer support
- TCP congestion control (minimal Reno-like)
- Improved ARP cache
- UDP implementation
- ICMP echo reply
- Filesystem integration
- SMP support
- ARM hardware deployment
- AI-optimized ARM node clustering

---

## Status

✔ Boots on QEMU  
✔ MMU enabled  
✔ PCIe initialized  
✔ VirtIO-Net operational  
✔ ARP functioning  
✔ IPv4 validated  
✔ TCP handshake working  
✔ HTTP serving pipeline implemented  
✔ Chrome connection compatibility  

Under active architectural refinement.

---

## Author

Aether WebOS is developed as part of a long-term systems research initiative focused on low-level architecture, networking, and ARM-based computing platforms.

---

**Aether WebOS — Built from first principles.**