#include "pcie.h"
#include "uart.h"
#include "config.h"
#include "utils.h"

/**
 * PCIe ECAM Formula: base + (bus << 20) | (dev << 15) | (func << 12) | reg
 * * NOTE: We use volatile uint32_t to ensure 32-bit aligned bus accesses. 
 * Any sub-32-bit access to ECAM can trigger an External Abort on certain controllers.
 */
uint32_t pcie_read_config(uint32_t bus, uint32_t dev, uint32_t func, uint32_t reg) {
    // Ensure the address is within the mapped 2MB/32MB block
    uint64_t address = PCIE_EXT_CFG_DATA | (bus << 20) | (dev << 15) | (func << 12) | (reg & ~3);
    
    asm volatile("dsb sy"); 
    asm volatile("isb");    // Instruction barrier to ensure MMU/Pipeline sync
    
    uint32_t val = *(volatile uint32_t*)address;
    
    asm volatile("dsb sy");
    return val;
}

void pcie_init() {
    uart_puts("\r\n[INFO] PCIe: Initializing Aether OS PCIe Subsystem...\r\n");

#ifdef BOARD_VIRT
    uart_puts("[DEBUG] Platform: QEMU VIRT (Target ECAM: 0x3f000000)\r\n");
#else
    uart_puts("[DEBUG] Platform: RPi4 BCM2711\r\n");
    /* Broadcom-specific Power-on Sequence */
    volatile uint32_t* rgr1_sw_init = (volatile uint32_t*)(PCIE_REG_BASE + 0x9210);
    volatile uint32_t* pcie_status   = (volatile uint32_t*)(PCIE_REG_BASE + 0x4068);

    uart_puts("[DEBUG] RPi4: Waking up Controller...\r\n");
    *rgr1_sw_init |= (1 << 0);
    for(volatile int i = 0; i < 500000; i++); 

    if (!(*pcie_status & (1 << 7))) {
        uart_puts("[WARN] RPi4: PCIe Link is DOWN. Check power/firmware.\r\n");
    }
#endif

    // --- ENUMERATION ---
    uart_puts("[INFO] PCIe: Scanning Bus 0...\r\n");
    
    int found_count = 0;
    for (uint32_t dev = 0; dev < 32; dev++) {
        for (uint32_t func = 0; func < 8; func++) {
            // Read Vendor/Device ID
            uint32_t id = pcie_read_config(0, dev, func, 0);
            
            // 0xFFFFFFFF = No device; 0x00000000 = Master abort/Uninitialized
            if (id == 0xFFFFFFFF || id == 0x00000000 || id == 0xFFFF0000) {
                if (func == 0) break; 
                continue;
            }

            found_count++;
            uart_puts("  + Found 00:");
            uart_put_int(dev);
            uart_puts(".");
            uart_put_int(func);
            uart_puts(" [ID: ");
            uart_put_hex(id);
            uart_puts("] ");

            uint16_t vendor = id & 0xFFFF;
            uint16_t device_id = (id >> 16) & 0xFFFF;

            if (vendor == 0x1AF4) {
                uart_puts("(VirtIO: ");
                // Modern VirtIO IDs are 0x1040+, Transitional are 0x1000-0x103F
                if (device_id == 0x1000 || device_id == 0x1041) uart_puts("Net)");
                else if (device_id == 0x1001 || device_id == 0x1042) uart_puts("Block)");
                else uart_puts("Other)");
            } else if (vendor == 0x1B36) {
                uart_puts("(QEMU PCI Bridge)");
            }

            uart_puts("\r\n");

            // Multi-Function Check: Header Type (Offset 0x0C), Bit 7 of the byte
            if (func == 0) {
                uint32_t header = pcie_read_config(0, dev, 0, 0x0C);
                if (!(header & (1 << 23))) break; 
            }
        }
    }

    if (found_count == 0) {
        uart_puts("[WARN] PCIe: Bus scan returned no devices.\r\n");
    } else {
        uart_puts("[OK] PCIe: Scan complete.\r\n");
    }
}