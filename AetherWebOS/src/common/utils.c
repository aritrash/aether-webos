#include "utils.h"
#include "uart.h"
#include <stddef.h>

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

void* memset(void* s, int c, size_t n) {
    volatile unsigned char* p = (volatile unsigned char*)s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

void str_clear(char* str) {
    if (str) str[0] = '\0';
}

void str_append(char* str, const char* data) {
    if (!str || !data) return;
    while (*str) str++; 
    while (*data) *str++ = *data++;
    *str = '\0';
}

/**
 * str_append_kv_int: Formats "key": value for JSON.
 */
void str_append_kv_int(char* str, const char* key, uint64_t value) {
    str_append(str, "\"");
    str_append(str, key);
    str_append(str, "\":");
    
    char buf[21];
    int i = 0;
    if (value == 0) {
        buf[i++] = '0';
    } else {
        while (value > 0) {
            buf[i++] = (value % 10) + '0';
            value /= 10;
        }
    }
    
    // Reverse the string for correct order
    for (int j = 0; j < i / 2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i - j - 1];
        buf[i - j - 1] = tmp;
    }
    buf[i] = '\0';
    
    str_append(str, buf);
    str_append(str, ","); 
}