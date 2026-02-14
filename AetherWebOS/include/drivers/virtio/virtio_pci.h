#ifndef VIRTIO_PCI_H
#define VIRTIO_PCI_H

#include "drivers/virtio/virtio_net.h"

/**
 * The Master Device Structure
 * This holds the virtual addresses for the different configuration blocks
 * found during the PCI capability walk.
 */
struct virtio_pci_device {
    uint32_t bus;
    uint32_t dev;
    uint32_t func;

    // Virtual pointers to the mapped BAR regions
    struct virtio_pci_common_cfg *common;
    struct virtio_net_config     *device;
    uint32_t                     *isr;
    uint8_t                      *notify_base;
    uint32_t                      notify_off_multiplier;
};

// Function prototypes for Roheet and Aritrash
void virtio_pci_scan_caps(struct virtio_pci_device *vdev);
void virtio_pci_init(uint32_t bus, uint32_t dev, uint32_t func);

#endif