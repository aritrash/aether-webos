#include "drivers/virtio/virtio_ring.h"
#include "drivers/virtio/virtio_pci.h"
#include "drivers/virtio/virtio_net.h"
#include "kernel/memory.h" // Needed for virtual_to_phys translation
#include "drivers/uart.h"

/* Helper to get physical address from a virtual pointer */
static uint64_t get_phys(void* virt) {
    // For now, if using Identity Mapping: return (uint64_t)virt;
    // Once Adrija's MMU code is full: return mmu_translate_v2p(virt);
    return (uint64_t)virt; 
}

void virtqueue_init(struct virtqueue *vq, uint16_t size, void *p) {
    uintptr_t base = (uintptr_t)p;
    vq->size = size;

    // 1. Descriptor Table (Requires 16-byte alignment)
    vq->desc = (struct virtq_desc *)base;
    base += sizeof(struct virtq_desc) * size;

    // 2. Available Ring (Requires 2-byte alignment)
    vq->avail = (struct virtq_avail *)base;
    base += sizeof(uint16_t) * (3 + size); // flags + idx + ring[size] + used_event

    // 3. Used Ring (Requires 4-byte alignment in VirtIO 1.0)
    base = (base + 3) & ~3; 
    vq->used = (struct virtq_used *)base;

    // Initialize Free List
    vq->free_head = 0;
    vq->num_free = size;
    for (uint16_t i = 0; i < size - 1; i++) {
        vq->desc[i].next = i + 1;
    }
    
    vq->last_used_idx = 0;
}

uint16_t virtqueue_add_descriptor(struct virtqueue *vq, uint64_t virt_addr, uint32_t len, uint16_t flags) {
    if (vq->num_free == 0) return 0xFFFF;

    uint16_t head = vq->free_head;
    struct virtq_desc *desc = &vq->desc[head];

    vq->free_head = desc->next;
    vq->num_free--;

    // CRITICAL: Convert Virtual Address to Physical for the Hardware
    desc->addr  = get_phys((void*)virt_addr);
    desc->len   = len;
    desc->flags = flags;
    desc->next  = 0;

    return head;
}

void virtqueue_notify(struct virtio_pci_device *vdev, uint16_t queue_index) {
    // 1. Select the queue in common config to get its notification offset
    vdev->common->queue_select = queue_index;
    uint16_t notify_off = vdev->common->queue_notify_off;

    // 2. Calculate the specific doorbell address
    uintptr_t notify_addr = (uintptr_t)vdev->notify_base + (notify_off * vdev->notify_off_multiplier);

    // 3. Ring the doorbell (write the queue index)
    *(volatile uint16_t *)notify_addr = queue_index;
}