#include "drivers/ethernet/ethernet.h"
#include "drivers/uart.h"
// #include "net/arp.h"  <-- Future home for Pritam/Aritrash's ARP logic
// #include "net/ipv4.h" <-- Future home for IP logic

void ethernet_handle_packet(uint8_t *data, uint32_t len) {
    if (len < sizeof(struct eth_header)) {
        return; 
    }

    struct eth_header *eth = (struct eth_header *)data;
    uint16_t type = ntohs(eth->ethertype);

    // Pointer arithmetic to find where the payload (ARP/IP) starts
    uint8_t *payload = data + sizeof(struct eth_header);
    uint32_t payload_len = len - sizeof(struct eth_header);

    switch (type) {
        case ETH_TYPE_ARP:
            // arp_handle(payload, payload_len);
            break;
            
        case ETH_TYPE_IPV4:
            // ipv4_handle(payload, payload_len);
            break;

        default:
            // Log dropped unknown frames for Roheet's WebUI telemetry
            break;
    }
}