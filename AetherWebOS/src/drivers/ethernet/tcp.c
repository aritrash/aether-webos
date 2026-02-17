#include "drivers/ethernet/tcp.h"
#include "drivers/ethernet/ethernet.h"
#include "drivers/ethernet/ipv4.h"
#include "utils.h"


static uint32_t local_seq = 1000; // simple static initial sequence

void tcp_handle(uint8_t *segment,
                uint32_t len,
                uint32_t src_ip,
                uint32_t dest_ip)
{
    if (len < sizeof(struct tcp_header))
        return;

    struct tcp_header *tcp = (struct tcp_header *)segment;

    uint8_t flags = tcp->flags;

    /* Convert multi-byte fields */
    uint32_t incoming_seq = ntohl(tcp->seq);
    uint16_t src_port = ntohs(tcp->src_port);
    uint16_t dest_port = ntohs(tcp->dest_port);

    /* Detect SYN */
    if (flags & TCP_SYN)
    {
        struct tcp_header reply;

        reply.src_port  = htons(dest_port);
        reply.dest_port = htons(src_port);

        reply.seq = htonl(local_seq);
        reply.ack_seq = htonl(incoming_seq + 1);

        reply.data_offset = (5 << 4); // header length = 5 * 4 = 20 bytes
        reply.flags = TCP_SYN | TCP_ACK;

        reply.window = htons(1024);
        reply.checksum = 0;  // TODO: implement checksum properly
        reply.urg_ptr = 0;

        /* Send via IP layer */
        ipv4_handle(src_ip,
                6,  // protocol TCP
                (uint8_t *)&reply,
                sizeof(reply));

        local_seq++;
    }
}
