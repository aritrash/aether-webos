#ifndef UART_H
#define UART_H

#include "config.h"

/* * The UART0_BASE is now defined in config.h based on the BOARD flag.
 * RPi4: 0xFE201000
 * VIRT: 0x09000000
 */

/* PL011 Register Offsets */
#define UART0_DR      ((volatile unsigned int*)(UART0_BASE + 0x00))
#define UART0_RSRECR  ((volatile unsigned int*)(UART0_BASE + 0x04))
#define UART0_FR      ((volatile unsigned int*)(UART0_BASE + 0x18))
#define UART0_ILPR    ((volatile unsigned int*)(UART0_BASE + 0x20))
#define UART0_IBRD    ((volatile unsigned int*)(UART0_BASE + 0x24))
#define UART0_FBRD    ((volatile unsigned int*)(UART0_BASE + 0x28))
#define UART0_LCRH    ((volatile unsigned int*)(UART0_BASE + 0x2C))
#define UART0_CR      ((volatile unsigned int*)(UART0_BASE + 0x30))
#define UART0_IFLS    ((volatile unsigned int*)(UART0_BASE + 0x34))
#define UART0_IMSC    ((volatile unsigned int*)(UART0_BASE + 0x38))
#define UART0_RIS     ((volatile unsigned int*)(UART0_BASE + 0x3C))
#define UART0_MIS     ((volatile unsigned int*)(UART0_BASE + 0x40))
#define UART0_ICR     ((volatile unsigned int*)(UART0_BASE + 0x44))

/* * GPIO configuration is specific to the BCM2711 SoC (RPi4).
 * The QEMU 'virt' board uses a direct PL011 mapping that doesn't 
 * require GPIO pin muxing.
 */
#ifdef BOARD_RPI4
#define GPIO_BASE    (PERIPHERAL_START + 0x200000)
#define GPFSEL1      ((volatile unsigned int*)(GPIO_BASE + 0x04))
#define GPIO_PUP_PDN_CNTRL_REG0 ((volatile unsigned int*)(GPIO_BASE + 0xE4))
#endif

/* Function Prototypes */
void uart_init();
void uart_putc(unsigned char c);
void uart_puts(const char* str);
unsigned char uart_getc();

#endif