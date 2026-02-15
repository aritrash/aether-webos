#include "drivers/virtio/virtio_ring.h"
#include "drivers/virtio/virtio_pci.h"
#include "drivers/virtio/virtio_net.h"
#include "kernel/memory.h"
#include "drivers/uart.h"

static uint64_t get_phys(void* virt) {
    return (uint64_t)virt; 
}

static inline uintptr_t align_up(uintptr_t addr, uintptr_t align) {
    return (addr + align - 1) & ~(align - 1);
}

/* =========================
   virtqueue_init
   ========================= */

void virtqueue_init(struct virtqueue *vq, uint16_t size, void *p)
{
    uintptr_t base = align_up((uintptr_t)p, 16);

    vq->size = size;

    /* 1. Descriptor Table (16-byte aligned) */
    vq->desc = (struct virtq_desc *)base;
    base += sizeof(struct virtq_desc) * size;

    /* 2. Available Ring (2-byte aligned) */
    vq->avail = (struct virtq_avail *)base;

    base += sizeof(uint16_t) * 2;          // flags + idx
    base += sizeof(uint16_t) * size;       // ring[size]
    base += sizeof(uint16_t);              // used_event

    /* 3. Used Ring (4-byte aligned for VirtIO 1.0) */
    base = align_up(base, 4);
    vq->used = (struct virtq_used *)base;

    /* Initialize descriptor free list */
    vq->free_head = 0;
    vq->num_free = size;

    for (uint16_t i = 0; i < size - 1; i++) {
        vq->desc[i].next = i + 1;
        vq->desc[i].flags = 0;
    }

    vq->desc[size - 1].next = 0;
    vq->desc[size - 1].flags = 0;

    /* Initialize rings */
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
                                  uint64_t virt_addr,
                                  uint32_t len,
                                  uint16_t flags)
{
    if (vq->num_free == 0)
        return 0xFFFF;

    uint16_t head = vq->free_head;
    struct virtq_desc *desc = &vq->desc[head];

    vq->free_head = desc->next;
    vq->num_free--;

    desc->addr  = get_phys((void*)virt_addr);
    desc->len   = len;
    desc->flags = flags;
    desc->next  = 0;

    return head;
}

/* =========================
   virtqueue_push_available
   ========================= */

void virtqueue_push_available(struct virtqueue *vq,
                              struct virtio_pci_device *vdev,
                              uint16_t queue_index,
                              uint16_t desc_head)
{
    uint16_t idx = vq->avail->idx;

    /* Publish descriptor index */
    vq->avail->ring[idx % vq->size] = desc_head;

    /* Ensure descriptor + ring visible before idx update */
    asm volatile("dsb sy" : : : "memory");

    /* Increment available index */
    vq->avail->idx = idx + 1;

    /* Ring the doorbell */
    virtqueue_notify(vdev, queue_index);
}

/* =========================
   virtqueue_pop_used
   ========================= */

uint16_t virtqueue_pop_used(struct virtqueue *vq)
{
    if (vq->last_used_idx == vq->used->idx)
        return 0xFFFF;

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
   ========================= */

void virtqueue_notify(struct virtio_pci_device *vdev,
                      uint16_t queue_index)
{
    /* Select queue */
    vdev->common->queue_select = queue_index;

    /* Read notification offset */
    uint16_t notify_off = vdev->common->queue_notify_off;

    /* Calculate doorbell address */
    uintptr_t notify_addr =
        (uintptr_t)vdev->notify_base +
        (notify_off * vdev->notify_off_multiplier);

    /* Ring doorbell */
    *(volatile uint16_t *)notify_addr = queue_index;
}
