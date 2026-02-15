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

#endif