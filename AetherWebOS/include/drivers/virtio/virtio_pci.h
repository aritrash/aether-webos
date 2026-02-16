#ifndef VIRTIO_PCI_H
#define VIRTIO_PCI_H

#include <stdint.h>

/* --- VirtIO Capability Types --- */
#define VIRTIO_PCI_CAP_COMMON_CFG   1
#define VIRTIO_PCI_CAP_NOTIFY_CFG   2
#define VIRTIO_PCI_CAP_ISR_CFG      3
#define VIRTIO_PCI_CAP_DEVICE_CFG   4

/**
 * struct virtio_pci_common_cfg: The Control Panel
 * This matches the official VirtIO 1.0 specification for Modern PCI.
 */
struct virtio_pci_common_cfg {
    /* About the whole device */
    uint32_t device_feature_select;
    uint32_t device_feature;
    uint32_t driver_feature_select;
    uint32_t driver_feature;
    uint16_t config_msix_vector;
    uint16_t num_queues;
    uint8_t  device_status;
    uint8_t  config_generation;

    /* About a specific virtqueue */
    uint16_t queue_select;
    uint16_t queue_size;
    uint16_t queue_msix_vector;
    uint16_t queue_enable;
    uint16_t queue_notify_off;
    
    /* 64-bit addresses split for 32-bit bus access */
    uint32_t queue_desc_lo;
    uint32_t queue_desc_hi;
    uint32_t queue_avail_lo;
    uint32_t queue_avail_hi;
    uint32_t queue_used_lo;
    uint32_t queue_used_hi;
} __attribute__((packed));

/**
 * struct virtio_net_config: The Network Identity
 * Contains the MAC address and link status.
 */
struct virtio_net_config {
    uint8_t  mac[6];
    uint16_t status;
    uint16_t max_virtqueue_pairs;
    uint16_t mtu;
} __attribute__((packed));

/* Forward declaration for the virtqueue structure */
struct virtqueue;

/**
 * The Master Device Structure
 * Holds the virtual addresses for the mapped BAR regions.
 */
struct virtio_pci_device {
    uint32_t bus;
    uint32_t dev;
    uint32_t func;

    struct virtio_pci_common_cfg *common;
    struct virtio_net_config     *device;
    volatile uint32_t            *isr;
    volatile uint8_t             *notify_base;
    uint32_t                      notify_off_multiplier;

    struct virtqueue *rx_vq;
    struct virtqueue *tx_vq;
};

/* --- Global Reference for Aether WebOS Portal --- */
/**
 * global_vnet_dev: Pointer to the initialized VirtIO-Net device.
 * This is defined in virtio_pci.c and used by portal.c for UI stats.
 */
extern struct virtio_pci_device *global_vnet_dev;

/* --- Function Prototypes --- */

/**
 * virtio_pci_init: Entry point called by PCIe discovery.
 */
void virtio_pci_init(uint32_t bus, uint32_t dev, uint32_t func);

/**
 * virtio_pci_cap_lookup: Parses PCI capabilities to find VirtIO regions.
 */
void virtio_pci_cap_lookup(struct virtio_pci_device *vdev);

/**
 * virtio_pci_reset: Instructs the device to stop all operations
 * and reset its internal state to default.
 */
void virtio_pci_reset(struct virtio_pci_device *vdev);

#endif /* VIRTIO_PCI_H */