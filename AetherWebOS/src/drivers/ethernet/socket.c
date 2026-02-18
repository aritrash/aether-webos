#include "drivers/ethernet/socket.h"
#include "drivers/uart.h"
#include "drivers/ethernet/tcp.h"
#include "portal.h"
#include "common/utils.h"

/*
    Original Developer: Adrija Ghosh
    Lead Developer: Aritrash Sarkar

    Lead's Note: Bridging TCP state to the Application Layer (Portal).
    This version now passes src_ip and src_port so the handler
    can successfully route the HTTP response back to the browser.
    Date: 18.02.2026
*/

/* =====================================================
   TCP Stubs & Externs
   ===================================================== */

void tcp_send_rst(uint32_t dst_ip, uint16_t dst_port, uint16_t src_port) {
    uart_puts("[TCP] RST Triggered (Stub: Logic Pending)\r\n");
}

// External refs for the new HTTP logic
extern void tcp_send_data(uint32_t dst_ip, uint16_t dst_port, uint16_t src_port, uint8_t *data, uint32_t len);
extern const char* aether_index_html;

/* =====================================================
   Application Bridge
   ===================================================== */

/**
 * portal_socket_wrapper: The high-level HTTP responder.
 * Routes traffic to HTML or JSON endpoints based on the GET request path.
 */
void portal_socket_wrapper(uint8_t *data, size_t len, uint32_t src_ip, uint16_t src_port) {
    if (len < 10) return;

    // 1. Force a TCB lookup to ensure we are synced
    struct tcp_control_block *tcb = tcp_find_tcb(src_ip, src_port);
    if (!tcb) return;

    // 2. Combine HTTP Header + Body into the TCB buffer (if available) or send together
    // Chrome often hangs if it doesn't see "Content-Length" when "Connection: close" is used.
    
    if (str_contains((char*)data, "GET / ")) {
        uart_puts("[PORTAL] Executing Forced Response...\r\n");
        
        uint32_t html_len = str_len(aether_index_html);
        tcp_send_data(src_ip, src_port, 80, (uint8_t*)aether_index_html, html_len);
        
        // 3. THE FIX: Force the FIN with an explicit ACK of the GET request
        tcp_send_fin(src_ip, src_port, 80);
    }
}

/* =====================================================
   Socket Registry Logic
   ===================================================== */

#define MAX_LISTENERS 16

static struct socket_listener listeners[MAX_LISTENERS];
static int listener_count = 0;

void socket_init(void) {
    listener_count = 0;
    // Bind the wrapper to Port 80
    socket_register(80, portal_socket_wrapper);
    uart_puts("[OK] Socket Layer: Port 80 Listener Registered.\r\n");
}

int socket_register(uint16_t port, socket_handler_t handler) {
    if (listener_count >= MAX_LISTENERS)
        return -1;

    listeners[listener_count].port = port;
    listeners[listener_count].handler = handler;
    listener_count++;
    return 0;
}

socket_handler_t socket_lookup(uint16_t port) {
    for (int i = 0; i < listener_count; i++) {
        if (listeners[i].port == port)
            return listeners[i].handler;
    }
    return 0;
}

/**
 * socket_handle_packet: Dispatches incoming data to registered apps.
 */
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

    // Pass the payload AND the connection info to the handler
    handler(tcp_payload, payload_len, src_ip, src_port);
}