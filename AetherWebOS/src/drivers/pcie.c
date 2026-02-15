#include "pcie.h"
#include "uart.h"
#include "config.h"
#include "utils.h"
#include "drivers/virtio/virtio_pci.h"
#include "drivers/usb/xhci.h"
#include "common/io.h"
#include <stddef.h>

// 1. Linker Fix: The Global Device Tracker
static uint32_t total_pci_devices = 0;

static struct pci_driver driver_registry[] = {
    {0x1AF4, 0x1000, 0x0,                 virtio_pci_init, "VirtIO Net Controller"}, 
    {0x0,    0x0,    PCI_CLASS_CODE_XHCI, xhci_init,       "Generic xHCI USB 3.0"}, 
    {0x1B36, 0x0001, 0x0,                 NULL,            "QEMU PCI Bridge"},
    {0, 0, 0, NULL, NULL}
};

// Allocation pool for devices with zeroed BARs (starts at 256MB)
static uint32_t next_pci_mem_addr = 0x10000000;

/**
 * get_total_pci_devices: Used by portal.c for the WebOS dashboard
 */
uint32_t get_total_pci_devices() {
    return total_pci_devices;
}

uint32_t pcie_read_config(uint32_t bus, uint32_t dev, uint32_t func, uint32_t reg) {
    uint64_t offset = ((bus & 0xFF) << 20) | ((dev & 0x1F) << 15) | ((func & 0x07) << 12) | (reg & 0xFFC);
    uintptr_t addr = (uintptr_t)PCIE_EXT_CFG_DATA + offset;
    uint32_t val = mmio_read32(addr);
    uint32_t byte_offset = reg & 0x3;
    return (byte_offset == 0) ? val : (val >> (byte_offset * 8));
}

void pcie_write_config(uint32_t bus, uint32_t dev, uint32_t func, uint32_t reg, uint32_t val) {
    uint64_t offset = ((bus & 0xFF) << 20) | ((dev & 0x1F) << 15) | ((func & 0x07) << 12) | (reg & 0xFFC);
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
    uint32_t id_reg = pcie_read_config(bus, dev, func, 0x00);
    uint16_t vendor = id_reg & 0xFFFF;
    uint32_t class_reg = pcie_read_config(bus, dev, func, 0x08);
    uint32_t class_code = (class_reg >> 8) & 0xFFFFFF;

    // 2. Logic Fix: Track every valid hardware discovery
    total_pci_devices++;

    for (int i = 0; driver_registry[i].name != NULL; i++) {
        struct pci_driver *drv = &driver_registry[i];
        if ((drv->vendor_id != 0 && vendor == drv->vendor_id) || 
            (drv->class_code != 0 && class_code == drv->class_code)) {
            
            for (int bar_idx = 0; bar_idx < 6; bar_idx++) {
                uint32_t bar_offset = 0x10 + (bar_idx * 4);
                uint32_t bar_val = pcie_read_config(bus, dev, func, bar_offset);
                
                if (bar_val != 0 && (bar_val & ~0xF) == 0) {
                    pcie_write_config(bus, dev, func, bar_offset, next_pci_mem_addr);
                    uart_puts("    [PCI] Assigned ");
                    uart_put_hex(next_pci_mem_addr);
                    uart_puts(" to BAR ");
                    uart_put_int(bar_idx);
                    uart_puts("\r\n");
                    
                    next_pci_mem_addr += 0x100000; 
                }
            }

            uint32_t cmd = pcie_read_config(bus, dev, func, 0x04) & 0xFFFF;
            pcie_write_config(bus, dev, func, 0x04, cmd | 0x0006);

            uart_puts("    -> Matching Driver: ");
            uart_puts(drv->name);
            uart_puts("\r\n");

            if (drv->init) drv->init(bus, dev, func);
            return;
        }
    }
}

void pcie_init() {
    uart_puts("\r\n[INFO] PCIe: Initializing Aether WebOS PCIe Subsystem...\r\n");
    total_pci_devices = 0; // Reset tracker before scan

    for (uint32_t dev = 0; dev < 32; dev++) {
        uint32_t vendor = pcie_read_config(0, dev, 0, 0x00) & 0xFFFF;
        if (vendor == 0xFFFF || vendor == 0x0000) continue;

        pcie_dump_header(0, dev, 0);
        pcie_probe_device(0, dev, 0);
    }
    uart_puts("[OK] PCIe Enumeration Complete.\r\n");
}