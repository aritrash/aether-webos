#ifndef VIRTIO_PCI_H
#define VIRTIO_PCI_H

#include <stdint.h>

/* * Forward Declarations: 
 * We don't need the full definition of these structs here to define pointers.
 * This prevents circular include loops.
 */
struct virtio_pci_common_cfg;
struct virtio_net_config;
struct virtqueue;

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
    volatile uint32_t            *isr;
    volatile uint8_t             *notify_base;
    uint32_t                      notify_off_multiplier;

    // The Virtqueues (One for RX, one for TX)
    struct virtqueue *rx_vq;
    struct virtqueue *tx_vq;
};

/* --- Function Prototypes --- */

/**
 * Scans the PCI capability list for a specific device to find 
 * Common, Notify, ISR, and Device configurations.
 */
void virtio_pci_scan_caps(struct virtio_pci_device *vdev);

/**
 * Entry point for the PCIe registry. Matches Vendor 0x1AF4.
 */
void virtio_pci_init(uint32_t bus, uint32_t dev, uint32_t func);

#endif