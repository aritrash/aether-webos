#ifndef VIRTIO_RING_H
#define VIRTIO_RING_H

#include <stdint.h>
#include <stddef.h>

/* --- Descriptor Flags --- */
#define VIRTQ_DESC_F_NEXT    1   // Marks this descriptor as continuing in a chain
#define VIRTQ_DESC_F_WRITE   2   // Marks this buffer as writeable by hardware (for RX)
#define VIRTQ_DESC_F_INDIRECT 4  // Advanced: buffer contains list of descriptors

/* ==========================================================================
   VIRTIO 1.0 STRUCTURES (Strict Alignment Required)
   ========================================================================== */

/**
 * struct virtq_desc: The "Postal Address"
 * 16-byte alignment. Describes a single buffer in RAM.
 */
struct virtq_desc {
    uint64_t addr;   /* PHYSICAL address of the data buffer */
    uint32_t len;    /* Length of the data buffer */
    uint16_t flags;  /* NEXT / WRITE / INDIRECT */
    uint16_t next;   /* Index of the next descriptor if F_NEXT is set */
} __attribute__((packed, aligned(16)));

/**
 * struct virtq_avail: The "Outbox"
 * 2-byte alignment. The OS puts descriptor indices here for the hardware to process.
 */
struct virtq_avail {
    uint16_t flags;     /* 1 = No Interrupts */
    uint16_t idx;       /* Index where the OS will put the next descriptor */
    uint16_t ring[];    /* Array of descriptor indices (size = vq->size) */
    /* Note: used_event follows the ring array at the very end */
} __attribute__((packed, aligned(2)));

/**
 * struct virtq_used_elem: The "Receipt"
 */
struct virtq_used_elem {
    uint32_t id;   /* Index of the start of the completed descriptor chain */
    uint32_t len;  /* Total bytes written by the hardware (Crucial for RX) */
} __attribute__((packed));

/**
 * struct virtq_used: The "Inbox"
 * 4-byte alignment. Hardware puts receipts here when it's finished.
 */
struct virtq_used {
    uint16_t flags;           /* 1 = No Notification */
    uint16_t idx;             /* Index where the Hardware will put the next receipt */
    struct virtq_used_elem ring[]; 
    /* Note: avail_event follows the ring array at the very end */
} __attribute__((packed, aligned(4)));

/* ==========================================================================
   KERNEL TRACKING STRUCTURE
   ========================================================================== */

/**
 * struct virtqueue: The Master Controller
 * Used by the OS to track its own progress through the hardware rings.
 */
struct virtqueue {
    uint16_t size;              /* Number of descriptors in the ring (e.g., 256) */
    uint16_t queue_index;       /* 0 for RX, 1 for TX */

    struct virtq_desc *desc;    /* Ptr to the shared Descriptor Table */
    struct virtq_avail *avail;  /* Ptr to the shared Available Ring */
    struct virtq_used *used;    /* Ptr to the shared Used Ring */

    uint16_t free_head;         /* Index of the next available free descriptor */
    uint16_t num_free;          /* How many descriptors are currently unused */
    uint16_t last_used_idx;     /* Last receipt we processed from the Used Ring */
};

// Forward declaration for the notify helper
struct virtio_pci_device; 

/* --- Function Prototypes for the Implementation Team --- */

/* Pritam: Initializes the tracking struct and binds it to the PCI device */
void virtqueue_init(struct virtqueue *vq, uint16_t size, uint16_t index, void *p);
void virtio_pci_bind_queue(struct virtio_pci_device *vdev, struct virtqueue *vq);

/* Common: Adds a buffer to the descriptor table */
uint16_t virtqueue_add_descriptor(struct virtqueue *vq, uint64_t virt_addr, uint32_t len, uint16_t flags);

/* Roheet: Updates the Available ring index and notifies the hardware */
void virtqueue_push_available(struct virtio_pci_device *vdev, struct virtqueue *vq, uint16_t desc_head);

/* Adrija: Checks the Used ring and returns a processed descriptor ID */
int virtqueue_pop_used(struct virtqueue *vq, uint32_t *len_out);

void virtqueue_notify(struct virtio_pci_device *vdev, uint16_t queue_index);

#endif