#ifndef HEALTH_H
#define HEALTH_H

#include <stdint.h>

typedef struct {
    unsigned long rx_packets;
    unsigned long tx_packets;
    unsigned long dropped_packets;
    unsigned long buffer_usage;
    
    // Task: Connection Telemetry & Health Requirements
    unsigned long tcp_active;      // Track ESTABLISHED TCBs
    unsigned long retransmissions; // Count of retransmitted segments
    unsigned long checksum_errors; // Log corrupted packets from Pritam's logic
} net_stats_t;

// The global instance defined in health.c
extern net_stats_t global_net_stats; 

// Task: JSON Expansion - Provides visibility to Portal UI
void net_get_telemetry_json(char* buffer);

// Task: Timeout Watchdog - Identifies stalled SYN_RCVD connections
void health_check_syn_timeouts(void);

// Task: Error Reporting - Hook for Pritam's logic
void health_report_checksum_error(void);

void health_update_tcp_stats(void);

#endif