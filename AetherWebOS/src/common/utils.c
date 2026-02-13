#include "utils.h"
#include "uart.h"

void uart_put_hex(uint64_t d) {
    char n;
    uart_puts("0x");
    for (int i = 60; i >= 0; i -= 4) {
        n = (d >> i) & 0xF;
        // Convert 0-15 to '0'-'9' and 'A'-'F'
        n += (n > 9) ? ('A' - 10) : '0';
        uart_putc(n);
    }
}

void uart_put_int(uint64_t n) {
    if (n == 0) {
        uart_putc('0');
        return;
    }
    char buf[20];
    int i = 0;
    while (n > 0) {
        buf[i++] = (n % 10) + '0';
        n /= 10;
    }
    while (--i >= 0) uart_putc(buf[i]);
}