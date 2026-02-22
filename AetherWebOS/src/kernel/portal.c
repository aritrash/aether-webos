#include "portal.h"
#include "drivers/pcie.h"
#include "kernel/timer.h"
#include "drivers/uart.h"
#include "drivers/virtio/virtio_pci.h"
#include "kernel/memory.h"
#include "config.h"
#include "utils.h"
#include "kernel/health.h"
#include "drivers/ethernet/tcp_state.h"
#include "drivers/ethernet/tcp.h"
#include "drivers/ethernet/ipv4.h"

extern struct virtio_pci_device *global_vnet_dev;
extern void print_ipv6(uint8_t addr[16]);
extern uint32_t aether_ip;

static portal_state_t current_state;
static int portal_active = 0;
static int dashboard_initialized = 0;

/* ===================================================== */
/* STATE REFRESH                                         */
/* ===================================================== */

void portal_refresh_state()
{
    current_state.uptime_ms     = get_system_uptime_ms();
    current_state.heap_usage_kb = get_heap_usage() / 1024;
    current_state.device_count  = get_total_pci_devices();

    current_state.packets_rx = global_net_stats.rx_packets;
    current_state.packets_tx = global_net_stats.tx_packets;

    if (global_vnet_dev && global_vnet_dev->device)
    {
        current_state.link_status = 1;

        for (int i = 0; i < 6; i++)
            current_state.mac[i] = global_vnet_dev->device->mac[i];
    }
    else
    {
        current_state.link_status = 0;
    }
}

/* ===================================================== */
/* START                                                  */
/* ===================================================== */

void portal_start()
{
    portal_active = 1;
    dashboard_initialized = 0;
    uart_puts("\033[2J\033[H");
}

/* ===================================================== */
/* NETWORK DASHBOARD                                     */
/* ===================================================== */

void portal_render_net_dashboard()
{
    if (!portal_active)
        return;

    /* Move cursor home ONLY */
    uart_puts("\033[H");

    uart_puts("#################################################\n");
    uart_puts("#           AETHER NETWORK DASHBOARD            #\n");
    uart_puts("#################################################\n\n");

    uart_puts("[LINK LAYER]\n");
    uart_puts(" - Interface: eth0 (VirtIO-Net-PCI)\n");
    uart_puts(" - Status:    ");
    if (current_state.link_status)
        uart_puts("UP (10Gbps Full-Duplex)\n");
    else
        uart_puts("DOWN\n");

    uart_puts(" - MAC:       ");
    for (int i = 0; i < 6; i++)
    {
        uart_put_hex_byte(current_state.mac[i]);
        if (i < 5) uart_putc(':');
    }
    uart_puts("\n\n");

    uart_puts("[NETWORK LAYER]\n");
    uart_puts(" - IPv4 Addr: ");
    uart_print_ip(aether_ip);
    uart_puts("\n");

    uart_puts(" - IPv6 Addr: ");
    uint8_t local_ipv6[16] =
    {
        0xfe,0x80,0,0,0,0,0,0,
        0x52,0x54,0x00,0xff,
        0xfe,0x12,0x34,0x56
    };
    print_ipv6(local_ipv6);
    uart_puts("\n\n");

    uart_puts("[TRAFFIC STATS]\n");
    uart_puts(" - RX Frames:        ");
    uart_put_int(global_net_stats.rx_packets);
    uart_puts("\n");

    uart_puts(" - TX Frames:        ");
    uart_put_int(global_net_stats.tx_packets);
    uart_puts("\n");

    uart_puts(" - Collisions/Drops: ");
    uart_put_int(global_net_stats.dropped_packets);
    uart_puts("\n");

    uart_puts(" - Mem Buffers:      ");
    uart_put_int(global_net_stats.buffer_usage);
    uart_puts("\n\n");

    uart_puts("[TRANSPORT LAYER]\n");
    uart_puts(" - TCP Listeners: Port 80 (HTTP)\n");
    uart_puts(" - Active TCBs:      ");
    uart_put_int(tcp_get_active_count());
    uart_puts("\n");

    uart_puts("\n-------------------------------------------------\n");
    uart_puts(" [ESC] Main Portal  |  [ENTER] Refresh\n");
}

/* ===================================================== */
/* TERMINAL MODE                                          */
/* ===================================================== */

void portal_render_terminal()
{
    if (!portal_active)
        return;

    uart_puts("\033[2J\033[H");

    uart_puts("===========================================\n");
    uart_puts("         AETHER WebOS :: Portal            \n");
    uart_puts("===========================================\n");

    uart_puts("[STATUS]  System Online\n");

    uint64_t ms = current_state.uptime_ms;
    uint32_t sec = (uint32_t)(ms / 1000);

    uart_puts("[UPTIME]  ");
    uart_put_int(sec / 60); uart_puts("m ");
    uart_put_int(sec % 60); uart_puts("s ");
    uart_put_int(ms % 1000); uart_puts("ms\n");

    uart_puts("[MEMORY]  Used: ");
    uart_put_int(current_state.heap_usage_kb);
    uart_puts(" KB\n");

    uart_puts("[NETWORK] ");
    if (current_state.link_status)
        uart_puts("UP\n");
    else
        uart_puts("DOWN\n");

    uart_puts("-------------------------------------------\n");
    uart_puts("Aether Ready.\n");
}

/* ===================================================== */
/* CONFIRM PROMPT                                         */
/* ===================================================== */

void portal_render_confirm_prompt()
{
    uart_puts("\033[2J\033[H");
    uart_puts("#########################################\n");
    uart_puts("#       CONFIRM SYSTEM SHUTDOWN?        #\n");
    uart_puts("#    [ENTER] Confirm | [ESC] Cancel     #\n");
    uart_puts("#########################################\n");
}

/* ===================================================== */
/* STATIC HTTP PAGE                                      */
/* ===================================================== */

const char* aether_index_html = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Connection: close\r\n\r\n"
    "<html>"
    "<head><title>Aether WebOS</title></head>"
    "<body style='background:#000; color:#0f0; font-family:monospace;'>"
    "<h1>Aether OS v0.1.7</h1>"
    "<p>Kernel Status: ONLINE</p>"
    "<p>Team: Roheet, Pritam, Adrija, Ankana</p>"
    "</body>"
    "</html>";

void portal_serve_index(uint32_t src_ip, uint16_t src_port) {
    uint32_t len = 0;
    while (aether_index_html[len] != '\0') len++;
    tcp_send_data(src_ip, src_port, 80, (uint8_t*)aether_index_html, len);
}