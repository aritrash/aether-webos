#include "uart.h"
#include "utils.h"
#include "gic.h"

typedef struct {
    uint64_t x[31]; 
} trap_frame_t;

void handle_sync_exception(uint64_t esr, uint64_t elr, uint64_t far, trap_frame_t *frame) {
    uart_puts("\r\n--- [!!!] KERNEL PANIC: SYNCHRONOUS ABORT [!!!] ---\r\n");

    uint32_t ec = (esr >> 26) & 0x3F;
    uint32_t iss = esr & 0xFFFFFF;

    // Decode Exception Class
    uart_puts("Condition: ");
    switch(ec) {
        case 0x00: uart_puts("Unknown/Undefined Instruction"); break;
        case 0x21: uart_puts("Instruction Abort (Lower EL)"); break;
        case 0x25: uart_puts("Data Abort (EL1)"); break;
        case 0x3C: uart_puts("Breakpoint"); break;
        default:   uart_puts("Other Syndrome: "); uart_put_hex(ec);
    }
    
    uart_puts("\r\nESR_EL1: "); uart_put_hex(esr);
    uart_puts("\r\nISS:     "); uart_put_hex(iss); // Syndrome info
    uart_puts("\r\nELR_EL1: "); uart_put_hex(elr);
    uart_puts("\r\nFAR_EL1: "); uart_put_hex(far);
    uart_puts("\r\n");

    uart_puts("\r\n--- General Purpose Registers ---\r\n");
    for (int i = 0; i < 31; i++) {
        uart_puts("x");
        uart_put_int(i);
        if (i < 10) uart_puts(":  "); else uart_puts(": ");
        
        uart_put_hex(frame->x[i]);

        // 3 columns for better scannability
        if ((i + 1) % 3 == 0) uart_puts("\r\n");
        else uart_puts("   ");
    }

    uart_puts("\r\n\r\nLR (x30): "); uart_put_hex(frame->x[30]);
    uart_puts("\r\n--------------------------------------------------\r\n");
    
    while(1);
}

void handle_irq_exception(uint64_t esr, uint64_t elr, uint64_t far, trap_frame_t *frame) {
    uint32_t iar = GICC_IAR;
    uint32_t irq_id = iar & 0x3FF; // Get the ID (bits 0-9)

    if (irq_id == TIMER_IRQ_ID) {
        uart_puts("."); // Heartbeat!
        
        // Reset Timer (Your existing logic)
        uint64_t freq;
        asm volatile ("mrs %0, cntfrq_el0" : "=r" (freq));
        asm volatile ("msr cntp_tval_el0, %0" : : "r" (freq / 2));
    }

    // Tell the GIC we are done with this interrupt
    GICC_EOIR = iar;
}