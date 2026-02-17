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
   Queue Setup
   ============================================ */

void virtio_net_setup_queues(struct virtio_pci_device *vdev)
{
    uart_puts("[NET] Setting up RX/TX queues...\r\n");

    /* RX Queue */
    vdev->common->queue_select = RX_QUEUE_INDEX;
    uint16_t rx_size = vdev->common->queue_size;

    virtqueue_init(&rx_queue, rx_size, RX_QUEUE_INDEX, rx_ring_mem);
    virtio_pci_bind_queue(vdev, &rx_queue);

    /* Pre-fill RX buffers */
    for (uint16_t i = 0; i < rx_size; i++) {

        uint16_t id = virtqueue_add_descriptor(
            &rx_queue,
            (uint64_t)&rx_buffers[i][0],
            RX_BUF_SIZE,
            VIRTQ_DESC_F_WRITE
        );

        virtqueue_push_available(vdev, &rx_queue, id);
    }

    /* TX Queue */
    vdev->common->queue_select = TX_QUEUE_INDEX;
    uint16_t tx_size = vdev->common->queue_size;

    virtqueue_init(&tx_queue, tx_size, TX_QUEUE_INDEX, tx_ring_mem);
    virtio_pci_bind_queue(vdev, &tx_queue);

    uart_puts("[NET] Queues Ready\r\n");
}


/* ============================================
   Poll RX Queue
   ============================================ */

void virtio_net_poll(struct virtio_pci_device *vdev)
{
    uint32_t len;

    // 1. Check if the hardware has placed a packet in the Used Ring
    int id = virtqueue_pop_used(&rx_queue, &len);

    // If id is -1, there are no new packets to process
    if (id < 0)
        return;

    // 2. Adjust for the VirtIO-Net Header
    // Every buffer starts with a 12-byte virtio_net_hdr
    uint8_t *full_buffer = rx_buffers[id];
    uint8_t *eth_frame = full_buffer + sizeof(struct virtio_net_hdr);
    
    // The 'len' returned by the device includes the header, so we subtract it
    uint32_t eth_len = 0;
    if (len > sizeof(struct virtio_net_hdr)) {
        eth_len = len - sizeof(struct virtio_net_hdr);
    }

    // 3. Update Ankana's Health Stats
    global_net_stats.rx_packets++;
    
    // Log to UART for debugging (Optional, can be removed for performance)
    uart_puts("[NET] Packet RX (Len: ");
    uart_put_int(eth_len);
    uart_puts(")\r\n");

    // 4. Pass the clean Ethernet frame to the protocol dispatcher
    // This will trigger Roheet's ARP or Pritam's IP logic
    if (eth_len > 0) {
        ethernet_handle_packet(eth_frame, eth_len);
    }

    // 5. REFILL: We must return the descriptor to the Available Ring
    // We reuse the exact same buffer and ID to keep memory static
    uint16_t new_id = virtqueue_add_descriptor(
        &rx_queue,
        (uint64_t)full_buffer, // Point back to the start of the 4KB page
        RX_BUF_SIZE,
        VIRTQ_DESC_F_WRITE    // Device needs permission to write to this
    );

    // 6. Notify the device that a new slot is open
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