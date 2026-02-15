#include "uart.h"
#include "utils.h"
#include "gic.h"
#include "timer.h"
#include "mmu.h"
#include "kernel/memory.h" // Standardize this include path
#include "pcie.h"
#include "drivers/usb/xhci.h"

extern void exceptions_init(void);
extern void enable_interrupts(void); // Ensure this is defined in your assembly/utils

// Linker symbols
extern char _binary_assets_banner_txt_start[];
extern char _binary_assets_banner_txt_end[];

void kernel_main() {
    uart_init();

    // 1. Splash & Identity
    char* banner = _binary_assets_banner_txt_start;
    char* banner_end = _binary_assets_banner_txt_end;
    while (banner < banner_end) {
        if (*banner == '\n') uart_putc('\r');
        uart_putc(*banner);
        banner++;
    }

    uart_puts("\r\n[INFO] Kernel Loaded at: ");
    uart_put_hex((uint64_t)&kernel_main);
    uart_puts("\r\n[OK] Aether Core Online.");

    // 2. Fundamental CPU/Arch Setup
    exceptions_init();
    uart_puts("\r\n[OK] Exception Vector Table Loaded.\r\n");

    // 3. Memory Infrastructure (THE CRITICAL ORDER)
    // First, map the world with the MMU
    mmu_init(); 
    uart_puts("[OK] MMU Active (L3 Surgical Mapping Enabled).\r\n");

    // Second, wake up the Heap so pcie_init can allocate memory
    kmalloc_init();
    uart_puts("[OK] Kernel Heap Ready.\r\n");

    // 4. Device Discovery & Drivers
    // Now pcie_init can safely call kmalloc and ioremap
    pcie_init();
    uart_puts("\r\n[OK] PCIe Enumeration & Driver Handshakes Complete.\r\n");

    // 5. Interrupts & System Services
    gic_init();
    timer_init();
    uart_puts("[OK] GIC and Global Timer (1Hz) Online.\r\n");

    enable_interrupts();
    uart_puts("[OK] Interrupts Unmasked (EL1). System is Operational.\r\n");

    uart_puts("[INFO] Aether OS Entering Idle State (WFI)...\r\n");

    while(1) {
        asm volatile("wfi"); 
    }
}