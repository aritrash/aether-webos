#include "kernel/health.h"
#include "drivers/uart.h"
#include "common/utils.h" 

net_stats_t global_net_stats = {0, 0, 0, 0};

void net_get_telemetry_json(char* buffer) {
    // Using our bare-metal safe formatter
    mini_sprintf_telemetry(buffer, 
                           global_net_stats.rx_packets, 
                           global_net_stats.tx_packets, 
                           global_net_stats.dropped_packets,
                           global_net_stats.buffer_usage);
}