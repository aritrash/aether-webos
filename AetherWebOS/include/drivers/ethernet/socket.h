#ifndef AETHER_SOCKET_H
#define AETHER_SOCKET_H

#include <stdint.h>
#include <drivers/ethernet/tcp/tcp.h>

void socket_dispatch(tcp_tcb_t *tcb,
                     uint8_t *payload,
                     uint16_t len);

#endif