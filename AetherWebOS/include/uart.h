#ifndef UART_H
#define UART_H

/* * BCM2711 Peripheral Base Address 
 * For Raspberry Pi 4, this is 0xFE000000
 */
#define PERIPHERAL_BASE 0xFE000000

/* * PL011 UART Base Address 
 * Offset from peripheral base is 0x201000
 */
#define UART0_BASE (PERIPHERAL_BASE + 0x201000)

/* PL011 Register Offsets */
#define UART0_DR     ((volatile unsigned int*)(UART0_BASE + 0x00))
#define UART0_RSRECR ((volatile unsigned int*)(UART0_BASE + 0x04))
#define UART0_FR     ((volatile unsigned int*)(UART0_BASE + 0x18))
#define UART0_ILPR   ((volatile unsigned int*)(UART0_BASE + 0x20))
#define UART0_IBRD   ((volatile unsigned int*)(UART0_BASE + 0x24))
#define UART0_FBRD   ((volatile unsigned int*)(UART0_BASE + 0x28))
#define UART0_LCRH   ((volatile unsigned int*)(UART0_BASE + 0x2C))
#define UART0_CR     ((volatile unsigned int*)(UART0_BASE + 0x30))
#define UART0_IFLS   ((volatile unsigned int*)(UART0_BASE + 0x34))
#define UART0_IMSC   ((volatile unsigned int*)(UART0_BASE + 0x38))
#define UART0_RIS    ((volatile unsigned int*)(UART0_BASE + 0x3C))
#define UART0_MIS    ((volatile unsigned int*)(UART0_BASE + 0x40))
#define UART0_ICR    ((volatile unsigned int*)(UART0_BASE + 0x44))

/* GPIO Base Address for Pin Configuration */
#define GPIO_BASE    (PERIPHERAL_BASE + 0x200000)
#define GPFSEL1      ((volatile unsigned int*)(GPIO_BASE + 0x04))
#define GPIO_PUP_PDN_CNTRL_REG0 ((volatile unsigned int*)(GPIO_BASE + 0xE4))

/* Function Prototypes */
void uart_init();
void uart_putc(unsigned char c);
void uart_puts(const char* str);
unsigned char uart_getc();

#endif