#include "drivers/ethernet/socket.h"
#include "drivers/uart.h"

/*
    Original Developer: Adrija Ghosh

    Lead's Note: This module is the "Reception Desk" of 
    our TCP stack. It maintains a registry of active 
    listeners (port-handler pairs) and dispatches incoming 
    TCP payloads to the correct application logic. 

    Changes Made: 
    - Fixed directory include paths.
    - Implemented tcp_send_rst stub to satisfy linker.
    - Added portal_socket_wrapper to bridge portal_get_json 
      into the void-return handler system.
    Date: 17.02.2026
*/

/* =====================================================
   TCP Stubs & Externs
   ===================================================== */

void tcp_send_rst(uint32_t dst_ip, uint16_t dst_port, uint16_t src_port) {
    uart_puts("[TCP] RST Triggered (Stub: Logic Pending)\r\n");
}

// Your actual telemetry generator from portal.c
extern char* portal_get_json(void);

/* =====================================================
   Application Bridge
   ===================================================== */

/**
 * portal_socket_wrapper: Bridges the JSON generator to the socket layer.
 * Matches the 'socket_handler_t' signature.
 */
void portal_socket_wrapper(uint8_t *data, size_t len) {
    // Suppress unused warnings for bare-metal builds
    (void)data;
    (void)len;

    // Trigger the JSON generation
    char* json_out = portal_get_json();
    (void)json_out;
    
    // For v0.1.6, we print to UART to confirm the socket hit.
    // In v0.1.7, we will pass json_out to tcp_send().
    uart_puts("[SOCKET] Port 80 hit! Telemetry Ready.\r\n");
}

/* =====================================================
   Socket Registry Logic
   ===================================================== */

#define MAX_LISTENERS 16

static struct socket_listener listeners[MAX_LISTENERS];
static int listener_count = 0;

void socket_init(void)
{
    listener_count = 0;
    // Register the wrapper, not the raw generator
    socket_register(80, portal_socket_wrapper);
    uart_puts("[OK] Socket Layer Online: Listening on Port 80.\r\n");
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
    for (int i = 0; i < listener_count; i++) {
        if (listeners[i].port == port)
            return listeners[i].handler;
    }
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

    // Call the wrapper (or any registered handler)
    handler(tcp_payload, payload_len);
}