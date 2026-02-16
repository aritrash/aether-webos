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