#ifndef MMU_H
#define MMU_H

#include <stdint.h>
#include <stddef.h>

// --- MAIR_EL1 Attribute Encoding ---
#define MT_DEVICE_nGnRnE       0x0
#define MT_NORMAL_NC           0x1
#define MT_NORMAL_WB           0x2

#define MT_ATTR_DEVICE_nGnRnE  0x00
#define MT_ATTR_NORMAL_NC      0x44
#define MT_ATTR_NORMAL_WB      0xFF

// --- Descriptor Definitions ---
#define MM_TYPE_BLOCK          0x1      // L1/L2 Block
#define MM_TYPE_TABLE          0x3      // L1/L2 Table Pointer
#define MM_TYPE_PAGE           0x3      // L3 Page (4KB)
#define MM_ACCESS_FLAG         (1ULL << 10)
#define MM_SHARED              (3ULL << 8)

// --- Attribute Indexing (Lower Attributes [4:2]) ---
#define MM_ATTR_DEVICE_INDEX   (MT_DEVICE_nGnRnE << 2)
#define MM_ATTR_NORMAL_INDEX   (MT_NORMAL_NC     << 2)
#define MM_ATTR_CACHED_INDEX   (MT_NORMAL_WB     << 2)

// --- Flag Combinations (Protection Attributes) ---

// L2 Block Mappings (2MB) - Used for RAM and high-level peripheral ranges
#define PROT_NORMAL            (MM_TYPE_BLOCK | (MT_NORMAL_WB << 2) | MM_ACCESS_FLAG | MM_SHARED)
#define PROT_DEVICE            (MM_TYPE_BLOCK | MM_ATTR_DEVICE_INDEX | MM_ACCESS_FLAG)
#define PROT_NORMAL_CACHED     (MM_TYPE_BLOCK | MM_ATTR_CACHED_INDEX | MM_ACCESS_FLAG | MM_SHARED)

// L3 Page Mappings (4KB) - Used by ioremap for surgical precision
#define PROT_DEVICE_PAGE       (MM_TYPE_PAGE  | MM_ATTR_DEVICE_INDEX | MM_ACCESS_FLAG)
#define PROT_NORMAL_PAGE       (MM_TYPE_PAGE  | MM_ATTR_NORMAL_INDEX | MM_ACCESS_FLAG | MM_SHARED)

// --- Function Prototypes ---
extern void mmu_init();

/**
 * Maps a virtual memory region to a physical memory region.
 * Handles L3 page table entry population.
 */
extern void mmu_map_region(uintptr_t va, uintptr_t pa, size_t size, uint64_t flags);
void clean_cache_range(uintptr_t start, uintptr_t end);

#endif