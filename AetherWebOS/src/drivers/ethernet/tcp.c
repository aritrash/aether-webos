#include "drivers/ethernet/tcp.h"
#include "drivers/ethernet/ipv4.h"
#include "utils.h"
#include "kernel/timer.h"
#include "drivers/uart.h"

/* =====================================================
   TCP Logic Implementation
   Changes: 
   - Uses unified TCB and Enum from tcp.h (which includes tcp_state.h).
   - Suppresses 'reply' variable warning until send logic is linked.
   - Updates 'last_activity' for Health Watchdog.
   ===================================================== */

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
        // Only allocate if we are listening on this port (Port 80 assumed open)
        if (dest_port == 80) {
            tcb = tcp_allocate_tcb();
            if (!tcb) return;

            tcb->remote_ip = src_ip;
            tcb->remote_port = src_port;
            tcb->local_port = dest_port;
            tcb->rcv_nxt = incoming_seq + 1; // Expecting their next byte
            tcb->state = TCP_SYN_RCVD;
            
            /* Task: Timeout Watchdog
               Initialize the timestamp so health.c can track its age. */
            tcb->last_activity = get_system_uptime_ms(); 

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

            /* * IMPORTANT: The 'reply' variable is cast to void to prevent 
             * 'set but not used' warnings until the send implementation is ready.
             */
            (void)reply; 
            
            // TODO: Hand over to Pritam's checksum logic and IPv4 send.
            // tcp_send_packet(tcb, &reply, sizeof(reply), NULL, 0);
            
            uart_puts("[TCP] SYN-ACK prepared for remote client.\r\n");
        }
    } 
    // 4. Handle established connection or final ACK
    else if (tcb) {
        /* Refresh watchdog timestamp for every valid packet received */
        tcb->last_activity = get_system_uptime_ms();
        
        tcp_state_transition(tcb, tcp->flags);
    }
}