#ifndef AETHER_TCP_INTERNAL_H
#define AETHER_TCP_INTERNAL_H

#include <stdint.h>
#include <stddef.h>
#include "common/utils.h" // For htons/htonl

#define TCP_PROTO_NUMBER 6
#define TCP_DEFAULT_WINDOW 8192  // Increased for Chrome buffer comfort
#define TCP_MAX_CONNS 4

/* Flags: Standard bitmasks */
#define TCP_FLAG_FIN 0x01
#define TCP_FLAG_SYN 0x02
#define TCP_FLAG_RST 0x04
#define TCP_FLAG_PSH 0x08
#define TCP_FLAG_ACK 0x10

typedef enum {
    TCP_STATE_CLOSED = 0,
    TCP_STATE_LISTEN,
    TCP_STATE_SYN_RECEIVED,
    TCP_STATE_ESTABLISHED,
    TCP_STATE_CLOSE_WAIT,
    TCP_STATE_LAST_ACK,
    TCP_STATE_TIME_WAIT     // Added for future-proofing Chrome reloads
} tcp_state_t;

/* TCP Header: Packed for DMA/Network alignment */
typedef struct __attribute__((packed)) {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq;
    uint32_t ack;
    uint8_t  offset_reserved;
    uint8_t  flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_ptr;
} tcp_hdr_t;

/* TCB: The per-connection state */
typedef struct tcp_tcb {
    uint32_t local_ip;      /* Host Order */
    uint32_t remote_ip;     /* Host Order */
    uint16_t local_port;    /* Host Order */
    uint16_t remote_port;   /* Host Order */

    tcp_state_t state;

    /* Sequence Numbers */
    uint32_t snd_una;       /* Unacknowledged */
    uint32_t snd_nxt;       /* Next to send */
    uint32_t rcv_nxt;       /* Next expected */
    
    uint16_t rcv_wnd;       /* Our window */
    uint16_t snd_wnd;       /* Peer window */

    struct tcp_tcb *next;
} tcp_tcb_t;

/* Global state symbols */
extern tcp_tcb_t *tcp_tcb_list;
extern uint32_t tcp_global_isn;

/* --- Internal Pipeline Protos --- */

tcp_tcb_t *tcp_allocate_tcb(void);
void tcp_remove_tcb(tcp_tcb_t *tcb);

/* Updated lookup to handle the full 4-tuple */
tcp_tcb_t *tcp_find_tcb(uint32_t src_ip, uint16_t src_port, 
                        uint32_t dst_ip, uint16_t dst_port);

void tcp_send_segment(tcp_tcb_t *tcb, uint8_t flags, const uint8_t *payload, uint16_t payload_len);
void tcp_send_synack(tcp_tcb_t *tcb);
void tcp_send_ack(tcp_tcb_t *tcb);
void tcp_send_fin(tcp_tcb_t *tcb);
void tcp_send_rst(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, uint32_t seq, uint32_t ack);

uint16_t tcp_compute_checksum(uint32_t src_ip, uint32_t dst_ip, const uint8_t *segment, uint16_t length);
int tcp_validate_checksum(uint32_t src_ip, uint32_t dst_ip, const uint8_t *segment, uint16_t length);

#endif