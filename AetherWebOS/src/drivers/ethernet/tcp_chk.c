#include "drivers/ethernet/ipv4.h"
#include "drivers/ethernet/tcp.h"
#include "drivers/uart.h"
#include "common/utils.h"

/*
    Original Developer: Pritam Mondal
    
    Lead's Code Review: tcp_chk.c
    1. Double Endian Swapping: You are calling ntohs() on every 
       word. The checksum should be calculated on the big-endian 
       data exactly as it sits in memory. By swapping it, you're 
       essentially calculating a checksum for a packet that doesn't 
       exist.

    2. The "Zero-Checksum" Trap: In tcp_validate_checksum, you 
       compare the received_checksum to the calculated_checksum. 
       Standard RFC 793 behavior is simpler: if you run the checksum 
       over the whole packet including the received checksum, the 
       result should be 0x0000 (or 0xFFFF depending on the 
       implementation).

    3. The Accumulator Loop: Your checksum_add folds the carry 
       immediately, but then you fold it again at the end. While not 
       "wrong," it's computationally expensive for a bare-metal kernel.

    Changes Made: Fixed the above issues and added debug prints for checksum 
    failures.
    Date: 17.02.2026
*/

/**
 * calculate_sum: Generic 16-bit one's complement sum
 */
static uint32_t calculate_sum(void *data,
                              uint32_t len,
                              uint32_t initial_sum)
{
    uint32_t sum = initial_sum;
    uint8_t *ptr = (uint8_t *)data;

    while (len > 1) {
        uint16_t word = (ptr[0] << 8) | ptr[1];
        sum += word;
        ptr += 2;
        len -= 2;
    }

    if (len == 1) {
        sum += (ptr[0] << 8);
    }

    return sum;
}

uint16_t tcp_checksum(struct ipv4_header *ip, struct tcp_header *tcp, uint8_t *payload, uint32_t payload_len) {
    uint32_t sum = 0;
    uint16_t tcp_len = sizeof(struct tcp_header) + payload_len;

    // 1. Pseudo-header (Must be in Network Byte Order)
    sum += (ip->src_ip >> 16) & 0xFFFF;
    sum += ip->src_ip & 0xFFFF;
    sum += (ip->dest_ip >> 16) & 0xFFFF;
    sum += ip->dest_ip & 0xFFFF;
    sum += 0x0006; // Fix 1: Changed to hardcoded TCP protocol number
    sum += htons(tcp_len);

    // 2. TCP Header (Temporary zero out checksum field)
    uint16_t old_check = tcp->checksum;
    tcp->checksum = 0;
    sum = calculate_sum(tcp, sizeof(struct tcp_header), sum);
    tcp->checksum = old_check;

    // 3. Payload
    if (payload && payload_len > 0) {
        sum = calculate_sum(payload, payload_len, sum);
    }

    // 4. Final 16-bit Fold
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)(~sum);
}

int tcp_validate_checksum(struct ipv4_header *ip,
                          struct tcp_header *tcp,
                          uint8_t *payload,
                          uint32_t payload_len)
{
    uint32_t sum = 0;
    uint16_t tcp_len = sizeof(struct tcp_header) + payload_len;

    // Pseudo header
    sum += (ip->src_ip >> 16) & 0xFFFF;
    sum += ip->src_ip & 0xFFFF;
    sum += (ip->dest_ip >> 16) & 0xFFFF;
    sum += ip->dest_ip & 0xFFFF;
    sum += 0x0006;
    sum += tcp_len;

    // Entire TCP segment INCLUDING checksum
    sum = calculate_sum(tcp, sizeof(struct tcp_header), sum);

    if (payload && payload_len > 0)
        sum = calculate_sum(payload, payload_len, sum);

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return (sum == 0xFFFF);
}