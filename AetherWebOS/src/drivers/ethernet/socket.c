#include "socket.h"

extern void tcp_send_rst(uint32_t dst_ip,
                         uint16_t dst_port,
                         uint16_t src_port);

extern void portal_get_json(uint8_t *data, size_t len);

#define MAX_LISTENERS 16

static struct socket_listener listeners[MAX_LISTENERS];
static int listener_count = 0;

void socket_init(void)
{
    listener_count = 0;
    socket_register(80, portal_get_json);
}

int socket_register(uint16_t port, socket_handler_t handler)
{
    if (listener_count >= MAX_LISTENERS)
        return -1;

    listeners[listener_count].port = port;
    listeners[listener_count].handler = handler;
    listener_count++;
    return 0;
}

socket_handler_t socket_lookup(uint16_t port)
{
    for (int i = 0; i < listener_count; i++)
        if (listeners[i].port == port)
            return listeners[i].handler;

    return 0;
}

void socket_handle_packet(uint16_t dst_port,
                          uint8_t *tcp_payload,
                          size_t payload_len,
                          uint32_t src_ip,
                          uint16_t src_port)
{
    socket_handler_t handler = socket_lookup(dst_port);

    if (!handler) {
        tcp_send_rst(src_ip, src_port, dst_port);
        return;
    }

    handler(tcp_payload, payload_len);
}
