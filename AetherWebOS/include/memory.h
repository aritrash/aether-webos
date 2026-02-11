#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096
#define HEAP_START 0x1000000 // Start at 16MB to stay well clear of kernel code
#define HEAP_SIZE  0x1000000 // 16MB of heap for now

void kmalloc_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);

#endif