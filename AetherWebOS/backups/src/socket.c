#include "drivers/ethernet/socket.h"
#include "drivers/uart.h"
#include "drivers/ethernet/tcp.h"
#include "portal.h"
#include "common/utils.h"

/* =====================================================
   TCP Stub
   ===================================================== */

void tcp_send_rst(uint32_t dst_ip,
                  uint16_t dst_port,
                  uint16_t src_port)
{
    uart_puts("[SOCKET] RST requested (not implemented)\r\n");
}

/* =====================================================
   External References
   ===================================================== */

extern void tcp_send_data(uint32_t dst_ip,
                          uint16_t dst_port,
                          uint16_t src_port,
                          uint8_t *data,
                          uint32_t len);

extern void tcp_send_fin(uint32_t dst_ip,
                         uint16_t dst_port,
                         uint16_t src_port);

extern const char* aether_index_html;

/* =====================================================
   HTTP Handler
   ===================================================== */

void portal_socket_wrapper(uint8_t *data,
                           size_t len,
                           uint32_t src_ip,
                           uint16_t src_port)
{
    uart_puts("[PORTAL] Wrapper entered\r\n");

    if (len == 0)
        return;

    if (!str_contains((char*)data, "GET "))
        return;

    uart_puts("[PORTAL] GET detected\r\n");

    const char *body = aether_index_html;
    uint32_t body_len = str_len(body);

    char header[256];

    int header_len = ksnprintf(header,
                               sizeof(header),
                               "HTTP/1.0 200 OK\r\n"
                               "Content-Length: %u\r\n"
                               "Connection: close\r\n"
                               "Content-Type: text/html\r\n"
                               "\r\n",
                               body_len);

    uart_puts("[PORTAL] Sending header\r\n");

    tcp_send_data(src_ip,
                  src_port,
                  80,
                  (uint8_t*)header,
                  header_len);

    uart_puts("[PORTAL] Sending body\r\n");

    tcp_send_data(src_ip,
                  src_port,
                  80,
                  (uint8_t*)body,
                  body_len);

    uart_puts("[PORTAL] Sending FIN\r\n");

    tcp_send_fin(src_ip,
                 src_port,
                 80);
}

/* =====================================================
   Socket Registry
   ===================================================== */

#define MAX_LISTENERS 16

static struct socket_listener listeners[MAX_LISTENERS];
static int listener_count = 0;

void socket_init(void)
{
    listener_count = 0;
    socket_register(80, portal_socket_wrapper);
    uart_puts("[OK] Socket Layer Initialized (Port 80 bound)\r\n");
}

int socket_register(uint16_t port,
                    socket_handler_t handler)
{
    if (listener_count >= MAX_LISTENERS)
        return -1;

    listeners[listener_count].port = port;
    listeners[listener_count].handler = handler;
    listener_count++;

    return 0;
}

/* MUST match header declaration */
socket_handler_t socket_lookup(uint16_t port)
{
    for (int i = 0; i < listener_count; i++)
    {
        if (listeners[i].port == port)
            return listeners[i].handler;
    }

    return 0;
}

/* =====================================================
   Entry From TCP Layer
   ===================================================== */

void socket_handle_packet(uint16_t dst_port,
                          uint8_t *tcp_payload,
                          size_t payload_len,
                          uint32_t src_ip,
                          uint16_t src_port)
{
    uart_puts("[SOCKET] Packet dispatched\r\n");

    socket_handler_t handler = socket_lookup(dst_port);

    if (!handler)
    {
        uart_puts("[SOCKET] No handler for port\r\n");
        return;
    }

    handler(tcp_payload,
            payload_len,
            src_ip,
            src_port);
}