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
#include "kernel/health.h"      // For health_check_syn_timeouts()
#include "drivers/ethernet/tcp.h" // For tcp_init_stack()
#include "drivers/ethernet/socket.h" // For socket_init()

#include "drivers/virtio/virtio_pci.h"
#include "drivers/virtio/virtio_net.h"

/* =====================================================
   System Modes for UI Arbiter
   Lead Developer: Aritrash Sarkar
   Changes: - Cleaned up v0.1.5 bridge legacy code.
            - Replaced Setup Wizard with Network Dashboard (F10).
            - Native Network Polling active for TCP stack.
            - Integrated Ankana's Health Watchdog & TCP Init.
    Date: 17.02.2026
   ===================================================== */

// Define the Aether OS Network Identity
uint8_t aether_mac[6] = AETHER_MAC_ADDR;
uint32_t aether_ip = AETHER_IP_ADDR; // 10.0.2.15 for QEMU User Mode

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

    /* 1. Splash Screen */
    char *banner = _binary_assets_banner_txt_start;
    char *banner_end = _binary_assets_banner_txt_end;

    while (banner < banner_end) {
        if (*banner == '\n')
            uart_putc('\r');
        uart_putc(*banner);
        banner++;
    }

    uart_puts("\r\n[OK] Aether Core Online.\r\n");

    /* 2. Core Subsystems */
    exceptions_init();
    mmu_init();
    kmalloc_init();
    pcie_init();
    gic_init();
    timer_init();
    enable_interrupts();

    // Initialize Native TCP Stack
    tcp_init_stack(); 
    socket_init();

    /* 3. Portal Start */
    uint64_t last_refresh = 0;
    uint64_t last_health_check = 0; 
    int esc_state = 0;
    portal_start();

    /* 4. Main Loop */
    while (1) {
        uint64_t current_time = get_system_uptime_ms();

        static kernel_mode_t last_mode = -1;

        if (current_time - last_refresh >= 100) {

            portal_refresh_state();

            if (current_mode != last_mode) {
                uart_puts("\033[2J\033[H");
                last_mode = current_mode;

                // Force one full render on mode change
                switch (current_mode) {
                    case MODE_CONFIRM_SHUTDOWN:
                        portal_render_confirm_prompt();
                        break;

                    case MODE_NET_STATS:
                        portal_render_net_dashboard();
                        break;

                    case MODE_PORTAL:
                    default:
                        portal_render_terminal();
                        break;
                }
            }
            else {
                // Only dynamic updates
                switch (current_mode) {
                    case MODE_NET_STATS:
                        portal_render_net_dashboard();
                        break;

                    case MODE_PORTAL:
                        portal_render_terminal();
                        break;

                    default:
                        // Confirm screen: do nothing
                        break;
                }
            }

            last_refresh = current_time;
        }

        /* INPUT HANDLING */
        if (!(*UART0_FR & (1 << 4))) {
            unsigned char c = uart_getc();

            /* Escape handling (F-Keys) */
            if (c == 0x1B) {
                if (uart_is_empty()) { 
                    current_mode = MODE_PORTAL; 
                    esc_state = 0;
                    uart_puts("\033[2J\033[H");
                    continue;
                }
                esc_state = 1;
                continue;
            }
            if (esc_state == 1 && c == '[') {
                esc_state = 2;
                continue;
            }
            if (esc_state == 2) {
                if (c == '1') esc_state = 3;      
                else if (c == '2') esc_state = 10; 
                else esc_state = 0;
                continue;
            }

            /* F8 = Shutdown Mode */
            if (esc_state == 3 && c == '8') {
                uart_puts("\033[2J\033[H");
                current_mode = MODE_CONFIRM_SHUTDOWN;
                esc_state = 0;
                continue;
            }

            /* F10 = Network Dashboard */
            if (esc_state == 10 && c == '1') {
                uart_puts("\033[2J\033[H");
                current_mode = MODE_NET_STATS;
                esc_state = 0;
                continue;
            }

            /* General Mode Interaction */
            if (current_mode == MODE_CONFIRM_SHUTDOWN) {
                if (c == '\r' || c == '\n') kernel_shutdown();
                if (c == 0x1B) current_mode = MODE_PORTAL;
            }
            else if (current_mode == MODE_NET_STATS) {
                if (c == '\r' || c == '\n' || c == 0x1B) {
                    current_mode = MODE_PORTAL;
                    uart_puts("\033[2J\033[H");
                }
            }
            esc_state = 0;
        }

        /* NETWORK POLLING & REAPING */
        if (global_vnet_dev) {
            virtio_net_poll(global_vnet_dev);
            net_tx_reaper(); 
        }

        /* Sleep until next timer tick or packet IRQ */
        //asm volatile("wfi");
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