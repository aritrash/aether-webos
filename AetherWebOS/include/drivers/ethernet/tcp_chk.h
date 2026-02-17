#ifndef TCP_CHK_H
#define TCP_CHK_H

#include <stdint.h>
#include "drivers/ethernet/ipv4.h"
#include "drivers/ethernet/tcp.h"

/*
 * Calculates TCP checksum using:
 * - IPv4 pseudo header
 * - TCP header
 * - TCP payload
 */
uint16_t tcp_checksum(struct ipv4_header *ip,
                      struct tcp_header *tcp,
                      uint8_t *payload,
                      uint32_t payload_len);

/*
 * Returns:
 * 1 → Valid
 * 0 → Corrupt
 */
int tcp_validate_checksum(struct ipv4_header *ip,
                          struct tcp_header *tcp,
                          uint8_t *payload,
                          uint32_t payload_len);

#endif
