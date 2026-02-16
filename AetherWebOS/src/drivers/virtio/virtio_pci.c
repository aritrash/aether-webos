#include "drivers/virtio/virtio_pci.h"
#include "drivers/virtio/virtio_net.h"
#include "drivers/pcie.h"
#include "kernel/memory.h"
#include "common/io.h"
#include "uart.h"
#include "utils.h"

/**
 * THE LINKER FIX: Global reference definition.
 * This satisfies the extern in virtio_pci.h and portal.c
 */
struct virtio_pci_device *global_vnet_dev = NULL;

static void virtio_pci_map_capability(struct virtio_pci_device *vdev, 
                                      uint8_t type, uint8_t bar, uint32_t offset, 
                                      uint32_t length, uint8_t cap_ptr) 
{
    uint32_t bar_low = pcie_read_config(vdev->bus, vdev->dev, vdev->func, 0x10 + (bar * 4));
    uint64_t phys_addr = (uint64_t)(bar_low & ~0xF);

    if ((bar_low & 0x6) == 0x4) { // 64-bit BAR detected
        uint32_t bar_high = pcie_read_config(vdev->bus, vdev->dev, vdev->func, 0x10 + (bar * 4) + 4);
        phys_addr |= ((uint64_t)bar_high << 32);
    }

    if (phys_addr == 0) return;

    // Use ioremap to map the hardware physical range into our virtual address space
    void* virt_addr = ioremap(phys_addr + offset, length);
    if (!virt_addr) return;

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
            // The notify multiplier is located 16 bytes into the capability structure
            vdev->notify_off_multiplier = pcie_read_config(vdev->bus, vdev->dev, vdev->func, cap_ptr + 16);
            break;
    }
}

void virtio_pci_cap_lookup(struct virtio_pci_device *vdev) {
    // 0x34 is the PCI Capability Pointer register
    uint8_t cap_ptr = (uint8_t)(pcie_read_config(vdev->bus, vdev->dev, vdev->func, 0x34) & 0xFF);

    while (cap_ptr != 0 && cap_ptr < 0xFF) {
        uint32_t d0 = pcie_read_config(vdev->bus, vdev->dev, vdev->func, cap_ptr);
        uint8_t cap_id = d0 & 0xFF;
        uint8_t next = (d0 >> 8) & 0xFF;

        if (cap_id == 0x09) { // 0x09 = Vendor Specific Capability (VirtIO)
            uint8_t type = (d0 >> 24) & 0xFF; 
            uint32_t d1 = pcie_read_config(vdev->bus, vdev->dev, vdev->func, cap_ptr + 4);
            uint8_t bar = d1 & 0xFF;
            
            uint32_t offset = pcie_read_config(vdev->bus, vdev->dev, vdev->func, cap_ptr + 8);
            uint32_t length = pcie_read_config(vdev->bus, vdev->dev, vdev->func, cap_ptr + 12);

            uart_puts("  -> Found VirtIO Cap Type: ");
            uart_put_int(type);
            uart_puts(" (BAR "); uart_put_int(bar); uart_puts(")\r\n");

            virtio_pci_map_capability(vdev, type, bar, offset, length, cap_ptr);
        }
        cap_ptr = next;
    }
}

void virtio_pci_init(uint32_t bus, uint32_t dev, uint32_t func) {
    uart_puts("[INFO] VirtIO: Initializing PCI Transport...\r\n");
    
    struct virtio_pci_device *vdev = kmalloc(sizeof(struct virtio_pci_device));
    if (!vdev) {
        uart_puts("[ERROR] VirtIO: Out of memory for vdev allocation.\r\n");
        return;
    }

    vdev->bus = bus; vdev->dev = dev; vdev->func = func;
    vdev->common = NULL; vdev->device = NULL; vdev->isr = NULL; vdev->notify_base = NULL;

    // Walk the capabilities to map Common, Notify, ISR, and Device regions
    virtio_pci_cap_lookup(vdev);

    if (vdev->common && vdev->device) {
        // ASSIGNMENT: Bridge the live hardware to the Portal/WebUI
        global_vnet_dev = vdev;
        
        uart_puts("[OK] VirtIO: Mapping Successful.\r\n");
        virtio_net_init(vdev);
    } else {
        uart_puts("[ERROR] VirtIO: Critical Capability Banks missing.\r\n");
    }
}

/**
 * virtio_pci_reset: Standard VirtIO 1.0 Reset Sequence
 */
void virtio_pci_reset(struct virtio_pci_device *vdev) {
    if (!vdev || !vdev->common) return;

    uart_puts("[VIRTIO] Resetting device state...\r\n");

    /* * 1. Setting device_status to 0 triggers a reset.
     * This stops all DMA and clears all configuration.
     */
    vdev->common->device_status = 0;

    /* * 2. The spec requires the driver to wait until the device 
     * confirms the reset by reading 0 back from the status register.
     */
    while (vdev->common->device_status != 0) {
        asm volatile("nop");
    }

    uart_puts("[OK] VirtIO Device Parked.\r\n");
}