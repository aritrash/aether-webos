#ifndef VIRTIO_RING_H
#define VIRTIO_RING_H

#include <stdint.h>
#include <stddef.h>

#define VIRTQ_DESC_F_NEXT   1
#define VIRTQ_DESC_F_WRITE  2

/* =========================
   Descriptor Table Entry
   ========================= */
struct virtq_desc {
    uint64_t addr;   /* Physical address */
    uint32_t len;    /* Length of buffer */
    uint16_t flags;  /* NEXT / WRITE */
    uint16_t next;   /* Next descriptor in chain */
} __attribute__((packed));

/* =========================
   Available Ring
   ========================= */
struct virtq_avail {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[];
} __attribute__((packed));

/* =========================
   Used Ring Elements
   ========================= */
struct virtq_used_elem {
    uint32_t id;   /* Index of descriptor head */
    uint32_t len;  /* Total bytes written */
} __attribute__((packed));

/* =========================
   Used Ring
   ========================= */
struct virtq_used {
    uint16_t flags;
    uint16_t idx;
    struct virtq_used_elem ring[];
} __attribute__((packed));

/* =========================
   Master Virtqueue
   ========================= */
struct virtqueue {
    uint16_t size;

    struct virtq_desc *desc;
    struct virtq_avail *avail;
    struct virtq_used *used;

    uint16_t free_head;
    uint16_t num_free;

    uint16_t last_used_idx;
};

// Forward declaration to avoid circular include with virtio_pci.h
struct virtio_pci_device; 

void virtqueue_init(struct virtqueue *vq, uint16_t size, void *p);
uint16_t virtqueue_add_descriptor(struct virtqueue *vq, uint64_t virt_addr, uint32_t len, uint16_t flags);
void virtqueue_notify(struct virtio_pci_device *vdev, uint16_t queue_index);

#endif
