#include "drivers/virtio/virtio_net.h"
#include "drivers/virtio/virtio_pci.h"
#include "common/io.h"
#include "uart.h"
#include "utils.h"

void virtio_net_init(struct virtio_pci_device *vdev) {
    uart_puts("[INFO] VirtIO-Net: Starting Hardware Handshake...\r\n");

    uintptr_t common = (uintptr_t)vdev->common;
    uintptr_t device = (uintptr_t)vdev->device;

    // 1. Reset Device
    mmio_write32(common + 0x14, 0); 
    while (mmio_read32(common + 0x14) != 0); 

    // 2. Acknowledge and set Driver bit
    uint32_t status = VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;
    mmio_write32(common + 0x14, status);

    // 3. Negotiate Features (64-bit total)
    // Page 0: Standard features (like MAC)
    mmio_write32(common + 0x00, 0); // device_feature_select = 0
    uint32_t f0 = mmio_read32(common + 0x04);
    
    // Page 1: Modern features (like VERSION_1)
    mmio_write32(common + 0x00, 1); // device_feature_select = 1
    uint32_t f1 = mmio_read32(common + 0x04);

    uint32_t accept0 = 0;
    uint32_t accept1 = 0;

    // Accept MAC (bit 5 of Page 0)
    if (f0 & (1 << 5)) accept0 |= (1 << 5);
    
    // MANDATORY: Accept VERSION_1 (bit 0 of Page 1, which is bit 32 total)
    if (f1 & (1 << 0)) accept1 |= (1 << 0);

    // Write back accepted features
    mmio_write32(common + 0x08, 0); // driver_feature_select = 0
    mmio_write32(common + 0x0C, accept0);
    
    mmio_write32(common + 0x08, 1); // driver_feature_select = 1
    mmio_write32(common + 0x0C, accept1);

    // 4. Finalize Features
    status |= VIRTIO_STATUS_FEATURES_OK;
    mmio_write32(common + 0x14, status);

    // 5. Verification
    if (!(mmio_read32(common + 0x14) & VIRTIO_STATUS_FEATURES_OK)) {
        uart_puts("[ERROR] VirtIO-Net: Hardware rejected feature set (VERSION_1 missing?)\r\n");
        return;
    }

    // 6. Device is LIVE
    status |= VIRTIO_STATUS_DRIVER_OK;
    mmio_write32(common + 0x14, status);
    uart_puts("[OK] VirtIO-Net: Handshake Complete. Device is LIVE.\r\n");

    // 7. Read MAC Address surgically
    // MAC is 6 bytes. We read two 32-bit aligned dwords and shift.
    uart_puts("[INFO] Hardware MAC: ");
    uint32_t mac_low = mmio_read32(device);      // Bytes 0, 1, 2, 3
    uint32_t mac_high = mmio_read32(device + 4); // Bytes 4, 5
    
    uint8_t mac[6];
    mac[0] = (mac_low >> 0) & 0xFF;
    mac[1] = (mac_low >> 8) & 0xFF;
    mac[2] = (mac_low >> 16) & 0xFF;
    mac[3] = (mac_low >> 24) & 0xFF;
    mac[4] = (mac_high >> 0) & 0xFF;
    mac[5] = (mac_high >> 8) & 0xFF;

    for (int i = 0; i < 6; i++) {
        uart_put_hex(mac[i]);
        if (i < 5) uart_puts(":");
    }
    uart_puts("\r\n");
}