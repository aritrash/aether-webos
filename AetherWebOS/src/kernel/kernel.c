#include "uart.h"
#include "utils.h"
#include "gic.h"
#include "timer.h"
#include "mmu.h"
#include "kernel/memory.h"
#include "pcie.h"
#include "drivers/usb/xhci.h"
#include "portal.h"

// External assembly linkages
extern void exceptions_init(void);
extern void enable_interrupts(void); 

// Binary assets (Banner)
extern char _binary_assets_banner_txt_start[];
extern char _binary_assets_banner_txt_end[];

/**
 * AETHER OS Kernel Entry Point
 */
void kernel_main() {
    uart_init();

    // 1. Splash & Identity (UART Debug Mode)
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

    // 3. Memory Infrastructure
    mmu_init(); 
    uart_puts("[OK] MMU Active (L3 Surgical Mapping Enabled).\r\n");

    kmalloc_init();
    uart_puts("[OK] Kernel Heap Ready.\r\n");

    // 4. Device Discovery & Drivers
    pcie_init();
    uart_puts("\r\n[OK] PCIe Enumeration & Driver Handshakes Complete.\r\n");

    // 5. Interrupts & System Services
    gic_init();
    timer_init();
    uart_puts("[OK] GIC and Global Timer (100Hz) Online.\r\n");

    // Start the hardware heartbeat
    enable_interrupts();
    uart_puts("[OK] Interrupts Unmasked (EL1). System is Operational.\r\n");

    // --- THE HANDOVER ---
    // Keep debug logs visible until the hardware audit is finished.
    uart_puts("\r\n[OK] Boot Sequence Complete. Starting Portal...\r\n");
    
    // Brief delay so the user can actually read the success logs
    for(int i = 0; i < 5000000; i++) asm volatile("nop");

    /* * INITIALIZE THE UI TRACKER 
     * Setting last_refresh to 0 ensures the portal_render_terminal() 
     * triggers immediately on the first pass of the while(1) loop.
     */
    uint64_t last_refresh = 0;
    portal_start(); 

    while(1) {
        uint64_t current_time = get_system_uptime_ms();

        // Refresh the Portal Terminal UI every 100ms (10 FPS)
        if (current_time - last_refresh >= 100) {
            portal_refresh_state();
            portal_render_terminal(); 
            last_refresh = current_time;
        }

        /*
         * Wait For Interrupt (WFI)
         * The CPU will sleep here and only wake up when the 10ms timer tick 
         * or a hardware interrupt (like VirtIO/Keyboard) occurs.
         */
        asm volatile("wfi"); 
    }
}