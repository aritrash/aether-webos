#ifndef ARP_H
#define ARP_H

#include <stdint.h>

#define ARP_HTYPE_ETH  0x0001
#define ARP_PTYPE_IPV4 0x0800

#define ARP_OPCODE_REQUEST 1
#define ARP_OPCODE_REPLY   2

struct arp_packet {
    uint16_t htype;
    uint16_t ptype;
    uint8_t  hlen;
    uint8_t  plen;
    uint16_t opcode;

    uint8_t  sender_mac[6];
    uint32_t sender_ip;

    uint8_t  target_mac[6];
    uint32_t target_ip;

} __attribute__((packed));

void arp_handle(uint8_t *data, uint32_t len);

#endif
