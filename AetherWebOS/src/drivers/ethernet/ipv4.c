#include "drivers/ethernet/ipv4.h"
#include "drivers/uart.h"
#include "common/utils.h"        // ntohs(), htons(), ntohl()
#include "kernel/health.h"       // <-- For health_report_checksum_error()


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

        case IP_PROTO_TCP:
            uart_puts("[IP] TCP Packet detected\r\n");
            break;

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
