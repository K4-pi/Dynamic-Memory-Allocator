#ifndef _MEMORY_ALLOC_H
#define _MEMORY_ALLOC_H

#include <unistd.h>
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
 * @brief returns size of the allocated memory by user 
 *
 * @param void *addr -> address of allocated memory 
 *
 * @return ssize_t, -1 on uncorrect address, on success returns size of allocated memory from user
 *
 */
ssize_t size_memory(void *addr);

/**
 * @brief Free a block of memory from the heap
 *
 * @param void *addr -> address of allocated memory 
 *
 * @return void 
 *
 */
void free_memory(void *addr);

void print_blocks();
void heap_info();
void is_free(void *addr);

#endif
