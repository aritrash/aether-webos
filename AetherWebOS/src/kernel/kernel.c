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

// System Modes for UI Arbiter
typedef enum {
    MODE_PORTAL,
    MODE_CONFIRM_SHUTDOWN,
    MODE_SETUP_WIZARD
} kernel_mode_t;

static kernel_mode_t current_mode = MODE_PORTAL;

extern void exceptions_init(void);
extern void enable_interrupts(void); 
extern char _binary_assets_banner_txt_start[];
extern char _binary_assets_banner_txt_end[];

void kernel_shutdown(void);

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

    uart_puts("\r\n[OK] Aether Core Online.\r\n");

    // 2. Fundamental Setup
    exceptions_init();
    mmu_init(); 
    kmalloc_init();
    pcie_init();
    gic_init();
    timer_init();
    enable_interrupts();

    uint64_t last_refresh = 0;
    int esc_state = 0;
    portal_start(); 

    while(1) {
        uint64_t current_time = get_system_uptime_ms();

        // --- 1. UI RENDER ARBITER (10 FPS) ---
        if (current_time - last_refresh >= 100) {
            if (uart_is_writable()) {
                portal_refresh_state();
                
                switch(current_mode) {
                    case MODE_CONFIRM_SHUTDOWN:
                        portal_render_confirm_prompt(); 
                        break;
                    case MODE_SETUP_WIZARD:
                        portal_render_wizard();
                        break;
                    case MODE_PORTAL:
                    default:
                        portal_render_terminal(); 
                        break;
                }
                last_refresh = current_time;
            }
        }

        // --- 2. INPUT HANDLING STATE MACHINE ---
        if (!(*UART0_FR & (1 << 4))) { 
            unsigned char c = uart_getc();

            // Handling Esc Sequences (F-Keys)
            if (c == 0x1B) { esc_state = 1; continue; }
            if (esc_state == 1 && c == '[') { esc_state = 2; continue; }
            
            if (esc_state == 2) {
                // Sequence Detection Logic
                if (c == '1') esc_state = 3; // F7 or F10 prefix
                else if (c == '2') esc_state = 10; // F10 specific prefix [21~
                else esc_state = 0;
                continue;
            }

            // F7 Sequence Detection (Shutdown)
            if (esc_state == 3 && c == '8') {
                current_mode = MODE_CONFIRM_SHUTDOWN;
                esc_state = 0;
                continue;
            }

            // F10 Sequence Detection (Setup Wizard)
            if (esc_state == 10 && c == '1') {
                current_mode = MODE_SETUP_WIZARD;
                esc_state = 0;
                continue;
            }

            // Mode-Specific Key Handling (Enter / Escape)
            if (current_mode == MODE_CONFIRM_SHUTDOWN) {
                if (c == '\r' || c == '\n') kernel_shutdown();
                if (c == 0x1B) current_mode = MODE_PORTAL; 
            } 
            else if (current_mode == MODE_SETUP_WIZARD) {
                if (c == '\r' || c == '\n') {
                    // 1. Show the BIOS-style loading animation
                    portal_render_loading();
                    
                    // 2. Fire the activation string to the Python Bridge
                    uart_puts("CMD_START_BRIDGE\r\n");
                    
                    // 3. Return to the main Portal
                    current_mode = MODE_PORTAL;
                }
                if (c == 0x1B) current_mode = MODE_PORTAL;
                uart_puts("\033[2J\033[H");
            }

            esc_state = 0; // Reset state machine
        }

        asm volatile("wfi"); 
    }
}

void kernel_shutdown() {
    uart_puts("\033[2J\033[H"); 
    uart_puts("===========================================\r\n");
    uart_puts("       AETHER WebOS :: SYSTEM HALTING      \r\n");
    uart_puts("===========================================\r\n");

    asm volatile("msr daifset, #2"); 
    
    if (global_vnet_dev) {
        virtio_pci_reset(global_vnet_dev);
    }

    uart_puts("[INFO] Aether Core: Shutdown Complete.\r\n");
    psci_system_off();
    while(1) { asm volatile("wfi"); }
}