#include "drivers/ethernet/ipv4.h"
#include "drivers/uart.h"
#include "common/utils.h"        // ntohs(), htons(), ntohl()
#include "kernel/health.h"       // <-- For health_report_checksum_error()
#include "drivers/ethernet/tcp_chk.h"
#include "kernel/memory.h"       // For kmalloc() in tcp_send_data
#include "drivers/virtio/virtio_net.h"  // For global_vnet_dev reference

/* Our OS IP: 10.0.0.2 */
#define AETHER_IP  0x0A000002


/* ===============================
   Internet Checksum (RFC 791)
   =============================== */

uint16_t ipv4_checksum(void *data, size_t len)
{
    uint32_t sum = 0;
    uint16_t *ptr = (uint16_t *)data;

    /* Sum all 16-bit words */
    while (len > 1) {

        uint16_t val = *ptr;
        ptr++;

        sum += ntohs(val);
        len -= 2;
    }

    /* Handle odd byte */
    if (len) {
        sum += (*(uint8_t *)ptr) << 8;
    }

    /* Fold to 16 bits */
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)(~sum);
}


/* ===============================
   IPv4 Packet Handler
   =============================== */

void ipv4_handle(uint8_t *data, uint32_t len)
{
    if (len < sizeof(struct ipv4_header)) {
        return;
    }

    struct ipv4_header *ip = (struct ipv4_header *)data;


    /* -------------------------------
       Version + Header Length
       ------------------------------- */

    uint8_t version = ip->version_ihl >> 4;
    uint8_t ihl     = ip->version_ihl & 0x0F;

    if (version != 4 || ihl < 5) {
        uart_puts("[IP] Invalid header\r\n");
        return;
    }

    uint32_t header_len = ihl * 4;

    if (len < header_len) {
        return;
    }


    /* -------------------------------
       Checksum Verification
       ------------------------------- */

    uint16_t old_sum = ip->checksum;
    ip->checksum = 0;

    uint16_t calc = ipv4_checksum(ip, header_len);

    ip->checksum = old_sum;

    if (calc != old_sum) {
        uart_puts("[IP] Bad checksum. Dropped\r\n");

        /* Health monitor hook */
        health_report_checksum_error();

        return;
    }


    /* -------------------------------
       Length Validation
       ------------------------------- */

    uint16_t total_len = ntohs(ip->total_len);

    if (total_len < header_len || total_len > len) {
        return;
    }


    /* -------------------------------
       Destination IP Check
       ------------------------------- */

    uint32_t dst = ntohl(ip->dest_ip);

    if (dst != AETHER_IP) {
        /* Packet not for us */
        return;
    }


    /* ===============================
       Protocol Dispatch
       =============================== */

    switch (ip->protocol) {

         case IP_PROTO_TCP: {        
             struct tcp_header *tcp =
                 (struct tcp_header *)(data + header_len);
         
             uint32_t tcp_len =
                 total_len - header_len;
         
             uint8_t *payload =
                 (uint8_t *)tcp + sizeof(struct tcp_header);
         
             uint32_t payload_len =
                 tcp_len - sizeof(struct tcp_header);
         
         
             /* Validate TCP checksum */
             if (tcp_validate_checksum(ip,
                                       tcp,
                                       payload,
                                       payload_len)) {
         
                 uart_puts("[TCP] Valid packet\r\n");
         
             } else {
         
                 uart_puts("[TCP] Corrupt packet dropped\r\n");
             }
         
             break;
         }


        case IP_PROTO_UDP:
            uart_puts("[IP] UDP Packet detected\r\n");
            break;

        case IP_PROTO_ICMP:
            uart_puts("[IP] ICMP Packet detected\r\n");
            break;

        default:
            uart_puts("[IP] Unknown protocol\r\n");
            break;
    }
}

// Reference the global VirtIO-Net device from kernel.c/pcie.c
extern struct virtio_pci_device *global_vnet_dev;

void ipv4_send(uint32_t dst_ip, uint8_t protocol, uint8_t *data, uint32_t len) {
    if (!global_vnet_dev) return;

    uint32_t total_len = sizeof(struct ipv4_header) + len;
    struct ipv4_header *pkt = (struct ipv4_header *)kmalloc(total_len);
    if (!pkt) return;

    // 1. Fill IPv4 Header
    pkt->version_ihl = 0x45; // Version 4, Header Length 5 (20 bytes)
    pkt->tos = 0;
    pkt->total_len = htons(total_len);
    pkt->id = htons(1);      // Simple ID for now
    pkt->flags_fragment = 0;
    pkt->ttl = 64;           // Standard TTL
    pkt->protocol = protocol;
    pkt->checksum = 0;       // Calculate this last
    pkt->src_ip = AETHER_IP_ADDR;
    pkt->dest_ip = dst_ip;

    // 2. IP Checksum (Just over the 20-byte header)
    // You can reuse Pritam's logic or a simple 16-bit accumulator
    uint32_t sum = 0;
    uint16_t *ptr = (uint16_t *)pkt;
    for (int i = 0; i < 10; i++) sum += ptr[i];
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    pkt->checksum = (uint16_t)(~sum);

    // 3. Copy Payload (The TCP Segment)
    memcpy((uint8_t *)pkt + sizeof(struct ipv4_header), data, len);

    // 4. Hand off to Ethernet/VirtIO
    // For QEMU user-mode networking, we can send this directly to the gateway MAC
    uint8_t gateway_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56}; 
    virtio_net_send(global_vnet_dev, gateway_mac, 0x0800, (uint8_t *)pkt, total_len);

    kfree(pkt);
}