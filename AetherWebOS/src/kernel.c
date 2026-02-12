#include "uart.h"
#include "utils.h"
#include "gic.h"
#include "timer.h"
#include "mmu.h"
#include "memory.h"
#include "pcie.h"

extern void exceptions_init(void);

// These symbols are generated automatically by the linker
extern char _binary_assets_banner_txt_start[];
extern char _binary_assets_banner_txt_end[];

void kernel_main() {
    uart_init();

    // Calculate length and print the embedded binary blob
    char* banner = _binary_assets_banner_txt_start;
    char* banner_end = _binary_assets_banner_txt_end;

    while (banner < banner_end) {
        // Handle CRLF: If your txt has \n, some serial monitors need \r too
        if (*banner == '\n') uart_putc('\r');
        uart_putc(*banner);
        banner++;
    }

    uart_puts("\r\n[INFO] Kernel Loaded at: ");
    uart_put_hex((uint64_t)&kernel_main); // Verify this matches 0x4008...
    uart_puts("\r\n");
    uart_puts("\r\n[OK] Aether Core Online.");
    uart_puts("\r\n[OK] UART Initialized - 115200 baud, 8N1.");
    uart_puts("\r\n[INFO] Booting on Raspberry Pi 4 (BCM2711)\r\n");

    exceptions_init();
    uart_puts("[OK] Exception Vector Table Loaded.\r\n");
    mmu_init();
    uart_puts("[OK] MMU and Caches Enabled.\r\n");
    uart_puts("[DEBUG] Post-MMU check: Still alive!\r\n");

    pcie_init();
    uart_puts("[OK] PCIe Enumeration Complete.\r\n");
    uint32_t vendor_id = pcie_read_config(0, 0, 0, 0);
    uart_puts("[DEBUG] Bus 0 Dev 0 ID: ");
    uart_put_hex(vendor_id);
    kmalloc_init();
    uart_puts("\r\n[OK] Kernel Heap Initialized.\r\n");
    //Enable the GIC and Timer
    gic_init();
    uart_puts("[OK] GIC Initialized.\r\n");
    timer_init();
    uart_puts("[OK] Timer Initialized (1Hz heartbeat).\r\n");
    enable_interrupts();
    uart_puts("[OK] Interrupts Unmasked (DAIF cleared).\r\n");

    uart_puts("[INFO] System entering Operational Idle Loop...\r\n");

    while(1) {
        // 'wfi' puts the CPU into a low-power standby state 
        // until an interrupt occurs.
        asm volatile("wfi"); 
    }
}