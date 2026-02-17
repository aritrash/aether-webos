#ifndef TCP_H
#define TCP_H

#include <stdint.h>

/* =====================================================
    TCP Flag Definitions
   ===================================================== */
#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10

/* =====================================================
    TCP Finite State Machine (FSM) States
    Synchronized with tcp_state.h to prevent enum mismatches.
   ===================================================== */
typedef enum {
    TCP_CLOSED,       // 0: No connection state
    TCP_LISTEN,       // 1: Waiting for SYN from client
    TCP_SYN_RCVD,     // 2: SYN received, SYN-ACK sent
    TCP_ESTABLISHED,  // 3: Connection open, data can flow
    TCP_FIN_WAIT_1,   // 4: We initiated close
    TCP_FIN_WAIT_2,   // 5: Client ACK'd our FIN
    TCP_CLOSE_WAIT,   // 6: Client initiated close
    TCP_LAST_ACK,     // 7: We sent our final FIN
    TCP_TIME_WAIT     // 8: Waiting to ensure remote received our ACK
} tcp_state_t;

/* =====================================================
    TCP Packet Header Structure (RFC 793)
   ===================================================== */
struct tcp_header {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq;
    uint32_t ack_seq;

    uint8_t  data_offset;  /* Upper 4 bits = header length */
    uint8_t  flags;

    uint16_t window;
    uint16_t checksum;
    uint16_t urg_ptr;
} __attribute__((packed));

/* =====================================================
    TCP Control Block (TCB)
    Aligned with tcp_state.h while keeping Health fields.
   ===================================================== */
struct tcp_control_block {
    // 1. Connection Identifier (The 4-tuple)
    uint32_t remote_ip;
    uint16_t remote_port;
    uint16_t local_port;

    // 2. State Information
    tcp_state_t state;

    // 3. Sequence Numbers
    uint32_t snd_una;    // Send unacknowledged
    uint32_t snd_nxt;    // Next sequence number to send
    uint32_t rcv_nxt;    // Next sequence number we expect

    // 4. Flow Control
    uint16_t snd_wnd;    // Send window
    uint16_t rcv_wnd;    // Receive window

    // 5. Application Link & Health Watchdog
    void (*data_callback)(uint8_t *payload, uint32_t len);
    uint64_t last_activity; // Task: Required for Timeout Watchdog
};

/* =====================================================
    Stack Configuration (Synchronized to 8)
   ===================================================== */
#define MAX_TCP_CONN 8
extern struct tcp_control_block tcb_table[MAX_TCP_CONN];

/* =====================================================
    Function Prototypes
   ===================================================== */
void tcp_handle(uint8_t *segment, uint32_t len, uint32_t src_ip, uint32_t dest_ip);
void tcp_init_stack(void);
struct tcp_control_block* tcp_allocate_tcb(void);
struct tcp_control_block* tcp_find_tcb(uint32_t remote_ip, uint16_t remote_port);
void tcp_state_transition(struct tcp_control_block* tcb, uint8_t flags);
int tcp_get_active_count();

#endif