#include "drivers/virtio/virtio_ring.h"
#include "drivers/virtio/virtio_pci.h"
#include "drivers/virtio/virtio_net.h"
#include "kernel/memory.h"
#include "drivers/uart.h"

/**
 * get_phys: Identity mapping helper
 */
static uint64_t get_phys(void* virt) {
    return (uint64_t)virt; 
}

/* ==========================================================================
   HELPER: virtqueue_notify (Must be at the top)
   ========================================================================== */
void virtqueue_notify(struct virtio_pci_device *vdev, uint16_t queue_index) {
    // 1. Tell common config which queue we are talking about
    vdev->common->queue_select = queue_index;
    
    // 2. Get the specific doorbell offset for this queue
    uint16_t notify_off = vdev->common->queue_notify_off;

    // 3. Calculate absolute doorbell address
    uintptr_t notify_addr = (uintptr_t)vdev->notify_base + 
                            (notify_off * vdev->notify_off_multiplier);

    // 4. Ring the doorbell (write the queue index to the BAR)
    *(volatile uint16_t *)notify_addr = queue_index;
}

/* ==========================================================================
   INIT: virtqueue_init
   ========================================================================== */
void virtqueue_init(struct virtqueue *vq, uint16_t size, uint16_t index, void *p) {
    uintptr_t base = (uintptr_t)p;
    vq->size = size;
    vq->queue_index = index;

    // Descriptor Table (16-byte aligned)
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
    for (uint16_t i = 0; i < size - 1; i++) {
        vq->desc[i].next = i + 1;
    }
    vq->desc[size - 1].next = 0xFFFF;
    vq->last_used_idx = 0;
}

/* ==========================================================================
   ROHEET'S JOB: virtqueue_push_available
   ========================================================================== */
void virtqueue_push_available(struct virtio_pci_device *vdev, struct virtqueue *vq, uint16_t desc_head) {
    uint16_t idx = vq->avail->idx;
    vq->avail->ring[idx % vq->size] = desc_head;

    // Memory Barrier
    asm volatile("dsb sy" : : : "memory");

    vq->avail->idx = idx + 1;

    // Now this call will work because it's defined above!
    virtqueue_notify(vdev, vq->queue_index);
}

/* ==========================================================================
   PRITAM'S JOB: virtio_pci_bind_queue
   ========================================================================== */
void virtio_pci_bind_queue(struct virtio_pci_device *vdev, struct virtqueue *vq) {
    vdev->common->queue_select = vq->queue_index;

    uint64_t desc_phys  = get_phys(vq->desc);
    uint64_t avail_phys = get_phys(vq->avail);
    uint64_t used_phys  = get_phys(vq->used);

    vdev->common->queue_desc_lo = (uint32_t)(desc_phys & 0xFFFFFFFF);
    vdev->common->queue_desc_hi = (uint32_t)(desc_phys >> 32);

    vdev->common->queue_avail_lo = (uint32_t)(avail_phys & 0xFFFFFFFF);
    vdev->common->queue_avail_hi = (uint32_t)(avail_phys >> 32);

    vdev->common->queue_used_lo = (uint32_t)(used_phys & 0xFFFFFFFF);
    vdev->common->queue_used_hi = (uint32_t)(used_phys >> 32);

    vdev->common->queue_enable = 1;
}

/* ==========================================================================
   COMMON: virtqueue_add_descriptor
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

void virtqueue_notify(struct virtio_pci_device *vdev, uint16_t queue_index) {
    // 1. Select the queue in common config to get its notification offset
    vdev->common->queue_select = queue_index;
    uint16_t notify_off = vdev->common->queue_notify_off;

    // 2. Calculate the specific doorbell address
    uintptr_t notify_addr = (uintptr_t)vdev->notify_base + (notify_off * vdev->notify_off_multiplier);

    // 3. Ring the doorbell (write the queue index)
    *(volatile uint16_t *)notify_addr = queue_index;
}

/* =========================
   virtqueue_push_available
   ========================= */

void virtqueue_push_available(struct virtqueue *vq,
                              struct virtio_pci_device *vdev,
                              uint16_t queue_index,
                              uint16_t desc_head)
{
    // Step 1: Get current index
    uint16_t idx = vq->avail->idx;

    // Step 2: Place descriptor head into the ring
    vq->avail->ring[idx % vq->size] = desc_head;

    // Step 3: Memory barrier before publishing
    asm volatile("dsb sy" : : : "memory");

    // Step 4: Increment available index
    vq->avail->idx = idx + 1;

    // Step 5: Ring the doorbell
    virtqueue_notify(vdev, queue_index);
}



/* =========================
   virtio_pci_bind_queue
   ========================= */

void virtio_pci_bind_queue(struct virtio_pci_device *vdev,
                           struct virtqueue *vq)
{
    uint64_t desc_phys;
    uint64_t avail_phys;
    uint64_t used_phys;


    /* ----------------------------------
       1. Select Queue
       ---------------------------------- */

    vdev->common->queue_select = vq->queue_index;


    /* ----------------------------------
       2. Get Physical Addresses
       ---------------------------------- */

    desc_phys  = get_phys(vq->desc);
    avail_phys = get_phys(vq->avail);
    used_phys  = get_phys(vq->used);


    uart_puts("[VIRTIO] Binding queue ");
    uart_putc('0' + vq->queue_index);
    uart_puts("\n");


    /* ----------------------------------
       3. Program Descriptor Table
       ---------------------------------- */

    vdev->common->queue_desc_lo =
        (uint32_t)(desc_phys & 0xFFFFFFFF);

    vdev->common->queue_desc_hi =
        (uint32_t)(desc_phys >> 32);


    /* ----------------------------------
       4. Program Available Ring
       ---------------------------------- */

    vdev->common->queue_avail_lo =
        (uint32_t)(avail_phys & 0xFFFFFFFF);

    vdev->common->queue_avail_hi =
        (uint32_t)(avail_phys >> 32);


    /* ----------------------------------
       5. Program Used Ring
       ---------------------------------- */

    vdev->common->queue_used_lo =
        (uint32_t)(used_phys & 0xFFFFFFFF);

    vdev->common->queue_used_hi =
        (uint32_t)(used_phys >> 32);


    /* ----------------------------------
       6. Enable Queue
       ---------------------------------- */

    vdev->common->queue_enable = 1;


    uart_puts("[VIRTIO] Queue enabled\n");
}
