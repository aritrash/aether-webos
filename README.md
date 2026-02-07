Project Overview
-------------------

**Aether** is a high-performance, 64-bit ARM (AArch64) kernel designed for server environments. Unlike traditional operating systems that render a local GUI, Aether offloads its interface to a web-based frontend, serving a management UI over the network to minimize local CPU overhead.

Toolchain Setup
------------------

### **Linux (Ubuntu / Kali)**

Most Linux distros have the AArch64 cross-compiler available in their standard repositories.

```Bash
  # Update and install dependencies  
  sudo apt update  
  sudo apt install -y gcc-aarch64-none-elf binutils-aarch64-none-elf qemu-system-arm make git 
```
_Note for Kali users: Ensure your apt sources are updated to avoid version mismatches with QEMU._

### **Windows (MSYS2)**

For Windows devs, MSYS2 provides the best environment for GNU-like building.

1.  Download and install [MSYS2](https://www.msys2.org/).
    
2.  Open the **MSYS2 UCRT64** terminal.
    
3.  Run the following command:
    

```Bash
pacman -S mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-qemu mingw-w64-ucrt-x86_64-arm-none-eabi-gcc git   `
```
_Note: Ensure C:\\msys64\\ucrt64\\bin is added to your Windows PATH environment variable._

System Architecture
----------------------

### Boot Flow

1.  **Firmware/QEMU** loads the kernel binary into RAM at 0x40080000.
    
2.  **boot.S** (Entry Point):
    
    *   Disables secondary CPU cores (multicore support is a Milestone 3 goal).
        
    *   Sets the Stack Pointer (sp) to a safe memory region.
        
    *   Zeros out the .bss section.
        
    *   Jumps to kernel\_main().
        
3.  **kernel\_main()**: Initializes UART for debugging and prepares the network stack.
    

### Memory Mapping (Physical)

|**Address**| **Description** |
| `0x09000000` | UART PL011 (Serial Output) |
| `0x40000000` | Start of RAM |
| `0x40080000` | Kernel Entry Point (Aether Base) |

Building & Running
---------------------

### 1\. Compilation

We use a custom linker script (linker.ld) to ensure the AArch64 binary is structured correctly for the virt machine.

```Bash
# Assemble boot.S  
aarch64-none-elf-gcc -c src/boot.S -o boot.o  

# Compile Kernel (C example)  
aarch64-none-elf-gcc -ffreestanding -c src/kernel.c -o kernel.o  

# Link everything  
aarch64-none-elf-ld -T linker.ld boot.o kernel.o -o aether.elf
```
### 2\. Execution (QEMU)

To run Aether in a headless server environment with serial output:

```Bash
qemu-system-aarch64 -M virt -cpu cortex-a57 -display none -serial stdio -kernel aether.elf

```
Roadmap: The Web GUI
-----------------------

The unique "Aether Approach" involves:

1.  **VirtIO-Net Driver:** Implementation of the virtual network interface.
    
2.  **LumenStack:** A custom, minimal TCP/IP stack.
    
3.  **The Portal:** An embedded HTTP responder that serves the Aether Management Console (React/Plain HTML) to any connected client.
    

Contribution Guidelines
--------------------------

*   **Commit Messages:** Use prefix labels: feat:, fix:, asm:, or docs:.
    
*   **Registers:** All hardware-touching code must be documented with the specific ARM Register being manipulated (e.g., TTBR0\_EL1 for page tables).
    
*   **Endianness:** Remember that ARMv8 is bi-endian, but we are targeting **Little Endian** for this project.
