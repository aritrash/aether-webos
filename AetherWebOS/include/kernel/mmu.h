#ifndef MMU_H
#define MMU_H

#include <stdint.h>

// --- MAIR_EL1 Attribute Encoding ---
// Attr 0: Device Memory (nGnRnE - non-Gathering, non-Reordering, no Early Write Ack)
#define MT_DEVICE_nGnRnE       0x0
// Attr 1: Normal Memory, Non-Cacheable
#define MT_NORMAL_NC           0x1
// Attr 2: Normal Memory, Write-Back Cacheable (Used for RAM speedup)
#define MT_NORMAL_WB           0x2

#define MT_ATTR_DEVICE_nGnRnE  0x00
#define MT_ATTR_NORMAL_NC      0x44
#define MT_ATTR_NORMAL_WB      0xFF

// --- Descriptor Definitions ---
#define MM_TYPE_BLOCK          0x1
#define MM_TYPE_TABLE          0x3
#define MM_ACCESS_FLAG         (1ULL << 10)
#define MM_SHARED              (3ULL << 8)

// --- Attribute Indexing (Lower Attributes [4:2]) ---
#define MM_ATTR_DEVICE_INDEX   (MT_DEVICE_nGnRnE << 2)
#define MM_ATTR_NORMAL_INDEX   (MT_NORMAL_NC     << 2)
#define MM_ATTR_CACHED_INDEX   (MT_NORMAL_WB     << 2)

// --- Flag Combinations (Protection Attributes) ---
// Use for RAM without caching (Safe initial state)
#define PROT_NORMAL            (MM_TYPE_BLOCK | MM_ATTR_NORMAL_INDEX | MM_ACCESS_FLAG | MM_SHARED)

// Use for Hardware Peripherals (UART, GPIO, PCIe, GIC)
// nGnRnE ensures that the CPU does not reorder or cache hardware writes/reads.
#define PROT_DEVICE            (MM_TYPE_BLOCK | MM_ATTR_DEVICE_INDEX | MM_ACCESS_FLAG)

// Use for RAM with D-Cache enabled (High performance)
#define PROT_NORMAL_CACHED     (MM_TYPE_BLOCK | MM_ATTR_CACHED_INDEX | MM_ACCESS_FLAG | MM_SHARED)

// --- Function Prototypes ---
void mmu_init(void);

#endif