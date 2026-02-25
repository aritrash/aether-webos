#include "drivers/uart.h"
#include "common/utils.h"
#include "kernel/gic.h"
#include "kernel/timer.h"
#include "kernel/mmu.h"
#include "kernel/memory.h"
#include "drivers/pcie.h"
#include "drivers/usb/xhci.h"
#include "portal.h"
#include "drivers/psci.h"
#include "kernel/health.h"
#include "drivers/ethernet/tcp/tcp.h"

#include "drivers/virtio/virtio_pci.h"
#include "drivers/virtio/virtio_net.h"
#include "kernel/mode.h"

/* =====================================================
   Aether WebOS Kernel
   ===================================================== */

/* Network Identity (HOST ORDER IP) */
uint8_t  aether_mac[6] = AETHER_MAC_ADDR;
uint32_t aether_ip     = AETHER_IP_ADDR;

/* Make this GLOBAL so uart_debug can see it */
kernel_mode_t current_mode = MODE_PORTAL;

/* =====================================================
   External Symbols
   ===================================================== */

extern void exceptions_init(void);
extern void enable_interrupts(void);

extern char _binary_assets_banner_txt_start[];
extern char _binary_assets_banner_txt_end[];

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

    uint32_t aether_ip = ntohl(AETHER_IP_ADDR);
    /* Splash */
    char *banner     = _binary_assets_banner_txt_start;
    char *banner_end = _binary_assets_banner_txt_end;

    while (banner < banner_end) {
        if (*banner == '\n')
            uart_putc('\r');
        uart_putc(*banner++);
    }

    uart_puts("\r\n[OK] Aether Core Online.\r\n");

    /* Core */
    exceptions_init();
    mmu_init();
    kmalloc_init();
    pcie_init();
    gic_init();
    timer_init();
    enable_interrupts();

    tcp_init();

    /* UI Setup */
    uint64_t last_refresh = 0;
    int esc_state = 0;

    portal_start();

    /* =====================================================
       Main Loop
       ===================================================== */

    while (1) {

        uint64_t current_time = get_system_uptime_ms();
        static kernel_mode_t last_mode = -1;

        /* -------------------------------------------------
           UI REFRESH
           ------------------------------------------------- */

        if (current_time - last_refresh >= 100) {

            portal_refresh_state();

            if (current_mode != last_mode) {

                uart_puts("\033[2J\033[H");
                last_mode = current_mode;

                switch (current_mode) {

                    case MODE_CONFIRM_SHUTDOWN:
                        portal_render_confirm_prompt();
                        break;

                    case MODE_NET_STATS:
                        portal_render_net_dashboard();
                        break;

                    case MODE_DEBUG:
                        uart_puts("===========================================\r\n");
                        uart_puts("           AETHER DEBUG CONSOLE            \r\n");
                        uart_puts("===========================================\r\n\r\n");
                        break;

                    case MODE_PORTAL:
                    default:
                        portal_render_terminal();
                        break;
                }
            }
            else {
                /* IMPORTANT: Do NOT redraw debug screen */
                switch (current_mode) {

                    case MODE_NET_STATS:
                        portal_render_net_dashboard();
                        break;

                    case MODE_DEBUG:
                    default:
                        break;
                }
            }

            last_refresh = current_time;
        }

        /* -------------------------------------------------
           INPUT HANDLING
           ------------------------------------------------- */

        if (!(*UART0_FR & (1 << 4))) {

            unsigned char c = uart_getc();

            if (c == 0x1B) {
                esc_state = 1;
                continue;
            }

            if (esc_state == 1) {
                if (c == '[') {
                    esc_state = 2;
                    continue;
                }
                else if (c == 'O') {
                    esc_state = 20;   /* F1–F4 */
                    continue;
                }
                else {
                    esc_state = 0;
                }
            }

            /* F1–F4 (ESC O P/Q/R/S) */
            if (esc_state == 20) {
                if (c == 'P') {  /* F1 */
                    uart_puts("\033[2J\033[H");
                    current_mode = MODE_DEBUG;
                }
                esc_state = 0;
                continue;
            }

            /* F5+ (ESC [ number ~) */
            if (esc_state == 2) {

                static int num = 0;

                if (c >= '0' && c <= '9') {
                    num = num * 10 + (c - '0');
                    continue;
                }

                if (c == '~') {

                    switch (num) {

                        case 18:   /* F7 */
                            uart_puts("\033[2J\033[H");
                            current_mode = MODE_CONFIRM_SHUTDOWN;
                            break;

                        case 21:   /* F10 */
                            uart_puts("\033[2J\033[H");
                            current_mode = MODE_NET_STATS;
                            break;

                        case 19:   /* F8 */
                            /* Add if needed */
                            break;
                    }

                    num = 0;
                    esc_state = 0;
                    continue;
                }

                esc_state = 0;
            }

            esc_state = 0;
        }

        /* -------------------------------------------------
           NETWORK POLLING
           ------------------------------------------------- */

        if (global_vnet_dev) {
            virtio_net_poll(global_vnet_dev);
            net_tx_reaper();
        }

        health_update_tcp_stats();
    }
}

/* =====================================================
   Shutdown Routine
   ===================================================== */

void kernel_shutdown(void)
{
    uart_puts("\033[2J\033[H");
    uart_puts("===========================================\r\n");
    uart_puts("       AETHER WebOS :: SYSTEM HALTING      \r\n");
    uart_puts("===========================================\r\n");

    asm volatile("msr daifset, #2");

    if (global_vnet_dev)
        virtio_pci_reset(global_vnet_dev);

    uart_puts("[INFO] Shutdown Complete.\r\n");

    psci_system_off();

    while (1)
        asm volatile("wfi");
}