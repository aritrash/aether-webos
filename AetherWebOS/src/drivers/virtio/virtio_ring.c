#include "drivers/virtio/virtio_ring.h"
#include "drivers/virtio/virtio_pci.h"
#include "drivers/virtio/virtio_net.h"
#include "kernel/health.h"
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

    global_net_stats.buffer_usage++; // <--- YOUR HOOK

    desc->addr  = get_phys((void*)virt_addr);
    desc->len   = len;
    desc->flags = flags;
    desc->next  = 0;

    return head;
}

/* ==========================================================================
   virtqueue_push_available: Publishes descriptor to hardware (Modern PCI)
   Developer: Roheet Purkayastha (Patched)
   ========================================================================== */
void virtqueue_push_available(struct virtio_pci_device *vdev,
                               struct virtqueue *vq,
                               uint16_t desc_head)
{
    uint16_t idx = vq->avail->idx;

    /* 1. Write descriptor head into avail ring */
    vq->avail->ring[idx % vq->size] = desc_head;

    /*
     * 2. Ensure descriptor table + ring entry
     *    are globally visible before updating idx
     */
    asm volatile("dsb st" ::: "memory");

    /* 3. Publish new available index */
    vq->avail->idx = idx + 1;

    /*
     * 4. FULL barrier before notifying device
     *    Ensures idx write reaches memory before MMIO notify
     */
    asm volatile("dsb sy" ::: "memory");

    /* 5. Ring the doorbell */
    virtqueue_notify(vdev, vq->queue_index);
}


/* ==========================================================================
   virtio_pci_bind_queue: Registers the rings with the PCI device (FIXED)
   Developer: Pritam Mondal (Patched)
   ========================================================================== */
void virtio_pci_bind_queue(struct virtio_pci_device *vdev,
                           struct virtqueue *vq)
{
    /* 1. Select queue */
    vdev->common->queue_select = vq->queue_index;

    uart_puts("[VIRTIO] Binding queue ");
    uart_put_int(vq->queue_index);
    uart_puts("\r\n");

    /* 2. Program queue size (CRITICAL STEP) */
    vdev->common->queue_size = vq->size;

    /* Ensure size write completes */
    asm volatile("dsb sy" ::: "memory");

    /* 3. Program physical addresses */
    uint64_t desc_phys  = get_phys(vq->desc);
    uint64_t avail_phys = get_phys(vq->avail);
    uint64_t used_phys  = get_phys(vq->used);

    vdev->common->queue_desc_lo = (uint32_t)(desc_phys & 0xFFFFFFFF);
    vdev->common->queue_desc_hi = (uint32_t)(desc_phys >> 32);

    vdev->common->queue_avail_lo = (uint32_t)(avail_phys & 0xFFFFFFFF);
    vdev->common->queue_avail_hi = (uint32_t)(avail_phys >> 32);

    vdev->common->queue_used_lo = (uint32_t)(used_phys & 0xFFFFFFFF);
    vdev->common->queue_used_hi = (uint32_t)(used_phys >> 32);

    /* 4. Memory barrier before enabling */
    asm volatile("dsb sy" ::: "memory");

    /* 5. Enable queue */
    vdev->common->queue_enable = 1;

    uart_puts("[VIRTIO] Queue enabled\r\n");
}

/* ==========================================================================
   virtqueue_pop_used: Adrija's logic integrated into Aether Core.
   Developer: Adrija Ghosh
   ========================================================================== */

int virtqueue_pop_used(struct virtqueue *vq, uint32_t *len_out) {
    // 'dsb sy' ensures all previous memory instructions are complete across the whole system
    __asm__ volatile("dsb sy" : : : "memory");
    // 1. Check if hardware has processed anything
    if (vq->last_used_idx == *(volatile uint16_t *)&vq->used->idx) {
        return -1; 
    }

    // Memory Barrier: Ensure we see what the hardware wrote
    asm volatile("dmb ish" : : : "memory");

    // 2. Grab the "receipt" from the hardware
    struct virtq_used_elem *receipt = &vq->used->ring[vq->last_used_idx % vq->size];
    uint16_t desc_id = (uint16_t)receipt->id;
    
    if (len_out) {
        *len_out = receipt->len; 
    }

    // 3. YOUR TELEMETRY HOOKS
    global_net_stats.buffer_usage--; 
    
    // Logic to distinguish RX (usually queue 0) from TX (usually queue 1)
    if (vq->queue_index == 0) {
        global_net_stats.rx_packets++;
    } else if (vq->queue_index == 1) {
        global_net_stats.tx_packets++;
    }

    // 4. Move counter and recycle the descriptor back to the free list
    vq->last_used_idx++;
    vq->desc[desc_id].next = vq->free_head;
    vq->free_head = desc_id;
    vq->num_free++;

    return (int)desc_id;
}