#ifndef _MEMORY_ALLOC_H
#define _MEMORY_ALLOC_H

#include <stddef.h>

/**
 * @brief Allocate a block of memory on the heap
 *
 * @param size_t size -> Size of allocated memory in bytes
 *
 * @return On success address of allocated memory, else (void *)-1 on error
 *
 */
void *allocate(size_t size);

/**
 * @brief Free a block of memory from the heap
 *
 * @param void *buffer -> address of allocated memory 
 *
 * @return void 
 *
 */
void free_memory(void *buffer);

void print_blocks();
void heap_info();
void is_free(void *addr);

#endif
