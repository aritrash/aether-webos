#include "kernel/health.h"
#include "uart.h"
#include "utils.h" 

// IMPORTANT: Do NOT put 'typedef struct' here. 
// It is already inside "kernel/health.h".

// The global struct definition (this allocates the actual memory)
net_stats_t global_net_stats = {0, 0, 0, 0};

// The JSON Exporter for the Portal UI
void net_get_telemetry_json(char* buffer) {
    // Format: {"rx": 102, "tx": 45, "err": 0, "buf": 5}
    sprintf(buffer, "{\"rx\": %lu, \"tx\": %lu, \"err\": %lu, \"buf\": %lu}", 
            global_net_stats.rx_packets, 
            global_net_stats.tx_packets, 
            global_net_stats.dropped_packets,
            global_net_stats.buffer_usage);
}