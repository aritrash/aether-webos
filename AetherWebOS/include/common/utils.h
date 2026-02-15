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

#endif