#include "drivers/ethernet/tcp_chk.h"
#include "drivers/uart.h"
#include "common/utils.h"


/* ============================================
   Internal Helper: Add 16-bit words safely
   ============================================ */
static uint32_t checksum_add(uint32_t sum, uint16_t val)
{
    sum += val;

    /* Carry wrap */
    if (sum & 0x10000) {
        sum = (sum & 0xFFFF) + 1;
    }

    return sum;
}


/* ============================================
   TCP Checksum (RFC 793 + Pseudo Header)
   ============================================ */
uint16_t tcp_checksum(struct ipv4_header *ip,
                      struct tcp_header *tcp,
                      uint8_t *payload,
                      uint32_t payload_len)
{
    uint32_t sum = 0;


    /* =====================================
       1. Pseudo Header
       ===================================== */

    uint32_t src = ntohl(ip->src_ip);
    uint32_t dst = ntohl(ip->dest_ip);

    /* Source IP */
    sum = checksum_add(sum, (src >> 16) & 0xFFFF);
    sum = checksum_add(sum, src & 0xFFFF);

    /* Destination IP */
    sum = checksum_add(sum, (dst >> 16) & 0xFFFF);
    sum = checksum_add(sum, dst & 0xFFFF);

    /* Zero + Protocol */
    sum = checksum_add(sum, IP_PROTO_TCP);

    /* TCP length */
    uint16_t tcp_len =
        sizeof(struct tcp_header) + payload_len;

    sum = checksum_add(sum, tcp_len);


    /* =====================================
       2. TCP Header
       ===================================== */

    uint16_t *ptr = (uint16_t *)tcp;
    uint32_t len = sizeof(struct tcp_header);

    /* Save & clear checksum field */
    uint16_t old = tcp->checksum;
    tcp->checksum = 0;

    while (len > 1) {

        uint16_t word = *ptr;
        ptr++;

        sum = checksum_add(sum, ntohs(word));
        len -= 2;
    }


    /* =====================================
       3. TCP Payload
       ===================================== */

    ptr = (uint16_t *)payload;
    len = payload_len;

    while (len > 1) {

        uint16_t word = *ptr;
        ptr++;

        sum = checksum_add(sum, ntohs(word));
        len -= 2;
    }

    /* Odd byte */
    if (len) {
        sum = checksum_add(sum, (*(uint8_t *)ptr) << 8);
    }


    /* Restore checksum */
    tcp->checksum = old;


    /* =====================================
       4. Fold to 16 bits
       ===================================== */

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)(~sum);
}


/* ============================================
   Validation Gatekeeper
   ============================================ */
int tcp_validate_checksum(struct ipv4_header *ip,
                          struct tcp_header *tcp,
                          uint8_t *payload,
                          uint32_t payload_len)
{
    uint16_t received = tcp->checksum;

    uint16_t calculated =
        tcp_checksum(ip, tcp, payload, payload_len);

    if (received == calculated) {
        return 1;   /* Valid */
    }

    uart_puts("[TCP] Bad checksum dropped\r\n");

    return 0;       /* Corrupt */
}
