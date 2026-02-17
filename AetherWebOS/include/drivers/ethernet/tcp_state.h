#ifndef TCP_STATE_H
#define TCP_STATE_H

#include <stdint.h>

/* * TCP States as per RFC 793 
 */
typedef enum {
    TCP_CLOSED,       // No connection state
    TCP_LISTEN,       // Waiting for SYN from client
    TCP_SYN_RCVD,     // SYN received, SYN-ACK sent
    TCP_ESTABLISHED,  // Connection open, data can flow
    TCP_FIN_WAIT_1,   // We initiated close
    TCP_FIN_WAIT_2,   // Client ACK'd our FIN
    TCP_CLOSE_WAIT,   // Client initiated close
    TCP_LAST_ACK,     // We sent our final FIN
    TCP_TIME_WAIT     // Waiting to ensure remote received our ACK
} tcp_state_t;

/**
 * struct tcp_control_block (TCB)
 * This is the "Identity Card" for every active connection.
 */
struct tcp_control_block {
    // 1. Connection Identifier (The 4-tuple)
    uint32_t remote_ip;
    uint16_t remote_port;
    uint16_t local_port;

    // 2. State Information
    tcp_state_t state;

    // 3. Sequence Numbers
    uint32_t snd_una;    // Send unacknowledged
    uint32_t snd_nxt;    // Next sequence number to send (ISN starts here)
    uint32_t rcv_nxt;    // Next sequence number we expect to receive

    // 4. Flow Control
    uint16_t snd_wnd;    // Send window
    uint16_t rcv_wnd;    // Receive window (How much buffer we have left)

    // 5. Application Link
    // Pointer to a function that processes data (e.g., HTTP GET parser)
    void (*data_callback)(uint8_t *payload, uint32_t len);
};

/* --- Global Management --- */

#define MAX_TCP_CONN 8

// Master table managed by Aritrash
extern struct tcp_control_block tcb_table[MAX_TCP_CONN];

/* --- Prototypes for the Team --- */

void tcp_init_stack(void);
struct tcp_control_block* tcp_allocate_tcb(void);
struct tcp_control_block* tcp_find_tcb(uint32_t remote_ip, uint16_t remote_port);
void tcp_state_transition(struct tcp_control_block* tcb, uint8_t flags);

#endif