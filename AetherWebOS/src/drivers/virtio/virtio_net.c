#include "drivers/virtio/virtio_net.h"
#include "drivers/virtio/virtio_pci.h"
#include "drivers/virtio/virtio_ring.h"
#include "drivers/ethernet/ethernet.h"
#include "common/io.h"
#include "config.h"
#include "uart.h"
#include "utils.h"
#include "kernel/memory.h"
#include "kernel/health.h"



/* Forward Declarations */
void virtio_net_setup_queues(struct virtio_pci_device *vdev);
void virtio_net_poll(struct virtio_pci_device *vdev);


/* ============================================
   Configuration
   ============================================ */

#define RX_QUEUE_INDEX  0
#define TX_QUEUE_INDEX  1

#define RX_QUEUE_SIZE   256
#define TX_QUEUE_SIZE   256
#define RX_BUF_SIZE     2048

/* ============================================
   Global Queues
   ============================================ */

static struct virtqueue rx_queue;
static struct virtqueue tx_queue;

/* RX Buffers (Aligned for DMA safety) */
static uint8_t rx_buffers[RX_QUEUE_SIZE][RX_BUF_SIZE]
__attribute__((aligned(4096)));

/* Ring Memory */
static uint8_t rx_ring_mem[PAGE_SIZE * 4]
__attribute__((aligned(4096)));

static uint8_t tx_ring_mem[PAGE_SIZE * 4]
__attribute__((aligned(4096)));


/* ============================================
   VirtIO-Net Initialization
   ============================================ */

void virtio_net_init(struct virtio_pci_device *vdev)
{
    uart_puts("[INFO] VirtIO-Net: Starting Hardware Handshake...\r\n");

    uintptr_t common = (uintptr_t)vdev->common;
    uintptr_t device = (uintptr_t)vdev->device;

    /* 1. Reset */
    mmio_write32(common + 0x14, 0);
    while (mmio_read32(common + 0x14) != 0);

    /* 2. Acknowledge + Driver */
    uint32_t status = VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;
    mmio_write32(common + 0x14, status);

    /* 3. Feature negotiation */
    mmio_write32(common + 0x00, 0);
    uint32_t f0 = mmio_read32(common + 0x04);

    mmio_write32(common + 0x00, 1);
    uint32_t f1 = mmio_read32(common + 0x04);

    uint32_t accept0 = 0;
    uint32_t accept1 = 0;

    if (f0 & (1 << 5)) accept0 |= (1 << 5);   // MAC
    if (f0 & (1 << 27)) accept0 |= (1 << 27); // ANY_LAYOUT (Highly recommended)
    if (f1 & (1 << 0)) accept1 |= (1 << 0);   // VERSION_1

    mmio_write32(common + 0x08, 0);
    mmio_write32(common + 0x0C, accept0);

    mmio_write32(common + 0x08, 1);
    mmio_write32(common + 0x0C, accept1);

    status |= VIRTIO_STATUS_FEATURES_OK;
    mmio_write32(common + 0x14, status);

    if (!(mmio_read32(common + 0x14) & VIRTIO_STATUS_FEATURES_OK)) {
        uart_puts("[ERROR] VirtIO-Net: Feature negotiation failed\r\n");
        return;
    }

    status |= VIRTIO_STATUS_DRIVER_OK;
    mmio_write32(common + 0x14, status);

    uart_puts("[OK] VirtIO-Net: Device LIVE\r\n");

    /* Read MAC */
    uart_puts("[INFO] Hardware MAC: ");
    uint32_t mac_low  = mmio_read32(device);
    uint32_t mac_high = mmio_read32(device + 4);

    uint8_t mac[6];
    mac[0] = (mac_low >> 0) & 0xFF;
    mac[1] = (mac_low >> 8) & 0xFF;
    mac[2] = (mac_low >> 16) & 0xFF;
    mac[3] = (mac_low >> 24) & 0xFF;
    mac[4] = (mac_high >> 0) & 0xFF;
    mac[5] = (mac_high >> 8) & 0xFF;

    for (int i = 0; i < 6; i++) {
        uart_put_hex(mac[i]);
        if (i < 5) uart_puts(":");
    }
    uart_puts("\r\n");

    /* Setup Queues */
    virtio_net_setup_queues(vdev);
}


/* ============================================
   Queue Setup (Modern VirtIO PCI, ARM64 Safe)
   ============================================ */

void virtio_net_setup_queues(struct virtio_pci_device *vdev)
{
    uart_puts("[NET] Setting up RX/TX queues...\r\n");

    /* =========================
       RX QUEUE
       ========================= */

    vdev->common->queue_select = RX_QUEUE_INDEX;

    uint16_t rx_size = vdev->common->queue_size;
    if (rx_size == 0) {
        uart_puts("[ERROR] RX queue size is 0!\r\n");
        return;
    }

    virtqueue_init(&rx_queue, rx_size, RX_QUEUE_INDEX, rx_ring_mem);
    virtio_pci_bind_queue(vdev, &rx_queue);

    /*
     * Pre-fill ALL RX buffers first
     * Do NOT notify per descriptor.
     */
    for (uint16_t i = 0; i < rx_size; i++) {

        uint16_t id = virtqueue_add_descriptor(
            &rx_queue,
            (uint64_t)&rx_buffers[i][0],
            RX_BUF_SIZE,
            VIRTQ_DESC_F_WRITE
        );

        /* Manually publish into avail ring without notify */
        uint16_t idx = rx_queue.avail->idx;
        rx_queue.avail->ring[idx % rx_queue.size] = id;
        rx_queue.avail->idx = idx + 1;
    }

    /*
     * Ensure descriptor table + avail ring
     * are globally visible before notify
     */
    asm volatile("dsb sy" ::: "memory");

    /* Single notify for entire RX batch */
    virtqueue_notify(vdev, RX_QUEUE_INDEX);


    /* =========================
       TX QUEUE
       ========================= */

    vdev->common->queue_select = TX_QUEUE_INDEX;

    uint16_t tx_size = vdev->common->queue_size;
    if (tx_size == 0) {
        uart_puts("[ERROR] TX queue size is 0!\r\n");
        return;
    }

    virtqueue_init(&tx_queue, tx_size, TX_QUEUE_INDEX, tx_ring_mem);
    virtio_pci_bind_queue(vdev, &tx_queue);

    /*
     * No need to prefill TX.
     * TX descriptors are allocated dynamically.
     */

    asm volatile("dsb sy" ::: "memory");

    uart_puts("[NET] Queues Ready\r\n");
}



/* ============================================
   Poll RX Queue
   ============================================ */

void virtio_net_poll(struct virtio_pci_device *vdev)
{

    uint32_t len;

    /* Ensure DMA writes are visible to CPU */
    asm volatile("dsb sy" ::: "memory");

    int id = virtqueue_pop_used(&rx_queue, &len);

    if (id < 0)
        return;

    /* Memory barrier AFTER popping used ring */
    asm volatile("dsb sy" ::: "memory");

    uint8_t *full_buffer = rx_buffers[id];

    /* Modern VirtIO v1.0 header is 12 bytes */
    #define VIRTIO_NET_HDR_SZ 12
    uint8_t *eth_frame = full_buffer + VIRTIO_NET_HDR_SZ;


    uint32_t eth_len = 0;
    if (len > sizeof(struct virtio_net_hdr)) {
        eth_len = len - sizeof(struct virtio_net_hdr);
    }

    if (eth_len == 0)
        goto refill;

    /* Debug MAC verification */
    uart_puts("[RX] Dest MAC: ");
    for (int i = 0; i < 6; i++) {
        uart_put_hex(eth_frame[i]);
        if (i < 5) uart_puts(":");
    }
    uart_puts("\r\n");

    uint16_t ethertype = (eth_frame[12] << 8) | eth_frame[13];
    uart_puts("[RX] EtherType: ");
    uart_put_hex(ethertype);
    uart_puts("\r\n");

    global_net_stats.rx_packets++;

    ethernet_handle_packet(eth_frame, eth_len);

refill:
    uint16_t new_id = virtqueue_add_descriptor(
        &rx_queue,
        (uint64_t)full_buffer,
        RX_BUF_SIZE,
        VIRTQ_DESC_F_WRITE
    );

    virtqueue_push_available(vdev, &rx_queue, new_id);
}


/**
 * net_tx_reaper: Reclaims memory after packets are sent.
 * This should be called in your main kernel loop.
 */
void net_tx_reaper() {
    uint32_t len;
    int desc_id;

    // Use the local tx_queue pointer initialized in setup_queues
    // We use &tx_queue because it's a static struct in this file.
    while ((desc_id = virtqueue_pop_used(&tx_queue, &len)) != -1) {
        
        // 1. Get the address stored in the descriptor
        // This is the pointer we kmalloc'd in ethernet_send()
        void* buffer_to_free = (void*)tx_queue.desc[desc_id].addr;
        
        // 2. Safety check: Don't free NULL or obvious garbage
        if (buffer_to_free) {
            kfree(buffer_to_free);
        }
        
        // 3. Update Ankana's stats
        // We decrement usage because the buffer is back in the heap
        if (global_net_stats.buffer_usage > 0) {
            global_net_stats.buffer_usage--;
        }
        
        // Increment total TX count for Roheet's WebUI
        global_net_stats.tx_packets++;
    }
}

/**
 * virtio_net_send: The physical exit point for all Aether OS traffic.
 * Wraps data in an Ethernet frame and pushes to VirtIO TX Ring.
 */

extern uint8_t aether_mac[6];

void virtio_net_send(struct virtio_pci_device *vdev, uint8_t *dest_mac, uint16_t type, uint8_t *data, uint32_t len) {
    if (!vdev) return;

    // 1. Calculate total size: VirtIO Header + Ethernet Header + Payload
    uint32_t eth_len = 14 + len; 
    uint32_t total_len = sizeof(struct virtio_net_hdr) + eth_len;

    // 2. Allocate a single contiguous buffer for the whole frame
    // This will be freed later by Ankana's net_tx_reaper()
    uint8_t *buffer = (uint8_t *)kmalloc(total_len);
    if (!buffer) return;

    // 3. Initialize the VirtIO-Net Header (Zero it out for standard send)
    struct virtio_net_hdr *vhdr = (struct virtio_net_hdr *)buffer;
    memset(vhdr, 0, sizeof(struct virtio_net_hdr));

    // 4. Construct Ethernet Header (Directly after VirtIO header)
    uint8_t *eth_start = buffer + sizeof(struct virtio_net_hdr);
    
    // Destination MAC
    memcpy(eth_start, dest_mac, 6);
    // Source MAC (Aether OS Identity)
    memcpy(eth_start + 6, aether_mac, 6);
    // Protocol Type (e.g., 0x0800 for IPv4)
    eth_start[12] = (type >> 8) & 0xFF;
    eth_start[13] = type & 0xFF;

    // 5. Copy the Payload (The IP/TCP packet from Pritam/Roheet)
    memcpy(eth_start + 14, data, len);

    // 6. Add to TX Queue
    uint16_t id = virtqueue_add_descriptor(
        &tx_queue,
        (uint64_t)buffer,
        total_len,
        0 // Flags: 0 because this is a READ-ONLY buffer for the device
    );

    // 7. Notify Device
    virtqueue_push_available(vdev, &tx_queue, id);
    
    // Update global usage for the F10 Dashboard
    global_net_stats.buffer_usage++;
}