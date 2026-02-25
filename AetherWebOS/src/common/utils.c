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
    // Suppress unused variable warnings by commenting them out or using them
    for (int i = 3; i >= 0; i--) {
        // uint8_t octet = (ip >> (i * 8)) & 0xFF;
        // itoa logic would go here
        (void)ip; // Temporary suppression
    }
}

uint32_t str_len(const char* s) {
    uint32_t len = 0;
    while (s[len]) len++;
    return len;
}

int str_contains(const char* haystack, const char* needle) {
    if (!haystack || !needle) return 0;
    for (int i = 0; haystack[i]; i++) {
        int j = 0;
        while (needle[j] && haystack[i+j] == needle[j]) j++;
        if (!needle[j]) return 1;
    }
    return 0;
}

/* =========================================
   Internal helper: unsigned int to string
   ========================================= */
static int utoa_dec(char *buf, unsigned int value)
{
    char tmp[16];
    int i = 0;

    if (value == 0) {
        buf[0] = '0';
        return 1;
    }

    while (value > 0) {
        tmp[i++] = '0' + (value % 10);
        value /= 10;
    }

    int len = i;

    for (int j = 0; j < len; j++)
        buf[j] = tmp[len - j - 1];

    return len;
}

/* =========================================
   Minimal snprintf (supports %u %d %s)
   ========================================= */
int ksnprintf(char *out,
              unsigned int out_size,
              const char *fmt,
              ...)
{
    if (!out || out_size == 0)
        return 0;

    char *dst = out;
    unsigned int remaining = out_size - 1; // reserve null

    va_list args;
    va_start(args, fmt);

    while (*fmt && remaining > 0)
    {
        if (*fmt != '%') {
            *dst++ = *fmt++;
            remaining--;
            continue;
        }

        fmt++; // skip %

        if (*fmt == 'u') {
            unsigned int val = va_arg(args, unsigned int);
            char num[16];
            int len = utoa_dec(num, val);

            for (int i = 0; i < len && remaining > 0; i++) {
                *dst++ = num[i];
                remaining--;
            }
        }
        else if (*fmt == 'd') {
            int val = va_arg(args, int);

            if (val < 0) {
                if (remaining > 0) {
                    *dst++ = '-';
                    remaining--;
                }
                val = -val;
            }

            char num[16];
            int len = utoa_dec(num, (unsigned int)val);

            for (int i = 0; i < len && remaining > 0; i++) {
                *dst++ = num[i];
                remaining--;
            }
        }
        else if (*fmt == 's') {
            char *str = va_arg(args, char*);
            while (*str && remaining > 0) {
                *dst++ = *str++;
                remaining--;
            }
        }

        fmt++;
    }

    va_end(args);

    *dst = '\0';

    return (int)(dst - out);
}

/* ============================================================
 *                  BYTE ORDER CONVERSION
 * ============================================================ */

/*
 * AArch64 is little-endian.
 * Network byte order is big-endian.
 * So we must byte-swap.
 */

uint16_t htons(uint16_t x)
{
    return (uint16_t)(
        ((x & 0x00FF) << 8) |
        ((x & 0xFF00) >> 8)
    );
}

uint16_t ntohs(uint16_t x)
{
    return htons(x);
}

uint32_t htonl(uint32_t x)
{
    return ((x & 0x000000FF) << 24) |
           ((x & 0x0000FF00) << 8)  |
           ((x & 0x00FF0000) >> 8)  |
           ((x & 0xFF000000) >> 24);
}

uint32_t ntohl(uint32_t x)
{
    return htonl(x);
}