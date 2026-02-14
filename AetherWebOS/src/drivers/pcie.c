#include "pcie.h"
#include "uart.h"
#include "config.h"
#include "utils.h"
#include "virtio_pci.h"
#include "virtio_net.h"
#include <stddef.h>

// Note: As you and the team add more drivers, include their headers here.
// #include "include/drivers/usb/xhci.h" // For Pritam

/**
 * The Driver Registry Table.
 * Drivers are matched first by Vendor/Device ID. 
 * If Device ID is 0xFFFF, it matches any device from that vendor.
 * If Class Code is matched, it allows generic drivers (like xHCI).
 */
static struct pci_driver driver_registry[] = {
    // Vendor, Device, Class, Init Function, Name
    {0x1AF4, 0xFFFF, 0xFFFFFF, virtio_pci_init, "VirtIO Controller"},
    {0x1B36, 0xFFFF, 0xFFFFFF, NULL,            "QEMU PCI Bridge"},
    
    /* Pritam's Future Entry:
    {0x1106, 0x3483, 0x0C0330, xhci_init,       "VIA xHCI USB 3.0 Controller"},
    */
    
    {0, 0, 0, NULL, NULL} // Sentinel entry to mark end of table
};

/**
 * PCIe ECAM Formula: base + (bus << 20) | (dev << 15) | (func << 12) | reg
 */
uint32_t pcie_read_config(uint32_t bus, uint32_t dev, uint32_t func, uint32_t reg) {
    uint64_t address = PCIE_EXT_CFG_DATA | (bus << 20) | (dev << 15) | (func << 12) | (reg & ~3);
    
    asm volatile("dsb sy"); 
    asm volatile("isb");
    
    uint32_t val = *(volatile uint32_t*)address;
    
    asm volatile("dsb sy");
    return val;
}

/**
 * Probe a specific device against the driver registry.
 */
void pcie_probe_device(uint32_t bus, uint32_t dev, uint32_t func) {
    uint32_t id = pcie_read_config(bus, dev, func, 0);
    uint16_t vendor = id & 0xFFFF;
    uint16_t device = (id >> 16) & 0xFFFF;
    
    // Read Class Code info (Offset 0x08)
    uint32_t class_reg = pcie_read_config(bus, dev, func, 0x08);
    uint32_t class_code = (class_reg >> 8) & 0xFFFFFF;

    for (int i = 0; driver_registry[i].vendor_id != 0; i++) {
        struct pci_driver *drv = &driver_registry[i];
        
        // Match logic: Exact Vendor AND (Exact Device OR Wildcard 0xFFFF)
        if (vendor == drv->vendor_id && (drv->device_id == 0xFFFF || device == drv->device_id)) {
            uart_puts(" -> Matching Driver: ");
            uart_puts(drv->name);
            uart_puts("\r\n");

            if (drv->init) {
                drv->init(bus, dev, func);
            }
            return;
        }
        
        // Secondary Match: Class Code (for generic controllers like xHCI)
        if (drv->class_code != 0xFFFFFF && class_code == drv->class_code) {
            uart_puts(" -> Matching Class Driver: ");
            uart_puts(drv->name);
            uart_puts("\r\n");

            if (drv->init) {
                drv->init(bus, dev, func);
            }
            return;
        }
    }
    
    uart_puts(" -> No specialized driver found.\r\n");
}

void pcie_init() {
    uart_puts("\r\n[INFO] PCIe: Initializing Aether OS PCIe Subsystem...\r\n");

#ifdef BOARD_VIRT
    uart_puts("[DEBUG] Platform: QEMU VIRT (Target ECAM: 0x3f000000)\r\n");
#else
    uart_puts("[DEBUG] Platform: RPi4 BCM2711\r\n");
    volatile uint32_t* rgr1_sw_init = (volatile uint32_t*)(PCIE_REG_BASE + 0x9210);
    volatile uint32_t* pcie_status   = (volatile uint32_t*)(PCIE_REG_BASE + 0x4068);

    uart_puts("[DEBUG] RPi4: Waking up Controller...\r\n");
    *rgr1_sw_init |= (1 << 0);
    for(volatile int i = 0; i < 500000; i++); 

    if (!(*pcie_status & (1 << 7))) {
        uart_puts("[WARN] RPi4: PCIe Link is DOWN.\r\n");
    }
#endif

    uart_puts("[INFO] PCIe: Scanning Bus 0...\r\n");
    
    int found_count = 0;
    uint32_t bus = 0;

    for (uint32_t dev = 0; dev < 32; dev++) {
        for (uint32_t func = 0; func < 8; func++) {
            uint32_t id = pcie_read_config(bus, dev, func, 0);
            
            if (id == 0xFFFFFFFF || id == 0x00000000 || id == 0xFFFF0000) {
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

            // Use the registry to identify and initialize the hardware
            pcie_probe_device(bus, dev, func);

            // Multi-Function Check: Header Type (Offset 0x0C)
            if (func == 0) {
                uint32_t header = pcie_read_config(bus, dev, 0, 0x0C);
                if (!(header & (1 << 23))) break; 
            }
        }
    }

    if (found_count == 0) {
        uart_puts("[WARN] PCIe: Bus scan returned no devices.\r\n");
    } else {
        uart_puts("[OK] PCIe: Scan complete.\r\n");
    }
}