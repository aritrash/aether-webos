#ifndef IPV6_H
#define IPV6_H

#include <stdint.h>

struct ipv6_header {
    uint32_t vtc_flow;
    uint16_t payload_len;
    uint8_t  next_header;
    uint8_t  hop_limit;
    uint8_t  src_addr[16];
    uint8_t  dest_addr[16];
} __attribute__((packed));

void ipv6_handle(uint8_t *data, uint32_t len);
void print_ipv6(uint8_t addr[16]);


#endif
