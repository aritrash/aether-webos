#include "kernel/health.h"
#include "drivers/ethernet/tcp/tcp_internal.h"
#include "common/utils.h"
#include "drivers/uart.h"

/* =====================================================
   Global System Health Stats
   ===================================================== */

net_stats_t global_net_stats = {0};

/* =====================================================
   Checksum Error Reporting
   ===================================================== */

void health_report_checksum_error(void)
{
    global_net_stats.checksum_errors++;
    global_net_stats.dropped_packets++;
}

/* =====================================================
   Telemetry JSON Export
   ===================================================== */

void net_get_telemetry_json(char* buffer)
{
    mini_sprintf_telemetry(buffer,
                           global_net_stats.rx_packets,
                           global_net_stats.tx_packets,
                           global_net_stats.dropped_packets,
                           global_net_stats.buffer_usage,
                           global_net_stats.tcp_active,
                           global_net_stats.retransmissions,
                           global_net_stats.checksum_errors);
}

/* =====================================================
   TCP Connection Monitoring
   ===================================================== */

void health_update_tcp_stats(void)
{
    uint32_t established_count = 0;

    tcp_tcb_t *cur = tcp_tcb_list;

    while (cur) {

        if (cur->state == TCP_STATE_ESTABLISHED)
            established_count++;

        cur = cur->next;
    }

    global_net_stats.tcp_active = established_count;
}