#include "uart.h"
#include "utils.h"
#include "gic.h"
#include "timer.h"
#include "mmu.h"
#include "kernel/memory.h"
#include "pcie.h"
#include "drivers/usb/xhci.h"
#include "portal.h"
#include "psci.h"
#include "drivers/virtio/virtio_pci.h"

// External assembly linkages
extern void exceptions_init(void);
extern void enable_interrupts(void); 

// Binary assets (Banner)
extern char _binary_assets_banner_txt_start[];
extern char _binary_assets_banner_txt_end[];

// Forward declaration of the shutdown orchestrator
void kernel_shutdown(void);

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
    uart_puts("\r\n[OK] Boot Sequence Complete. Starting Portal...\r\n");
    
    // Brief delay so the user can read the success logs
    for(int i = 0; i < 5000000; i++) asm volatile("nop");

    /* INITIALIZE THE UI TRACKER */
    uint64_t last_refresh = 0;
    portal_start(); 

    while(1) {
        uint64_t current_time = get_system_uptime_ms();

        // 1. UI Refresh Logic (10 FPS)
        if (current_time - last_refresh >= 100) {
            // Safety: Only render if UART isn't choked
            if (uart_is_writable()) {
                portal_refresh_state();
                portal_render_terminal(); 
                last_refresh = current_time;
            }
        }

        // 2. Input Handling (Interaction Phase)
        // Check if a key was pressed (UART Flag Register bit 4 is RXFE - Receive FIFO Empty)
        // We use the raw register check here to avoid blocking inside uart_getc
        if (!(*UART0_FR & (1 << 4))) { 
            char c = uart_getc();
            if (c == 'q' || c == 'Q') {
                kernel_shutdown();
            }
        }

        /*
         * Wait For Interrupt (WFI)
         * CPU sleeps to save power until the next timer tick or keypress.
         */
        asm volatile("wfi"); 
    }
}

/**
 * kernel_shutdown: Graceful exit sequence
 */
void kernel_shutdown() {
    // 1. Clear Portal and show final status
    uart_puts("\033[2J\033[H"); 
    uart_puts("===========================================\r\n");
    uart_puts("       AETHER WebOS :: SYSTEM HALTING      \r\n");
    uart_puts("===========================================\r\n");

    // 2. Mask interrupts to prevent re-entry
    asm volatile("msr daifset, #2");
    uart_puts("[INFO] Interrupts masked.\r\n");

    // 3. Park the VirtIO Network hardware
    if (global_vnet_dev) {
        virtio_pci_reset(global_vnet_dev);
    }

    // 4. Final log and flush
    uart_puts("[INFO] Aether Core: Goodbye.\r\n");
    
    // Tiny delay for UART FIFO to empty
    for(int i = 0; i < 2000000; i++) asm volatile("nop");

    // 5. Signal PSCI to Power Off
    psci_system_off();

    // Catch-all
    while(1) { asm volatile("wfi"); }
}