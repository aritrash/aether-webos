#ifndef VIRTIO_NET_H
#define VIRTIO_NET_H

#include <stdint.h>

/* --- VIRTIO PCI VENDOR/DEVICE IDS --- */
#define VIRTIO_VENDOR_ID          0x1AF4
#define VIRTIO_NET_DEVICE_ID      0x1000 // Transitional
#define VIRTIO_NET_MODERN_ID      0x1041 // Modern (VirtIO 1.0+)

/* --- VIRTIO PCI CAPABILITY TYPES --- */
#define VIRTIO_PCI_CAP_COMMON_CFG  1
#define VIRTIO_PCI_CAP_NOTIFY_CFG  2
#define VIRTIO_PCI_CAP_ISR_CFG     3
#define VIRTIO_PCI_CAP_DEVICE_CFG  4

/* --- DEVICE STATUS BITS (The Handshake) --- */
#define VIRTIO_STATUS_ACKNOWLEDGE  1
#define VIRTIO_STATUS_DRIVER       2
#define VIRTIO_STATUS_DRIVER_OK    4
#define VIRTIO_STATUS_FEATURES_OK  8
#define VIRTIO_STATUS_NEEDS_RESET  64
#define VIRTIO_STATUS_FAILED       128

/* --- VIRTIO NET FEATURE BITS --- */
#define VIRTIO_NET_F_CSUM          (1ULL << 0)
#define VIRTIO_NET_F_MAC           (1ULL << 5)
#define VIRTIO_NET_F_STATUS        (1ULL << 16)
#define VIRTIO_NET_F_MTU           (1ULL << 19)

/**
 * 1. PCI Capability Structure
 * Used to find where Common, Notify, and Device configs live in the BARs.
 */
struct virtio_pci_cap {
    uint8_t cap_vndr;      /* 0x09 for Vendor Specific */
    uint8_t cap_next;      /* Next capability pointer */
    uint8_t cap_len;       /* Length of this structure */
    uint8_t cfg_type;      /* VIRTIO_PCI_CAP_* */
    uint8_t bar;           /* Which BAR to map (0-5) */
    uint8_t padding[3];
    uint32_t offset;       /* Offset within the BAR */
    uint32_t length;       /* Length of the config block */
} __attribute__((packed));

/**
 * 2. Common Configuration Structure
 * The "Control Panel" for the device.
 */
struct virtio_pci_common_cfg {
    uint32_t device_feature_select; /* read-write */
    uint32_t device_feature;        /* read-only */
    uint32_t driver_feature_select; /* read-write */
    uint32_t driver_feature;        /* read-write */
    uint16_t msix_config;           /* read-write */
    uint16_t num_queues;            /* read-only */
    uint8_t  device_status;         /* read-write */
    uint8_t  config_generation;     /* read-only */

    /* About a specific virtqueue */
    uint16_t queue_select;          /* read-write */
    uint16_t queue_size;            /* read-write */
    uint16_t queue_msix_vector;     /* read-write */
    uint16_t queue_enable;          /* read-write */
    uint16_t queue_notify_off;      /* read-only */
    uint64_t queue_desc;            /* read-write */
    uint64_t queue_avail;           /* read-write */
    uint64_t queue_used;            /* read-write */
} __attribute__((packed));

/**
 * 3. Network Device Configuration
 * Specific to the Ethernet card (MAC address, etc).
 */
struct virtio_net_config {
    uint8_t  mac[6];
    uint16_t status;
    uint16_t max_virtqueue_pairs;
    uint16_t mtu;
} __attribute__((packed));

/**
 * 4. VirtIO Ring Descriptor
 * The "Envelope" used for DMA transfers.
 */
struct virtq_desc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
#define VIRTQ_DESC_F_NEXT     1
#define VIRTQ_DESC_F_WRITE    2
#define VIRTQ_DESC_F_INDIRECT 4
    uint16_t next;
} __attribute__((packed));

struct virtio_pci_device;
void virtio_pci_init(uint32_t bus, uint32_t dev, uint32_t func);
void virtio_net_init(struct virtio_pci_device *vdev);

#endif