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
#include "drivers/virtio/virtio_net.h"

/* =====================================================
   System Modes for UI Arbiter
   ===================================================== */

typedef enum {
    MODE_PORTAL,
    MODE_CONFIRM_SHUTDOWN,
    MODE_SETUP_WIZARD
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

    /* -------------------------------------------------
       1. Splash Screen
       ------------------------------------------------- */

    char *banner = _binary_assets_banner_txt_start;
    char *banner_end = _binary_assets_banner_txt_end;

    while (banner < banner_end) {
        if (*banner == '\n')
            uart_putc('\r');

        uart_putc(*banner);
        banner++;
    }

    uart_puts("\r\n[OK] Aether Core Online.\r\n");


    /* -------------------------------------------------
       2. Core Subsystems
       ------------------------------------------------- */

    exceptions_init();
    mmu_init();
    kmalloc_init();
    pcie_init();

    gic_init();
    timer_init();

    enable_interrupts();


    /* -------------------------------------------------
       3. Portal
       ------------------------------------------------- */

    uint64_t last_refresh = 0;
    int esc_state = 0;

    portal_start();


    /* -------------------------------------------------
       4. Main Loop
       ------------------------------------------------- */

    while (1) {

        uint64_t current_time = get_system_uptime_ms();


        /* ===============================
           UI RENDER (10 FPS)
           =============================== */

        if (current_time - last_refresh >= 100) {

            if (uart_is_writable()) {

                portal_refresh_state();

                switch (current_mode) {

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


        /* ===============================
           INPUT HANDLING
           =============================== */

        if (!(*UART0_FR & (1 << 4))) {

            unsigned char c = uart_getc();


            /* Escape handling (F-Keys) */

            if (c == 0x1B) {
                esc_state = 1;
                continue;
            }

            if (esc_state == 1 && c == '[') {
                esc_state = 2;
                continue;
            }

            if (esc_state == 2) {

                if (c == '1')
                    esc_state = 3;
                else if (c == '2')
                    esc_state = 10;
                else
                    esc_state = 0;

                continue;
            }


            /* F7 = Shutdown */

            if (esc_state == 3 && c == '8') {

                current_mode = MODE_CONFIRM_SHUTDOWN;
                esc_state = 0;
                continue;
            }


            /* F10 = Setup */

            if (esc_state == 10 && c == '1') {

                current_mode = MODE_SETUP_WIZARD;
                esc_state = 0;
                continue;
            }


            /* Mode Handling */

            if (current_mode == MODE_CONFIRM_SHUTDOWN) {

                if (c == '\r' || c == '\n')
                    kernel_shutdown();

                if (c == 0x1B)
                    current_mode = MODE_PORTAL;
            }

            else if (current_mode == MODE_SETUP_WIZARD) {

                if (c == '\r' || c == '\n') {

                    portal_render_loading();

                    uart_puts("CMD_START_BRIDGE\r\n");

                    current_mode = MODE_PORTAL;
                }

                if (c == 0x1B)
                    current_mode = MODE_PORTAL;

                uart_puts("\033[2J\033[H");
            }

            esc_state = 0;
        }


        /* ===============================
           NETWORK POLLING (IMPORTANT)
           =============================== */

        if (global_vnet_dev) {
            virtio_net_poll(global_vnet_dev);
        }


        /* ===============================
           Sleep until interrupt
           =============================== */

        asm volatile("wfi");
    }
}


/* =====================================================
   Shutdown Handler
   ===================================================== */

void kernel_shutdown(void)
{
    uart_puts("\033[2J\033[H");

    uart_puts("===========================================\r\n");
    uart_puts("       AETHER WebOS :: SYSTEM HALTING       \r\n");
    uart_puts("===========================================\r\n");

    /* Disable IRQs */
    asm volatile("msr daifset, #2");


    /* Reset Network Device */

    if (global_vnet_dev) {
        virtio_pci_reset(global_vnet_dev);
    }


    uart_puts("[INFO] Aether Core: Shutdown Complete.\r\n");


    /* Power off */
    psci_system_off();

    while (1) {
        asm volatile("wfi");
    }
}
