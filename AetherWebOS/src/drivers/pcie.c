#include "pcie.h"
#include "uart.h"
#include "config.h"
#include "utils.h"
#include "drivers/virtio/virtio_pci.h"
#include "drivers/virtio/virtio_net.h"
#include "drivers/usb/xhci.h"
#include "common/io.h"
#include <stddef.h>

/* * Revised Driver Registry:
 * Wildcard 0 means "Match anything for this field".
 * Class Code 0x0C0330 is the industry standard for xHCI USB 3.0.
 */
static struct pci_driver driver_registry[] = {
    {0x1AF4, 0xFFFF, 0x0,      virtio_pci_init, "VirtIO Controller"},
    {0x0,    0x0,    0x0C0330, xhci_init,       "Generic xHCI USB 3.0"},
    {0x1B36, 0xFFFF, 0x0,      NULL,            "QEMU PCI Bridge"},
    {0, 0, 0, NULL, NULL}
};

/**
 * pcie_read_config: Calculates the ECAM address and performs a safe MMIO read.
 */
uint32_t pcie_read_config(uint32_t bus, uint32_t dev, uint32_t func, uint32_t reg) {
    // 1. Precise ECAM Address calculation
    uint64_t offset = ((bus & 0xFF) << 20) | 
                      ((dev & 0x1F) << 15) | 
                      ((func & 0x07) << 12) | 
                      (reg & 0xFFF);
                      
    uint64_t address = PCIE_EXT_CFG_DATA + offset;
    
    // 2. Perform aligned read via our safe MMIO helper
    uintptr_t aligned_addr = (uintptr_t)address & ~0x3;
    uint32_t val = mmio_read32(aligned_addr);

    // 3. Handle sub-dword shifting
    uint32_t shift = (reg & 0x3) * 8;
    uint32_t result = (val >> shift);

    // Filter 16-bit registers (Vendor, Device, Command, Status)
    if (reg < 0x08) {
        return result & 0xFFFF;
    }

    return result;
}

void pcie_write_config(uint32_t bus, uint32_t dev, uint32_t func, uint32_t reg, uint32_t val) {
    uint64_t offset = ((bus & 0xFF) << 20) | 
                      ((dev & 0x1F) << 15) | 
                      ((func & 0x07) << 12) | 
                      (reg & 0xFFF);
    mmio_write32(PCIE_EXT_CFG_DATA + offset, val);
}

void pcie_dump_header(uint32_t bus, uint32_t dev, uint32_t func) {
    uart_puts("\r\n--- [DEBUG] PCI Header: 00:");
    uart_put_int(dev);
    uart_puts(".");
    uart_put_int(func);
    uart_puts(" ---\r\n");

    for (uint32_t reg = 0; reg < 64; reg += 16) {
        uart_put_hex(reg);
        uart_puts(": ");
        for (uint32_t i = 0; i < 16; i += 4) {
            uart_put_hex(pcie_read_config(bus, dev, func, reg + i));
            uart_puts(" ");
        }
        uart_puts("\r\n");
    }
}

void pcie_probe_device(uint32_t bus, uint32_t dev, uint32_t func) {
    uint16_t vendor = (uint16_t)pcie_read_config(bus, dev, func, 0x00);
    uint16_t device = (uint16_t)pcie_read_config(bus, dev, func, 0x02);
    
    uint32_t class_reg = pcie_read_config(bus, dev, func, 0x08);
    uint32_t class_code = (class_reg >> 8) & 0xFFFFFF;

    for (int i = 0; driver_registry[i].name != NULL; i++) {
        struct pci_driver *drv = &driver_registry[i];
        int match = 0;

        // Logic: Match by explicit ID or Match by Class Code
        if (drv->vendor_id != 0 && vendor == drv->vendor_id) {
            if (drv->device_id == 0xFFFF || device == drv->device_id) match = 1;
        } else if (drv->class_code != 0 && class_code == drv->class_code) {
            match = 1;
        }

        if (match) {
            // CRITICAL: Enable Memory Space (bit 1) and Bus Master (bit 2)
            uint16_t command = (uint16_t)pcie_read_config(bus, dev, func, 0x04);
            pcie_write_config(bus, dev, func, 0x04, command | 0x06);
            
            uart_puts("    -> Matching Driver: ");
            uart_puts(drv->name);
            uart_puts(" [Handshake Ready]\r\n");
            
            if (drv->init) drv->init(bus, dev, func);
            return;
        }
    }
}

void pcie_init() {
    uart_puts("\r\n[INFO] PCIe: Initializing Aether OS PCIe Subsystem...\r\n");
    uart_puts("[INFO] PCIe: Scanning Bus 0 (ECAM: ");
    uart_put_hex(PCIE_EXT_CFG_DATA);
    uart_puts(")...\r\n");
    
    int found_count = 0;

    for (uint32_t dev = 0; dev < 32; dev++) {
        // Heartbeat dot every 8 devices
        if (dev % 8 == 0) uart_puts("."); 

        for (uint32_t func = 0; func < 8; func++) {
            uint32_t vendor = pcie_read_config(0, dev, func, 0x00);
            
            // 0xFFFF = No device. Skip immediately to stop ghost matching.
            if (vendor == 0xFFFF || vendor == 0x0000) {
                if (func == 0) break; 
                continue;
            }

            found_count++;
            uart_puts("\r\n  + Device Found at 00:");
            uart_put_int(dev);
            uart_puts(".");
            uart_put_int(func);
            uart_puts(" [ID: ");
            uart_put_hex(vendor);
            uart_puts("]");

            pcie_dump_header(0, dev, func);
            pcie_probe_device(0, dev, func);
        }
    }
    
    if (found_count == 0) {
        uart_puts("\r\n[WARN] PCIe: No physical devices found. Verify config.h offsets.");
    } else {
        uart_puts("\r\n[OK] PCIe Enumeration Complete.\r\n");
    }
}