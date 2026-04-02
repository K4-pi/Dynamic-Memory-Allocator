#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "memory_alloc.h"

#define PAGE_SIZE 4096

static void *get_block(size_t size);
static void *add_block(size_t size);

typedef struct {
  size_t blocks_count;
  size_t pages_count;
} HEAP_HEADER;

typedef struct {
  void *previous;
  void *next;
  size_t size;
  _Bool is_free;
} Block;

void *heap_start = NULL;
void *heap_end = NULL;

#define GET_HEAP_HEADER ((HEAP_HEADER *)heap_start)
#define FIRST_BLOCK ((Block *)((HEAP_HEADER *)heap_start + 1)) // first block after header
#define LAST_BLOCK (((Block *)FIRST_BLOCK)->previous)

#define GET_BLOCK_AT_ADDRESS(addr) (((Block *)addr) - 1)

/**
 * @brief Iterates through all allocated blocks and finds smallest 
 *   	  possible block to allocate memory, if there is no free 
 *   	  calls function to create new block
 *
 * @param size_t size -> size of the block needed to allocate 
 *
 * @return (void *) returns address to block
 */
static void *get_block(size_t size) {
  Block *current = FIRST_BLOCK;

  for (size_t i = 0; i < GET_HEAP_HEADER->blocks_count; i++) {

    if (current->is_free && current->size >= size)
      return current;

    current = current->next;
  }

  return add_block(size);
}

/**
 * @brief Takes the first free address after the last block, 
 *        checks if there is enough memory left on page (if not moves heap break point to fit in new page),
 *        creates new block and returns the address of a newly allocated block
 *
 * @param size_t size -> size of the block needed to allocate 
 *
 * @return (void *) returns address of block
 */
static void *add_block(size_t size) {
  Block *last = LAST_BLOCK;

  void *next_free = (char *)(last + 1) + last->size;

  if ((size_t)(heap_end - next_free) < size + sizeof(Block)) {
    if (sbrk(PAGE_SIZE) == (void *)-1) {
      perror("Add page error");
      return (void *)-1;
    }

    heap_end = (char *)heap_end + PAGE_SIZE;
    GET_HEAP_HEADER->pages_count++;
  }
  void *new_block = next_free;

  ((Block *)new_block)->size = size;
  ((Block *)new_block)->is_free = false;
  ((Block *)new_block)->previous = last;
  ((Block *)new_block)->next = FIRST_BLOCK;

  GET_HEAP_HEADER->blocks_count++;
  FIRST_BLOCK->previous = new_block;

  last->next = new_block;
  return new_block;
}

void *allocate(size_t size) {

  // TODO: Implement simple LOCK

  if (heap_start == NULL) {
    heap_start = sbrk(PAGE_SIZE);
    heap_end = ((char *)heap_start) + PAGE_SIZE;
    GET_HEAP_HEADER->pages_count = 1;

    if (heap_start == (void *)-1) {
      perror("sbrk error");
      heap_start = NULL;
      return (void *)-1;
    }

    HEAP_HEADER *heap_header = heap_start;
    heap_header->blocks_count = 1;
    heap_header->pages_count = 1;

    Block *first_block = (Block *)(heap_header + 1);

    first_block->size = size;
    first_block->is_free = false;
    first_block->previous = first_block;
    first_block->next = first_block;

    return (void *)(first_block + 1);
  }

  Block *block = get_block(size);

  if (block == (void *)-1)
    return (void *)-1;

  return (void *)(block + 1);
}

void free_memory(void *addr) {

  if (!addr) return;

  Block *block_to_free = GET_BLOCK_AT_ADDRESS(addr);

  block_to_free->is_free = true;
}

/* 
 * =================================
 * +     TEMPORARY FOR DEBUGE      +
 * =================================
 */
void is_free(void *addr) {

  if (GET_BLOCK_AT_ADDRESS(addr)->is_free) printf("IS FREE\n");
  else printf("NOT FREE\n");
}

void print_blocks() {
  Block *current = FIRST_BLOCK;

  for (size_t i = 0; i < GET_HEAP_HEADER->blocks_count; i++) {
    printf("| previous -> %p | current -> %p | next ->  %p |\n", current->previous, current, current->next);
    current = (Block *)current->next;
  }
}

void heap_info() {
  printf("\nheader size = %lu\n\n", sizeof(Block));
  printf("Heap start = %p\n", heap_start);
  printf("Heap end   = %p\n", heap_end);
  printf("Heap header size = %lu\n\n", sizeof(HEAP_HEADER));
  printf("first block = %p\n", FIRST_BLOCK);
  printf("last block = %p\n\n", LAST_BLOCK);
  printf("Blocks count = %lu\n", GET_HEAP_HEADER->blocks_count);
  printf("Pages count = %lu\n", GET_HEAP_HEADER->pages_count);
}
