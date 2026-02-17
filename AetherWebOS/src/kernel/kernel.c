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
#include "kernel/health.h"  // For health_check_syn_timeouts()
#include "drivers/ethernet/tcp.h" // For tcp_init_stack()

#include "drivers/virtio/virtio_pci.h"
#include "drivers/virtio/virtio_net.h"

/* =====================================================
   System Modes for UI Arbiter
   Lead Developer: Aritrash Sarkar
   Changes: - Cleaned up v0.1.5 bridge legacy code.
            - Replaced Setup Wizard with Network Dashboard (F10).
            - Native Network Polling active for TCP stack.
    Date: 17.02.2026
   ===================================================== */

// Define the Aether OS Network Identity
uint8_t aether_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56}; 
uint32_t aether_ip = 0x0F02000A;

typedef enum {
    MODE_PORTAL,
    MODE_CONFIRM_SHUTDOWN,
    MODE_NET_STATS
} kernel_mode_t;

static kernel_mode_t current_mode = MODE_PORTAL;

/* =====================================================
   External Symbols
   ===================================================== */
extern void exceptions_init(void);
extern void enable_interrupts(void);
extern char _binary_assets_banner_txt_start[];
extern char _binary_assets_banner_txt_end[];

/* Global VirtIO-Net device (from PCI probe) */
extern struct virtio_pci_device *global_vnet_dev;

/* =====================================================
   Prototypes
   ===================================================== */
void kernel_shutdown(void);

/* =====================================================
   Kernel Entry
   ===================================================== */
void kernel_main(void)
{
    uart_init();
    // ... (Banner rendering code) ...

    /* 2. Core Subsystems */
    exceptions_init();
    mmu_init();
    kmalloc_init();
    pcie_init();
    gic_init();
    timer_init();
    enable_interrupts();
    
    // Task: TCP Stack Initialization
    tcp_init_stack(); 

    /* 3. Portal Start */
    uint64_t last_refresh = 0;
    uint64_t last_health_check = 0; // NEW: Timer for Watchdog
    int esc_state = 0;
    portal_start();

    /* 4. Main Loop */
    while (1) {
        uint64_t current_time = get_system_uptime_ms();

        /* Task: Timeout Watchdog (Runs every 1 second) 
           Identifies connections in SYN_RCVD for too long. */
        if (current_time - last_health_check >= 1000) {
            health_check_syn_timeouts();
            last_health_check = current_time;
        }

        /* UI RENDER (10 FPS) */
        if (current_time - last_refresh >= 100) {
            // ... (Existing UI rendering switch case) ...
            last_refresh = current_time;
        }

        /* INPUT HANDLING */
        // ... (Existing UART input logic) ...

        /* NETWORK POLLING & REAPING */
        if (global_vnet_dev) {
            virtio_net_poll(global_vnet_dev);
            net_tx_reaper(); 
        }

        /* Sleep until next timer tick or packet IRQ */
        asm volatile("wfi");
    }
}

void kernel_shutdown(void)
{
    uart_puts("\033[2J\033[H");
    uart_puts("===========================================\r\n");
    uart_puts("       AETHER WebOS :: SYSTEM HALTING       \r\n");
    uart_puts("===========================================\r\n");

    asm volatile("msr daifset, #2");

    if (global_vnet_dev) {
        virtio_pci_reset(global_vnet_dev);
    }

    uart_puts("[INFO] Aether Core: Shutdown Complete.\r\n");
    psci_system_off();

    while (1) { asm volatile("wfi"); }
}