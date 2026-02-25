#include "drivers/ethernet/tcp/tcp.h"
#include "drivers/ethernet/ipv4.h"
#include "drivers/ethernet/tcp/tcp_internal.h"
#include "drivers/ethernet/socket.h"
#include "drivers/uart.h"
#include "config.h"
#include "kernel/memory.h"
#include "kernel/timer.h"
#include "common/utils.h"

extern uint32_t aether_ip;

/* ===================================================== */
/* Helper: Build Pseudo Header                          */
/* ===================================================== */

static void build_pseudo(struct ipv4_header *pseudo,
                         uint32_t src_ip,
                         uint32_t dst_ip,
                         uint16_t tcp_len)
{
    memset(pseudo, 0, sizeof(struct ipv4_header));

    pseudo->src_ip   = htonl(src_ip);
    pseudo->dest_ip  = htonl(dst_ip);
    pseudo->protocol = 6;
    pseudo->total_len = htons(tcp_len);
}

/* ===================================================== */
/* Send SYN+ACK                                          */
/* ===================================================== */

static void tcp_send_synack(struct tcp_control_block *tcb)
{
    struct tcp_header reply;
    memset(&reply, 0, sizeof(reply));

    reply.src_port  = htons(tcb->local_port);
    reply.dest_port = htons(tcb->remote_port);
    reply.seq       = htonl(tcb->snd_nxt);
    reply.ack_seq   = htonl(tcb->rcv_nxt);
    reply.data_offset = (5 << 4);
    reply.flags     = TCP_SYN | TCP_ACK;
    reply.window    = htons(tcb->rcv_wnd);

    struct ipv4_header pseudo;
    build_pseudo(&pseudo, aether_ip,
                 tcb->remote_ip,
                 sizeof(struct tcp_header));

    reply.checksum = tcp_checksum(&pseudo, &reply, 0, 0);

    ipv4_send(tcb->remote_ip, 6,
              (uint8_t *)&reply,
              sizeof(reply));

    tcb->snd_nxt += 1;
    tcb->last_activity = get_system_uptime_ms();
}

/* ===================================================== */
/* Send ACK                                              */
/* ===================================================== */

static void tcp_send_ack(struct tcp_control_block *tcb)
{
    struct tcp_header reply;
    memset(&reply, 0, sizeof(reply));

    reply.src_port  = htons(tcb->local_port);
    reply.dest_port = htons(tcb->remote_port);
    reply.seq       = htonl(tcb->snd_nxt);
    reply.ack_seq   = htonl(tcb->rcv_nxt);
    reply.data_offset = (5 << 4);
    reply.flags     = TCP_ACK;
    reply.window    = htons(tcb->rcv_wnd);

    struct ipv4_header pseudo;
    build_pseudo(&pseudo, aether_ip,
                 tcb->remote_ip,
                 sizeof(struct tcp_header));

    reply.checksum = tcp_checksum(&pseudo, &reply, 0, 0);

    ipv4_send(tcb->remote_ip, 6,
              (uint8_t *)&reply,
              sizeof(reply));

    tcb->last_activity = get_system_uptime_ms();
}

/* ===================================================== */
/* Send TCP Data                                         */
/* ===================================================== */

void tcp_send_data(uint32_t dst_ip,
                   uint16_t dst_port,
                   uint16_t src_port,
                   uint8_t *data,
                   uint32_t len)
{
    struct tcp_control_block *tcb =
        tcp_find_tcb(dst_ip, dst_port);

    if (!tcb || tcb->state != TCP_ESTABLISHED)
        return;

    uint32_t packet_len = sizeof(struct tcp_header) + len;

    struct tcp_header *reply = kmalloc(packet_len);
    if (!reply)
        return;

    memset(reply, 0, packet_len);

    reply->src_port  = htons(src_port);
    reply->dest_port = htons(dst_port);
    reply->seq       = htonl(tcb->snd_nxt);
    reply->ack_seq   = htonl(tcb->rcv_nxt);
    reply->data_offset = (5 << 4);
    reply->flags     = TCP_ACK | TCP_PSH;
    reply->window    = htons(tcb->rcv_wnd);

    memcpy((uint8_t *)reply + sizeof(struct tcp_header),
           data,
           len);

    struct ipv4_header pseudo;
    build_pseudo(&pseudo, aether_ip,
                 dst_ip,
                 packet_len);

    reply->checksum =
        tcp_checksum(&pseudo,
                     reply,
                     (uint8_t *)reply + sizeof(struct tcp_header),
                     len);

    ipv4_send(dst_ip, 6,
              (uint8_t *)reply,
              packet_len);

    tcb->snd_nxt += len;
    tcb->last_activity = get_system_uptime_ms();

    kfree(reply);
}

/* ===================================================== */
/* TCP Handler                                           */
/* ===================================================== */

void tcp_handle(uint8_t *segment,
                uint32_t len,
                uint32_t src_ip,
                uint32_t dest_ip)
{
    if (len < sizeof(struct tcp_header))
        return;

    struct tcp_header *tcp = (struct tcp_header *)segment;

    uint32_t header_len = (tcp->data_offset >> 4) * 4;
    if (len < header_len)
        return;

    uint8_t *payload = segment + header_len;
    uint32_t payload_len = len - header_len;

    struct ipv4_header pseudo;
    build_pseudo(&pseudo, src_ip, dest_ip, len);

    if (!tcp_validate_checksum(&pseudo, tcp,
                               payload, payload_len))
        return;

    uint16_t src_port  = ntohs(tcp->src_port);
    uint16_t dest_port = ntohs(tcp->dest_port);
    uint32_t incoming_seq = ntohl(tcp->seq);
    uint32_t incoming_ack = ntohl(tcp->ack_seq);

    struct tcp_control_block *tcb =
        tcp_find_tcb(src_ip, src_port);

    /* ========================= SYN ========================= */

    if ((tcp->flags & TCP_SYN) &&
        !(tcp->flags & TCP_ACK) &&
        !tcb &&
        dest_port == 80)
    {
        tcb = tcp_allocate_tcb();
        if (!tcb)
            return;

        tcb->remote_ip   = src_ip;
        tcb->remote_port = src_port;
        tcb->local_port  = dest_port;

        tcb->snd_nxt = 0;
        tcb->rcv_nxt = incoming_seq + 1;
        tcb->rcv_wnd = 8192;
        tcb->state   = TCP_SYN_RCVD;
        tcb->last_activity = get_system_uptime_ms();

        tcp_send_synack(tcb);
        return;
    }

    if (!tcb)
        return;

    tcb->last_activity = get_system_uptime_ms();

    /* ========================= HANDSHAKE COMPLETE ========================= */

    if (tcb->state == TCP_SYN_RCVD &&
        (tcp->flags & TCP_ACK) &&
        !(tcp->flags & TCP_SYN) &&
        incoming_ack == tcb->snd_nxt)
    {
        tcb->state = TCP_ESTABLISHED;
        return;  // pure ACK handled
    }

    /* ========================= PAYLOAD ========================= */

    if (tcb->state == TCP_ESTABLISHED &&
        payload_len > 0)
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
        return;
    }

    /* ========================= FIN ========================= */

    if ((tcp->flags & TCP_FIN) &&
        tcb->state == TCP_ESTABLISHED)
    {
        tcb->rcv_nxt += 1;
        tcp_send_ack(tcb);
        tcb->state = TCP_CLOSE_WAIT;
        return;
    }

    /* ========================= RST ========================= */

    if (tcp->flags & TCP_RST)
    {
        tcb->state = TCP_CLOSED;
        return;
    }
}

/* ===================================================== */
/* Send FIN                                              */
/* ===================================================== */

void tcp_send_fin(uint32_t dst_ip,
                  uint16_t dst_port,
                  uint16_t src_port)
{
    struct tcp_control_block *tcb =
        tcp_find_tcb(dst_ip, dst_port);

    if (!tcb || tcb->state != TCP_ESTABLISHED)
        return;

    struct tcp_header reply;
    memset(&reply, 0, sizeof(reply));

    reply.src_port  = htons(src_port);
    reply.dest_port = htons(dst_port);
    reply.seq       = htonl(tcb->snd_nxt);
    reply.ack_seq   = htonl(tcb->rcv_nxt);
    reply.data_offset = (5 << 4);
    reply.flags     = TCP_FIN | TCP_ACK;
    reply.window    = htons(tcb->rcv_wnd);

    struct ipv4_header pseudo;
    build_pseudo(&pseudo, aether_ip,
                 dst_ip,
                 sizeof(struct tcp_header));

    reply.checksum =
        tcp_checksum(&pseudo,
                     &reply,
                     0,
                     0);

    ipv4_send(dst_ip, 6,
              (uint8_t *)&reply,
              sizeof(reply));

    tcb->snd_nxt += 1;
    tcb->state = TCP_LAST_ACK;
    tcb->last_activity = get_system_uptime_ms();
}