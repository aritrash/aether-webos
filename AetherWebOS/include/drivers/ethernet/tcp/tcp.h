#ifndef AETHER_TCP_H
#define AETHER_TCP_H

#include <stdint.h>

typedef struct tcp_tcb tcp_tcb_t;

/**
 * Initialize TCP subsystem and global ISN.
 */
void tcp_init(void);

/**
 * Entry point for segments from the IPv4 layer.
 * Parameters must match the TCB 4-tuple lookup.
 */
void tcp_input_process(uint8_t *segment, uint16_t len, uint32_t src_ip, uint32_t dst_ip);

/**
 * Application Interface (used by socket.c)
 */
void tcp_send_data(tcp_tcb_t *tcb, const uint8_t *data, uint16_t len);
void tcp_close(tcp_tcb_t *tcb);
void tcp_abort(tcp_tcb_t *tcb);

#endif