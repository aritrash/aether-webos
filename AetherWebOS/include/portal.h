#ifndef PORTAL_H
#define PORTAL_H

#include <stdint.h>

typedef struct {
    uint8_t  mac[6];
    uint32_t ip_addr;
    uint8_t  link_status;
    uint32_t packets_rx;
    uint32_t packets_tx;
    uint64_t uptime_ms;
    uint32_t heap_usage_kb;
    uint32_t device_count;
    uint8_t  cpu_load;
} portal_state_t;

/* Lifecycle */
void portal_start(void);
void portal_refresh_state(void);
char* portal_get_json(void);

/* HTTP Router Entry */
void portal_handle_http(uint8_t *payload,
                        uint32_t len,
                        uint32_t src_ip,
                        uint16_t src_port);

/* Renderers */
void portal_render_terminal(void);
void portal_render_confirm_prompt(void);
void portal_render_net_dashboard(void);
void portal_render_loading(void);

#endif
