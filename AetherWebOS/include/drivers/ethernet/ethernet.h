#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdint.h>

#define ETH_TYPE_IPV4 0x0800
#define ETH_TYPE_ARP  0x0806
#define ETH_ADDR_LEN  6

/**
 * struct eth_header - Standard Ethernet II Header
 * We use __attribute__((packed)) to ensure the 14-byte size is exact.
 */
struct eth_header {
    uint8_t  dest_mac[ETH_ADDR_LEN];
    uint8_t  src_mac[ETH_ADDR_LEN];
    uint16_t ethertype; // Note: This arrives in Big-Endian (Network Byte Order)
} __attribute__((packed));

/**
 * swap_uint16: Necessary for AArch64 (Little-Endian) to handle 
 * Network Byte Order (Big-Endian).
 */
static inline uint16_t ntohs(uint16_t val) {
    return (uint16_t)((val << 8) | (val >> 8));
}

#define htons(x) ntohs(x)

#endif