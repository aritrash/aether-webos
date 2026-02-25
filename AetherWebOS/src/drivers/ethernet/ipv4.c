#include "drivers/ethernet/ipv4.h"
#include "drivers/ethernet/ethernet.h"
#include "drivers/ethernet/tcp/tcp.h"

#include "drivers/uart.h"
#include "kernel/memory.h"
#include "kernel/health.h"
#include "common/utils.h"
#include "drivers/virtio/virtio_net.h"

/* Global identity (HOST ORDER) */
extern uint32_t aether_ip;
extern struct virtio_pci_device *global_vnet_dev;

/* ============================================================
 *                  IPv4 HEADER CHECKSUM
 * ============================================================ */

uint16_t ipv4_checksum(void *data, size_t len)
{
    uint32_t sum = 0;
    uint8_t *ptr = (uint8_t *)data;

    while (len > 1) {
        uint16_t word = ((uint16_t)ptr[0] << 8) | ptr[1];
        sum += word;
        ptr += 2;
        len -= 2;
    }

    if (len == 1)
        sum += ((uint16_t)ptr[0] << 8);

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return (uint16_t)(~sum);
}

/* ============================================================
 *                  IPv4 RX HANDLER
 * ============================================================ */

void ipv4_handle(uint8_t *data, uint32_t len)
{
    if (len < sizeof(struct ipv4_header))
        return;

    struct ipv4_header *ip = (struct ipv4_header *)data;

    /* Version + IHL validation */
    uint8_t version = ip->version_ihl >> 4;
    uint8_t ihl     = ip->version_ihl & 0x0F;

    if (version != 4 || ihl < 5)
        return;

    uint32_t header_len = ihl * 4;

    if (len < header_len)
        return;

    /* Validate header checksum */
    uint16_t original_checksum = ip->checksum;
    ip->checksum = 0;

    uint16_t computed = ipv4_checksum(ip, header_len);

    ip->checksum = original_checksum;

    if (computed != original_checksum) {
        health_report_checksum_error();
        return;
    }

    uint16_t total_len = ntohs(ip->total_len);

    if (total_len < header_len || total_len > len)
        return;

    /* Convert IPs to HOST order */
    uint32_t src_ip = ntohl(ip->src_ip);
    uint32_t dst_ip = ntohl(ip->dest_ip);

    /* Drop packets not for us */
    if (dst_ip != aether_ip)
        return;

    uint8_t *payload = data + header_len;
    uint32_t payload_len = total_len - header_len;

    /* Protocol demux */
    switch (ip->protocol)
    {
        case IP_PROTO_TCP:
            tcp_input_process(payload,
                      payload_len,
                      src_ip,
                      dst_ip);
            break;

        case IP_PROTO_UDP:
            uart_debugps("[IP] UDP packet\r\n");
            break;

        case IP_PROTO_ICMP:
            uart_debugps("[IP] ICMP packet\r\n");
            break;

        default:
            break;
    }
}

/* ============================================================
 *                  IPv4 TX
 * ============================================================ */

void ipv4_send(uint32_t dst_ip,
               uint8_t protocol,
               const uint8_t *payload,
               uint32_t payload_len)
{
    if (!global_vnet_dev)
        return;

    uint32_t total_len =
        sizeof(struct ipv4_header) + payload_len;

    struct ipv4_header *pkt =
        (struct ipv4_header *)kmalloc(total_len);

    if (!pkt)
        return;

    memset(pkt, 0, total_len);

    pkt->version_ihl    = 0x45;  /* IPv4, header=20 bytes */
    pkt->tos            = 0;
    pkt->total_len      = htons(total_len);
    pkt->id             = htons(1);
    pkt->flags_fragment = htons(0);
    pkt->ttl            = 64;
    pkt->protocol       = protocol;
    pkt->checksum       = 0;

    pkt->src_ip  = htonl(aether_ip);
    pkt->dest_ip = htonl(dst_ip);

    pkt->checksum =
        ipv4_checksum(pkt, sizeof(struct ipv4_header));

    /* Copy payload */
    memcpy((uint8_t *)pkt + sizeof(struct ipv4_header),
           payload,
           payload_len);

    /* QEMU user-mode default gateway MAC */
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