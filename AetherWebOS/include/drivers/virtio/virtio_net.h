#ifndef VIRTIO_NET_H
#define VIRTIO_NET_H

#include <stdint.h>
/* * We include the ring structures here so that virtio_net_config 
 * and our init functions can reference virtqueue related logic 
 * without redefinition errors.
 */
#include "drivers/virtio/virtio_ring.h"

/* --- VIRTIO PCI VENDOR/DEVICE IDS --- */
#define VIRTIO_VENDOR_ID           0x1AF4
#define VIRTIO_NET_DEVICE_ID       0x1000 // Transitional
#define VIRTIO_NET_MODERN_ID       0x1041 // Modern (VirtIO 1.0+)

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
#define VIRTIO_NET_F_CSUM           (1ULL << 0)
#define VIRTIO_NET_F_MAC            (1ULL << 5)
#define VIRTIO_NET_F_STATUS         (1ULL << 16)
#define VIRTIO_NET_F_MTU            (1ULL << 19)

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


/* Forward declaration for the master device struct */
struct virtio_pci_device;

/* --- Function Prototypes --- */
void virtio_pci_init(uint32_t bus, uint32_t dev, uint32_t func);
void virtio_net_init(struct virtio_pci_device *vdev);

/* Network polling */
void virtio_net_poll(struct virtio_pci_device *vdev);

// include/drivers/virtio/virtio_net.h
struct virtio_net_hdr {
    uint8_t flags;
    uint8_t gso_type;
    uint16_t hdr_len;
    uint16_t gso_size;
    uint16_t csum_start;
    uint16_t csum_offset;
    // uint16_t num_buffers; // Only if VIRTIO_F_MRG_RXBUF is negotiated
} __attribute__((packed));

typedef struct virtio_net_hdr virtio_net_rx_hdr_t;
typedef struct virtio_net_hdr virtio_net_tx_hdr_t;
void net_tx_reaper();
void virtio_net_setup_queues(struct virtio_pci_device *vdev);
#endif
