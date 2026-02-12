#include "mmu.h"
#include "uart.h"
#include "pcie.h"
#include "config.h"

__attribute__((aligned(4096), section(".pgtbl"))) uint64_t l1_table[512];
__attribute__((aligned(4096), section(".pgtbl"))) uint64_t l2_ram[512];
__attribute__((aligned(4096), section(".pgtbl"))) uint64_t l2_periph[512];
__attribute__((aligned(4096), section(".pgtbl"))) uint64_t l2_pcie_cfg[512];

void mmu_init() {
    uart_puts("[INFO] MMU: Starting configuration...\r\n");

    // 1. Setup MAIR_EL1
    // Attr 0: Device nGnRnE, Attr 1: Normal Non-Cacheable, Attr 2: Normal Write-Back
    uint64_t mair = (0x00LL << (8 * MT_DEVICE_nGnRnE)) | 
                    (0x44LL << (8 * MT_NORMAL_NC))    |
                    (0xFFLL << (8 * MT_NORMAL_WB)); 
    asm volatile("msr mair_el1, %0" : : "r" (mair));

    // 2. Clear tables
    for(int i = 0; i < 512; i++) {
        l1_table[i] = 0; l2_ram[i] = 0; l2_periph[i] = 0; l2_pcie_cfg[i] = 0;
    }

    // 3. L1 Mapping
    l1_table[RAM_START >> 30] = (uint64_t)l2_ram | MM_TYPE_TABLE;
    l1_table[PERIPHERAL_START >> 30] = (uint64_t)l2_periph | MM_TYPE_TABLE;
    l1_table[PCIE_EXT_CFG_DATA >> 30] = (uint64_t)l2_pcie_cfg | MM_TYPE_TABLE;

    // 4. Fill L2 RAM
    for (int i = 0; i < 512; i++) {
        uint64_t addr = RAM_START + ((uint64_t)i * (2 * 1024 * 1024));
        l2_ram[i] = addr | PROT_NORMAL | (1 << 10); 
    }

    // 5. Fill L2 Peripheral
    for (int i = 0; i < 512; i++) {
        uint64_t addr = (PERIPHERAL_START & ~0x3FFFFFFF) + ((uint64_t)i * (2 * 1024 * 1024));
        if (addr + (2 * 1024 * 1024) > PERIPHERAL_START && addr <= PERIPHERAL_END) { 
            l2_periph[i] = addr | PROT_DEVICE | (1 << 10);
        }
    }

    // 6. Fill L2 PCIe Config (Correcting the attributes)
    for (int i = 0; i < 512; i++) {
        uint64_t phys_addr = PCIE_PHYS_ECAM + ((uint64_t)i * (2 * 1024 * 1024));
        
        // Use PROT_DEVICE for ECAM to prevent speculative reads/reordering
        l2_pcie_cfg[i] = phys_addr | PROT_DEVICE | (1 << 10); 
    }

    // 7. TCR Setup (39-bit VA, 40-bit PA)
    uint64_t tcr = (25LL << 0) | (2LL << 32) | (3LL << 10) | (3LL << 8) | (3LL << 12);
    asm volatile("msr tcr_el1, %0" : : "r" (tcr));

    // 8. Set TTBR0 and Flush
    asm volatile("msr ttbr0_el1, %0" : : "r" (l1_table));
    asm volatile("dsb sy");
    asm volatile("tlbi vmalle1is"); 
    asm volatile("dsb sy");
    asm volatile("isb");

    // 9. Enable MMU
    uint64_t sctlr;
    asm volatile("mrs %0, sctlr_el1" : "=r" (sctlr));
    sctlr |= 0x1;           // Enable MMU
    sctlr &= ~(1 << 2);     // Data Cache OFF
    sctlr |= (1 << 12);     // Instruction Cache ON
    asm volatile("msr sctlr_el1, %0" : : "r" (sctlr));
    asm volatile("isb");

    uart_puts("[OK] MMU ACTIVE.\r\n");
}