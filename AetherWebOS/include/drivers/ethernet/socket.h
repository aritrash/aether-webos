#ifndef SOCKET_H
#define SOCKET_H

#include <stdint.h>
#include <stddef.h>

typedef void (*socket_handler_t)(uint8_t *data, size_t len);

struct socket_listener {
    uint16_t port;
    socket_handler_t handler;
};

void socket_init(void);
int socket_register(uint16_t port, socket_handler_t handler);
socket_handler_t socket_lookup(uint16_t port);

void socket_handle_packet(uint16_t dst_port,
                          uint8_t *tcp_payload,
                          size_t payload_len,
                          uint32_t src_ip,
                          uint16_t src_port);

#endif
