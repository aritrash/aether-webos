#include "kernel/health.h"
#include "drivers/ethernet/tcp.h"
#include "drivers/ethernet/tcp_state.h" // Fixed: Added for tcb_table and MAX_TCP_CONN
#include "kernel/timer.h"
#include "common/utils.h"
#include "drivers/uart.h"

/* =====================================================
   Global System Health Stats
   ===================================================== */
net_stats_t global_net_stats = {0};

/**
 * Task: Error Reporting
 * Hook for Pritam's checksum logic.
 */
void health_report_checksum_error(void) {
    global_net_stats.checksum_errors++;
    global_net_stats.dropped_packets++; 
}

/**
 * Task: JSON Expansion
 * Updated to match the 7-parameter signature needed for the Portal UI.
 * Make sure to update the declaration in utils.h as well!
 */
void net_get_telemetry_json(char* buffer) {
    mini_sprintf_telemetry(buffer, 
                           global_net_stats.rx_packets, 
                           global_net_stats.tx_packets, 
                           global_net_stats.dropped_packets,
                           global_net_stats.buffer_usage,
                           global_net_stats.tcp_active,
                           global_net_stats.retransmissions,
                           global_net_stats.checksum_errors);
}

/**
 * Task: TCB Monitoring & Timeout Watchdog
 */
void health_check_syn_timeouts(void) {
    uint32_t established_count = 0;
    uint64_t now = get_system_uptime_ms();
    const uint64_t SYN_TIMEOUT_MS = 10000; 
    
    for (int i = 0; i < MAX_TCP_CONN; i++) {
        // Alignment Fix: Using the correct state from tcp_state.h
        if (tcb_table[i].state == TCP_CLOSED) {
            continue;
        }

        if (tcb_table[i].state == TCP_ESTABLISHED) {
            established_count++;
        }

        /* Task: Timeout Watchdog for SYN_RCVD */
        if (tcb_table[i].state == TCP_SYN_RCVD) {
            if ((now - tcb_table[i].last_activity) > SYN_TIMEOUT_MS) {
                
                // Logic Fix: Explicit cleanup of the TCB slot
                tcb_table[i].state = TCP_CLOSED; 
                tcb_table[i].remote_ip = 0;
                tcb_table[i].remote_port = 0;
                tcb_table[i].local_port = 0;

                global_net_stats.dropped_packets++;
                
                uart_puts("[HEALTH] Watchdog: Reaped stale SYN_RCVD connection.\r\n");
            }
        }
    }
    
    global_net_stats.tcp_active = established_count;
}