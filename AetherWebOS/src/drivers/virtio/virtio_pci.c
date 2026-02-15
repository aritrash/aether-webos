#include "drivers/virtio/virtio_pci.h"
#include "drivers/virtio/virtio_net.h"
#include "drivers/pcie.h"
#include "kernel/memory.h"
#include "common/io.h"
#include "uart.h"
#include "utils.h"

static void virtio_pci_map_capability(struct virtio_pci_device *vdev, 
                                      uint8_t type, uint8_t bar, uint32_t offset, 
                                      uint32_t length, uint8_t cap_ptr) 
{
    uint32_t bar_low = pcie_read_config(vdev->bus, vdev->dev, vdev->func, 0x10 + (bar * 4));
    uint64_t phys_addr = (uint64_t)(bar_low & ~0xF);

    if ((bar_low & 0x6) == 0x4) { // 64-bit
        uint32_t bar_high = pcie_read_config(vdev->bus, vdev->dev, vdev->func, 0x10 + (bar * 4) + 4);
        phys_addr |= ((uint64_t)bar_high << 32);
    }

    if (phys_addr == 0) return;

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
            vdev->notify_off_multiplier = pcie_read_config(vdev->bus, vdev->dev, vdev->func, cap_ptr + 16);
            break;
    }
}

void virtio_pci_cap_lookup(struct virtio_pci_device *vdev) {
    uint8_t cap_ptr = (uint8_t)(pcie_read_config(vdev->bus, vdev->dev, vdev->func, 0x34) & 0xFF);

    while (cap_ptr != 0 && cap_ptr < 0xFF) {
        uint32_t d0 = pcie_read_config(vdev->bus, vdev->dev, vdev->func, cap_ptr);
        uint8_t cap_id = d0 & 0xFF;
        uint8_t next = (d0 >> 8) & 0xFF;

        if (cap_id == 0x09) { // Vendor Specific (VirtIO)
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
    if (!vdev) return;

    vdev->bus = bus; vdev->dev = dev; vdev->func = func;
    vdev->common = NULL; vdev->device = NULL; vdev->isr = NULL; vdev->notify_base = NULL;

    virtio_pci_cap_lookup(vdev);

    if (vdev->common && vdev->device) {
        uart_puts("[OK] VirtIO: Mapping Successful.\r\n");
        virtio_net_init(vdev);
    } else {
        uart_puts("[ERROR] VirtIO: Critical Capability Banks missing.\r\n");
    }
}