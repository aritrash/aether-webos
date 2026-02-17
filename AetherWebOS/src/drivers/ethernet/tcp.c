#include "drivers/ethernet/tcp.h"
#include "drivers/ethernet/tcp_state.h" // Aritrash's state machine
#include "drivers/ethernet/ipv4.h"
#include "utils.h"
#include "kernel/timer.h"
#include "drivers/uart.h"

void tcp_handle(uint8_t *segment, uint32_t len, uint32_t src_ip, uint32_t dest_ip) {
    if (len < sizeof(struct tcp_header)) return;

    struct tcp_header *tcp = (struct tcp_header *)segment;
    uint16_t src_port = ntohs(tcp->src_port);
    uint16_t dest_port = ntohs(tcp->dest_port);
    uint32_t incoming_seq = ntohl(tcp->seq);

    // 1. Look for an existing connection
    struct tcp_control_block *tcb = tcp_find_tcb(src_ip, src_port);

    // 2. Handle NEW connection (SYN)
    if ((tcp->flags & TCP_SYN) && !tcb) {
        // Only allocate if we are listening on this port (Adrija's check)
        // For now, we'll assume Port 80 is always open
        if (dest_port == 80) {
            tcb = tcp_allocate_tcb();
            if (!tcb) return;

            tcb->remote_ip = src_ip;
            tcb->remote_port = src_port;
            tcb->local_port = dest_port;
            tcb->rcv_nxt = incoming_seq + 1; // Expecting their next byte
            tcb->state = TCP_SYN_RCVD;
            tcb->last_activity = get_system_uptime_ms();  // <-- ADDED LINE

            // 3. Prepare SYN-ACK Response
            struct tcp_header reply;
            reply.src_port = htons(dest_port);
            reply.dest_port = htons(src_port);
            reply.seq = htonl(tcb->snd_nxt);
            reply.ack_seq = htonl(tcb->rcv_nxt);
            reply.data_offset = (5 << 4);
            reply.flags = TCP_SYN | TCP_ACK;
            reply.window = htons(tcb->rcv_wnd);
            reply.checksum = 0; 
            reply.urg_ptr = 0;

            // Increment our sequence number for the SYN we just sent
            tcb->snd_nxt++;

            /* * IMPORTANT: Pass to Pritam's upcoming checksum logic 
             * and then to IPv4 send.
             */
            // tcp_send_packet(tcb, &reply, sizeof(reply), NULL, 0);
            
            uart_puts("[TCP] SYN-ACK sent to ");
            // uart_put_ip(src_ip); // If you have this helper
            uart_puts("\r\n");
        }
    } 
    // 4. Handle established connection or final ACK
    else if (tcb) {
        tcp_state_transition(tcb, tcp->flags);
    }
}
