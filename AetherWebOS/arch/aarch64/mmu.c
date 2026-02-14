#include "mmu.h"
#include "uart.h"
#include "pcie.h"
#include "config.h"

/* * Static Page Tables (4KB granules).
 * Each table is 4KB (512 entries * 8 bytes).
 */
__attribute__((aligned(4096), section(".pgtbl"))) uint64_t l1_table[512];
__attribute__((aligned(4096), section(".pgtbl"))) uint64_t l2_ram[512];
__attribute__((aligned(4096), section(".pgtbl"))) uint64_t l2_periph[512];

/* Dynamic L3 Support (Used for ioremap/vmalloc range) */
__attribute__((aligned(4096), section(".pgtbl"))) uint64_t l2_dynamic[512];
__attribute__((aligned(4096), section(".pgtbl"))) uint64_t l3_table[512];

void mmu_init() {
    uart_puts("[INFO] MMU: Starting configuration...\r\n");

    // 1. Setup MAIR_EL1
    uint64_t mair = (0x00LL << (8 * MT_DEVICE_nGnRnE)) | 
                    (0x44LL << (8 * MT_NORMAL_NC))    |
                    (0xFFLL << (8 * MT_NORMAL_WB)); 
    asm volatile("msr mair_el1, %0" : : "r" (mair));

    // 2. Clear all tables
    for(int i = 0; i < 512; i++) {
        l1_table[i] = 0; l2_ram[i] = 0; l2_periph[i] = 0; 
        l2_dynamic[i] = 0; l3_table[i] = 0;
    }

    // 3. L1 Table Setup
    l1_table[0] = (uint64_t)l2_periph | MM_TYPE_TABLE;
    l1_table[RAM_START >> 30] = (uint64_t)l2_ram | MM_TYPE_TABLE;
    l1_table[2] = (uint64_t)l2_dynamic | MM_TYPE_TABLE; // 0x80000000

    /* --- ECAM Block Mapping Fix ---
     * Instead of a 3rd level table, we map the 2MB block directly in L2.
     * Ending in 0x1 (Block) instead of 0x3 (Table).
     */
    uint32_t pcie_l1_idx = (uint32_t)(PCIE_EXT_CFG_DATA >> 30);
    uint32_t pcie_l2_idx = (uint32_t)((PCIE_EXT_CFG_DATA >> 21) & 0x1FF);
    uint64_t pcie_desc = PCIE_PHYS_ECAM | PROT_DEVICE | (1 << 10);

    if (pcie_l1_idx == 1) {
        l2_ram[pcie_l2_idx] = pcie_desc;
    } else if (pcie_l1_idx == 0) {
        l2_periph[pcie_l2_idx] = pcie_desc;
    }

    // Link L2 Dynamic to L3 Table
    l2_dynamic[0] = (uint64_t)l3_table | MM_TYPE_TABLE;

    // 4. Fill L2 RAM (Identity mapping)
    for (int i = 0; i < 512; i++) {
        uint64_t addr = RAM_START + ((uint64_t)i * (2 * 1024 * 1024));
        if ((RAM_START >> 30) == pcie_l1_idx && i == pcie_l2_idx) continue;
        l2_ram[i] = addr | PROT_NORMAL | (1 << 10); 
    }

    // 5. Fill L2 Peripheral (Including Heap range)
    for (int i = 0; i < 512; i++) {
        uint64_t addr = (uint64_t)i * (2 * 1024 * 1024);
        if (addr < 0x40000000) {
            if (pcie_l1_idx == 0 && i == pcie_l2_idx) continue;
            
            if (addr < (64 * 1024 * 1024)) {
                l2_periph[i] = addr | PROT_NORMAL | (1 << 10);
            } else {
                l2_periph[i] = addr | PROT_DEVICE | (1 << 10);
            }
        }
    }

    // 7. TCR Setup
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
    sctlr |= 0x1;           // MMU ON
    sctlr &= ~(1 << 2);     // D-Cache OFF
    sctlr |= (1 << 12);     // I-Cache ON
    asm volatile("msr sctlr_el1, %0" : : "r" (sctlr));
    asm volatile("isb");

    uart_puts("[OK] MMU ACTIVE (L3 Support: 0x80000000 range enabled).\r\n");
}

void mmu_map_region(uintptr_t va, uintptr_t pa, size_t size, uint64_t flags) {
    uintptr_t v_addr = va;
    uintptr_t p_addr = pa & ~0xFFFULL;
    size_t mapped = 0;

    while (mapped < size) {
        uint32_t l3_idx = (v_addr >> 12) & 0x1FF;
        l3_table[l3_idx] = p_addr | flags | MM_ACCESS_FLAG;
        v_addr += 4096;
        p_addr += 4096;
        mapped += 4096;
    }

    asm volatile("dsb sy");
    asm volatile("tlbi vmalle1is"); 
    asm volatile("dsb sy");
    asm volatile("isb");
}