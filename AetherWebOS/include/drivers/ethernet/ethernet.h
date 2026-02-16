#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdint.h>

#define ETH_TYPE_IPV4 0x0800
#define ETH_TYPE_ARP  0x0806
#define ETH_ADDR_LEN  6


/* =====================================================
   Ethernet Header
   ===================================================== */

struct eth_header {
    uint8_t  dest_mac[ETH_ADDR_LEN];
    uint8_t  src_mac[ETH_ADDR_LEN];
    uint16_t ethertype; // Big-Endian
} __attribute__((packed));


/* =====================================================
   Byte Order Helpers
   ===================================================== */

static inline uint16_t ntohs(uint16_t val) {
    return (uint16_t)((val << 8) | (val >> 8));
}

#define htons(x) ntohs(x)


/* =====================================================
   Ethernet API (IMPORTANT)
   ===================================================== */

void ethernet_handle_packet(uint8_t *data, uint32_t len);


#endif
