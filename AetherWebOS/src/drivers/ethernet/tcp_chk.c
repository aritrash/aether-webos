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
static uint32_t calculate_sum(void *data, uint32_t len, uint32_t initial_sum) {
    uint32_t sum = initial_sum;
    uint16_t *ptr = (uint16_t *)data;

    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }

    if (len == 1) {
        sum += *((uint8_t *)ptr);
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
    sum += htons(6); // Protocol TCP (6) padded to 16-bit
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

int tcp_validate_checksum(struct ipv4_header *ip, struct tcp_header *tcp, uint8_t *payload, uint32_t payload_len) {
    // For validation, we don't zero out the checksum. 
    // If the packet is valid, the sum over everything (including pseudo-header) 
    // will result in 0xFFFF when inverted, or 0 when not.
    uint16_t calc = tcp_checksum(ip, tcp, payload, payload_len);
    
    if (calc == tcp->checksum) return 1;

    // Debugging print for the Lead
    uart_puts("[TCP] Checksum Failure! RX: ");
    uart_put_hex_byte(tcp->checksum >> 8); uart_put_hex_byte(tcp->checksum & 0xFF);
    uart_puts(" Expected: ");
    uart_put_hex_byte(calc >> 8); uart_put_hex_byte(calc & 0xFF);
    uart_puts("\r\n");
    
    return 0;
}