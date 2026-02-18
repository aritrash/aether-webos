#ifndef IPV4_H
#define IPV4_H

#include <stdint.h>
#include <stddef.h>

/* IPv4 Header (Minimum 20 bytes) */
struct ipv4_header {
    uint8_t  version_ihl;     // Version (4 bits) + IHL (4 bits)
    uint8_t  tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t flags_fragment;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
} __attribute__((packed));

/* Protocol Numbers */
#define IP_PROTO_ICMP   1
#define IP_PROTO_TCP    6
#define IP_PROTO_UDP    17


/* Public API */
uint16_t ipv4_checksum(void *data, size_t len);

void ipv4_handle(uint8_t *data, uint32_t len);
void ipv4_send(uint32_t dst_ip, uint8_t protocol, uint8_t *data, uint32_t len);

#endif
