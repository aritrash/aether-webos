#ifndef TCP_H
#define TCP_H

#include <stdint.h>
#include "drivers/ethernet/tcp_state.h" // Include state definitions

#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10

struct tcp_header {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq;
    uint32_t ack_seq;
    uint8_t  data_offset;
    uint8_t  flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urg_ptr;
} __attribute__((packed));

void tcp_handle(uint8_t *segment, uint32_t len, uint32_t src_ip, uint32_t dest_ip);

#endif