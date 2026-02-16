#include "drivers/psci.h"
#include "drivers/uart.h"

/**
 * psci_call: The assembly bridge between the Kernel (EL1) and Firmware.
 * Standard ARM calling convention: Function ID in x0, params in x1-x3.
 */
static uint64_t psci_call(uint32_t function_id, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    register uint64_t r0 asm("x0") = function_id;
    register uint64_t r1 asm("x1") = arg1;
    register uint64_t r2 asm("x2") = arg2;
    register uint64_t r3 asm("x3") = arg3;

    asm volatile(
        "hvc #0"  // Hypervisor Call
        : "=r"(r0)
        : "r"(r0), "r"(r1), "r"(r2), "r"(r3)
        : "memory"
    );

    return r0;
}

void psci_system_off(void) {
    uart_puts("[PSCI] Sending SYSTEM_OFF signal...\r\n");
    psci_call(PSCI_SYSTEM_OFF, 0, 0, 0);
    
    // If it returns, something went wrong with the firmware interface
    while(1) { asm volatile("wfi"); }
}

void psci_system_reset(void) {
    uart_puts("[PSCI] Sending SYSTEM_RESET signal...\r\n");
    psci_call(PSCI_SYSTEM_RESET, 0, 0, 0);
    
    while(1) { asm volatile("wfi"); }
}