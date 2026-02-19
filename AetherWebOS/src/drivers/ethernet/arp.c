#include "ethernet/arp.h"
#include "drivers/virtio/virtio_net.h"
#include "ethernet/ethernet.h"
#include "common/utils.h"
#include "uart.h"

#define ARP_CACHE_SIZE 4

struct arp_entry {
    uint32_t ip;       // Stored in HOST order
    uint8_t  mac[6];
};

static struct arp_entry arp_cache[ARP_CACHE_SIZE];

/* External values */
extern uint32_t aether_ip;   // HOST order
extern uint8_t  aether_mac[6];

/* ================================
   ARP Cache Insert (Host Order)
   ================================ */
static void arp_cache_insert(uint32_t ip_host_order, uint8_t *mac)
{
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].ip == 0 || arp_cache[i].ip == ip_host_order) {
            arp_cache[i].ip = ip_host_order;
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

    uart_puts("[ARP] Packet\r\n");

    uint32_t sip = __builtin_bswap32(arp->sender_ip);
    uint32_t tip = __builtin_bswap32(arp->target_ip);

    uart_puts("Sender IP: ");
    uart_put_hex(sip);
    uart_puts("\r\n");

    uart_puts("Target IP: ");
    uart_put_hex(tip);
    uart_puts("\r\n");

    /* Validate hardware type (Ethernet) */
    if (__builtin_bswap16(arp->htype) != ARP_HTYPE_ETH)
        return;

    /* Validate protocol type (IPv4) */
    if (__builtin_bswap16(arp->ptype) != ARP_PTYPE_IPV4)
        return;

    /* Learn sender (store IP in host order inside cache) */
    uint32_t sender_ip_host = __builtin_bswap32(arp->sender_ip);
    arp_cache_insert(sender_ip_host, arp->sender_mac);

    /* Only handle ARP requests */
    if (__builtin_bswap16(arp->opcode) != ARP_OPCODE_REQUEST)
        return;

    /* IMPORTANT:
       arp->target_ip is in NETWORK order.
       aether_ip must also be in NETWORK order.
    */
    uint32_t target_ip_host = __builtin_bswap32(arp->target_ip);

    if (target_ip_host != aether_ip)
        return;

    /* ==============================
       Construct ARP Reply
       ============================== */
    struct arp_packet reply;

    reply.htype  = __builtin_bswap16(ARP_HTYPE_ETH);
    reply.ptype  = __builtin_bswap16(ARP_PTYPE_IPV4);
    reply.hlen   = 6;
    reply.plen   = 4;
    reply.opcode = __builtin_bswap16(ARP_OPCODE_REPLY);

    /* Sender = us */
    memcpy(reply.sender_mac, aether_mac, 6);
    reply.sender_ip =  __builtin_bswap32(aether_ip);   // already network order

    /* Target = original sender */
    memcpy(reply.target_mac, arp->sender_mac, 6);
    reply.target_ip = arp->sender_ip;   // already network order

    uart_puts("[ARP] Reply triggered\r\n");

    ethernet_send(
        arp->sender_mac,
        ETH_TYPE_ARP,
        (uint8_t *)&reply,
        sizeof(struct arp_packet)
    );
}
