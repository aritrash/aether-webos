#ifndef PORTAL_H
#define PORTAL_H

#include <stdint.h>

/**
 * portal_state_t: The master state object for the Aether WebOS ecosystem.
 * This struct is mirrored by Roheet's React state management.
 */
typedef struct {
    // Connectivity & Networking
    uint8_t  mac[6];
    uint32_t ip_addr;     /* Placeholder for DHCP/Static IP */
    uint8_t  link_status; /* 1 = Up, 0 = Down */
    uint32_t packets_rx;  /* Real-time counter for Sniffer App */
    uint32_t packets_tx;

    // Clock & Timing
    uint64_t uptime_ms;
    
    // System Health & Topology
    uint32_t heap_usage_kb;
    uint32_t device_count;
    uint8_t  cpu_load;    /* Calculated via Timer IRQ duty cycle */
} portal_state_t;

/* --- Lifecycle & Rendering --- */
void portal_start(void);
void portal_refresh_state(void);
char* portal_get_json(void);

/* --- Terminal UI (ANSI) --- */
void portal_render_terminal(void);
void portal_render_confirm_prompt(void); 

/* --- AetherBridge UI --- */
void portal_render_net_dashboard();     
void portal_handle_input(char c);
void portal_render_loading();

#endif /* PORTAL_H */