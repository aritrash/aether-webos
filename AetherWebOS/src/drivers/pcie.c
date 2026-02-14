#include "pcie.h"
#include "uart.h"
#include "config.h"
#include "utils.h"
#include "drivers/virtio/virtio_pci.h"
#include "drivers/virtio/virtio_net.h"
#include "drivers/usb/xhci.h"
#include <stddef.h>

/**
 * The Driver Registry Table.
 * Drivers are matched by Vendor/Device ID or Generic Class Code.
 */
static struct pci_driver driver_registry[] = {
    // Vendor, Device, Class, Init Function, Name
    {0x1AF4, 0xFFFF, 0xFFFFFF, virtio_pci_init, "VirtIO Controller"},
    {0xFFFF, 0xFFFF, 0x0C0330, xhci_init,       "Generic xHCI USB 3.0"},
    {0x1B36, 0xFFFF, 0xFFFFFF, NULL,            "QEMU PCI Bridge"},
    {0, 0, 0, NULL, NULL} // Sentinel
};

/**
 * pcie_read_config: Accesses the ECAM (Enhanced Configuration Access Mechanism).
 * Calculated as: base + (bus << 20) | (dev << 15) | (func << 12) | reg
 */
uint32_t pcie_read_config(uint32_t bus, uint32_t dev, uint32_t func, uint32_t reg) {
    uint64_t address = PCIE_EXT_CFG_DATA | (bus << 20) | (dev << 15) | (func << 12) | (reg & ~3);
    
    // Memory Barrier to ensure order
    asm volatile("dsb sy"); 
    
    uint32_t val = *(volatile uint32_t*)address;
    
    asm volatile("dsb sy");
    return val;
}

/**
 * pcie_write_config: Essential for writing to BARs and Command registers.
 */
void pcie_write_config(uint32_t bus, uint32_t dev, uint32_t func, uint32_t reg, uint32_t val) {
    uint64_t address = PCIE_EXT_CFG_DATA | (bus << 20) | (dev << 15) | (func << 12) | (reg & ~3);
    
    asm volatile("dsb sy");
    *(volatile uint32_t*)address = val;
    asm volatile("dsb sy");
}

/**
 * pcie_probe_device: Matches hardware against our software driver registry.
 */
void pcie_probe_device(uint32_t bus, uint32_t dev, uint32_t func) {
    uint32_t id = pcie_read_config(bus, dev, func, 0);
    uint16_t vendor = id & 0xFFFF;
    uint16_t device = (id >> 16) & 0xFFFF;
    
    // Offset 0x08 contains Class Code (24-bit)
    uint32_t class_reg = pcie_read_config(bus, dev, func, 0x08);
    uint32_t class_code = (class_reg >> 8) & 0xFFFFFF;

    for (int i = 0; driver_registry[i].vendor_id != 0; i++) {
        struct pci_driver *drv = &driver_registry[i];
        
        // Match 1: Vendor + Device ID
        if (vendor == drv->vendor_id && (drv->device_id == 0xFFFF || device == drv->device_id)) {
            uart_puts(" -> Matching Driver: ");
            uart_puts(drv->name);
            uart_puts("\r\n");

            if (drv->init) drv->init(bus, dev, func);
            return;
        }
        
        // Match 2: Generic Class Code (e.g., any USB xHCI controller)
        if (drv->class_code != 0xFFFFFF && class_code == drv->class_code) {
            uart_puts(" -> Matching Class Driver: ");
            uart_puts(drv->name);
            uart_puts("\r\n");

            if (drv->init) drv->init(bus, dev, func);
            return;
        }
    }
    
    uart_puts(" -> No specialized driver found.\r\n");
}

/**
 * pcie_init: The entry point for Aether OS hardware discovery.
 */
void pcie_init() {
    uart_puts("\r\n[INFO] PCIe: Initializing Aether OS PCIe Subsystem...\r\n");

#ifdef BOARD_VIRT
    uart_puts("[DEBUG] Platform: QEMU VIRT (ECAM @ 0x70000000)\r\n");
#else
    // RPi4 Hardware Wakeup Logic
    uart_puts("[DEBUG] Platform: RPi4 BCM2711\r\n");
    volatile uint32_t* rgr1_sw_init = (volatile uint32_t*)(PCIE_REG_BASE + 0x9210);
    *rgr1_sw_init |= (1 << 0);
    for(volatile int i = 0; i < 500000; i++); 
#endif

    uart_puts("[INFO] PCIe: Scanning Bus 0...\r\n");
    
    int found_count = 0;
    for (uint32_t dev = 0; dev < 32; dev++) {
        for (uint32_t func = 0; func < 8; func++) {
            uint32_t id = pcie_read_config(0, dev, func, 0);
            
            // Skip non-existent devices (0xFFFF or 0x0000)
            if (id == 0xFFFFFFFF || id == 0x00000000) {
                if (func == 0) break; 
                continue;
            }

            found_count++;
            uart_puts("  + Found 00:");
            uart_put_int(dev);
            uart_puts(".");
            uart_put_int(func);
            uart_puts(" [ID: ");
            uart_put_hex(id);
            uart_puts("]");

            pcie_probe_device(0, dev, func);

            // Multi-Function Check: Byte 2 of offset 0x0C contains Header Type
            // If bit 7 is set, the device has multiple functions.
            if (func == 0) {
                uint32_t header_type = (pcie_read_config(0, dev, 0, 0x0C) >> 16) & 0xFF;
                if (!(header_type & 0x80)) break; 
            }
        }
    }

    if (found_count == 0) {
        uart_puts("[WARN] PCIe: No devices found on Bus 0.\r\n");
    } else {
        uart_puts("[OK] PCIe: Discovery complete.\r\n");
    }
}