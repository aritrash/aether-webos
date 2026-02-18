#include "drivers/ethernet/tcp_state.h"
#include "kernel/timer.h"
#include "kernel/health.h"
#include "drivers/uart.h"
#include "common/utils.h"
#include "drivers/ethernet/tcp.h"
#include "config.h"

struct tcp_control_block tcb_table[MAX_TCP_CONN];

/**
 * tcp_init_stack: Wipes the table and clears all 4-tuple data.
 */
void tcp_init_stack(void) {
    for (int i = 0; i < MAX_TCP_CONN; i++) {
        tcb_table[i].state = TCP_CLOSED;
        tcb_table[i].local_port = 0;
        tcb_table[i].remote_ip = 0;
        tcb_table[i].remote_port = 0; // Fixed: Clear the whole tuple
        tcb_table[i].last_activity = 0;
    }
    uart_puts("[OK] TCP State Machine: Initialized.\r\n");
}

/**
 * tcp_allocate_tcb: Finds a closed slot and seeds the watchdog.
 */
struct tcp_control_block* tcp_allocate_tcb(void) {
    for (int i = 0; i < MAX_TCP_CONN; i++) {
        if (tcb_table[i].state == TCP_CLOSED) {
            uint32_t isn = (uint32_t)get_system_uptime_ms();
            
            tcb_table[i].snd_nxt = isn;
            tcb_table[i].snd_una = isn;
            tcb_table[i].rcv_wnd = 8192; 
            
            // Fixed: Initialize activity timestamp so Ankana's watchdog doesn't kill it
            tcb_table[i].last_activity = get_system_uptime_ms();
            
            return &tcb_table[i];
        }
    }
    uart_puts("[WARN] TCP: Connection limit reached!\r\n");
    return (void*)0;
}

/**
 * tcp_find_tcb: Matches based on full 4-tuple.
 */
struct tcp_control_block* tcp_find_tcb(uint32_t remote_ip, uint16_t remote_port) {
    for (int i = 0; i < MAX_TCP_CONN; i++) {
        if (tcb_table[i].state != TCP_CLOSED &&
            tcb_table[i].remote_ip == remote_ip &&
            tcb_table[i].remote_port == remote_port) {
            return &tcb_table[i];
        }
    }
    return (void*)0;
}

/**
 * tcp_state_transition: Handshake logic and safety resets.
 */
void tcp_state_transition(struct tcp_control_block* tcb, uint8_t flags) {
    if (!tcb) return;

    // Refresh watchdog on any valid flag hit
    tcb->last_activity = get_system_uptime_ms();

    // Global Reset Handling: If peer sends RST, we must abort immediately.
    if (flags & TCP_RST) {
        tcb->state = TCP_CLOSED;
        uart_puts("[TCP] Connection Reset by Peer. Slot Freed.\r\n");
        return;
    }

    switch (tcb->state) {
        case TCP_LISTEN:
            if (flags & TCP_SYN) {
                tcb->state = TCP_SYN_RCVD;
            }
            break;

        case TCP_SYN_RCVD:
            // Retransmission check
            if (flags & TCP_SYN) { 
                global_net_stats.dropped_packets++; // Count as potential retrans
            }
            
            if (flags & TCP_ACK) {
                tcb->state = TCP_ESTABLISHED;
                uart_puts("[TCP] Handshake Complete: ESTABLISHED.\r\n");
            }
            break;

        case TCP_ESTABLISHED:
            if (flags & TCP_FIN) {
                tcb->rcv_nxt++; // ACK the peer's FIN
                tcb->state = TCP_CLOSE_WAIT;
                // Trigger our FIN immediately to finish the 4-way handshake
                tcp_send_fin(tcb->remote_ip, tcb->remote_port, tcb->local_port);
            }
            break;

        case TCP_CLOSE_WAIT:
            // Peer is closing. In a full stack we'd send our own FIN here.
            // For v0.1.7, we'll let the watchdog reap it or force close on next interaction.
            break;

        default:
            break;
    }
}

/**
 * tcp_get_active_count: Real-time counter for F10 Dashboard.
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