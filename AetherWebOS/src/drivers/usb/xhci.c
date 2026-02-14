#include "drivers/usb/xhci.h"
#include "drivers/pcie.h"
#include "kernel/memory.h"
#include "common/io.h"
#include "uart.h"
#include "utils.h"

/* xHCI Constants */
#define XHCI_EXT_CAP_LEGACY     1
#define USB_BIOS_OWNED         (1 << 16)
#define USB_OS_OWNED           (1 << 24)

// Register Offsets
#define XHCI_CAP_HCIVERSION    0x02
#define XHCI_CAP_HCCPARAMS1    0x10
#define XHCI_OP_USBCMD         0x00
#define XHCI_OP_USBSTS         0x04

/**
 * xhci_claim_ownership: BIOS -> OS Handshake.
 * Prevents the BIOS/SMM from fighting the kernel for the USB controller.
 */
static void xhci_claim_ownership(uintptr_t base) {
    uint32_t hcc = mmio_read32(base + XHCI_CAP_HCCPARAMS1);
    uint32_t offset = (hcc >> 16) << 2; 

    if (!offset) return;

    uintptr_t ext_ptr = base + offset;
    int timeout = 1000000;

    while (ext_ptr) {
        uint32_t header = mmio_read32(ext_ptr);
        uint8_t cap_id   = header & 0xFF;
        uint8_t next_ptr = (header >> 8) & 0xFF;

        if (cap_id == XHCI_EXT_CAP_LEGACY) {
            uart_puts("[USB] Legacy Support found. Requesting ownership...\r\n");

            uint32_t ctrl = mmio_read32(ext_ptr + 4);
            ctrl |= USB_OS_OWNED;
            mmio_write32(ext_ptr + 4, ctrl);

            // Guarded wait for BIOS release
            while ((mmio_read32(ext_ptr + 4) & USB_BIOS_OWNED) && --timeout > 0) {
                asm volatile("nop");
            }

            if (timeout <= 0) {
                uart_puts("[WARN] USB: BIOS ownership release timed out!\r\n");
            } else {
                uart_puts("[OK] USB: BIOS ownership released.\r\n");
            }
            return;
        }

        if (!next_ptr) break;
        ext_ptr = base + (next_ptr << 2);
    }
}

/**
 * Main xHCI Initializer
 */
void xhci_init(uint32_t bus, uint32_t dev, uint32_t func) {
    uart_puts("[INFO] USB: Initializing xHCI Controller...\r\n");

    // 1. Resolve 32-bit or 64-bit BAR0
    uint32_t bar_low = pcie_read_config(bus, dev, func, 0x10);
    uint64_t phys_addr = (uint64_t)(bar_low & ~0xF);

    if ((bar_low & 0x6) == 0x4) { // 64-bit BAR detection
        uint32_t bar_high = pcie_read_config(bus, dev, func, 0x14);
        phys_addr |= ((uint64_t)bar_high << 32);
        uart_puts("[INFO] USB: 64-bit BAR0 detected.\r\n");
    }

    // 2. Map the register space into vmalloc range (0x80000000+)
    uintptr_t base = (uintptr_t)ioremap(phys_addr, 0x4000);
    if (!base) {
        uart_puts("[ERROR] USB: ioremap failed.\r\n");
        return;
    }

    // 3. Print Hardware Version (Proves MMU + xHCI are talking)
    uint32_t version = mmio_read16(base + XHCI_CAP_HCIVERSION);
    uart_puts("[INFO] USB: xHCI Version: ");
    uart_put_hex(version);
    uart_puts("\r\n");

    // 4. Handshake and Reset
    xhci_claim_ownership(base);

    uint8_t cap_len = (uint8_t)(mmio_read32(base) & 0xFF);
    uintptr_t op_base = base + cap_len;

    uart_puts("[INFO] USB: Resetting Controller...\r\n");

    // Stop the controller
    uint32_t usb_cmd = mmio_read32(op_base + XHCI_OP_USBCMD);
    usb_cmd &= ~1; 
    mmio_write32(op_base + XHCI_OP_USBCMD, usb_cmd);
    
    // Tiny delay
    for(volatile int i=0; i<50000; i++) asm volatile("nop");

    // Trigger Reset (HCRST)
    mmio_write32(op_base + XHCI_OP_USBCMD, mmio_read32(op_base + XHCI_OP_USBCMD) | (1 << 1));

    // Wait for reset bit to clear
    int reset_timeout = 1000000;
    while ((mmio_read32(op_base + XHCI_OP_USBCMD) & (1 << 1)) && --reset_timeout > 0) {
        asm volatile("nop");
    }

    // Wait for CNR (Controller Not Ready) in USBSTS to clear
    int cnr_timeout = 1000000;
    while ((mmio_read32(op_base + XHCI_OP_USBSTS) & (1 << 11)) && --cnr_timeout > 0) {
        asm volatile("nop");
    }

    if (reset_timeout <= 0 || cnr_timeout <= 0) {
        uart_puts("[ERROR] USB: xHCI Controller Reset Timeout!\r\n");
    } else {
        uart_puts("[OK] USB: xHCI Host Controller is READY.\r\n");
    }
}