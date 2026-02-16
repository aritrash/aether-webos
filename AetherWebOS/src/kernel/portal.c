#include "portal.h"
#include "drivers/pcie.h"
#include "kernel/timer.h" 
#include "drivers/uart.h"  
#include "drivers/virtio/virtio_pci.h"
#include "kernel/memory.h"
#include "config.h"        
#include "utils.h"
#include "kernel/health.h"

extern struct virtio_pci_device *global_vnet_dev;

static portal_state_t current_state;
static char json_buffer[1024]; // Increased for more detailed telemetry
static int portal_active = 0; 

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
 * portal_render_loading: Renders a BIOS-style progress bar.
 */
void portal_render_loading() {
    uart_puts("\r\n ACTIVATING BRIDGE: [");
    for (int i = 0; i < 20; i++) {
        uart_puts("#"); // Filling the bar
        // Delay to simulate work (Approx 50ms)
        for (volatile int d = 0; d < 1000000; d++); 
    }
    uart_puts("] 100%\r\n");
    uart_puts(" [OK] Bridge Hot. Signal sent to Host.\r\n");
    for (volatile int d = 0; d < 5000000; d++); // Pause to show success
}

/**
 * portal_render_wizard: The "AetherServer Setup Wizard" (F10)
 */
void portal_render_wizard() {
    uart_puts("\033[1;2H\033[1;37;44m"); // Bold White on Red
    uart_puts("#################################################\r\n");
    uart_puts("\033[2;2H#                                               #\r\n");
    uart_puts("\033[3;2H#       AETHER SERVER SETUP WIZARD v0.1.5       #\r\n");
    uart_puts("\033[4;2H#                                               #\r\n");
    uart_puts("\033[5;2H#################################################\r\n\033[0m");
    
    uart_puts(" This wizard will bridge your AArch64 Kernel to\r\n");
    uart_puts(" the Aether WebOS via WebSocket (Port 8080).\r\n\r\n");
    
    uart_puts(" [INFO] Host TCP Link: 127.0.0.1:1234\r\n");
    uart_puts(" [INFO] Protocol: AetherData/v1\r\n\r\n");
    
    uart_puts(" Press [ENTER] to START AetherBridge\r\n");
    uart_puts(" Press [ESC] to CANCEL\r\n\r\n");
    uart_puts("-------------------------------------------------\r\n");
}

void portal_render_terminal() {
    if (!portal_active) return;

    uart_puts("\033[H"); // Reset cursor to 0,0
    
    uart_puts("===========================================\r\n");
    uart_puts("         AETHER WebOS :: Portal v0.1.5     \r\n");
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