#include "uart.h"
#include "utils.h"
#include "gic.h"
#include "config.h"

typedef struct {
    uint64_t x[31]; 
} trap_frame_t;

void handle_sync_exception(uint64_t esr, uint64_t elr, uint64_t far, trap_frame_t *frame) {
    uart_puts("\r\n--- [!!!] KERNEL PANIC: SYNCHRONOUS ABORT [!!!] ---\r\n");

    uint32_t ec = (esr >> 26) & 0x3F;
    uint32_t iss = esr & 0xFFFFFF;

    uart_puts("Condition: ");
    switch(ec) {
        case 0x00: uart_puts("Unknown/Undefined Instruction"); break;
        case 0x21: uart_puts("Instruction Abort (Lower EL)"); break;
        case 0x25: 
            uart_puts("Data Abort (EL1)"); 
            // Decode ISS for Data Abort
            if ((iss & 0x3F) == 0x05) uart_puts(" [Translation Fault Level 1]");
            if ((iss & 0x3F) == 0x06) uart_puts(" [Translation Fault Level 2]");
            if ((iss & 0x3F) == 0x0F) uart_puts(" [Permission Fault Level 3]");
            break;
        case 0x3C: uart_puts("Breakpoint"); break;
        default:   uart_puts("Other Syndrome: "); uart_put_hex(ec);
    }
    
    uart_puts("\r\nESR_EL1: "); uart_put_hex(esr);
    uart_puts("\r\nFAR_EL1: "); uart_put_hex(far);
    uart_puts("\r\nELR_EL1: "); uart_put_hex(elr);
    uart_puts("\r\n");

    uart_puts("\r\n--- General Purpose Registers ---\r\n");
    for (int i = 0; i < 31; i++) {
        // Formatting: "x02: 0x0000000000000000"
        uart_puts("x");
        if (i < 10) uart_puts("0"); // Leading zero for alignment
        uart_put_int(i);
        uart_puts(": ");
        uart_put_hex(frame->x[i]);

        // Print in 2 columns to save screen vertical space but keep it readable
        if ((i + 1) % 2 == 0) uart_puts("\r\n");
        else uart_puts("    ");
    }

    uart_puts("\r\n\r\nLR (x30): "); uart_put_hex(frame->x[30]);
    uart_puts("\r\n--------------------------------------------------\r\n");
    
    while(1);
}

void handle_irq_exception(uint64_t esr, uint64_t elr, uint64_t far, trap_frame_t *frame) {
    uint32_t iar;
#ifdef BOARD_RPI4
    iar = GICC_IAR;
#else
    // GICv3: Acknowledge the interrupt
    asm volatile("mrs %0, ICC_IAR1_EL1" : "=r" (iar));
#endif

    uint32_t irq_id = iar & 0x3FF;

    if (irq_id == TIMER_IRQ_ID) {
        uart_puts("."); // Heartbeat!
        
        uint64_t freq;
        asm volatile ("mrs %0, cntfrq_el0" : "=r" (freq));
        // Reset timer for next 1 second interval
        asm volatile ("msr cntp_tval_el0, %0" : : "r" (freq));
    }

#ifdef BOARD_RPI4
    GICC_EOIR = iar;
#else
    // GICv3: End of Interrupt
    asm volatile("msr ICC_EOIR1_EL1, %0" : : "r" (iar));
#endif
}