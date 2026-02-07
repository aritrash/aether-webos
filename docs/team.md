# Aether Special Ops Team

This document outlines the core roles and responsibilities for the Aether OS development team. Our objective is to build a high-efficiency ARM64 server kernel with a decoupled web-based interface.

* * *

## Project Architect

**Lead:** (Your Name/Handle)

The Architect is responsible for the "Big Picture" and the fundamental bootstrap of the system.

*   **Primary Ownership:**
    
    *   **Boot Protocol:** Handling the transition from the bootloader to `EL1` (Exception Level 1).
        
    *   **System Design:** Defining the kernel's ABI and how different modules (Memory, Drivers) communicate.
        
    *   **Exception Vector Table:** Implementing the master table to handle synchronous and asynchronous exceptions (Interrupts/Syscalls).
        
*   **Key Deliverables:** `boot.S`, `linker.ld`, and the core `kernel_main` loop.
    

* * *

## Memory Mapping Engineer

**Focus:** MMU (Memory Management Unit) & Address Spaces

In ARMv8-A, memory management is significantly more complex than x86 segmentation. This role is responsible for the translation of Virtual Addresses to Physical Addresses.

*   **Primary Ownership:**
    
    *   **Translation Tables:** Setting up the 4-level page table structure (L0–L3).
        
    *   **Memory Attributes:** Configuring the **MAIR** (Memory Attribute Indirection Register) to define cached (Normal) vs. non-cached (Device) memory.
        
    *   **Identity Mapping:** Ensuring the kernel doesn't crash the moment the MMU is enabled by mapping the kernel's physical location to its virtual location.
        
*   **Key Deliverables:** `mmu.c`, `paging.S`, and the Page Table Manager.
    

* * *

## Driver Developer

**Focus:** Hardware Communication (UART & VirtIO)

The Driver Dev is the bridge between the software and the bare metal. Without this role, the kernel is "blind and deaf."

*   **Primary Ownership:**
    
    *   **PL011 UART:** Implementing a robust serial driver for kernel logging.
        
    *   **Generic Interrupt Controller (GIC):** Managing how hardware devices signal the CPU.
        
    *   **VirtIO-Net:** (High Priority) Implementing the virtual network interface required to send/receive packets for the Web GUI.
        
*   **Key Deliverables:** `uart.c`, `gic.c`, and `virtio_net.c`.
    

* * *

## Web Developer

**Focus:** The "Portal" (Aether Management Console)

The Web Dev is unique in this project: they build the user interface that runs on the _client's_ browser, which communicates with the kernel.

*   **Primary Ownership:**
    
    *   **Management Dashboard:** Creating a lightweight, responsive UI (HTML/CSS/JS) to monitor server stats (CPU usage, Memory, Tasks).
        
    *   **Protocol Design:** Working with the Driver Dev to define a minimal binary or JSON-like protocol for the kernel to send data to the frontend.
        
    *   **Resource Optimization:** Ensuring the GUI assets are small enough to be embedded into the kernel binary or served efficiently.
        
*   **Key Deliverables:** `portal/index.html`, `dashboard.js`, and the GUI asset-to-C-header conversion script.
    

* * *

## Cross-Role Collaboration

| **Collaboration** | **Goal** |
|-------------------|------------------------------- |
| Architect + Memory | Defining where the kernel lives vs. where the stacks live. |
| Memory + Driver | Ensuring MMIO addresses are mapped as "Device Memory" (Non-cacheable). |
| Driver + Web | Designing the TCP/IP or raw packet bridge to serve the UI. |