#include <stdint.h>
#include <stddef.h>

#include "virtio_ring.h"
#include "virtio_pci.h"

/* =========================
   Internal Helpers
   ========================= */

static inline uintptr_t align_up(uintptr_t addr, uintptr_t align)
{
    return (addr + align - 1) & ~(align - 1);
}

/* =========================
   virtqueue_init
   ========================= */

void virtqueue_init(struct virtqueue *vq, uint16_t size, void *p)
{
    uintptr_t base = (uintptr_t)p;

    /* Enforce 16-byte alignment */
    base = align_up(base, 16);

    vq->size = size;

    /* -------------------------
       Descriptor Table
       ------------------------- */
    vq->desc = (struct virtq_desc *)base;
    base += sizeof(struct virtq_desc) * size;

    /* -------------------------
       Available Ring
       Layout:
       u16 flags
       u16 idx
       u16 ring[size]
       u16 used_event (optional)
       ------------------------- */
    vq->avail = (struct virtq_avail *)base;

    base += sizeof(uint16_t) * 2;        /* flags + idx */
    base += sizeof(uint16_t) * size;     /* ring entries */
    base += sizeof(uint16_t);            /* used_event */

    /* -------------------------
       Used Ring (16-byte aligned)
       Layout:
       u16 flags
       u16 idx
       struct virtq_used_elem ring[size]
       u16 avail_event (optional)
       ------------------------- */
    base = align_up(base, 16);

    vq->used = (struct virtq_used *)base;

    /* =========================
       Initialize Descriptor Free List
       ========================= */

    vq->free_head = 0;
    vq->num_free = size;

    for (uint16_t i = 0; i < size - 1; i++) {
        vq->desc[i].next = i + 1;
        vq->desc[i].flags = 0;
    }

    vq->desc[size - 1].next = 0;
    vq->desc[size - 1].flags = 0;

    /* =========================
       Initialize Rings
       ========================= */

    vq->avail->idx = 0;
    vq->avail->flags = 0;

    vq->used->idx = 0;
    vq->used->flags = 0;

    vq->last_used_idx = 0;
}

/* =========================
   virtqueue_add_descriptor
   ========================= */

uint16_t virtqueue_add_descriptor(struct virtqueue *vq,
                                   uint64_t addr,
                                   uint32_t len,
                                   uint16_t flags)
{
    if (vq->num_free == 0)
        return 0xFFFF; /* No free descriptors */

    uint16_t head = vq->free_head;
    struct virtq_desc *desc = &vq->desc[head];

    /* Pop from free list */
    vq->free_head = desc->next;
    vq->num_free--;

    desc->addr  = addr;
    desc->len   = len;
    desc->flags = flags;
    desc->next  = 0;

    return head;
}

/* =========================
   virtqueue_submit
   Publishes descriptor to avail ring
   ========================= */

void virtqueue_submit(struct virtqueue *vq, uint16_t desc_index)
{
    uint16_t avail_idx = vq->avail->idx;
    uint16_t slot = avail_idx % vq->size;

    vq->avail->ring[slot] = desc_index;

    /* Ensure descriptor writes are visible before updating idx */
    asm volatile("dmb ishst" : : : "memory");

    vq->avail->idx = avail_idx + 1;
}

/* =========================
   virtqueue_pop_used
   Returns used descriptor index or 0xFFFF
   ========================= */

uint16_t virtqueue_pop_used(struct virtqueue *vq)
{
    if (vq->last_used_idx == vq->used->idx)
        return 0xFFFF; /* Nothing used yet */

    uint16_t used_slot = vq->last_used_idx % vq->size;
    uint16_t id = vq->used->ring[used_slot].id;

    vq->last_used_idx++;

    /* Return descriptor to free list */

    vq->desc[id].next = vq->free_head;
    vq->free_head = id;
    vq->num_free++;

    return id;
}

/* =========================
   virtqueue_notify
   Doorbell write
   ========================= */

void virtqueue_notify(struct virtio_pci_device *vdev,
                      uint16_t queue_index)
{
    /*
     * For modern VirtIO PCI:
     * notify address =
     *   notify_base +
     *   (queue_notify_off * notify_off_multiplier)
     */

    uintptr_t notify_addr =
        (uintptr_t)vdev->notify_base +
        (vdev->queue_notify_off * vdev->notify_off_multiplier);

    volatile uint16_t *notify =
        (volatile uint16_t *)notify_addr;

    *notify = queue_index;
}
