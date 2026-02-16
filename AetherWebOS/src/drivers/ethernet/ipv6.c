#include <stdint.h>
#include "net/ipv6.h"
#include "uart.h"

/* ---------- helpers ---------- */

static uint16_t bswap16(uint16_t x) {
    return (x >> 8) | (x << 8);
}

static uint32_t bswap32(uint32_t x) {
    return  ((x >> 24) & 0x000000FF) |
            ((x >> 8)  & 0x0000FF00) |
            ((x << 8)  & 0x00FF0000) |
            ((x << 24) & 0xFF000000);
}

static void uart_print_hex16(uint16_t val) {
    char hex[] = "0123456789abcdef";
    int started = 0;

    for (int i = 12; i >= 0; i -= 4) {
        char c = hex[(val >> i) & 0xF];
        if (c != '0' || started || i == 0) {
            uart_putc(c);
            started = 1;
        }
    }
}

/* ---------- IPv6 printer with :: compression ---------- */

static void print_ipv6(uint8_t addr[16]) {
    uint16_t words[8];

    for (int i = 0; i < 8; i++) {
        words[i] = ((uint16_t)addr[i*2] << 8) | addr[i*2+1];
    }

    /* find longest zero run */
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

    if (best_len < 2)
        best_start = -1; lookups

    for (int i = 0; i < 8; i++) {

        if (i == best_start) {
            uart_puts("::");
            i += best_len - 1;
            continue;
        }

        uart_print_hex16(words[i]);

        if (i != 7)
            uart_putc(':');
    }
}

/* ---------- main handler ---------- */

void ipv6_handle(uint8_t *data, uint32_t len) {

    if (len < sizeof(struct ipv6_header)) {
        uart_puts("IPv6: packet too short\n");
        return;
    }

    struct ipv6_header *hdr = (struct ipv6_header*)data;

    uint32_t vtf = bswap32(hdr->vtc_flow);
    uint8_t version = (vtf >> 28) & 0xF;

    if (version != 6) {
        uart_puts("IPv6: invalid version\n");
        return;
    }

    uart_puts("IPv6 src ");
    print_ipv6(hdr->src_addr);
    uart_putc('\n');

    uart_puts("Next Header: ");

    switch (hdr->next_header) {
        case 58:
            uart_puts("ICMPv6\n");
            break;

        case 6:
            uart_puts("TCP\n");
            break;

        default:
            uart_puts("Unknown\n");
            break;
    }
}
