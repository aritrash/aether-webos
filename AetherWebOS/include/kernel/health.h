#ifndef HEALTH_H
#define HEALTH_H

typedef struct {
    unsigned long rx_packets;
    unsigned long tx_packets;
    unsigned long dropped_packets;
    unsigned long buffer_usage;
} net_stats_t;

// This is a DECLARATION (it doesn't take up space here)
extern net_stats_t global_net_stats; 

void net_get_telemetry_json(char* buffer);

#endif