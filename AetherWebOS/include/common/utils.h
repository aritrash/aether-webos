#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stddef.h>

// Forward declaration of UART function so utils knows it exists
void uart_putc(unsigned char c);

// Prints a 64-bit value in hexadecimal
void uart_put_hex(uint64_t d);

// Prints a 64-bit value in decimal
void uart_put_int(uint64_t n);

void enable_interrupts(void);
void disable_interrupts(void);
void* memset(void* s, int c, size_t n) __attribute__((nonnull (1)));

void str_clear(char* str);
void str_append(char* str, const char* data);
void str_append_kv_int(char* str, const char* key, uint64_t value);

void *memcpy(void *dest, const void *src, size_t n);
void mini_sprintf_telemetry(char* out, unsigned long rx, unsigned long tx, 
                            unsigned long err, unsigned long buf, 
                            unsigned long tcp, unsigned long rexmit, 
                            unsigned long cksum);
extern char* ltoa(unsigned long num, char* str);
uint32_t str_len(const char* s);
int str_contains(const char* haystack, const char* needle);
void uart_put_ip(uint32_t ip);

#include <stdarg.h>

int ksnprintf(char *out,
              unsigned int out_size,
              const char *fmt,
              ...);

/* Byte order conversion (little-endian CPU) */

uint16_t htons(uint16_t x);
uint16_t ntohs(uint16_t x);
uint32_t htonl(uint32_t x);
uint32_t ntohl(uint32_t x);

#endif