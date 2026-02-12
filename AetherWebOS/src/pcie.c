#include "pcie.h"
#include "uart.h"
#include "config.h"
#include "utils.h"

// ECAM Formula: base + (bus << 20) | (dev << 15) | (func << 12) | reg
uint32_t pcie_read_config(uint32_t bus, uint32_t dev, uint32_t func, uint32_t reg) {
    uint64_t address = PCIE_EXT_CFG_DATA | 
                       (bus << 20) | 
                       (dev << 15) | 
                       (func << 12) | 
                       (reg & ~3);
    
    return *(volatile uint32_t*)address;
}

void pcie_init() {
    uart_puts("\r\n[INFO] PCIe: Initializing on " 
#ifdef BOARD_VIRT
    "QEMU VIRT"
#else
    "RPi4 (BCM2711)"
#endif
    "...\r\n");

#ifdef BOARD_RPI4
    /* * Broadcom-specific Power-on Sequence. 
     * Required for physical RPi4 but causes crashes on QEMU Virt.
     */
    volatile uint32_t* rgr1_sw_init = (volatile uint32_t*)(PCIE_REG_BASE + 0x9210);
    volatile uint32_t* pcie_status   = (volatile uint32_t*)(PCIE_REG_BASE + 0x4068);

    uart_puts("[DEBUG] RPi4: Accessing Reset Register... ");
    uint32_t reset_val = *rgr1_sw_init;
    uart_put_hex(reset_val);
    uart_puts(" [OK]\r\n");

    uart_puts("[DEBUG] RPi4: Waking up Controller... ");
    *rgr1_sw_init |= (1 << 0);
    for(volatile int i = 0; i < 500000; i++); 
    uart_puts("[OK]\r\n");

    uart_puts("[DEBUG] RPi4: Reading PCIE_STATUS... ");
    uint32_t status = *pcie_status;
    uart_put_hex(status);
    uart_puts((status & (1 << 7)) ? " (LINK UP)\r\n" : " (LINK DOWN)\r\n");

    uart_puts("[DEBUG] RPi4: Reading Revision... ");
    uint32_t rev = *PCIE_RC_REVISION;
    uart_put_hex(rev);
    uart_puts(" [OK]\r\n");
#endif

    // --- ENUMERATION ---
    // This part is standard across most AArch64 systems using ECAM
    uart_puts("[INFO] PCIe: Scanning Bus 0 (ECAM Space)...\r\n");
    
    int found_count = 0;
    for (uint32_t dev = 0; dev < 32; dev++) {
        uint32_t id = pcie_read_config(0, dev, 0, 0);
        
        // Check for valid Vendor ID (not 0xFFFF or 0x0)
        if (id != 0xFFFFFFFF && id != 0) {
            uart_puts("  + Found Device at 00:");
            uart_put_hex(dev);
            uart_puts(" [ID: ");
            uart_put_hex(id);
            uart_puts("] ");

            // Identify known devices
            if (id == 0x34831106) uart_puts("(VIA VL805 USB 3.0)");
            if ((id & 0xFFFF) == 0x1AF4) uart_puts("(VirtIO Device)");
            
            uart_puts("\r\n");
            found_count++;
        }
    }

    if (found_count == 0) {
        uart_puts("[WARN] PCIe: No devices detected on Bus 0.\r\n");
    } else {
        uart_puts("[OK] PCIe: Enumeration complete.\r\n");
    }
}