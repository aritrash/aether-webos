#include <stdint.h>
#include <stddef.h>

#include "drivers/ethernet/tcp/tcp.h"
#include "drivers/ethernet/tcp/tcp_internal.h"
#include "drivers/ethernet/socket.h"
#include "kernel/memory.h"
#include "drivers/uart.h"
#include "common/utils.h"

/* Externs from ipv4.c/tcp.c */
extern uint32_t aether_ip;

/**
 * Main entry point for TCP segments from ipv4.c
 */
void tcp_input_process(uint8_t *segment, uint16_t len, uint32_t src_ip, uint32_t dst_ip) 
{
    if (len < sizeof(tcp_hdr_t)) return;

    tcp_hdr_t *hdr = (tcp_hdr_t *)segment;
    
    /* 1. Extract Header Fields (Convert from Network to Host order) */
    uint16_t src_port = ntohs(hdr->src_port);
    uint16_t dst_port = ntohs(hdr->dst_port);
    uint32_t seg_seq  = ntohl(hdr->seq);
    uint32_t seg_ack  = ntohl(hdr->ack);
    uint8_t  flags    = hdr->flags;
    
    /* Calculate payload offset */
    uint8_t header_len = (hdr->offset_reserved >> 4) * 4;
    if (header_len < 20 || header_len > len) return;

    uint8_t *payload = segment + header_len;
    uint16_t payload_len = len - header_len;

    /* 2. Validate Checksum */
    if (!tcp_validate_checksum(src_ip, dst_ip, segment, len)) {
        uart_debugps("[TCP] Checksum fail. Dropped.\n");
        return;
    }

    /* 3. TCB Lookup (4-tuple) */
    tcp_tcb_t *tcb = tcp_find_tcb(src_ip, src_port, dst_ip, dst_port);

    /* 4. Handle Passive Open (LISTEN state logic) */
    if (!tcb) {
        if ((flags & TCP_FLAG_SYN) && dst_port == 80) {
            uart_debugps("[TCP] New connection request (SYN)\n");
            
            tcb = tcp_allocate_tcb();
            if (!tcb) return;

            tcb->remote_ip   = src_ip;
            tcb->remote_port = src_port;
            tcb->local_port  = dst_port;
            tcb->local_ip    = dst_ip;

            /* Initialize sequence numbers */
            tcb->rcv_nxt = seg_seq + 1;
            tcb->snd_una = tcp_global_isn;
            tcb->snd_nxt = tcp_global_isn;
            tcp_global_isn += 1000; // Increment for next connection

            tcb->state = TCP_STATE_SYN_RECEIVED;
            tcp_send_synack(tcb);
        } else {
            /* No TCB and not a SYN? Send RST to tell the host to go away */
            tcp_send_rst(dst_ip, src_ip, dst_port, src_port, seg_ack, 0);
        }
        return;
    }

    /* 5. State Machine Processing */
    switch (tcb->state) {
        
        case TCP_STATE_SYN_RECEIVED:
            if (flags & TCP_FLAG_ACK) {
                tcb->snd_una = seg_ack;
                tcb->state = TCP_STATE_ESTABLISHED;
                uart_debugps("[TCP] 3-way handshake complete.\n");
            }
            break;

        case TCP_STATE_ESTABLISHED:
            /* Data handling */
            if (payload_len > 0) {
                // Check if segment is in-window (very basic check)
                if (seg_seq == tcb->rcv_nxt) {
                    tcb->rcv_nxt += payload_len;
                    
                    /* Bridge to HTTP layer (socket.c) */
                    socket_dispatch(tcb, payload, payload_len);
                    
                    /* Note: socket_dispatch should ideally trigger the ACK, 
                       but we do it here for reliability if it doesn't. */
                    tcp_send_ack(tcb);
                }
            }

            /* Remote host wants to close */
            if (flags & TCP_FLAG_FIN) {
                tcb->rcv_nxt = seg_seq + 1;
                tcp_send_ack(tcb);
                tcb->state = TCP_STATE_CLOSE_WAIT;
                
                /* In our simple WebServer, we just close back immediately */
                tcp_close(tcb);
            }
            break;

        case TCP_STATE_LAST_ACK:
            if (flags & TCP_FLAG_ACK) {
                uart_debugps("[TCP] Connection closed gracefully.\n");
                tcb->state = TCP_STATE_CLOSED;
                tcp_remove_tcb(tcb);
            }
            break;

        default:
            /* For simplicity in bare-metal, reset unknown state violations */
            if (flags & TCP_FLAG_RST) {
                tcp_remove_tcb(tcb);
            }
            break;
    }
}