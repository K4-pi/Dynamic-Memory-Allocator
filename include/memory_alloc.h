#ifndef _MEMORY_ALLOC_H
#define _MEMORY_ALLOC_H

#include <stddef.h>

/**
 * @brief Allocate a block of memory on the heap
 *
 * @param size -> Size of allocated memory in bytes
 *
 * @return On success address of allocated memory, else (void *)-1 on error
 *
 */
void *allocate(size_t size);

void heap_info();

#endif
