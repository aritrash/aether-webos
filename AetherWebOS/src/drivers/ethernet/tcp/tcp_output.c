#include <stdint.h>
#include <stddef.h>
#include "ethernet/ipv4.h"
#include "drivers/ethernet/tcp/tcp.h"
#include "drivers/ethernet/tcp/tcp_internal.h"
#include "drivers/uart.h"
#include "kernel/memory.h"
#include "common/utils.h"

/* External IPv4 delivery */
extern void ipv4_send(uint32_t dst_ip, uint8_t protocol, const uint8_t *data, uint32_t length);

/* ============================================================
 * CORE SEGMENT BUILDER
 * ============================================================ */

void tcp_send_segment(tcp_tcb_t *tcb, uint8_t flags, const uint8_t *payload, uint16_t payload_len)
{
    if (!tcb) return;

    uint16_t total_len = sizeof(tcp_hdr_t) + payload_len;

    /* 1. Allocate buffer for the full segment (Header + Data) */
    uint8_t *buffer = (uint8_t *)kmalloc(total_len);
    if (!buffer) {
        uart_debugps("[TCP] TX FATAL: kmalloc failed\n");
        return;
    }

    tcp_hdr_t *hdr = (tcp_hdr_t *)buffer;

    /* 2. Construct Header (All fields must be Network Byte Order) */
    hdr->src_port = htons(tcb->local_port);
    hdr->dst_port = htons(tcb->remote_port);
    
    hdr->seq = htonl(tcb->snd_nxt);
    hdr->ack = htonl(tcb->rcv_nxt);

    /* Data Offset: 5 (32-bit words) = 20 bytes. Reserved: 0. */
    hdr->offset_reserved = (5 << 4);
    hdr->flags = flags;

    /* Advertised Window: Tell Chrome how much we can buffer */
    hdr->window = htons(tcb->rcv_wnd);
    
    hdr->checksum = 0;
    hdr->urgent_ptr = 0;

    /* 3. Attach Payload */
    if (payload_len > 0 && payload != NULL) {
        memcpy(buffer + sizeof(tcp_hdr_t), payload, payload_len);
    }

    /* 4. Compute Checksum (Requires Pseudo-Header) */
    hdr->checksum = tcp_compute_checksum(tcb->local_ip, tcb->remote_ip, buffer, total_len);

    /* 5. Handover to IPv4 Layer */
    ipv4_send(tcb->remote_ip, IP_PROTO_TCP, buffer, total_len);

    /* 6. Update Sequence Space 
       SYN and FIN occupy 1 byte of sequence space each. */
    if (flags & (TCP_FLAG_SYN | TCP_FLAG_FIN)) {
        tcb->snd_nxt += 1;
    }
    tcb->snd_nxt += payload_len;

    /* Note: buffer is freed by net_tx_reaper in virtio_net.c 
       Do NOT kfree(buffer) here. */
}

/* ============================================================
 * CONTEXT-SPECIFIC HELPERS
 * ============================================================ */

void tcp_send_ack(tcp_tcb_t *tcb) {
    tcp_send_segment(tcb, TCP_FLAG_ACK, NULL, 0);
}

void tcp_send_synack(tcp_tcb_t *tcb) {
    tcp_send_segment(tcb, TCP_FLAG_SYN | TCP_FLAG_ACK, NULL, 0);
}

void tcp_send_fin(tcp_tcb_t *tcb) {
    tcp_send_segment(tcb, TCP_FLAG_FIN | TCP_FLAG_ACK, NULL, 0);
}

/**
 * Standard Reset (RST) for rejected connections.
 * Note: This doesn't require a TCB because it can be sent in response to invalid segments.
 */
void tcp_send_rst(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, uint32_t seq, uint32_t ack) {
    tcp_hdr_t hdr;
    memset(&hdr, 0, sizeof(tcp_hdr_t));

    hdr.src_port = htons(src_port);
    hdr.dst_port = htons(dst_port);
    hdr.seq = htonl(seq);
    hdr.ack = htonl(ack);
    hdr.offset_reserved = (5 << 4);
    hdr.flags = TCP_FLAG_RST | TCP_FLAG_ACK;
    hdr.window = 0;

    hdr.checksum = tcp_compute_checksum(src_ip, dst_ip, (uint8_t*)&hdr, sizeof(tcp_hdr_t));

    ipv4_send(dst_ip, IP_PROTO_TCP, (uint8_t*)&hdr, sizeof(tcp_hdr_t));
}