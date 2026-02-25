#ifndef AETHER_IPV4_H
#define AETHER_IPV4_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================
 *                      IPv4 HEADER
 * ============================================================ */

struct ipv4_header {
    uint8_t  version_ihl;      /* Version (4) + IHL (4) */
    uint8_t  tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t flags_fragment;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
} __attribute__((packed));

/* ============================================================
 *                      PROTOCOL NUMBERS
 * ============================================================ */

#define IP_PROTO_ICMP   1
#define IP_PROTO_TCP    6
#define IP_PROTO_UDP    17

/* ============================================================
 *                      PUBLIC API
 * ============================================================ */

uint16_t ipv4_checksum(void *data, size_t len);

void ipv4_handle(uint8_t *data, uint32_t len);

void ipv4_send(uint32_t dst_ip,
               uint8_t protocol,
               const uint8_t *payload,
               uint32_t payload_len);

#endif