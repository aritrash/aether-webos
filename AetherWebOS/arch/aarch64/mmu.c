#include "mmu.h"
#include "uart.h"
#include "pcie.h"
#include "config.h"

/**
 * AETHER OS Unified Page Table Structure
 * Groups L1, L2, and L3 tables to prevent linker drift.
 */
typedef struct {
    uint64_t l1[512];
    uint64_t l2_periph[512];
    uint64_t l2_ram[512];
    uint64_t l2_dynamic[512];
    uint64_t l3_pool[4][512];
} kernel_pt_t;

__attribute__((aligned(4096), section(".pgtbl"))) static kernel_pt_t kpt;

/**
 * clean_cache_range:
 * Forces data out of L1/L2 caches into physical RAM.
 */
static void clean_cache_range(uintptr_t start, uintptr_t end) {
    uintptr_t addr = start & ~63ULL; 
    for (; addr < end; addr += 64) {
        asm volatile("dc cvac, %0" : : "r" (addr) : "memory");
    }
    asm volatile("dsb sy; isb");
}

/**
 * mmu_map_region: Surgical mapping for ioremap (e.g., xHCI registers).
 */
void mmu_map_region(uintptr_t va, uintptr_t pa, size_t size, uint64_t flags) {
    uintptr_t v_addr = va;
    uintptr_t p_addr = pa & ~0xFFFULL;
    size_t mapped = 0;

    while (mapped < size) {
        uint32_t l2_idx = (v_addr >> 21) & 0x1FF;
        uint32_t l3_idx = (v_addr >> 12) & 0x1FF;

        // Dynamic pool starts at 0x80000000 (handled by l2_dynamic)
        if (l2_idx < 4) {
            kpt.l3_pool[l2_idx][l3_idx] = p_addr | flags | (1 << 10) | 0x3;
            uintptr_t entry_ptr = (uintptr_t)&kpt.l3_pool[l2_idx][l3_idx];
            clean_cache_range(entry_ptr, entry_ptr + 8);
        }
        v_addr += 4096; p_addr += 4096; mapped += 4096;
    }
    asm volatile("dsb sy; tlbi vmalle1is; dsb sy; isb");
}

void mmu_init() {
    uart_puts("[INFO] MMU: Configuring AETHER OS Memory Map...\r\n");

    // 1. Zero out everything manually
    uint64_t* p = (uint64_t*)&kpt;
    for(int i = 0; i < (sizeof(kernel_pt_t) / 8); i++) {
        asm volatile("str xzr, [%0], #8" : "+r"(p) :: "memory");
    }

    // 2. MAIR Setup: 0=Device-nGnRnE, 1=Normal-NC, 2=Normal-WB
    asm volatile("msr mair_el1, %0" : : "r" (0xFF4400));

    // 3. Link Tables
    kpt.l1[0] = (uintptr_t)kpt.l2_periph | 0x3; // 0GB - 1GB
    kpt.l1[1] = (uintptr_t)kpt.l2_ram    | 0x3; // 1GB - 2GB
    kpt.l1[2] = (uintptr_t)kpt.l2_dynamic | 0x3; // 2GB - 3GB

    // 4. Map Low Peripherals (UART/GIC) - Identity Map 0.0GB - 1.0GB
    for (int i = 0; i < 512; i++) {
        kpt.l2_periph[i] = ((uintptr_t)i << 21) | 0x405 | (0 << 2); 
    }

    // 5. Map RAM & Virtual PCIe ECAM - 1.0GB - 2.0GB (Virtual 0x40000000+)
    for (int i = 0; i < 512; i++) {
        uint64_t virtual_addr = 0x40000000 + ((uintptr_t)i << 21);
        
        // Handle Virtual ECAM Bridge: 0x70000000 -> PCIE_PHYS_BASE
        if (virtual_addr >= PCIE_EXT_CFG_DATA && virtual_addr < (PCIE_EXT_CFG_DATA + 0x08000000)) {
            uint64_t phys_target = PCIE_PHYS_BASE + (virtual_addr - PCIE_EXT_CFG_DATA);
            kpt.l2_ram[i] = phys_target | 0x405 | (0 << 2); // Device nGnRnE
        } else {
            // Identity map standard RAM
            kpt.l2_ram[i] = virtual_addr | 0x401 | (2 << 2); // Normal WB
        }
    }

    // 6. Link dynamic pool entries
    for (int i = 0; i < 4; i++) {
        kpt.l2_dynamic[i] = (uintptr_t)kpt.l3_pool[i] | 0x3;
    }

    // 7. Flush and Enable
    clean_cache_range((uintptr_t)&kpt, (uintptr_t)&kpt + sizeof(kpt));

    // TCR_EL1: 39-bit VA, 4KB granule, Inner Shareable
    uint64_t tcr = (25LL << 0) | (3LL << 10) | (3LL << 12) | (2LL << 32);
    asm volatile("msr tcr_el1, %0" : : "r" (tcr));
    asm volatile("msr ttbr0_el1, %0" : : "r" (&kpt.l1));
    asm volatile("isb");

    // SCTLR_EL1: Enable MMU (M) and Instruction Cache (I)
    uint64_t sctlr;
    asm volatile("mrs %0, sctlr_el1" : "=r" (sctlr));
    sctlr |= 0x1001; 
    asm volatile("msr sctlr_el1, %0" : : "r" (sctlr));
    asm volatile("isb");

    uart_puts("[OK] MMU ACTIVE: Identity & ECAM Bridge Online.\r\n");
}