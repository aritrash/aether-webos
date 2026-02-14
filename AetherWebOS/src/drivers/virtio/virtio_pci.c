#include "drivers/virtio/virtio_pci.h"
#include "drivers/virtio/virtio_net.h"
#include "drivers/pcie.h"
#include "kernel/memory.h"
#include "uart.h"
#include "utils.h"

/**
 * INTERNAL HELPER: virtio_pci_map_capability
 * Maps physical BAR offsets to virtual addresses using Adrija's ioremap.
 */
static void virtio_pci_map_capability(struct virtio_pci_device *vdev, 
                                     uint8_t type, uint8_t bar, uint32_t offset, uint8_t cap_ptr) 
{
    // Read the base address from the specific BAR
    uint32_t bar_val = pcie_read_config(vdev->bus, vdev->dev, vdev->func, 0x10 + (bar * 4));
    
    // BARs can be 64-bit, but for QEMU/RPi4 VirtIO, we usually deal with 32-bit BARs.
    // We mask the lower bits (flags) to get the physical base address.
    uint64_t phys_addr = (uint64_t)(bar_val & ~0xF);

    // Map a standard 4KB page via Adrija's L3 ioremap
    void* virt_addr = ioremap(phys_addr + offset, 4096);

    switch (type) {
        case VIRTIO_PCI_CAP_COMMON_CFG:
            vdev->common = (struct virtio_pci_common_cfg *)virt_addr;
            break;
            
        case VIRTIO_PCI_CAP_DEVICE_CFG:
            vdev->device = (struct virtio_net_config *)virt_addr;
            break;

        case VIRTIO_PCI_CAP_ISR_CFG:
            vdev->isr = (volatile uint32_t *)virt_addr;
            break;

        case VIRTIO_PCI_CAP_NOTIFY_CFG:
            vdev->notify_base = (volatile uint8_t *)virt_addr;
            // The notification multiplier is located at cap_ptr + 16 (4th dword of the cap)
            vdev->notify_off_multiplier = pcie_read_config(vdev->bus, vdev->dev, vdev->func, cap_ptr + 16);
            break;
    }
}

/**
 * ROHEET'S LOGIC: virtio_pci_cap_lookup
 * Walks the PCI capability list (starting at offset 0x34) to find VirtIO blocks.
 */
void virtio_pci_cap_lookup(struct virtio_pci_device *vdev) {
    // 0x34 is the offset for the Capability Pointer in the PCI Header
    uint8_t cap_ptr = pcie_read_config(vdev->bus, vdev->dev, vdev->func, 0x34) & 0xFF;

    uart_puts("[DEBUG] VirtIO: Starting Cap Walk at offset: ");
    uart_put_hex(cap_ptr);
    uart_puts("\r\n");

    while (cap_ptr != 0 && cap_ptr < 0xFF) {
        uint32_t header = pcie_read_config(vdev->bus, vdev->dev, vdev->func, cap_ptr);
        uint8_t cap_id = header & 0xFF;
        uint8_t next   = (header >> 8) & 0xFF;
        uint8_t len    = (header >> 16) & 0xFF;

        if (cap_id == 0x09) { // Vendor Specific Capability
            uint32_t type_data = pcie_read_config(vdev->bus, vdev->dev, vdev->func, cap_ptr + 3);
            uint8_t cfg_type = type_data & 0xFF;
            uint8_t bar      = (type_data >> 8) & 0xFF;
            
            uint32_t offset = pcie_read_config(vdev->bus, vdev->dev, vdev->func, cap_ptr + 8);
            uint32_t length = pcie_read_config(vdev->bus, vdev->dev, vdev->func, cap_ptr + 12);

            uart_puts("  -> Found VirtIO Cap Type: ");
            uart_put_int(cfg_type);
            uart_puts(" at BAR ");
            uart_put_int(bar);
            uart_puts("\r\n");

            // Perform the mapping via Adrija's ioremap
            virtio_pci_map_capability(vdev, cfg_type, bar, offset, cap_ptr);
        }

        cap_ptr = next; // Move to the next capability in the linked list
    }
}

/**
 * MAIN ENTRY: virtio_pci_init
 * Called by pcie.c when a VirtIO Vendor/Device ID match is found.
 */
void virtio_pci_init(uint32_t bus, uint32_t dev, uint32_t func) {
    uart_puts("[INFO] VirtIO: Initializing PCI Transport...\r\n");

    struct virtio_pci_device *vdev = kmalloc(sizeof(struct virtio_pci_device));
    if (!vdev) {
        uart_puts("[ERROR] VirtIO: Failed to allocate device structure.\r\n");
        return;
    }

    vdev->bus = bus;
    vdev->dev = dev;
    vdev->func = func;
    
    // Initialize pointers to NULL before scanning
    vdev->common = NULL;
    vdev->device = NULL;
    vdev->isr = NULL;
    vdev->notify_base = NULL;

    virtio_pci_cap_lookup(vdev);

    // Verify that the critical structures were found and mapped
    if (vdev->common && vdev->device) {
        uart_puts("[OK] VirtIO: Caps mapped. Passing to Network Driver.\r\n");
        virtio_net_init(vdev);
    } else {
        uart_puts("[ERROR] VirtIO: Missing essential capabilities (Common/Device).\r\n");
    }
}