#ifndef MMU_H
#define MMU_H

#include <stdint.h>

// Memory attributes in MAIR_EL1
#define MT_DEVICE_nGnRnE       0x0
#define MT_NORMAL_NC           0x1
#define MT_NORMAL_WB           0x2

#define MT_ATTR_DEVICE_nGnRnE  0x00
#define MT_ATTR_NORMAL_NC      0x44
#define MT_ATTR_NORMAL_WB      0xFF

// Table/Block descriptors
#define MM_TYPE_BLOCK          0x1
#define MM_TYPE_TABLE          0x3
#define MM_ACCESS_FLAG         (1ULL << 10)
#define MM_SHARED              (3ULL << 8)

// Memory attributes index (matches MAIR index shifted to bits [4:2])
#define MM_ATTR_DEVICE_INDEX   (MT_DEVICE_nGnRnE << 2)
#define MM_ATTR_NORMAL_INDEX   (MT_NORMAL_NC     << 2)
#define MM_ATTR_CACHED_INDEX   (MT_NORMAL_WB     << 2)

// Flag combinations
#define PROT_NORMAL            (MM_TYPE_BLOCK | MM_ATTR_NORMAL_INDEX | MM_ACCESS_FLAG | MM_SHARED)
#define PROT_DEVICE            (MM_TYPE_BLOCK | MM_ATTR_DEVICE_INDEX | MM_ACCESS_FLAG)
#define PROT_NORMAL_CACHED     (MM_TYPE_BLOCK | MM_ATTR_CACHED_INDEX | MM_ACCESS_FLAG | MM_SHARED)

void mmu_init(void);

#endif