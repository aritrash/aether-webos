#include "memory.h"
#include "uart.h"
#include "utils.h"

static uint64_t heap_ptr = HEAP_START;

void kmalloc_init() {
    uart_puts("\r\n[OK] Heap Allocator Initialized at 0x");
    uart_put_hex(HEAP_START);
    uart_puts("\r\n");
}

void* kmalloc(size_t size) {
    // Align size to 8 bytes for AArch64 performance
    size = (size + 7) & ~7;

    if (heap_ptr + size > HEAP_START + HEAP_SIZE) {
        uart_puts("[ERROR] KERNEL PANIC: Out of memory!\r\n");
        return NULL;
    }

    void* ptr = (void*)heap_ptr;
    heap_ptr += size;

    return ptr;
}

void kfree(void* ptr) {
    // In a bump allocator, free does nothing.
    // We will upgrade this to a Buddy/Slab allocator once the drivers are up.
}