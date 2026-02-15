#include "kernel/memory.h"
#include "mmu.h"
#include "uart.h"
#include "common/utils.h"

/* * Aether OS v0.1.2 :: Memory Management Subsystem
 * Logic by Roheet & Adrija
 */

#define PAGE_MASK (~(PAGE_SIZE - 1))

// Static pointers for Heap (kmalloc) and vmalloc (ioremap)
static uint64_t heap_ptr = HEAP_START;
static uint64_t vmalloc_ptr = 0x80000000; 

// Limits based on mmu.c L3 expansion (4 tables = 8MB)
#define VMALLOC_START 0x80000000
#define VMALLOC_MAX   (VMALLOC_START + (4 * 2 * 1024 * 1024))

void kmalloc_init() {
    uart_puts("[OK] Memory Subsystem: Online (Expanded L3 Support Active).\r\n");
}

/**
 * Standard Heap Allocator
 * Used for kernel structures (vdev, structures, etc.)
 */
void* kmalloc(size_t size) {
    // 8-byte alignment
    size = (size + 7) & ~7;

    if (heap_ptr + size > HEAP_START + HEAP_SIZE) {
        uart_puts("[ERROR] kmalloc: Kernel Heap Exhausted!\r\n");
        return NULL;
    }

    void* ptr = (void*)heap_ptr;
    heap_ptr += size;
    return ptr;
}

/**
 * Virtual Memory Allocator
 * reserves address space in the 0x80000000 range for MMIO
 */
void* vmalloc(size_t size) {
    // Always align allocation to page boundaries
    size = (size + PAGE_SIZE - 1) & PAGE_MASK;
    
    if (vmalloc_ptr + size > VMALLOC_MAX) {
        uart_puts("[ERROR] vmalloc: Out of virtual dynamic space (Pool exhausted)!\r\n");
        return NULL;
    }

    void* ptr = (void*)vmalloc_ptr;
    vmalloc_ptr += size;
    return ptr;
}

/**
 * ioremap: Maps physical hardware registers to virtual space.
 * * NOTE: This function handles the "Alignment Trap." Even if the 
 * phys_addr is not page-aligned, we map the page containing it 
 * and return the pointer with the correct internal offset.
 */
void* ioremap(uint64_t phys_addr, size_t size) {
    if (size == 0) return NULL;

    // 1. Calculate the start of the physical page (4KB boundary)
    uint64_t phys_page = phys_addr & PAGE_MASK;

    // 2. Calculate the offset within that page (e.g., if addr is 0x...1002, offset is 2)
    uint64_t offset = phys_addr & (PAGE_SIZE - 1);

    // 3. Calculate mapping size (must cover the offset + the requested size)
    size_t map_size = (offset + size + PAGE_SIZE - 1) & PAGE_MASK;

    // 4. Reserve virtual space in the dynamic range
    void *virt_page = vmalloc(map_size);
    if (!virt_page) {
        return NULL;
    }

    // 5. Commit the mapping to the MMU Level 3 tables
    // Flags: PROT_DEVICE_PAGE (nGnRnE) + Access Flag
    mmu_map_region((uintptr_t)virt_page, phys_page, map_size, PROT_DEVICE_PAGE);

    // 6. Return the virtual address adjusted by the original offset
    // If virt_page is 0x80001000 and offset is 2, returns 0x80001002
    return (void *)((uintptr_t)virt_page + offset);
}

uint64_t heap_current_ptr = HEAP_START; // The Linker needs this definition!

uint64_t get_heap_usage() {
    return (heap_current_ptr - HEAP_START);
}