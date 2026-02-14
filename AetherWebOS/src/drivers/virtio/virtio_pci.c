#include "drivers/virtio/virtio_pci.h"
#include "drivers/virtio/virtio_net.h"
#include "drivers/pcie.h"
#include "kernel/memory.h" // This pulls in the prototype for ioremap
#include "uart.h"

/* * INTERNAL HELPER: virtio_pci_map_capability
 * This is where Roheet's found offsets meet Adrija's ioremap
 */
static void virtio_pci_map_capability(struct virtio_pci_device *vdev, 
                                     uint8_t type, uint8_t bar, uint32_t offset) 
{
    uint32_t bar_val = pcie_read_config(vdev->bus, vdev->dev, vdev->func, 0x10 + (bar * 4));
    uint64_t phys_addr = (uint64_t)(bar_val & ~0xF);

    // Using Adrija's function (or the placeholder in memory.c)
    void* virt_addr = ioremap(phys_addr + offset, 4096);

    switch (type) {
        case VIRTIO_PCI_CAP_COMMON_CFG:
            vdev->common = (struct virtio_pci_common_cfg *)virt_addr;
            break;
        case VIRTIO_PCI_CAP_DEVICE_CFG:
            vdev->device = (struct virtio_net_config *)virt_addr;
            break;
        // ... (Notify and ISR cases)
    }
}

/* * ROHEET'S LOGIC: virtio_pci_cap_lookup
 */
void virtio_pci_cap_lookup(struct virtio_pci_device *vdev) {
    uint8_t cap_ptr = pcie_read_config(vdev->bus, vdev->dev, vdev->func, 0x34) & 0xFF;

    while (cap_ptr != 0) {
        uint32_t header = pcie_read_config(vdev->bus, vdev->dev, vdev->func, cap_ptr);
        uint8_t cap_id = header & 0xFF;
        uint8_t next   = (header >> 8) & 0xFF;

        if (cap_id == 0x09) {
            uint32_t data = pcie_read_config(vdev->bus, vdev->dev, vdev->func, cap_ptr + 3);
            uint8_t cfg_type = data & 0xFF;
            uint8_t bar      = (data >> 8) & 0xFF;
            uint32_t offset  = pcie_read_config(vdev->bus, vdev->dev, vdev->func, cap_ptr + 8);

            virtio_pci_map_capability(vdev, cfg_type, bar, offset);
        }
        cap_ptr = next;
    }
}

/*
 * MAIN ENTRY: virtio_pci_init
 */
void virtio_pci_init(uint32_t bus, uint32_t dev, uint32_t func) {
    // Allocation on the heap Adrija is managing
    struct virtio_pci_device *vdev = kmalloc(sizeof(struct virtio_pci_device));
    if (!vdev) return;

    vdev->bus = bus;
    vdev->dev = dev;
    vdev->func = func;

    virtio_pci_cap_lookup(vdev);

    if (vdev->common && vdev->device) {
        virtio_net_init(vdev);
    }
}