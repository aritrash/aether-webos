#include "uart.h"
#include "config.h"

/* =====================================
   Small Delay (Required for RPi4 GPIO)
   ===================================== */
static void delay(int count)
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
       Only applies to physical BCM2711
       --------------------------------- */

    /* Clear GPIO14 and GPIO15 */
    *GPFSEL1 &= ~((7 << 12) | (7 << 15));

    /* Set ALT0 for GPIO14, GPIO15 */
    *GPFSEL1 |=  ((4 << 12) | (4 << 15));

    /* Disable pull-up / pull-down */
    *GPIO_PUP_PDN_CNTRL_REG0 &= ~((3 << 28) | (3 << 30));

    delay(150);
#endif

    /* ---------------------------------
       Disable UART
       --------------------------------- */
    *UART0_CR = 0x0;

    /* ---------------------------------
       Disable Interrupts (Polling Mode)
       --------------------------------- */
    *UART0_IMSC = 0x0;

    /* Clear pending interrupts */
    *UART0_ICR = 0x7FF;

    /* ---------------------------------
       Baud Rate = 115200
       (Note: QEMU ignores baud, but RPi4 needs it)
       --------------------------------- */
    *UART0_IBRD = 26;
    *UART0_FBRD = 3;

    /* ---------------------------------
       8-bit, No Parity, 1 Stop, FIFO ON
       --------------------------------- */
    *UART0_LCRH = (1 << 4) | (3 << 5);

    /* ---------------------------------
       Enable UART, TX, RX
       --------------------------------- */
    *UART0_CR = (1 << 9) | (1 << 8) | 1;
}

/* =====================================
   Send One Character
   ===================================== */
void uart_putc(unsigned char c)
{
    /* Wait until TX FIFO not full */
    while (*UART0_FR & (1 << 5));

    *UART0_DR = c;
}

/* =====================================
   Send String
   ===================================== */
void uart_puts(const char *str)
{
    while (*str) {
        /* Convert LF to CRLF for terminal compatibility */
        if (*str == '\n')
            uart_putc('\r');

        uart_putc(*str++);
    }
}

/* =====================================
   Receive One Character
   ===================================== */
unsigned char uart_getc(void)
{
    /* Wait until RX FIFO not empty */
    while (*UART0_FR & (1 << 4));

    return (unsigned char)(*UART0_DR & 0xFF);
}