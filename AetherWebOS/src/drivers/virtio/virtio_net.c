#include "drivers/virtio/virtio_net.h"
#include "drivers/virtio/virtio_pci.h"
#include "common/io.h"
#include "uart.h"
#include "utils.h"

void virtio_net_init(struct virtio_pci_device *vdev) {
    uart_puts("[INFO] VirtIO-Net: Starting Hardware Handshake...\r\n");

    // Get the base address of the common configuration structure
    uintptr_t common = (uintptr_t)vdev->common;
    uintptr_t device = (uintptr_t)vdev->device;

    // 1. Reset the device
    mmio_write32(common + 0x14, 0); // device_status offset is 0x14 (20)
    while (mmio_read32(common + 0x14) != 0); 

    // 2. Set ACKNOWLEDGE (0x1) and DRIVER (0x2)
    uint8_t status = VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;
    mmio_write32(common + 0x14, status);

    // 4. Feature Negotiation
    mmio_write32(common + 0x00, 0); // device_feature_select = 0
    uint32_t features = mmio_read32(common + 0x04); // device_feature is at 0x04

    uint32_t accepted = 0;
    if (features & (1ULL << 32)) { // VIRTIO_NET_F_MAC is bit 32 (often features[1])
        // Note: For simplicity, we check if the MAC feature is present
    }
    
    // Most QEMU VirtIO nets provide MAC at bit 5 of feature set 0
    if (features & (1 << 5)) {
        accepted |= (1 << 5);
    }

    mmio_write32(common + 0x08, 0); // driver_feature_select = 0
    mmio_write32(common + 0x0C, accepted); // driver_feature is at 0x0C

    // 5. Set FEATURES_OK (0x8)
    status |= VIRTIO_STATUS_FEATURES_OK;
    mmio_write32(common + 0x14, status);

    // 6. Verify FEATURES_OK
    if (!(mmio_read32(common + 0x14) & VIRTIO_STATUS_FEATURES_OK)) {
        uart_puts("[ERROR] VirtIO-Net: Features rejected by hardware!\r\n");
        return;
    }

    // 7. Set DRIVER_OK (0x4)
    status |= VIRTIO_STATUS_DRIVER_OK;
    mmio_write32(common + 0x14, status);

    uart_puts("[OK] VirtIO-Net: Handshake Complete. Device is LIVE.\r\n");

    // 8. Output the MAC Address
    // The MAC address is in the Device Config space (vdev->device)
    uart_puts("[INFO] Hardware MAC: ");
    for (int i = 0; i < 6; i++) {
        // We use mmio_read32 and mask to get individual bytes safely
        uint8_t val = (uint8_t)(mmio_read32(device + i) & 0xFF);
        uart_put_hex(val);
        if (i < 5) uart_puts(":");
    }
    uart_puts("\r\n");
}