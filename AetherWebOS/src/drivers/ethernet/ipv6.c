#include <stdint.h>
#include "drivers/ethernet/ipv6.h"
#include "drivers/uart.h"
#include "common/utils.h" // For bswap if needed

/* ---------- helpers ---------- */

static uint32_t bswap32(uint32_t x) {
    return __builtin_bswap32(x); // Use the builtins for AArch64 efficiency
}
/* ---------- IPv6 printer with :: compression ---------- */

void print_ipv6(uint8_t addr[16]) {
    uint16_t words[8];
    for (int i = 0; i < 8; i++) {
        words[i] = ((uint16_t)addr[i*2] << 8) | addr[i*2+1];
    }

    int best_start = -1, best_len = 0;
    for (int i = 0; i < 8;) {
        if (words[i] == 0) {
            int j = i;
            while (j < 8 && words[j] == 0) j++;
            int len = j - i;
            if (len > best_len) {
                best_len = len;
                best_start = i;
            }
            i = j;
        } else {
            i++;
        }
    }

    if (best_len < 2) best_start = -1;

    for (int i = 0; i < 8; i++) {
        if (i == best_start) {
            uart_puts("::");
            i += best_len - 1;
            continue;
        }

        uart_print_hex16(words[i]);

        // Only print colon if not at the end and not right before compression
        if (i != 7 && (i + 1 != best_start)) {
            uart_putc(':');
        }
    }
}

/* ---------- main handler ---------- */

void ipv6_handle(uint8_t *data, uint32_t len) {
    if (len < sizeof(struct ipv6_header)) return;

    struct ipv6_header *hdr = (struct ipv6_header*)data;

    // The first 4 bytes contain Version, Traffic Class, and Flow Label
    uint32_t vtf = bswap32(hdr->vtc_flow);
    uint8_t version = (vtf >> 28) & 0xF;

    if (version != 6) return;

    uart_puts("[IPv6] src: ");
    print_ipv6(hdr->src_addr);
    uart_puts(" -> ");

    switch (hdr->next_header) {
        case 58: uart_puts("ICMPv6\r\n"); break;
        case 6:  uart_puts("TCP\r\n"); break;
        default: uart_puts("Unknown Proto\r\n"); break;
    }
}