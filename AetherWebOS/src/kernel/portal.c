#include "portal.h"
#include "drivers/pcie.h"
#include "kernel/timer.h" 
#include "drivers/uart.h"  
#include "drivers/virtio/virtio_pci.h"
#include "kernel/memory.h"
#include "config.h"        
#include "utils.h"

extern struct virtio_pci_device *global_vnet_dev;

static portal_state_t current_state;
static char json_buffer[512];
static int portal_active = 0; 

void uart_put_hex_byte(uint8_t byte) {
    const char *hex = "0123456789ABCDEF";
    uart_putc(hex[(byte >> 4) & 0xF]);
    uart_putc(hex[byte & 0xF]);
}

void portal_refresh_state() {
    current_state.uptime_ms = get_system_uptime_ms();
    current_state.heap_usage_kb = get_heap_usage() / 1024;
    current_state.device_count = get_total_pci_devices();

    if (global_vnet_dev && global_vnet_dev->device) {
        current_state.link_status = 1;
    }
}

char* portal_get_json() {
    str_clear(json_buffer);
    str_append(json_buffer, "{");
    str_append(json_buffer, "\"status\":\"online\",");
    str_append_kv_int(json_buffer, "uptime", current_state.uptime_ms);
    str_append_kv_int(json_buffer, "mem_kb", current_state.heap_usage_kb);
    str_append_kv_int(json_buffer, "devices", current_state.device_count);
    str_append(json_buffer, "\"link\":\"up\"");
    str_append(json_buffer, "}");
    return json_buffer;
}

void portal_start() {
    portal_active = 1;
    uart_puts("\033[2J\033[H"); 
}

/**
 * portal_render_confirm_prompt: Renders a red modal box for shutdown confirmation.
 */
void portal_render_confirm_prompt() {
    if (!portal_active) return;

    // Use absolute positioning to place the box in the center
    // Format: \033[Line;ColumnH
    uart_puts("\033[8;2H"); 
    uart_puts("\033[1;37;41m"); // Bright White on Red Background
    uart_puts("#########################################\r\n");
    uart_puts("\033[9;2H#                                       #\r\n");
    uart_puts("\033[10;2H#       CONFIRM SYSTEM SHUTDOWN?        #\r\n");
    uart_puts("\033[11;2H#    [ENTER] Confirm | [ESC] Cancel     #\r\n");
    uart_puts("\033[12;2H#                                       #\r\n");
    uart_puts("\033[13;2H#########################################\r\n");
    uart_puts("\033[0m"); // Reset colors
}

void portal_render_terminal() {
    if (!portal_active) return;

    uart_puts("\033[H"); 
    
    uart_puts("===========================================\r\n");
    uart_puts("         AETHER WebOS :: Portal v0.1.4     \r\n");
    uart_puts("===========================================\r\n");
    
    uint64_t ms = current_state.uptime_ms;
    uint32_t sec = (uint32_t)(ms / 1000);
    uint32_t min = sec / 60;
    
    uart_puts("[STATUS]  System Online   \r\n");
    
    uart_puts("[UPTIME]  ");
    uart_put_int(min);
    uart_puts("m ");
    uart_put_int(sec % 60);
    uart_puts("s ");
    uart_put_int(ms % 1000);
    uart_puts("ms  \r\n");

    uart_puts("[MEMORY]  Heap: ");
    uart_put_int(HEAP_SIZE / 1024 / 1024);
    uart_puts(" MB | Used: ");
    uart_put_int(current_state.heap_usage_kb);
    uart_puts(" KB  \r\n");

    uart_puts("[NETWORK] Link: ");
    if (global_vnet_dev) {
        uart_puts("UP | MAC: ");
        if (global_vnet_dev->device) {
            for(int i = 0; i < 6; i++) {
                uart_put_hex_byte(global_vnet_dev->device->mac[i]); 
                if(i < 5) uart_putc(':');
            }
            uart_puts("  ");
        } else {
            uart_puts("MAPPING...");
        }
    } else {
        uart_puts("DOWN | MAC: ??:??:??:??:??:??");
    }
    
    uart_puts("\r\n-------------------------------------------\r\n");
    uart_puts("Aether Ready. Waiting for WebClient...     \r\n");
}