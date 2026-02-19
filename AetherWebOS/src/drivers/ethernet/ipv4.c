#include "drivers/ethernet/ipv4.h"
#include "drivers/ethernet/ethernet.h"   // <-- FIX for ETH_TYPE_IPV4
#include "drivers/ethernet/tcp_chk.h"
#include "drivers/uart.h"
#include "kernel/memory.h"
#include "kernel/health.h"
#include "common/utils.h"
#include "drivers/virtio/virtio_net.h"

/* Use global identity from kernel */
extern uint32_t aether_ip;
extern struct virtio_pci_device *global_vnet_dev;


/* =====================================================
   Internet Checksum (RFC 791)
   ===================================================== */

uint16_t ipv4_checksum(void *data, size_t len)
{
    uint32_t sum = 0;
    uint16_t *ptr = (uint16_t *)data;

    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }

    if (len) {
        sum += (*(uint8_t *)ptr) << 8;
    }

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return (uint16_t)(~sum);
}


/* =====================================================
   IPv4 Packet Handler
   ===================================================== */

void ipv4_handle(uint8_t *data, uint32_t len)
{
    if (len < sizeof(struct ipv4_header))
        return;

    struct ipv4_header *ip = (struct ipv4_header *)data;

    uint8_t version = ip->version_ihl >> 4;
    uint8_t ihl     = ip->version_ihl & 0x0F;

    if (version != 4 || ihl < 5)
        return;

    uint32_t header_len = ihl * 4;

    if (len < header_len)
        return;

    /* Verify checksum */
    uint16_t received = ip->checksum;
    ip->checksum = 0;

    uint16_t calculated = ipv4_checksum(ip, header_len);
    ip->checksum = received;

    if (calculated != received) {
        health_report_checksum_error();
        return;
    }

    uint16_t total_len = ntohs(ip->total_len);

    if (total_len < header_len || total_len > len)
        return;

    /* Check destination IP (host order compare) */
    uint32_t dst_ip = __builtin_bswap32(ip->dest_ip);

    if (dst_ip != aether_ip)
        return;

    /* Dispatch by protocol */
    switch (ip->protocol) {

        case IP_PROTO_TCP: {

            uint8_t *tcp_segment = data + header_len;
            uint32_t tcp_len = total_len - header_len;

            if (tcp_len < sizeof(struct tcp_header))
                return;

            uint32_t src_ip = __builtin_bswap32(ip->src_ip);
            uint32_t dst_ip = __builtin_bswap32(ip->dest_ip);

            if (tcp_validate_checksum(ip,
                                    (struct tcp_header *)tcp_segment,
                                    tcp_segment + sizeof(struct tcp_header),
                                    tcp_len - sizeof(struct tcp_header))) {

                uart_puts("[TCP] Valid packet\r\n");

                tcp_handle(tcp_segment, tcp_len, src_ip, dst_ip);

            } else {
                uart_puts("[TCP] Corrupt packet dropped\r\n");
            }

            break;
        }


        case IP_PROTO_UDP:
            uart_puts("[IP] UDP Packet\r\n");
            break;

        case IP_PROTO_ICMP:
            uart_puts("[IP] ICMP Packet\r\n");
            break;

        default:
            break;
    }
}


/* =====================================================
   IPv4 Sender
   ===================================================== */

void ipv4_send(uint32_t dst_ip,
               uint8_t protocol,
               uint8_t *data,
               uint32_t len)
{
    if (!global_vnet_dev)
        return;

    uint32_t total_len = sizeof(struct ipv4_header) + len;

    struct ipv4_header *pkt =
        (struct ipv4_header *)kmalloc(total_len);
    if (!pkt)
        return;

    /* Fill header */

    pkt->version_ihl    = 0x45;
    pkt->tos            = 0;
    pkt->total_len      = htons(total_len);
    pkt->id             = htons(1);
    pkt->flags_fragment = 0;
    pkt->ttl            = 64;
    pkt->protocol       = protocol;
    pkt->checksum       = 0;

    /* Convert to network order */
    pkt->src_ip  = __builtin_bswap32(aether_ip);
    pkt->dest_ip = __builtin_bswap32(dst_ip);

    /* Compute checksum */
    pkt->checksum = ipv4_checksum(pkt, sizeof(struct ipv4_header));

    /* Copy payload */
    memcpy((uint8_t *)pkt + sizeof(struct ipv4_header),
           data,
           len);

    /* For now: send to gateway MAC (QEMU user mode) */
    uint8_t gateway_mac[6] =
        {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};

    ethernet_send(
        gateway_mac,
        ETH_TYPE_IPV4,
        (uint8_t *)pkt,
        total_len
    );

    kfree(pkt);
}
