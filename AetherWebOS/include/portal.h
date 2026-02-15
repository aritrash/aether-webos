#ifndef PORTAL_H
#define PORTAL_H

#include <stdint.h>

typedef struct {
    // Connectivity
    char mac[18];
    uint32_t ip_addr;     /* Placeholder for DHCP */
    uint8_t  link_status; /* 1 = Up, 0 = Down */

    // Clock (Using Global Timer)
    uint64_t uptime_ms;
    
    // System Health
    uint32_t heap_usage_kb;
    uint32_t device_count;
} portal_state_t;

void portal_refresh_state();
char* portal_get_json();
void portal_render_terminal();
void portal_start(); 

#endif