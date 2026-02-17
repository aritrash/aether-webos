#include "drivers/ethernet/ethernet.h"
#include "drivers/uart.h"
#include "drivers/virtio/virtio_net.h"
#include "drivers/virtio/virtio_ring.h"
#include "drivers/virtio/virtio_pci.h"
#include "kernel/memory.h"
#include "common/utils.h"
#include "drivers/ethernet/arp.h"  
#include "drivers/ethernet/ipv6.h" 
#include "drivers/ethernet/ipv4.h"

extern uint8_t aether_mac[6];
extern struct virtio_pci_device *global_vnet_dev;

void ethernet_send(uint8_t *dest_mac, uint16_t ethertype, uint8_t *payload, uint32_t len) {
    // 1. Safety check using the correct global name
    if (!global_vnet_dev || !global_vnet_dev->tx_vq) return;

    // 2. Use the correct struct name for the 12-byte header
    uint32_t v_hdr_size = sizeof(struct virtio_net_hdr); 
    uint32_t total_len = v_hdr_size + sizeof(struct eth_header) + len;
    
    // 3. Allocation
    uint8_t *buffer = (uint8_t *)kmalloc(total_len);
    if (!buffer) return;

    // 4. Zero out VirtIO header
    memset(buffer, 0, v_hdr_size);

    // 5. Build Ethernet header
    struct eth_header *eth = (struct eth_header *)(buffer + v_hdr_size);
    memcpy(eth->dest_mac, dest_mac, 6);
    memcpy(eth->src_mac, aether_mac, 6);
    eth->ethertype = __builtin_bswap16(ethertype);

    // 6. Copy payload (ARP/IP data)
    memcpy(buffer + v_hdr_size + sizeof(struct eth_header), payload, len);

    // 7. Sync cache so DMA controller sees it
    // Note: Assuming clean_cache_range is in include/kernel/mmu.h or similar
    // clean_cache_range((uintptr_t)buffer, (uintptr_t)buffer + total_len);

    // 8. Queue it up using the name from your .h (tx_vq)
    struct virtqueue *tx_q = global_vnet_dev->tx_vq;
    uint16_t head = virtqueue_add_descriptor(tx_q, (uintptr_t)buffer, total_len, 0);
    
    // 9. Notify hardware
    virtqueue_push_available(global_vnet_dev, tx_q, head);
}

void ethernet_handle_packet(uint8_t *data, uint32_t len) {
    if (len < sizeof(struct eth_header)) return;

    struct eth_header *eth = (struct eth_header *)data;
    
    // Use the bswap builtin or your ntohs macro
    uint16_t type = __builtin_bswap16(eth->ethertype);

    uint8_t *payload = data + sizeof(struct eth_header);
    uint32_t payload_len = len - sizeof(struct eth_header);

    switch (type) {
        case ETH_TYPE_ARP:
            arp_handle(payload, payload_len);
            break;
            
        case ETH_TYPE_IPV4:
            ipv4_handle(payload, payload_len);
            break;

        case 0x86DD:
            ipv6_handle(payload, payload_len);
            break;

        default:
            // Log dropped unknown frames for Ankana's health monitor
            break;
    }
}