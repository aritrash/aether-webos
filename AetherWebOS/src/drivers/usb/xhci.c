#include "drivers/usb/xhci.h"
#include "drivers/pcie.h"
#include "kernel/memory.h"
#include "uart.h"

/* xHCI Legacy Support Capability ID */
#define XHCI_EXT_CAP_LEGACY    1
#define USB_BIOS_OWNED        (1 << 16)
#define USB_OS_OWNED          (1 << 24)

/**
 * BIOS -> OS Handshake (The "Ownership" Logic)
 * Ensures the firmware/BIOS releases control of the USB controller
 * before Aether OS starts writing to the operational registers.
 */
static void xhci_claim_ownership(struct xhci_cap_regs *cap) {
    // HCC_PARAMS1 contains the pointer to the first Extended Capability
    uint32_t hcc = cap->hcc_params1;
    uint32_t offset = (hcc >> 16) << 2; // Offset is in DWORDS

    if (!offset) return;

    // Pointer to the first capability in the chain
    volatile uint32_t *ext = (volatile uint32_t *)((uint8_t*)cap + offset);

    while (ext) {
        uint32_t header = *ext;
        uint8_t cap_id   = header & 0xFF;
        uint8_t next_ptr = (header >> 8) & 0xFF;

        if (cap_id == XHCI_EXT_CAP_LEGACY) {
            uart_puts("[USB] Legacy Support found. Requesting ownership...\r\n");

            // The 'USB Legacy Support Control/Status' register is at ext + 1
            volatile uint32_t *ctrl = ext + 1;

            // 1. Set the OS Ownership bit
            *ctrl |= USB_OS_OWNED;

            // 2. Wait for BIOS to clear its ownership bit
            // In a real production kernel, we would add a timeout here.
            while (*ctrl & USB_BIOS_OWNED) {
                asm volatile("nop");
            }

            uart_puts("[OK] USB: BIOS ownership released.\r\n");
            return;
        }

        if (!next_ptr) break;
        ext = (volatile uint32_t *)((uint8_t*)cap + (next_ptr << 2));
    }
}

/**
 * Main xHCI Initializer
 * This matches the signature void (*pci_init_fn)(uint32_t, uint32_t, uint32_t)
 */
void xhci_init(uint32_t bus, uint32_t dev, uint32_t func) {
    uart_puts("[INFO] USB: Found xHCI Controller. Mapping registers...\r\n");

    // 1. Get Physical Address from BAR0 (PCI Offset 0x10)
    uint32_t bar_val = pcie_read_config(bus, dev, func, 0x10);
    uint64_t phys_addr = (uint64_t)(bar_val & ~0xF);

    // 2. Map using Adrija's ioremap
    // xHCI BARs are usually large; 16KB (0x4000) covers Cap + Op + Doorbell
    struct xhci_cap_regs *cap = (struct xhci_cap_regs *)ioremap(phys_addr, 0x4000);
    
    if (!cap) {
        uart_puts("[ERROR] USB: ioremap failed for xHCI.\r\n");
        return;
    }

    // 3. Perform the BIOS-OS Handshake
    xhci_claim_ownership(cap);

    // 4. Locate Operational Registers (Offset by cap_length)
    struct xhci_op_regs *op = (struct xhci_op_regs *)((uint8_t*)cap + cap->cap_length);

    uart_puts("[INFO] USB: Resetting Host Controller...\r\n");

    // 5. Reset Sequence: Stop -> Reset -> Wait
    op->usb_cmd &= ~1; // Stop bit (RS = 0)
    
    // Tiny delay to allow the stop to propagate
    for(volatile int i=0; i<10000; i++) asm volatile("nop");

    op->usb_cmd |= (1 << 1); // HCRST (Host Controller Reset)

    // Wait for the hardware to clear the reset bit
    while (op->usb_cmd & (1 << 1)) {
        asm volatile("nop");
    }

    // Wait for the controller to signal it is no longer halted
    while (op->usb_sts & 1) {
        asm volatile("nop");
    }

    uart_puts("[OK] USB: xHCI Host Controller is READY.\r\n");
}