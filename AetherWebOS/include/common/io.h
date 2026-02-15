#ifndef IO_H
#define IO_H

#include <stdint.h>

/**
 * mmio_read32: Strictly aligned 32-bit read.
 * We remove the manual shifting logic because unaligned 
 * access to Device memory triggers a Hardware Exception.
 */
static inline uint32_t mmio_read32(uintptr_t addr) {
    uint32_t val = *(volatile uint32_t*)addr;
    asm volatile("dsb ld" ::: "memory"); // Ensure load completes
    return val;
}

static inline void mmio_write32(uintptr_t addr, uint32_t val) {
    asm volatile("dsb st" ::: "memory"); // Ensure previous stores finish
    *(volatile uint32_t*)addr = val;
    asm volatile("dsb st" ::: "memory"); // Ensure this store finishes
}

static inline uint16_t mmio_read16(uintptr_t addr) {
    uint16_t val = *(volatile uint16_t*)addr;
    asm volatile("dsb ld" ::: "memory");
    return val;
}

#endif