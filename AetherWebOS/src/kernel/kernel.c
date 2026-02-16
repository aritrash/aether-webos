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

// Forward declaration
void kernel_shutdown(void);

/**
 * AETHER OS Kernel Entry Point
 */
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

    // 2. Fundamental Setup
    exceptions_init();
    mmu_init(); 
    kmalloc_init();
    pcie_init();
    gic_init();
    timer_init();
    enable_interrupts();

    uart_puts("\r\n[OK] Boot Sequence Complete. Starting Portal...\r\n");
    for(int i = 0; i < 5000000; i++) asm volatile("nop");

    /* INITIALIZE THE UI TRACKER */
    uint64_t last_refresh = 0;
    int esc_state = 0;
    int waiting_for_confirm = 0;
    portal_start(); 

    while(1) {
        uint64_t current_time = get_system_uptime_ms();

        // 1. UI Refresh Logic (10 FPS)
        if (current_time - last_refresh >= 100) {
            if (uart_is_writable()) {
                portal_refresh_state();
                
                if (waiting_for_confirm) {
                    // Overlays a "Confirm Shutdown?" box over the portal
                    portal_render_confirm_prompt(); 
                } else {
                    portal_render_terminal(); 
                }
                last_refresh = current_time;
            }
        }

        // 2. Advanced Input Handling (Sequence Detection)
        if (!(*UART0_FR & (1 << 4))) { 
            unsigned char c = uart_getc();

            // Case A: User is currently at the Confirmation Prompt
            if (waiting_for_confirm) {
                if (c == '\r' || c == '\n') kernel_shutdown(); // Confirmed
                if (c == 0x1B) waiting_for_confirm = 0;        // Cancel (ESC)
                continue;
            }

            // Case B: Parsing Escape Sequences for F7 / Ctrl+Alt+F7
            // Typical F7 Sequence: ESC [ 1 8 ~
            // Typical C+A+F7 Sequence: ESC [ 1 8 ; 7 ~
            if (c == 0x1B) {
                esc_state = 1; // Start of sequence
            } else if (esc_state == 1 && c == '[') {
                esc_state = 2;
            } else if (esc_state == 2 && c == '1') {
                esc_state = 3;
            } else if (esc_state == 3 && c == '8') {
                esc_state = 4;
            } else if (esc_state == 4) {
                if (c == '~') {
                    waiting_for_confirm = 1; // Trigger "Soft" Shutdown Prompt
                    esc_state = 0;
                } else if (c == ';') {
                    esc_state = 5; // Possible modifier coming up
                } else {
                    esc_state = 0;
                }
            } else if (esc_state == 5 && c == '7') {
                esc_state = 6; // '7' denotes Ctrl+Alt modifier in many terminals
            } else if (esc_state == 6 && c == '~') {
                kernel_shutdown(); // Trigger "Hard" Absolute Shutdown
            } else {
                esc_state = 0; // Reset state machine on mismatch
            }
        }

        /* Wait For Interrupt */
        asm volatile("wfi"); 
    }
}

/**
 * kernel_shutdown: Graceful exit sequence
 */
void kernel_shutdown() {
    uart_puts("\033[2J\033[H"); 
    uart_puts("===========================================\r\n");
    uart_puts("       AETHER WebOS :: SYSTEM HALTING      \r\n");
    uart_puts("===========================================\r\n");

    asm volatile("msr daifset, #2"); // Mask IRQs
    
    if (global_vnet_dev) {
        virtio_pci_reset(global_vnet_dev);
    }

    uart_puts("[INFO] Aether Core: Goodbye, Aritrash.\r\n");
    for(int i = 0; i < 2000000; i++) asm volatile("nop");

    psci_system_off();
    while(1) { asm volatile("wfi"); }
}