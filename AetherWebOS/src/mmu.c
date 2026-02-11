#include "mmu.h"
#include "uart.h"

/* * We use two L2 tables:
 * l2_ram: Maps 0GB - 1GB (Physical RAM)
 * l2_periph: Maps 3GB - 4GB (Physical Peripherals)
 */
__attribute__((aligned(4096), section(".pgtbl"))) uint64_t l1_table[512];
__attribute__((aligned(4096), section(".pgtbl"))) uint64_t l2_ram[512];
__attribute__((aligned(4096), section(".pgtbl"))) uint64_t l2_periph[512];

void mmu_init() {
    uart_puts("[INFO] MMU: Starting configuration...\r\n");

    // 1. Setup MAIR_EL1 (Memory Attribute Indirection Register)
    // Attr 0: Device, Attr 1: Normal Non-Cacheable
    uint64_t mair = (0x00LL << (8 * MT_DEVICE_nGnRnE)) | 
                    (0x44LL << (8 * MT_NORMAL_NC)); 
    asm volatile("msr mair_el1, %0" : : "r" (mair));

    // 2. Wipe tables to ensure clean slate
    for(int i = 0; i < 512; i++) {
        l1_table[i] = 0;
        l2_ram[i] = 0;
        l2_periph[i] = 0;
    }

    // 3. Setup L1 Table
    // Entry 0 points to RAM L2 table (0-1GB)
    l1_table[0] = (uint64_t)l2_ram | MM_TYPE_TABLE;
    // Entry 3 points to Peripheral L2 table (3-4GB)
    l1_table[3] = (uint64_t)l2_periph | MM_TYPE_TABLE;

    // 4. Fill L2 RAM Table (0 - 1GB)
    for (int i = 0; i < 512; i++) {
        uint64_t addr = (uint64_t)i * (2 * 1024 * 1024);
        // Map as Normal Memory
        l2_ram[i] = addr | PROT_NORMAL;
    }

    // 5. Fill L2 Peripheral Table (3GB - 4GB)
    for (int i = 0; i < 512; i++) {
        // Base address is 3GB (0xC0000000) + offset
        uint64_t addr = 0xC0000000LL + ((uint64_t)i * (2 * 1024 * 1024));
        
        // Pi 4 peripherals are at 0xFE000000. 
        // We mark the peripheral block as Device memory.
        if (addr >= 0xFC000000) { 
            l2_periph[i] = addr | PROT_DEVICE;
        } else {
            l2_periph[i] = addr | PROT_NORMAL;
        }
    }

    // 6. TCR Setup (32-bit/4GB flat mapping)
    // T0SZ=32, TG0=4KB, Inner/Outer WB Cacheable, Inner Shareable
    uint64_t tcr = (32LL << 0)   | // T0SZ
                   (0LL << 14)   | // TG0: 4KB
                   (3LL << 10)   | // IRGN0: WB
                   (3LL << 8)    | // ORGN0: WB
                   (3LL << 12)   | // SH0: Inner Shareable
                   (0LL << 32);    // IPS: 32-bit
    asm volatile("msr tcr_el1, %0" : : "r" (tcr));

    // Ensure table writes are synchronized before the hardware sees them
    asm volatile("dsb sy");
    asm volatile("isb");

    // 7. Set Table Base and Enable
    asm volatile("msr ttbr0_el1, %0" : : "r" (l1_table));
    asm volatile("isb");

    uint64_t sctlr;
    asm volatile("mrs %0, sctlr_el1" : "=r" (sctlr));
    
    sctlr |= 0x1;          // M: Enable MMU
    sctlr &= ~(1 << 2);    // C: Data Cache OFF (to be safe)
    sctlr &= ~(1 << 12);   // I: Instruction Cache OFF (most stable for transition)
    sctlr |= (1 << 11);    // EOS: Consistent exception entry

    asm volatile("dsb sy");
    asm volatile("msr sctlr_el1, %0" : : "r" (sctlr));
    asm volatile("isb");

    // If we make it here, the UART mapping in l2_periph is working!
    uart_puts("[OK] MMU ACTIVE: Successfully switched to Virtual Memory.\r\n");
}