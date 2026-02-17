#include "drivers/ethernet/tcp_chk.h"
#include "common/utils.h"
#include "drivers/uart.h"


/* =====================================================
   Internal 16-bit Adder (safe carry fold)
   ===================================================== */

static uint32_t checksum_add(uint32_t sum, uint16_t value)
{
    sum += value;

    /* Carry wrap */
    if (sum > 0xFFFF) {
        sum = (sum & 0xFFFF) + 1;
    }

    return sum;
}


/* =====================================================
   TCP Checksum (RFC 793 + IPv4 Pseudo Header)
   ===================================================== */

uint16_t tcp_checksum(struct ipv4_header *ip,
                      struct tcp_header *tcp,
                      uint8_t *payload,
                      uint32_t payload_len)
{
    uint32_t sum = 0;

    /* ================================
       1. IPv4 Pseudo Header
       ================================ */

    uint32_t src_ip = ntohl(ip->src_ip);
    uint32_t dst_ip = ntohl(ip->dest_ip);

    /* Source IP */
    sum = checksum_add(sum, (src_ip >> 16) & 0xFFFF);
    sum = checksum_add(sum,  src_ip        & 0xFFFF);

    /* Destination IP */
    sum = checksum_add(sum, (dst_ip >> 16) & 0xFFFF);
    sum = checksum_add(sum,  dst_ip        & 0xFFFF);

    /* Zero (8 bits) + Protocol (8 bits) */
    sum = checksum_add(sum, IP_PROTO_TCP);

    /* TCP Length */
    uint16_t tcp_length =
        sizeof(struct tcp_header) + payload_len;

    sum = checksum_add(sum, tcp_length);


    /* ================================
       2. TCP Header
       ================================ */

    uint16_t *ptr = (uint16_t *)tcp;
    uint32_t header_len = sizeof(struct tcp_header);

    uint16_t original_checksum = tcp->checksum;
    tcp->checksum = 0;

    while (header_len > 1) {
        uint16_t word = *ptr;
        sum = checksum_add(sum, ntohs(word));
        ptr++;
        header_len -= 2;
    }

    tcp->checksum = original_checksum;


    /* ================================
       3. TCP Payload
       ================================ */

    ptr = (uint16_t *)payload;

    while (payload_len > 1) {
        uint16_t word = *ptr;
        sum = checksum_add(sum, ntohs(word));
        ptr++;
        payload_len -= 2;
    }

    /* Odd byte handling */
    if (payload_len == 1) {
        uint8_t last = *((uint8_t *)ptr);
        sum = checksum_add(sum, (uint16_t)(last << 8));
    }


    /* ================================
       4. Final Fold
       ================================ */

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)(~sum);
}


/* =====================================================
   Validation Gatekeeper
   ===================================================== */

int tcp_validate_checksum(struct ipv4_header *ip,
                          struct tcp_header *tcp,
                          uint8_t *payload,
                          uint32_t payload_len)
{
    uint16_t received_checksum = tcp->checksum;

    uint16_t calculated_checksum =
        tcp_checksum(ip, tcp, payload, payload_len);

    if (received_checksum == calculated_checksum) {
        return 1;   /* Valid */
    }

    uart_puts("[TCP] Checksum invalid\r\n");
    return 0;       /* Corrupt */
}
