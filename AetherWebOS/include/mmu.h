#ifndef MMU_H
#define MMU_H

<<<<<<< HEAD
/* Memory Attribute Indirection Register (MAIR) values */
#define MAIR_DEVICE_nGnRnE 0x00
#define MAIR_NORMAL_NOCACHE 0x44
#define MAIR_VALUE (MAIR_DEVICE_nGnRnE << 0) | (MAIR_NORMAL_NOCACHE << 8)

/* Entry bits */
#define MMU_TABLE_ENTRY     0x3
#define MMU_BLOCK_ENTRY     0x1
#define MMU_ACCESS_FLAG     (1 << 10)
#define MMU_RO              (1 << 7)
#define MMU_MAIR_NORMAL     (1 << 2)
#define MMU_MAIR_DEVICE     (0 << 2)
=======
#include <stdint.h>

// Memory attributes in MAIR_EL1
// MT_DEVICE_nGnRnE: No Gathering, No Reordering, No No-Early-Write-Acknowledgement
#define MT_DEVICE_nGnRnE       0x0
#define MT_NORMAL_NC           0x1

// Attributes for MAIR register
#define MT_ATTR_DEVICE_nGnRnE  0x00
#define MT_ATTR_NORMAL_NC      0x44 // Non-cacheable
#define MT_ATTR_NORMAL_WB      0xFF // Write-back Cacheable (For later use)

// Table/Block descriptors
#define MM_TYPE_BLOCK          0x1
#define MM_TYPE_TABLE          0x3

// Block Entry Attributes
#define MM_ACCESS_FLAG         (1ULL << 10) // Must be 1 to prevent Access Flag fault
#define MM_USER_ACCESS         (1 << 6)
#define MM_READONLY            (1 << 7)
#define MM_SHARED              (3 << 8)  // Inner Shareable

// Memory attributes index (matches MAIR index shifted to bits [4:2] in descriptor)
#define MM_ATTR_DEVICE_INDEX   (0ULL << 2)
#define MM_ATTR_NORMAL_INDEX   (1ULL << 2)

// Flag combinations for Level 2 Blocks
#define PROT_NORMAL            (MM_TYPE_BLOCK | MM_ATTR_NORMAL_INDEX | MM_ACCESS_FLAG | MM_SHARED)
#define PROT_DEVICE            (MM_TYPE_BLOCK | MM_ATTR_DEVICE_INDEX | MM_ACCESS_FLAG)

void mmu_init(void);
>>>>>>> b4434c3 (Update: Enable MMU)

#endif