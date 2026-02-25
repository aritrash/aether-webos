#include <stdint.h>
#include <stddef.h>
#include "drivers/ethernet/tcp/tcp_internal.h"
#include "common/utils.h"
#include "ethernet/ipv4.h"

/**
 * Standard Internet Checksum: 16-bit one's complement sum.
 */
static uint32_t checksum_accumulate(const uint8_t *data, uint16_t length) {
    uint32_t sum = 0;
    const uint16_t *ptr = (const uint16_t *)data;

    while (length > 1) {
        sum += *ptr++;
        length -= 2;
    }

    /* Handle odd-length byte padding */
    if (length > 0) {
        sum += (uint32_t)(*(const uint8_t *)ptr);
    }

    return sum;
}

/**
 * Folds a 32-bit accumulator into a 16-bit one's complement result.
 */
static uint16_t checksum_finalize(uint32_t sum) {
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return (uint16_t)~sum;
}

/**
 * Computes the TCP Checksum including the mandatory IPv4 Pseudo-Header.
 */
uint16_t tcp_compute_checksum(uint32_t src_ip, uint32_t dst_ip, 
                               const uint8_t *segment, uint16_t tcp_len) {
    uint32_t sum = 0;

    /* 1. Pseudo-Header: Source and Destination IPs 
       (Already in network order from TCB) */
    sum += (src_ip & 0xFFFF);
    sum += (src_ip >> 16);
    sum += (dst_ip & 0xFFFF);
    sum += (dst_ip >> 16);

    /* 2. Pseudo-Header: Protocol (6) and TCP Length 
       Note: Values must be in Network Order during summation */
    sum += (uint32_t)htons(IP_PROTO_TCP);
    sum += (uint32_t)htons(tcp_len);

    /* 3. TCP Header + Payload */
    sum += checksum_accumulate(segment, tcp_len);

    /* 4. Final Wrap and Invert */
    return checksum_finalize(sum);
}

/**
 * Validates an incoming TCP segment.
 * If the checksum is correct, the result of the accumulation over the 
 * pseudo-header + segment (including the checksum field) must be 0xFFFF.
 */
int tcp_validate_checksum(uint32_t src_ip, uint32_t dst_ip, 
                          const uint8_t *segment, uint16_t length) {
    /* * In a production stack, you can simply run the sum over the packet.
     * If the result is 0, the checksum is valid.
     */
    uint16_t res = tcp_compute_checksum(src_ip, dst_ip, segment, length);
    
    /* When computing over a packet that already has a valid checksum,
       the result will be 0 because the checksum field is the inverse
       of the rest of the sum. */
    return (res == 0);
}