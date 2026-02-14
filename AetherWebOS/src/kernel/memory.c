#include "kernel/memory.h"
#include "mmu.h"
#include "uart.h"
#include "common/utils.h"

/* * Note: PAGE_SIZE and PAGE_MASK are expected to be defined in 
 * kernel/memory.h or mmu.h to avoid redefinition warnings.
 */
#define PAGE_MASK (~(PAGE_SIZE - 1))

/* Bump pointers for our two memory zones */
static uint64_t heap_ptr = HEAP_START;

/* * MUST match the L1 index used in mmu.init() 
 * 0x80000000 corresponds to L1 Index 2.
 */
static uint64_t vmalloc_ptr = 0x80000000; 

/**
 * kmalloc_init
 */
void kmalloc_init() {
    uart_puts("[OK] Memory Subsystem: Online (Heap @ ");
    uart_put_hex(HEAP_START);
    uart_puts(", vmalloc @ 0x80000000).\r\n");
}

/**
 * Standard Physical RAM Allocator (Bump Allocator)
 * Used for kernel structures and virtio rings.
 */
void* kmalloc(size_t size) {
    // 8-byte alignment for AArch64 performance and bus requirements
    size = (size + 7) & ~7;

    if (heap_ptr + size > HEAP_START + HEAP_SIZE) {
        uart_puts("[ERROR] KERNEL PANIC: Physical Heap Exhaustion!\r\n");
        return NULL;
    }

    void* ptr = (void*)heap_ptr;
    heap_ptr += size;

    return ptr;
}

/**
 * vmalloc: Reserves virtual space for device mappings.
 * These addresses do not point to physical RAM until mmu_map_region is called.
 */
void* vmalloc(size_t size) {
    // Round request up to the nearest page
    size = (size + PAGE_SIZE - 1) & PAGE_MASK;
    
    // Check for overflow of the 1GB dynamic segment (up to 0xBFFFFFFF)
    if (vmalloc_ptr + size > 0xBFFFFFFF) {
        uart_puts("[ERROR] KERNEL PANIC: vmalloc Virtual Space Exhaustion!\r\n");
        return NULL;
    }

    void* ptr = (void*)vmalloc_ptr;
    vmalloc_ptr += size;
    
    return ptr;
}

/**
 * virt_to_phys: Converts a kernel virtual address to a physical address.
 * Because Aether OS uses an Identity Map for the RAM segment (0x40000000+),
 * the VA is currently equal to the PA.
 */
uint64_t virt_to_phys(void *vaddr) {
    uintptr_t addr = (uintptr_t)vaddr;

    /* * SAFETY CHECK:
     * Hardware (VirtIO/USB) needs pointers to actual Physical RAM.
     * Addresses in the 0x80000000 range are VIRTUAL ONLY (vmalloc).
     */
    if (addr >= 0x80000000) {
        uart_puts("[ERROR] MEM: Attempted virt_to_phys on vmalloc address (");
        uart_put_hex(addr);
        uart_puts("). This will cause Hardware Faults!\r\n");
        return 0; 
    }

    return (uint64_t)addr;
}

/**
 * Adrija's ioremap: The definitive version with L3 support.
 * Maps hardware MMIO ranges into the Aether OS virtual address space.
 */
void* ioremap(uint64_t phys_addr, size_t size) {
    if (size == 0) return NULL;

    /* Align physical base down to 4KB page boundary */
    uint64_t phys_page = phys_addr & PAGE_MASK;

    /* Offset inside that first 4KB page */
    uint64_t offset = phys_addr & ~PAGE_MASK;

    /* Round total size up to cover all required pages */
    size_t map_size = (offset + size + PAGE_SIZE - 1) & PAGE_MASK;

    /* Request virtual address space in the 0x80000000+ range */
    void *virt = vmalloc(map_size);
    if (!virt) return NULL;

    /* * Map the pages into the L3 tables with Device attributes.
     * PROT_DEVICE_PAGE ensures nGnRnE (No gathering, No reordering, No early write-ack).
     */
    mmu_map_region(
        (uintptr_t)virt,
        phys_page,
        map_size,
        PROT_DEVICE_PAGE
    );

    /* Return the virtual pointer adjusted by the original physical offset */
    return (void *)((uintptr_t)virt + offset);
}