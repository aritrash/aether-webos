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

extern struct virtio_pci_device *global_vnet_dev;
extern void print_ipv6(uint8_t addr[16]);
extern uint32_t aether_ip;

static portal_state_t current_state;
static char json_buffer[1024]; // Increased for more detailed telemetry
static int portal_active = 0; 

void uart_print_ip(uint32_t ip) {
    for (int i = 0; i < 4; i++) {
        uart_put_int((ip >> (i * 8)) & 0xFF);
        if (i < 3) uart_putc('.');
    }
}

/* Helper to convert byte to hex for UART display */
void uart_put_hex_byte(uint8_t byte) {
    const char *hex = "0123456789ABCDEF";
    uart_putc(hex[(byte >> 4) & 0xF]);
    uart_putc(hex[byte & 0xF]);
}

void portal_refresh_state() {
    current_state.uptime_ms = get_system_uptime_ms();
    current_state.heap_usage_kb = get_heap_usage() / 1024;
    current_state.device_count = get_total_pci_devices();

    // Pull real-time networking stats from Ankana's module
    current_state.packets_rx = global_net_stats.rx_packets;
    current_state.packets_tx = global_net_stats.tx_packets;

    if (global_vnet_dev && global_vnet_dev->device) {
        current_state.link_status = 1;
        for(int i = 0; i < 6; i++) {
            current_state.mac[i] = global_vnet_dev->device->mac[i];
        }
    } else {
        current_state.link_status = 0;
    }
}

/**
 * portal_get_json: Serializes the current kernel state into a JSON string.
 * Optimized for Roheet's React frontend.
 */
char* portal_get_json() {
    str_clear(json_buffer);
    str_append(json_buffer, "{");

    // 1. System Info
    str_append(json_buffer, "\"system\":{");
    str_append_kv_int(json_buffer, "\"uptime\"", current_state.uptime_ms);
    str_append(json_buffer, ",\"status\":\"online\"},");

    // 2. Memory Info (AetherMonitor)
    str_append(json_buffer, "\"memory\":{");
    str_append_kv_int(json_buffer, "\"used_kb\"", current_state.heap_usage_kb);
    str_append(json_buffer, ",");
    str_append_kv_int(json_buffer, "\"total_kb\"", HEAP_SIZE / 1024);
    str_append(json_buffer, "},");

    // 3. Network Info (Sniffer/Monitor)
    str_append(json_buffer, "\"network\":{");
    str_append_kv_int(json_buffer, "\"link\"", current_state.link_status);
    str_append(json_buffer, ",");
    str_append_kv_int(json_buffer, "\"rx\"", current_state.packets_rx);
    str_append(json_buffer, ",");
    str_append_kv_int(json_buffer, "\"tx\"", current_state.packets_tx);
    str_append(json_buffer, "},");

    // 4. Explorer Info
    str_append(json_buffer, "\"explorer\":{");
    str_append_kv_int(json_buffer, "\"devices\"", current_state.device_count);
    str_append(json_buffer, "}");

    str_append(json_buffer, "}");
    return json_buffer;
}

void portal_start() {
    portal_active = 1;
    uart_puts("\033[2J\033[H"); 
}

void portal_render_confirm_prompt() {
    if (!portal_active) return;
    
    // Draw Modal Box in center of terminal
    uart_puts("\033[8;2H\033[1;37;41m"); // Bold White on Red
    uart_puts("#########################################\r\n");
    uart_puts("\033[9;2H#                                       #\r\n");
    uart_puts("\033[10;2H#       CONFIRM SYSTEM SHUTDOWN?        #\r\n");
    uart_puts("\033[11;2H#    [ENTER] Confirm | [ESC] Cancel     #\r\n");
    uart_puts("\033[12;2H#                                       #\r\n");
    uart_puts("\033[13;2H#########################################\r\n\033[0m");
}

/**
 * portal_render_net_dashboard: Native Network Dashboard (v0.1.6)
 * Replaces the old Setup Wizard. Provides real-time L2/L3 telemetry.
 */
void portal_render_net_dashboard() {
    if (!portal_active) return;

    uart_puts("\033[H"); // Home cursor
    
    // Header Block
    uart_puts("\033[1;37;44m"); // White text on Blue background
    uart_puts("#################################################\r\n");
    uart_puts("#           AETHER NETWORK DASHBOARD            #\r\n");
    uart_puts("#################################################\r\n\033[0m");
    
    // 1. Physical & Link Layer
    uart_puts("\r\n [LINK LAYER]   \r\n");
    uart_puts("  - Interface: eth0 (VirtIO-Net-PCI)   \r\n");
    uart_puts("  - Status:    ");
    if (current_state.link_status) {
        uart_puts("\033[32mUP (10Gbps Full-Duplex)   \033[0m\r\n");
    } else {
        uart_puts("\033[31mDOWN / NO CARRIER\033[0m\r\n");
    }

    uart_puts("  - MAC:       ");
    for(int i = 0; i < 6; i++) {
        uart_put_hex_byte(current_state.mac[i]); 
        if(i < 5) uart_putc(':');
    }
    uart_puts("   \r\n");

    // 2. Network Layer (L3) - THE REAL DEALS
    uart_puts("\r\n [NETWORK LAYER]   \r\n");
    
    uart_puts("  - IPv4 Addr: ");
    uart_print_ip(aether_ip); 
    uart_puts(" (Static)   \r\n");

    uart_puts("  - IPv6 Addr: ");
    // Generate the Link-Local address from the MAC if you want to be fancy,
    // but for now, we'll let Adrija's printer handle a standard LL address array
    uint8_t local_ipv6[16] = {0xfe, 0x80, 0, 0, 0, 0, 0, 0, 
                              0x52, 0x54, 0x00, 0xff, 0xfe, 0x12, 0x34, 0x56};
    print_ipv6(local_ipv6); 
    uart_puts(" (Link-Local)   \r\n");

    // 3. Traffic Statistics (Ankana's Health Stats)
    uart_puts("\r\n [TRAFFIC STATS]   \r\n");
    uart_puts("  - RX Frames: "); 
    uart_put_int(global_net_stats.rx_packets);
    uart_puts("   ");
    uart_puts("\r\n  - TX Frames: "); 
    uart_put_int(global_net_stats.tx_packets);
    uart_puts("   ");
    uart_puts("\r\n  - Collisions/Drops: "); 
    uart_put_int(global_net_stats.dropped_packets);
    uart_puts("   ");
    uart_puts("\r\n  - Mem Buffers: ");
    uart_put_int(global_net_stats.buffer_usage); // Tracks how many kmalloc'd buffers are active
    uart_puts("   \r\n");

    // 4. Transport Layer (L4)
    uart_puts("\r\n [TRANSPORT LAYER]\r\n");
    uart_puts("  - TCP Listeners: Port 80 (HTTP)  \r\n");
    uart_puts("  - Active TCBs:   ");
    uart_put_int(tcp_get_active_count()); // REAL DATA HOOKED UP!
    uart_puts("   \r\n");
    
    uart_puts("\r\n\r\n-------------------------------------------------\r\n");
    uart_puts(" [ESC] Main Portal  |  [ENTER] Refresh           \r\n");
}

void portal_render_terminal() {
    if (!portal_active) return;

    uart_puts("\033[H"); // Reset cursor to 0,0
    
    uart_puts("===========================================\r\n");
    uart_puts("         AETHER WebOS :: Portal v0.1.6     \r\n");
    uart_puts("===========================================\r\n");
    
    uint64_t ms = current_state.uptime_ms;
    uint32_t sec = (uint32_t)(ms / 1000);
    
    uart_puts("[STATUS]  System Online   \r\n");
    
    uart_puts("[UPTIME]  ");
    uart_put_int(sec / 60); uart_puts("m ");
    uart_put_int(sec % 60); uart_puts("s ");
    uart_put_int(ms % 1000); uart_puts("ms  \r\n");

    uart_puts("[MEMORY]  Heap: ");
    uart_put_int(HEAP_SIZE / 1024 / 1024);
    uart_puts(" MB | Used: ");
    uart_put_int(current_state.heap_usage_kb);
    uart_puts(" KB  \r\n");

    uart_puts("[NETWORK] Link: ");
    if (current_state.link_status) {
        uart_puts("UP | MAC: ");
        for(int i = 0; i < 6; i++) {
            uart_put_hex_byte(current_state.mac[i]); 
            if(i < 5) uart_putc(':');
        }
    } else {
        uart_puts("DOWN | MAC: ??:??:??:??:??:??");
    }
    
    uart_puts("\r\n-------------------------------------------\r\n");
    uart_puts("Aether Ready. Waiting for WebClient...     \r\n");
}