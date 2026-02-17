#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include "config.h" // Pull HEAP constants from here

void kmalloc_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);
void* vmalloc(size_t size);
void* ioremap(uint64_t phys_addr, size_t size);
uint64_t get_heap_usage(void);

typedef struct mem_header {
    size_t size;
    int is_free;
    struct mem_header *next;
} mem_header_t;

#endif