#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "memory_alloc.h"

#define PAGE_SIZE 4096

typedef struct {
  size_t blocks_count;
  size_t pages_count;
} HEAP_HEADER;

typedef struct Block {
  struct Block *previous;
  struct Block *next;
  size_t size;
  _Bool is_free;
} Block;

static void *get_block(size_t size);
static void *add_block(size_t size);
static Block *merge_blocks(Block *addr);

void *heap_start = NULL;
void *heap_end = NULL;

#define GET_HEAP_HEADER ((HEAP_HEADER *)heap_start)
#define FIRST_BLOCK ((Block *)((HEAP_HEADER *)heap_start + 1)) // first block after header
#define LAST_BLOCK (FIRST_BLOCK->previous)

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
  Block *best = NULL;

  for (size_t i = 0; i < GET_HEAP_HEADER->blocks_count; i++) {

    // TODO: slice to smaller blocks if possible
    if (current->is_free && current->size >= size) 
      if (!best || current->size < best->size)
        best = current; 

    current = current->next;
  }

  return best ? best : add_block(size);
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
  Block *new_block = (Block *)next_free;

  new_block->size = size;
  new_block->is_free = false;
  new_block->previous = last;
  new_block->next = FIRST_BLOCK;

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

  if (block == (void *)-1) return (void *)-1;

  return (void *)(block + 1);
}

/**
 * @brief Checks if given address is correct, sets block free status to true,
 *        and merges togheter with neighbor blocks if they free status is true 
 *
 * @param (void *) address of allocated memory 
 *
 * @return void
 */
void free_memory(void *addr) {

  if (!addr) return;

  Block *block_to_free = GET_BLOCK_AT_ADDRESS(addr); // Moves to the header of a allocated data 
  
  block_to_free->is_free = true;
  
  merge_blocks(block_to_free);
}

/**
 * @brief Checks the free status of neighbor blocks and merges them togheter with 
 *        given block to one block 
 *
 * @param (Block *) header address of Block 
 *
 * @return (Block *) address of merged Block 
 */
static Block *merge_blocks(Block *block) {
  
  Block *merge_block = block;

  // Merge previous
  if (merge_block->previous->is_free && merge_block->previous != LAST_BLOCK) {
    merge_block = merge_block->previous;

    block->next->previous = merge_block;
    merge_block->next = block->next;

    merge_block->size += block->size + sizeof(Block);

    GET_HEAP_HEADER->blocks_count--;
  }

  // Merge next
  Block *next_block = merge_block->next;

  if (next_block->is_free && next_block != FIRST_BLOCK) {
    merge_block->next = next_block->next;
    next_block->next->previous = merge_block;

    merge_block->size += next_block->size + sizeof(Block);
    
    GET_HEAP_HEADER->blocks_count--;
  }

  return merge_block;
}

/* 
 * =================================
 * +     TEMPORARY FOR DEBUG       +
 * =================================
 */
void is_free(void *addr) {

  if (GET_BLOCK_AT_ADDRESS(addr)->is_free) printf("IS FREE\n");
  else printf("NOT FREE\n");
}

void print_blocks() {
  Block *current = FIRST_BLOCK;

  for (size_t i = 0; i < GET_HEAP_HEADER->blocks_count; i++) {

    // Print active blocks
    printf("| previous -> %p | current -> %p | next ->  %p | IS_FREE = %d  BLOCK SIZE = %lu\n", 
            current->previous, current, current->next, current->is_free, current->size);
    
    current = (Block *)current->next;
  }
}

void heap_info() {
  printf("\nBlock header size = %lu\n\n", sizeof(Block));
  printf("Heap start = %p\n", heap_start);
  printf("Heap end   = %p\n", heap_end);
  printf("Heap header size = %lu\n\n", sizeof(HEAP_HEADER));
  printf("first block = %p\n", FIRST_BLOCK);
  printf("last block = %p\n\n", LAST_BLOCK);
  printf("Blocks count = %lu\n", GET_HEAP_HEADER->blocks_count);
  printf("Pages count = %lu\n", GET_HEAP_HEADER->pages_count);
}
