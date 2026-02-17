#include "drivers/ethernet/tcp_state.h"
#include "kernel/timer.h"
#include "drivers/uart.h"
#include "common/utils.h"
#include "drivers/ethernet/tcp.h"

struct tcp_control_block tcb_table[MAX_TCP_CONN];

/**
 * tcp_init_stack: Wipes the table at boot.
 * Called in kernel_main after kmalloc_init.
 */
void tcp_init_stack(void) {
    for (int i = 0; i < MAX_TCP_CONN; i++) {
        tcb_table[i].state = TCP_CLOSED;
        tcb_table[i].local_port = 0;
        tcb_table[i].remote_ip = 0;
    }
    uart_puts("[OK] TCP State Machine: Initialized.\r\n");
}

/**
 * tcp_allocate_tcb: Finds a closed slot for a new connection.
 * Used by Roheet when a new SYN arrives for a listening port.
 */
struct tcp_control_block* tcp_allocate_tcb(void) {
    for (int i = 0; i < MAX_TCP_CONN; i++) {
        if (tcb_table[i].state == TCP_CLOSED) {
            // Seed the Initial Sequence Number (ISN)
            // Using uptime as requested by Aritrash for v0.1.6
            uint32_t isn = (uint32_t)get_system_uptime_ms();
            
            tcb_table[i].snd_nxt = isn;
            tcb_table[i].snd_una = isn;
            tcb_table[i].rcv_wnd = 8192; // Default 8KB window
            
            return &tcb_table[i];
        }
    }
    uart_puts("[WARN] TCP: Connection limit reached!\r\n");
    return NULL;
}

/**
 * tcp_find_tcb: Matches incoming packets to existing sessions.
 */
struct tcp_control_block* tcp_find_tcb(uint32_t remote_ip, uint16_t remote_port) {
    for (int i = 0; i < MAX_TCP_CONN; i++) {
        if (tcb_table[i].state != TCP_CLOSED &&
            tcb_table[i].remote_ip == remote_ip &&
            tcb_table[i].remote_port == remote_port) {
            return &tcb_table[i];
        }
    }
    return NULL;
}

/**
 * tcp_state_transition: Primary logic for the server-side handshake.
 */
void tcp_state_transition(struct tcp_control_block* tcb, uint8_t flags) {
    if (!tcb) return;

    switch (tcb->state) {
        case TCP_LISTEN:
            if (flags & TCP_SYN) {
                tcb->state = TCP_SYN_RCVD;
                // Note: Roheet will handle sending the SYN-ACK after this
            }
            break;

        case TCP_SYN_RCVD:
            if (flags & TCP_ACK) {
                tcb->state = TCP_ESTABLISHED;
                uart_puts("[TCP] Link Established with Remote Host.\r\n");
            }
            break;

        case TCP_ESTABLISHED:
            if (flags & TCP_FIN) {
                tcb->state = TCP_CLOSE_WAIT;
                // Prepare to close the connection
            }
            break;

        default:
            // Handle unexpected flags for the current state (RST logic)
            break;
    }
}

/**
 * tcp_get_active_count: Returns the number of non-closed connections.
 * Used for the F10 Dashboard and Ankana's health monitor.
 */
int tcp_get_active_count() {
    int count = 0;
    for (int i = 0; i < MAX_TCP_CONN; i++) {
        if (tcb_table[i].state != TCP_CLOSED) {
            count++;
        }
    }
    return count;
}