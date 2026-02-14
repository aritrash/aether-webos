#include "virtio_net.h"
#include "virtio_pci.h"
#include "virtio_ring.h"
#include "uart.h"
#include "utils.h"

void virtio_net_init(struct virtio_pci_device *vdev) {
    uart_puts("[INFO] VirtIO-Net: Initializing Network Handshake...\r\n");

    // 1. Reset the device. Writing 0 to device_status signals a reset.
    vdev->common->device_status = 0;
    while (vdev->common->device_status != 0); // Wait for the device to clear status

    // 2. Set ACKNOWLEDGE bit (0x1): We have found the device.
    vdev->common->device_status |= VIRTIO_STATUS_ACKNOWLEDGE;

    // 3. Set DRIVER bit (0x2): Aether OS knows how to drive this card.
    vdev->common->device_status |= VIRTIO_STATUS_DRIVER;

    // 4. Feature Negotiation
    // Read features offered by the device (Device Features)
    vdev->common->device_feature_select = 0; // Look at features 0-31
    uint32_t features = vdev->common->device_feature;

    // We specifically want the MAC address feature
    uint32_t desired_features = 0;
    if (features & VIRTIO_NET_F_MAC) {
        desired_features |= VIRTIO_NET_F_MAC;
    }

    // Tell the device which features we've accepted
    vdev->common->driver_feature_select = 0;
    vdev->common->driver_feature = desired_features;

    // 5. Set FEATURES_OK (0x8): Tell the device we are done negotiating.
    vdev->common->device_status |= VIRTIO_STATUS_FEATURES_OK;

    // 6. Re-read status to ensure the device accepted our features.
    if (!(vdev->common->device_status & VIRTIO_STATUS_FEATURES_OK)) {
        uart_puts("[ERROR] VirtIO-Net: Device rejected feature set!\r\n");
        return;
    }

    // 7. Finalize Initialization: Set DRIVER_OK (0x4)
    vdev->common->device_status |= VIRTIO_STATUS_DRIVER_OK;
    uart_puts("[OK] VirtIO-Net: Device is now LIVE.\r\n");

    // 8. Output the MAC Address from the Device Config space
    uart_puts("[INFO] Hardware MAC: ");
    for (int i = 0; i < 6; i++) {
        uart_put_hex(vdev->device->mac[i]);
        if (i < 5) uart_puts(":");
    }
    uart_puts("\r\n");
}