#include "utils.h"
#include "uart.h"
#include <stddef.h>


void* memset(void* s, int c, size_t n) {
    volatile unsigned char* p = (volatile unsigned char*)s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

void *memcpy(void *dest, const void *src, size_t n) {
    char *d = dest;
    const char *s = src;
    while (n--) *d++ = *s++;
    return dest;
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

// helper to convert long to string
extern char* ltoa(unsigned long num, char* str) {
    char* p = str;
    unsigned long tmp = num;
    
    // Calculate length
    do {
        p++;
        tmp /= 10;
    } while (tmp);
    
    char* end = p;
    *p-- = '\0';
    
    // Fill digits backward
    do {
        *p-- = (num % 10) + '0';
        num /= 10;
    } while (num);
    
    return end; // returns pointer to null terminator
}

// Minimal sprintf supporting only %lu and strings
void mini_sprintf_telemetry(char* out, unsigned long rx, unsigned long tx, unsigned long err, 
                            unsigned long buf, unsigned long tcp, unsigned long rexmit, 
                            unsigned long cksum) {
    char* p = out;
    // Task: JSON Expansion with new keys
    char* keys[] = {"{\"rx\":", ",\"tx\":", ",\"err\":", ",\"buf\":", 
                    ",\"tcp_active\":", ",\"retransmissions\":", ",\"checksum_err\":"};
    unsigned long values[] = {rx, tx, err, buf, tcp, rexmit, cksum};

    for(int i = 0; i < 7; i++) {
        char* k = keys[i];
        while(*k) *p++ = *k++;
        p = ltoa(values[i], p);
    }
    *p++ = '}';
    *p = '\0';
}

void uart_put_ip(uint32_t ip) {
    char buf[4];
    for (int i = 3; i >= 0; i--) {
        char val = (ip >> (i * 8)) & 0xFF;
        // You'd need a simple itoa or similar here to print the 0-255 value
        // For now, even hex output is better than nothing!
    }
}