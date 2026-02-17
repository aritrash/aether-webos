#include "drivers/ethernet/tcp_state.h"
#include "kernel/timer.h"
#include "kernel/health.h"
#include "drivers/uart.h"
#include "common/utils.h"
#include "drivers/ethernet/tcp.h"

struct tcp_control_block tcb_table[MAX_TCP_CONN];

/**
 * tcp_init_stack: Wipes the table at boot.
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
 */
struct tcp_control_block* tcp_allocate_tcb(void) {
    for (int i = 0; i < MAX_TCP_CONN; i++) {
        if (tcb_table[i].state == TCP_CLOSED) {
            uint32_t isn = (uint32_t)get_system_uptime_ms();
            
            tcb_table[i].snd_nxt = isn;
            tcb_table[i].snd_una = isn;
            tcb_table[i].rcv_wnd = 8192; 
            
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
 * Now includes telemetry tracking for retransmissions.
 */
void tcp_state_transition(struct tcp_control_block* tcb, uint8_t flags) {
    if (!tcb) return;

    switch (tcb->state) {
        case TCP_LISTEN:
            if (flags & TCP_SYN) {
                tcb->state = TCP_SYN_RCVD;
            }
            break;

        case TCP_SYN_RCVD:
            // Task: Error Reporting / Retransmission Tracking
            if (flags & TCP_SYN) { 
                // If we receive a SYN while already in SYN_RCVD, it means our 
                // previous SYN-ACK likely didn't arrive.
                global_net_stats.retransmissions++; 
            }
            
            if (flags & TCP_ACK) {
                tcb->state = TCP_ESTABLISHED;
                uart_puts("[TCP] Link Established with Remote Host.\r\n");
            }
            break;

        case TCP_ESTABLISHED:
            if (flags & TCP_FIN) {
                tcb->state = TCP_CLOSE_WAIT;
            }
            break;

        default:
            break;
    }
}

/**
 * tcp_get_active_count: Returns the number of non-closed connections.
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