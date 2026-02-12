#include "mmu.h"
#include "uart.h"
#include "pcie.h"
#include "config.h"

// Page tables must be aligned to 4KB for the hardware MMU to process them
__attribute__((aligned(4096), section(".pgtbl"))) uint64_t l1_table[512];
__attribute__((aligned(4096), section(".pgtbl"))) uint64_t l2_ram[512];
__attribute__((aligned(4096), section(".pgtbl"))) uint64_t l2_periph[512];
__attribute__((aligned(4096), section(".pgtbl"))) uint64_t l2_pcie_cfg[512];

void mmu_init() {
    uart_puts("[INFO] MMU: Starting configuration for " 
#ifdef BOARD_VIRT
    "QEMU VIRT"
#else
    "RPi4 (BCM2711)"
#endif
    "...\r\n");

    // 1. Setup MAIR_EL1 (Memory Attribute Indirection Register)
    // MT_DEVICE_nGnRnE: 0x00 (Device Memory)
    // MT_NORMAL_NC:     0x44 (Normal Memory, Non-Cacheable)
    // MT_NORMAL_WB:     0xFF (Normal Memory, Write-Back Cacheable)
    uint64_t mair = (0x00LL << (8 * MT_DEVICE_nGnRnE)) | 
                    (0x44LL << (8 * MT_NORMAL_NC))    |
                    (0xFFLL << (8 * MT_NORMAL_WB)); 
    asm volatile("msr mair_el1, %0" : : "r" (mair));

    // 2. Wipe tables to ensure no stale data from previous boots
    for(int i = 0; i < 512; i++) {
        l1_table[i] = 0;
        l2_ram[i] = 0;
        l2_periph[i] = 0;
        l2_pcie_cfg[i] = 0;
    }

    // 3. Setup L1 Table (1GB entries)
    // Map RAM (Index 0 for RPi4, Index 1 for VIRT)
    l1_table[RAM_START >> 30] = (uint64_t)l2_ram | MM_TYPE_TABLE;
    
    // Map Peripherals (GIC, UART, etc.)
    l1_table[PERIPHERAL_START >> 30] = (uint64_t)l2_periph | MM_TYPE_TABLE;
    
    // Map PCIe Config Space (ECAM) 
    // Ensuring no collision: On Virt, this uses index 2 (0x80000000)
    l1_table[PCIE_EXT_CFG_DATA >> 30] = (uint64_t)l2_pcie_cfg | MM_TYPE_TABLE;

    // 4. Fill L2 RAM Table (2MB entries)
    for (int i = 0; i < 512; i++) {
        uint64_t addr = RAM_START + ((uint64_t)i * (2 * 1024 * 1024));
        l2_ram[i] = addr | PROT_NORMAL;
    }

    // 5. Fill L2 Peripheral Table
    // On VIRT, GIC Distributor is at 0x08000000 and Redistributors at 0x08100000.
    // We map any 2MB block that contains our defined hardware range.
    for (int i = 0; i < 512; i++) {
        uint64_t addr = (PERIPHERAL_START & ~0x3FFFFFFF) + ((uint64_t)i * (2 * 1024 * 1024));
        
        // Revised check: Does this 2MB block overlap with our peripheral range?
        if (addr + (2 * 1024 * 1024) > PERIPHERAL_START && addr <= PERIPHERAL_END) { 
            l2_periph[i] = addr | PROT_DEVICE;
        } else {
            l2_periph[i] = 0; 
        }
    }

    // 6. Fill L2 PCIe Config Table (ECAM Window)
    for (int i = 0; i < 512; i++) {
        uint64_t phys_addr;
#ifdef BOARD_VIRT
        // Physical ECAM for QEMU 'virt' machine is 0x10000000
        phys_addr = 0x10000000LL;
#else
        // Physical ECAM for BCM2711 is 0x600000000
        phys_addr = 0x600000000LL + ((uint64_t)i * (2 * 1024 * 1024));
#endif
        l2_pcie_cfg[i] = phys_addr | PROT_DEVICE;
    }

    // 7. TCR Setup (Translation Control Register)
    // T0SZ = 25 (39-bit Virtual Address space)
    // IPS = 2 (40-bit Physical Address space)
    uint64_t tcr = (25LL << 0)    | 
                   (2LL << 32)   | 
                   (3LL << 10)   | // Inner Shareable, Write-Back Read-Alloc Write-Alloc Cacheable
                   (3LL << 8)    | // Outer Shareable, Write-Back Read-Alloc Write-Alloc Cacheable
                   (3LL << 12); 
    asm volatile("msr tcr_el1, %0" : : "r" (tcr));

    // 8. Set Table Base and Flush TLB
    asm volatile("msr ttbr0_el1, %0" : : "r" (l1_table));
    asm volatile("dsb sy");
    asm volatile("tlbi vmalle1is"); 
    asm volatile("dsb sy");
    asm volatile("isb");

    // 9. Enable MMU (KEEP CACHES OFF FOR NOW)
    uint64_t sctlr;
    asm volatile("mrs %0, sctlr_el1" : "=r" (sctlr));
    
    sctlr |= 0x1;           // M: Enable MMU
    sctlr &= ~(1 << 2);     // C: Data Cache OFF 
    sctlr |= (1 << 12);     // I: Instruction Cache ON
    sctlr |= (1 << 11);     // EOS (Exception Exit is Context Synchronizing)
    
    asm volatile("msr sctlr_el1, %0" : : "r" (sctlr));
    asm volatile("isb");

    uart_puts("[OK] MMU ACTIVE: Page tables configured for selected BOARD.\r\n");
}