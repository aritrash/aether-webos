#include "net/arp.h"
#include "drivers/virtio/virtio_net.h"
#include "net/ethernet.h"
#include "kernel/string.h"

#define ARP_CACHE_SIZE 4

struct arp_entry {
    uint32_t ip;
    uint8_t  mac[6];
};

static struct arp_entry arp_cache[ARP_CACHE_SIZE];

/* External values */
extern uint32_t aether_ip;
extern uint8_t  aether_mac[6];

/* Store in cache */
static void arp_cache_insert(uint32_t ip, uint8_t *mac)
{
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].ip == 0 || arp_cache[i].ip == ip) {
            arp_cache[i].ip = ip;
            memcpy(arp_cache[i].mac, mac, 6);
            return;
        }
    }
}

void arp_handle(uint8_t *data, uint32_t len)
{
    if (len < sizeof(struct arp_packet))
        return;

    struct arp_packet *arp = (struct arp_packet *)data;

    if (arp->htype != __builtin_bswap16(ARP_HTYPE_ETH))
        return;

    if (arp->ptype != __builtin_bswap16(ARP_PTYPE_IPV4))
        return;

    /* Learn sender */
    arp_cache_insert(arp->sender_ip, arp->sender_mac);

    /* Is this for us? */
    if (arp->target_ip != aether_ip)
        return;

    if (__builtin_bswap16(arp->opcode) != ARP_OPCODE_REQUEST)
        return;

    /* Construct reply */
    struct arp_packet reply;

    reply.htype = __builtin_bswap16(ARP_HTYPE_ETH);
    reply.ptype = __builtin_bswap16(ARP_PTYPE_IPV4);
    reply.hlen  = 6;
    reply.plen  = 4;
    reply.opcode = __builtin_bswap16(ARP_OPCODE_REPLY);

    memcpy(reply.sender_mac, aether_mac, 6);
    reply.sender_ip = aether_ip;

    memcpy(reply.target_mac, arp->sender_mac, 6);
    reply.target_ip = arp->sender_ip;

    ethernet_send(
        arp->sender_mac,
        0x0806, /* ARP EtherType */
        (uint8_t *)&reply,
        sizeof(reply)
    );
}
