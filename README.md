# AETHER WEBOS
**"Surgical Precision in Bare-Metal AArch64"**

Aether WebOS is a custom, from-scratch 64-bit operating system designed for the ARMv8-A architecture (QEMU `virt` board). This version marks the transition from a pure "educational" kernel to a hardware-interfacing system capable of network and peripheral communication.

---

## Recent Milestones: The "Hardware Bridge"
We have successfully bridged the gap between the virtual CPU and physical-emulated hardware. 

* **Custom PCIe Enumerator:** Implemented a full ECAM-based scanner that identifies devices, parses Class Codes, and surgically assigns physical memory addresses (BARs) in the `0x10000000` range.
* **xHCI Host Controller:** Successfully initialized the USB 3.0 stack, including the BIOS-to-OS ownership handshake and controller reset.
* **VirtIO-Net Modern (1.0):** Achieved a live hardware handshake with the VirtIO-Net PCI device. The kernel now correctly negotiates 64-bit features and reads the hardware MAC address.

---

## System Architecture

### 1. Memory Management (MMU)
Aether uses a multi-level page table system with **L3 Surgical Mapping**.
- **Identity Mapping:** `0x40000000 - 0x48000000` (Kernel & RAM).
- **Peripheral Bridge:** `0x08000000 - 0x0A000000` (GIC, UART).
- **ECAM PCIe Bridge:** `0x70000000` (PCI Configuration Space).
- **PCIe MMIO Space:** `0x10000000+` (Dynamic BAR assignment).

### 2. The VirtIO Data Path (Current Focus)
We are currently implementing the **Virtqueue Shared Memory** model to enable high-speed Ethernet communication.
- **Descriptor Table:** The "Map" of where data sits in RAM.
- **Available Ring:** The "Outbox" where the OS sends commands to hardware.
- **Used Ring:** The "Inbox" where the hardware reports finished tasks.

---

## The Development Team & Jobs

| Lead / Member | Focus Area | Status |
| :--- | :--- | :--- |
| **Aritrash** | **Project Lead / Kernel& PCIe** | 🟢 LIVE |
| **Ankana** | **Web GUI & Frontend Architecture** | 🟡 DESIGNING |
| **Pritam** | **Memory Architect (Queue Binding) and drivers (xHCI)** | 🟡 IN PROGRESS |
| **Roheet** | **The Dispatcher (TX / Doorbell) and drivers (VirtIO-Net)** | 🟢 READY |
| **Adrija** | **The Harvester (RX / Garbage Collection) and ioremap** | 🟡 IN PROGRESS |

---

## Current Debug Logs (Successful Boot)
```text
  _____  _____ _____ _   _ _____ ____
 | ___ || ____|_   _| | | | ____|  _ \
 | |_| ||  _|   | | | |_| |  _| | |_) |
 | ___ || |___  | | |  _  | |___|  _ <
 |_| |_||_____| |_| |_| |_|_____|_| \_\
 =====================================
       AETHER OS :: Kernel v0.1.2
 =====================================
[INFO] Kernel Loaded at: 0x0000000040081280
[OK] Aether Core Online.
[OK] Exception Vector Table Loaded.
[INFO] MMU: Configuring AETHER OS Memory Map...
[OK] MMU ACTIVE: Identity & ECAM Bridge Online.
[OK] MMU Active (L3 Surgical Mapping Enabled).
[OK] Memory Subsystem: Online (Expanded L3 Support Active).
[OK] Kernel Heap Ready.

[INFO] PCIe: Initializing Aether OS PCIe Subsystem...

--- [DEBUG] PCI Header: 00:0.0 ---
0x0000000000000000: 0x0000000000081B36 0x0000000000000000 0x0000000006000000 0x0000000000000000
0x0000000000000010: 0x0000000000000000 0x0000000000000000 0x0000000000000000 0x0000000000000000
0x0000000000000020: 0x0000000000000000 0x0000000000000000 0x0000000000000000 0x0000000011001AF4
0x0000000000000030: 0x0000000000000000 0x0000000000000000 0x0000000000000000 0x0000000000000000
    -> Matching Driver: QEMU PCI Bridge

--- [DEBUG] PCI Header: 00:1.0 ---
0x0000000000000000: 0x00000000000D1B36 0x0000000000100000 0x000000000C033001 0x0000000000000000
0x0000000000000010: 0x0000000000000004 0x0000000000000000 0x0000000000000000 0x0000000000000000
0x0000000000000020: 0x0000000000000000 0x0000000000000000 0x0000000000000000 0x0000000011001AF4
0x0000000000000030: 0x0000000000000000 0x0000000000000090 0x0000000000000000 0x0000000000000100
    [PCI] Assigned 0x0000000010000000 to BAR 0
    -> Matching Driver: Generic xHCI USB 3.0
[INFO] USB: Initializing xHCI Controller...
[INFO] USB: 64-bit BAR0 Address detected: 0x0000000010000000
[INFO] USB: xHCI Version: 0x0000000000000100
[USB] Legacy Support found. Requesting ownership...
[OK] USB: BIOS ownership released.
[INFO] USB: Resetting Controller...
[OK] USB: xHCI Host Controller is READY.

--- [DEBUG] PCI Header: 00:2.0 ---
0x0000000000000000: 0x0000000010001AF4 0x0000000000100000 0x0000000002000000 0x0000000000000000
0x0000000000000010: 0x0000000000000001 0x0000000000000000 0x0000000000000000 0x0000000000000000
0x0000000000000020: 0x000000000000000C 0x0000000000000000 0x0000000000000000 0x0000000000011AF4
0x0000000000000030: 0x0000000000000000 0x0000000000000098 0x0000000000000000 0x0000000000000100
    [PCI] Assigned 0x0000000010100000 to BAR 0
    [PCI] Assigned 0x0000000010200000 to BAR 4
    -> Matching Driver: VirtIO Net Controller
[INFO] VirtIO: Initializing PCI Transport...
  -> Found VirtIO Cap Type: 5 (BAR 0)
  -> Found VirtIO Cap Type: 2 (BAR 4)
  -> Found VirtIO Cap Type: 4 (BAR 4)
  -> Found VirtIO Cap Type: 3 (BAR 4)
  -> Found VirtIO Cap Type: 1 (BAR 4)
[OK] VirtIO: Mapping Successful.
[INFO] VirtIO-Net: Starting Hardware Handshake...
[OK] VirtIO-Net: Handshake Complete. Device is LIVE.
[INFO] Hardware MAC: 0x0000000000000052:0x0000000000000054:0x0000000000000000:0x0000000000000012:0x0000000000000034:0x0000000000000056
[OK] PCIe Enumeration Complete.

[OK] PCIe Enumeration & Driver Handshakes Complete.
[INFO] GIC: Initializing Interrupt Controller...
[OK] GIC: Ready to receive interrupts.
[OK] GIC and Global Timer (1Hz) Online.
[OK] Interrupts Unmasked (EL1). System is Operational.
[INFO] Aether OS Entering Idle State (WFI)...
```
## How To Build & RUN
Clone the repository. Then open a terminal in the directory, and then:

1. Open docs/installers.md and follow the steps.

2. Run the installer after successful installation of the arm gnu toolchain from [text](https://developer.arm.com/-/media/Files/downloads/gnu/15.2.rel1/binrel/arm-gnu-toolchain-15.2.rel1-mingw-w64-i686-aarch64-none-elf.msi)

3. Locate the bin/ folder in the installed folder (generally C:\Program Files (x86)\Arm GNU Toolchain aarch64-none-elf\14.3 rel1\bin in Windows) and add it to your environment variables.

4. Open MSYS2 UCRT64 terminal, and then
```bash
nano ~/.bashrc
```
And to the end of that file, add 
```bash
export PATH="$PATH:<your_bin_folder_path>"
```
Then save the file using Ctrl + O, then Enter, then Ctrl + X.

5. Then, run the following command:
```bash
source ~/.bashrc
```

This should set the environment variable in your MSYS2 terminal. <strong>This is one-time only. Once done, its done - you don't have to repeat these steps to run the files every time. </strong>

###  Now to build, follow these steps:

```bash
make clean && make
```
Then, run

```bash
make BOARD=VIRT run
```