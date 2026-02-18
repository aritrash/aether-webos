#include "drivers/ethernet/tcp.h"
#include "drivers/ethernet/ipv4.h"
#include "drivers/ethernet/tcp_chk.h"
#include "drivers/ethernet/socket.h"
#include "utils.h"
#include "kernel/timer.h"
#include "drivers/uart.h"
#include "config.h"
#include "kernel/memory.h"

/* ===================================================== */

static void tcp_send_ack(struct tcp_control_block *tcb)
{
    struct tcp_header reply = {0};

    reply.src_port  = htons(tcb->local_port);
    reply.dest_port = htons(tcb->remote_port);
    reply.seq       = htonl(tcb->snd_nxt);
    reply.ack_seq   = htonl(tcb->rcv_nxt);
    reply.data_offset = (5 << 4);
    reply.flags     = TCP_ACK;
    reply.window    = htons(tcb->rcv_wnd);

    struct ipv4_header pseudo = {AETHER_IP_ADDR, tcb->remote_ip, 0,0,0,0,6,0};
    pseudo.total_len = htons(sizeof(struct tcp_header));

    reply.checksum = tcp_checksum(&pseudo, &reply, 0, 0);

    ipv4_send(tcb->remote_ip, 6, (uint8_t *)&reply, sizeof(reply));
}

/* ===================================================== */

void tcp_send_data(uint32_t dst_ip, uint16_t dst_port, uint16_t src_port, uint8_t *data, uint32_t len)
{
    struct tcp_control_block *tcb = tcp_find_tcb(dst_ip, dst_port);
    if (!tcb || tcb->state != TCP_ESTABLISHED)
        return;

    uint32_t packet_len = sizeof(struct tcp_header) + len;

    struct tcp_header *reply = kmalloc(packet_len);
    if (!reply) return;

    reply->src_port = htons(src_port);
    reply->dest_port = htons(dst_port);
    reply->seq = htonl(tcb->snd_nxt);
    reply->ack_seq = htonl(tcb->rcv_nxt);
    reply->data_offset = (5 << 4);
    reply->flags = TCP_ACK | TCP_PSH;
    reply->window = htons(tcb->rcv_wnd);
    reply->checksum = 0;
    reply->urg_ptr = 0;

    memcpy((uint8_t*)reply + sizeof(struct tcp_header), data, len);

    struct ipv4_header pseudo = {AETHER_IP_ADDR, dst_ip,0,0,0,0,6,0};
    pseudo.total_len = htons(packet_len);

    reply->checksum = tcp_checksum(&pseudo, reply, data, len);

    ipv4_send(dst_ip, 6, (uint8_t*)reply, packet_len);

    /* Advance AFTER transmit */
    tcb->snd_nxt += len;

    kfree(reply);
}

/* ===================================================== */

void tcp_handle(uint8_t *segment, uint32_t len, uint32_t src_ip, uint32_t dest_ip)
{
    if (len < sizeof(struct tcp_header)) return;

    struct tcp_header *tcp = (struct tcp_header *)segment;
    uint32_t header_len = (tcp->data_offset >> 4) * 4;
    uint8_t *payload = segment + header_len;
    uint32_t payload_len = len - header_len;

    struct ipv4_header pseudo_rx = {src_ip, dest_ip,0,0,0,0,6,0};

    if (!tcp_validate_checksum(&pseudo_rx, tcp, payload, payload_len))
        return;

    uint16_t src_port = ntohs(tcp->src_port);
    uint16_t dest_port = ntohs(tcp->dest_port);
    uint32_t incoming_seq = ntohl(tcp->seq);

    struct tcp_control_block *tcb = tcp_find_tcb(src_ip, src_port);

    /* SYN */
    if ((tcp->flags & TCP_SYN) && !tcb && dest_port == 80)
    {
        tcb = tcp_allocate_tcb();
        if (!tcb) return;

        tcb->remote_ip   = src_ip;
        tcb->remote_port = src_port;
        tcb->local_port  = dest_port;
        tcb->rcv_nxt     = incoming_seq + 1;
        tcb->state       = TCP_SYN_RCVD;

        tcp_send_ack(tcb);
        tcb->snd_nxt++;
        return;
    }

    if (!tcb) return;

    /* Promote state */
    if (tcb->state == TCP_SYN_RCVD && (tcp->flags & TCP_ACK))
        tcb->state = TCP_ESTABLISHED;

    /* Handle payload */
    if (payload_len > 0)
    {
        if (incoming_seq == tcb->rcv_nxt)
        {
            tcb->rcv_nxt += payload_len;

            tcp_send_ack(tcb);

            socket_handle_packet(dest_port,
                                 payload,
                                 payload_len,
                                 src_ip,
                                 src_port);
        }
    }

    /* FIN */
    if (tcp->flags & TCP_FIN)
    {
        tcb->rcv_nxt += 1;
        tcp_send_ack(tcb);
        tcb->state = TCP_CLOSE_WAIT;
    }
}

/* ===================================================== */

void tcp_send_fin(uint32_t dst_ip, uint16_t dst_port, uint16_t src_port)
{
    struct tcp_control_block *tcb = tcp_find_tcb(dst_ip, dst_port);
    if (!tcb) return;

    struct tcp_header reply = {0};

    reply.src_port = htons(src_port);
    reply.dest_port = htons(dst_port);
    reply.seq = htonl(tcb->snd_nxt);
    reply.ack_seq = htonl(tcb->rcv_nxt);
    reply.data_offset = (5 << 4);
    reply.flags = TCP_FIN | TCP_ACK;
    reply.window = htons(tcb->rcv_wnd);

    struct ipv4_header pseudo = {AETHER_IP_ADDR, dst_ip,0,0,0,0,6,0};
    pseudo.total_len = htons(sizeof(struct tcp_header));

    reply.checksum = tcp_checksum(&pseudo, &reply, 0, 0);

    ipv4_send(dst_ip, 6, (uint8_t *)&reply, sizeof(reply));

    tcb->snd_nxt += 1;
    tcb->state = TCP_LAST_ACK;
}
