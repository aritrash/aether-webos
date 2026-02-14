#include "drivers/virtio/virtio_pci.h"
#include "drivers/virtio/virtio_net.h"
#include "drivers/pcie.h"
#include "kernel/memory.h"
#include "common/io.h"
#include "uart.h"
#include "utils.h"

/**
 * INTERNAL HELPER: virtio_pci_map_capability
 */
static void virtio_pci_map_capability(struct virtio_pci_device *vdev, 
                                     uint8_t type, uint8_t bar, uint32_t offset, 
                                     uint32_t length, uint8_t cap_ptr) 
{
    // Read BAR from PCI config space (BARs start at 0x10)
    uint32_t bar_val = pcie_read_config(vdev->bus, vdev->dev, vdev->func, 0x10 + (bar * 4));
    
    // Physical address is the BAR value minus the low-order flag bits
    uint64_t phys_addr = (uint64_t)(bar_val & ~0xF);

    // Map the region using our new expanded L3 ioremap
    void* virt_addr = ioremap(phys_addr + offset, length);

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
            // Read the multiplier (located at cap_ptr + 16 in the capability struct)
            vdev->notify_off_multiplier = pcie_read_config(vdev->bus, vdev->dev, vdev->func, cap_ptr + 16);
            break;
    }
}

/**
 * virtio_pci_cap_lookup: The "Aligned" Walker
 */
void virtio_pci_cap_lookup(struct virtio_pci_device *vdev) {
    // Read 16-bit Status (Offset 0x06). pcie_read_config handles the alignment/shift.
    uint16_t status = (uint16_t)pcie_read_config(vdev->bus, vdev->dev, vdev->func, 0x06);

    uart_puts("[DEBUG] VirtIO: PCI Status: ");
    uart_put_hex(status);
    uart_puts("\r\n");

    if (!(status & 0x0010)) {
        uart_puts("[ERROR] VirtIO: Device reports no capabilities list.\r\n");
        return;
    }

    // Read Capability Pointer at 0x34
    uint8_t cap_ptr = (uint8_t)(pcie_read_config(vdev->bus, vdev->dev, vdev->func, 0x34) & 0xFF);

    uart_puts("[DEBUG] VirtIO: Starting Cap Walk at: ");
    uart_put_hex(cap_ptr);
    uart_puts("\r\n");

    while (cap_ptr != 0 && cap_ptr < 0xFF) {
        uint32_t header = pcie_read_config(vdev->bus, vdev->dev, vdev->func, cap_ptr);
        uint8_t cap_id = (uint8_t)(header & 0xFF);
        uint8_t next   = (uint8_t)((header >> 8) & 0xFF);

        if (cap_id == 0x09) { // Vendor Specific (VirtIO)
            // Extract cfg_type (byte 3) and bar (byte 4) from the cap structure
            uint32_t type_data = pcie_read_config(vdev->bus, vdev->dev, vdev->func, cap_ptr + 3);
            uint8_t cfg_type = (uint8_t)(type_data & 0xFF);
            uint8_t bar      = (uint8_t)((type_data >> 8) & 0xFF);
            
            uint32_t offset  = pcie_read_config(vdev->bus, vdev->dev, vdev->func, cap_ptr + 8);
            uint32_t length  = pcie_read_config(vdev->bus, vdev->dev, vdev->func, cap_ptr + 12);

            uart_puts("  -> Found VirtIO Cap Type: ");
            uart_put_int(cfg_type);
            uart_puts("\r\n");

            virtio_pci_map_capability(vdev, cfg_type, bar, offset, length, cap_ptr);
        }
        cap_ptr = next;
    }
}

void virtio_pci_init(uint32_t bus, uint32_t dev, uint32_t func) {
    uart_puts("[INFO] VirtIO: Initializing PCI Transport...\r\n");

    struct virtio_pci_device *vdev = kmalloc(sizeof(struct virtio_pci_device));
    if (!vdev) {
        uart_puts("[ERROR] VirtIO: kmalloc failed.\r\n");
        return;
    }

    vdev->bus = bus;
    vdev->dev = dev;
    vdev->func = func;
    vdev->common = NULL;
    vdev->device = NULL;
    vdev->isr = NULL;
    vdev->notify_base = NULL;

    virtio_pci_cap_lookup(vdev);

    if (vdev->common && vdev->device) {
        uart_puts("[OK] VirtIO: Transport Layer Ready. Calling Net Init.\r\n");
        virtio_net_init(vdev);
    } else {
        uart_puts("[ERROR] VirtIO: Handshake failed (Missing Caps).\r\n");
    }
}