#include "uart.h"
#include "config.h"

#include <stdint.h>

/* =====================================
    Small Delay (Required for RPi4 GPIO)
    ===================================== */
__attribute__((unused)) static void delay(int count)
{
    while (count--)
        asm volatile("nop");
}

/* =====================================
    Initialize PL011 UART
    ===================================== */
void uart_init(void)
{
#ifdef BOARD_RPI4
    /* ---------------------------------
        Configure GPIO14 & GPIO15 (ALT0)
        --------------------------------- */
    *GPFSEL1 &= ~((7 << 12) | (7 << 15));
    *GPFSEL1 |=  ((4 << 12) | (4 << 15));
    *GPIO_PUP_PDN_CNTRL_REG0 &= ~((3 << 28) | (3 << 30));
    delay(150);
#endif

    *UART0_CR = 0x0;   // Disable UART
    *UART0_IMSC = 0x0; // Disable Interrupts
    *UART0_ICR = 0x7FF; // Clear interrupts

    // Baud Rate = 115200
    *UART0_IBRD = 26;
    *UART0_FBRD = 3;

    // 8-bit, No Parity, 1 Stop, FIFO ON (Bit 4)
    *UART0_LCRH = (1 << 4) | (3 << 5);

    // Enable UART (Bit 0), TX (Bit 8), RX (Bit 9)
    *UART0_CR = (1 << 9) | (1 << 8) | 1;
}

/* =====================================
    Flow Control: Check if UART can accept data
    ===================================== */
/**
 * uart_is_writable: Returns 1 if the Transmit FIFO is NOT full.
 * This prevents "clobbering" the Portal UI during high-frequency refreshes.
 */
int uart_is_writable(void)
{
    // Bit 5 of Flag Register (FR) is TXFF (Transmit FIFO Full)
    return !(*UART0_FR & (1 << 5));
}

/* =====================================
    Transmission Helpers
    ===================================== */

void uart_putc(unsigned char c)
{
    while (!uart_is_writable()); // Wait until TX FIFO has space
    *UART0_DR = c;
}

void uart_puts(const char *str)
{
    while (*str) {
        if (*str == '\n')
            uart_putc('\r');
        uart_putc(*str++);
    }
}

/**
 * uart_put_int: Prints a 32-bit integer as a decimal string.
 */
void uart_put_int(uint32_t n) {
    if (n == 0) {
        uart_putc('0');
        return;
    }
    char buf[12];
    int i = 0;
    while (n > 0) {
        buf[i++] = (n % 10) + '0';
        n /= 10;
    }
    while (--i >= 0) uart_putc(buf[i]);
}

/**
 * uart_put_hex: Prints a 64-bit value in hex format (0x...)
 */
void uart_put_hex(uint64_t n) {
    char *hextab = "0123456789ABCDEF";
    uart_puts("0x");
    for (int i = 60; i >= 0; i -= 4) {
        uart_putc(hextab[(n >> i) & 0xF]);
    }
}

/* =====================================
    Reception Helpers
    ===================================== */

unsigned char uart_getc(void)
{
    // Bit 4 of Flag Register (FR) is RXFE (Receive FIFO Empty)
    while (*UART0_FR & (1 << 4));
    return (unsigned char)(*UART0_DR & 0xFF);
}

int uart_is_empty() {
    return (*UART0_FR & (1 << 4));
}