#ifndef TCP_CHK_H
#define TCP_CHK_H

#include <stdint.h>

#include "drivers/ethernet/ipv4.h"
#include "drivers/ethernet/tcp.h"

/*
 * Calculate TCP checksum using:
 * - IPv4 pseudo header
 * - TCP header
 * - Payload
 */
uint16_t tcp_checksum(struct ipv4_header *ip,
                      struct tcp_header *tcp,
                      uint8_t *payload,
                      uint32_t payload_len);

/*
 * Validate checksum
 * Returns:
 *   1 = valid
 *   0 = corrupt
 */
int tcp_validate_checksum(struct ipv4_header *ip,
                          struct tcp_header *tcp,
                          uint8_t *payload,
                          uint32_t payload_len);

#endif
