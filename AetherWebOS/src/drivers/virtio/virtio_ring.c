#include "drivers/virtio/virtio_ring.h"
#include "drivers/virtio/virtio_pci.h"
#include "drivers/virtio/virtio_net.h"
#include "kernel/memory.h"
#include "drivers/uart.h"
#include "utils.h"

/**
 * get_phys: Identity mapping helper for Aether OS
 */
static uint64_t get_phys(void* virt) {
    return (uint64_t)virt; 
}

/* ==========================================================================
   virtqueue_notify: Rings the doorbell to alert the device of new buffers
   ========================================================================== */
void virtqueue_notify(struct virtio_pci_device *vdev, uint16_t queue_index) {
    // 1. Tell common config which queue we are talking about
    vdev->common->queue_select = queue_index;
    
    // 2. Get the specific doorbell offset for this queue
    uint16_t notify_off = vdev->common->queue_notify_off;

    // 3. Calculate absolute doorbell address (MMIO)
    uintptr_t notify_addr = (uintptr_t)vdev->notify_base + 
                            (notify_off * vdev->notify_off_multiplier);

    // 4. Ring the doorbell (write the queue index to the PCI BAR)
    *(volatile uint16_t *)notify_addr = queue_index;
}

/* ==========================================================================
   virtqueue_init: Sets up the Descriptor Table, Available Ring, and Used Ring
   ========================================================================== */
void virtqueue_init(struct virtqueue *vq, uint16_t size, uint16_t index, void *p) {
    uintptr_t base = (uintptr_t)p;
    vq->size = size;
    vq->queue_index = index;

    // Descriptor Table (16-byte aligned per spec)
    vq->desc = (struct virtq_desc *)base;
    base += sizeof(struct virtq_desc) * size;

    // Available Ring (2-byte aligned)
    vq->avail = (struct virtq_avail *)base;
    base += 2 + 2 + (sizeof(uint16_t) * size) + 2; 

    // Used Ring (4-byte aligned)
    base = (base + 3) & ~3; 
    vq->used = (struct virtq_used *)base;

    vq->free_head = 0;
    vq->num_free = size;
    
    // Link the descriptors into a free list
    for (uint16_t i = 0; i < size - 1; i++) {
        vq->desc[i].next = i + 1;
    }
    vq->desc[size - 1].next = 0xFFFF;
    vq->last_used_idx = 0;
}

/* ==========================================================================
   virtqueue_add_descriptor: Grabs a descriptor from the free list
   ========================================================================== */
uint16_t virtqueue_add_descriptor(struct virtqueue *vq, uint64_t virt_addr, uint32_t len, uint16_t flags) {
    if (vq->num_free == 0) return 0xFFFF;

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

/* ==========================================================================
   virtqueue_push_available: Publishes the descriptor to the hardware
   ========================================================================== */
void virtqueue_push_available(struct virtio_pci_device *vdev, struct virtqueue *vq, uint16_t desc_head) {
    uint16_t idx = vq->avail->idx;
    vq->avail->ring[idx % vq->size] = desc_head;

    // Memory Barrier: Ensure the ring is updated before the index is incremented
    asm volatile("dsb sy" : : : "memory");

    vq->avail->idx = idx + 1;

    // Notify the hardware to process the new buffer
    virtqueue_notify(vdev, vq->queue_index);
}

/* ==========================================================================
   virtio_pci_bind_queue: Registers the rings with the PCI device
   ========================================================================== */
void virtio_pci_bind_queue(struct virtio_pci_device *vdev, struct virtqueue *vq) {
    // Select the specific queue for programming
    vdev->common->queue_select = vq->queue_index;

    uint64_t desc_phys  = get_phys(vq->desc);
    uint64_t avail_phys = get_phys(vq->avail);
    uint64_t used_phys  = get_phys(vq->used);

    uart_puts("[VIRTIO] Binding queue ");
    uart_put_int(vq->queue_index);
    uart_puts("\r\n");

    // Program Physical Addresses into 32-bit registers (split Lo/Hi)
    vdev->common->queue_desc_lo = (uint32_t)(desc_phys & 0xFFFFFFFF);
    vdev->common->queue_desc_hi = (uint32_t)(desc_phys >> 32);

    vdev->common->queue_avail_lo = (uint32_t)(avail_phys & 0xFFFFFFFF);
    vdev->common->queue_avail_hi = (uint32_t)(avail_phys >> 32);

    vdev->common->queue_used_lo = (uint32_t)(used_phys & 0xFFFFFFFF);
    vdev->common->queue_used_hi = (uint32_t)(used_phys >> 32);

    // Finalize the link
    vdev->common->queue_enable = 1;
    uart_puts("[VIRTIO] Queue enabled\r\n");
}

#include <stdint.h>

struct virtq_used_elem {
    uint32_t id;
    uint32_t len;
};

struct virtq_desc {
    uint32_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
};

struct virtqueue {
    uint16_t size;
    uint16_t last_used_idx;
    uint16_t free_head;
    uint16_t num_free;

    struct virtq_desc *desc;

    struct {
        uint16_t idx;
        struct virtq_used_elem *ring;
    } *used;
};

int virtqueue_pop_used(struct virtqueue *vq, uint32_t *len_out) {
    // 1. Check if there is a new receipt from the hardware
    if (vq->last_used_idx == *(volatile uint16_t *)&vq->used->idx) {
        return -1; // No new packet yet
    }

    // 2. Grab the receipt from the used ring
    struct virtq_used_elem *receipt = &vq->used->ring[vq->last_used_idx % vq->size];
    uint16_t desc_id = (uint16_t)receipt->id;
    *len_out = receipt->len; // Actual data length received

    // 3. Move the internal counter forward
    vq->last_used_idx++;

    // 4. Free the descriptor so it can be reused
    vq->desc[desc_id].next = vq->free_head;
    vq->free_head = desc_id;
    vq->num_free++;

    return (int)desc_id;
}
