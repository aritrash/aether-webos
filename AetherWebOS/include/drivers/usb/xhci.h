#ifndef XHCI_H
#define XHCI_H

#include <stdint.h>

/**
 * 1. xHCI Capability Registers (Read-Only)
 * Located at the very start of the BAR.
 */
struct xhci_cap_regs {
    uint8_t  cap_length;       // Offset to Operational Registers
    uint8_t  reserved;
    uint16_t hci_version;      // e.g., 0x0100 for 1.0
    uint32_t hcs_params1;      // Max slots, ports, etc.
    uint32_t hcs_params2;
    uint32_t hcs_params3;
    uint32_t hcc_params1;      // Capability pointers
    uint32_t dboff;            // Doorbell Offset
    uint32_t rtsoff;            // Runtime Registers Offset
    uint32_t hcc_params2;
} __attribute__((packed));

/**
 * 2. xHCI Operational Registers (Read-Write)
 * Located at (BAR + cap_length).
 */
struct xhci_op_regs {
    uint32_t usb_cmd;          // Bit 0: Run/Stop, Bit 1: Reset
    uint32_t usb_sts;          // Bit 0: HCHalted
    uint32_t page_size;
    uint8_t  reserved1[8];
    uint32_t dnctrl;
    uint64_t crcr;             // Command Ring Control Register
    uint8_t  reserved2[16];
    uint64_t dcbaap;           // Device Context Base Address Array Pointer
    uint32_t config;           // Max Device Slots Enabled
} __attribute__((packed));

#endif