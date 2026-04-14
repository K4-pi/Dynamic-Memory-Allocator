#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <wchar.h>

#include "memory_alloc.h"

#define PAGE_SIZE 4096 

typedef struct {
  size_t blocks_count;
  size_t pages_count;
} HEAP_HEADER;

typedef struct block_header_t {
  struct block_header_t *previous;
  struct block_header_t *next;
  struct content_header_t *content_addr;
  size_t data_size; // data size specified by user
  size_t size;      // Block on heap size
  _Bool is_free;
} block_header_t;

static void *get_block(size_t size);
static void *add_block(size_t size);
static block_header_t *merge_blocks(block_header_t *addr);
static void slice_block(block_header_t *block, size_t size);

static pthread_mutex_t heap_lock = PTHREAD_MUTEX_INITIALIZER;

void *heap_start = NULL;
void *heap_end = NULL;

#define GET_HEAP_HEADER ((HEAP_HEADER *)heap_start)
#define FIRST_BLOCK ((block_header_t *)((HEAP_HEADER *)heap_start + 1)) // first block after header
#define LAST_BLOCK (FIRST_BLOCK->previous)
#define GET_BLOCK_AT_ADDRESS(addr) (((block_header_t *)addr) - 1)


void *allocate(size_t size) {

  pthread_mutex_lock(&heap_lock);

  if (heap_start == NULL) {
    heap_start = sbrk(PAGE_SIZE);
    heap_end = ((char *)heap_start) + PAGE_SIZE;
    GET_HEAP_HEADER->pages_count = 1;

    if (heap_start == (void *)-1) {
      perror("sbrk error");
      heap_start = NULL;
      pthread_mutex_unlock(&heap_lock);
     
      return (void *)-1;
    }

    HEAP_HEADER *heap_header = heap_start;
    heap_header->blocks_count = 1;
    heap_header->pages_count = 1;

    block_header_t *first_block = (block_header_t *)(heap_header + 1);

    first_block->size = size;
    first_block->is_free = false;
    first_block->previous = first_block;
    first_block->next = first_block;
    
    pthread_mutex_unlock(&heap_lock);
    
    return (void *)(first_block + 1);
  }

  block_header_t *block = get_block(size);

  if (block == (void *)-1) {
    pthread_mutex_unlock(&heap_lock);
    return (void *)-1;
  } 

  block->data_size = size;

  pthread_mutex_unlock(&heap_lock);

  return (void *)(block + 1);
}

/**
 * @brief Iterates through all allocated blocks and finds smallest 
 *    	  possible block to allocate memory, if there is no free 
 *     	  calls function to create new block
 *
 * @param size_t size -> size of the block needed to allocate 
 *
 * @return (void *) returns address to block
 */
static void *get_block(size_t size) {
  block_header_t *current = FIRST_BLOCK;
  block_header_t *best = NULL;

  for (size_t i = 0; i < GET_HEAP_HEADER->blocks_count; i++) {

    if (current->is_free && current->size >= size) {
      if (!best || current->size < best->size) {
        best = current; 
      }
    }

    current = current->next;
  }

  if (!best) return add_block(size);
  
  slice_block(best, size);

  best->size = size;        
  best->is_free = false;
                            
  return best;              
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
  block_header_t *last = LAST_BLOCK;

  void *next_free = (char *)(last + 1) + last->size;

  if ((long long)((char *)heap_end - (char *)next_free) < (long long)(size + sizeof(block_header_t))) {
    if (sbrk(PAGE_SIZE) == (void *)-1) {
      perror("Add page error");
      return (void *)-1;
    }

    heap_end = (char *)heap_end + PAGE_SIZE;
    GET_HEAP_HEADER->pages_count++;
  }
  block_header_t *new_block = (block_header_t *)next_free;

  new_block->size = size;
  new_block->is_free = false;
  new_block->previous = last;
  new_block->next = FIRST_BLOCK;

  GET_HEAP_HEADER->blocks_count++;
  FIRST_BLOCK->previous = new_block;

  last->next = new_block;
  return new_block;
}

/**
 * @brief Takes given block and if there is at least sizeof(block_header_t) + 1 bytes
 *        too much it slices block into two and saves it in linked list
 *
 * @param block_header_t  block -> address of a block_header_t to slice
 * @param size_t size  -> size of the block needed to allocate 
 *
 * @return void 
 */
static void slice_block(block_header_t *block, size_t size) {

  if (block->size == size) return;

  // Slice block if there is enough memory for block_header_t header size + at least one byte
  if (block->size - size > sizeof(block_header_t)) {  
   
    block_header_t *sliced_block = (block_header_t *)(((char *)(block + 1)) + size);

    sliced_block->size = block->size - size - sizeof(block_header_t);
    sliced_block->is_free = true;

		block->size = size;

    sliced_block->previous = block;
    sliced_block->next = block->next;

    block->next->previous = sliced_block;
    block->next = sliced_block;

    GET_HEAP_HEADER->blocks_count++;
  }
}

ssize_t size_memory(void *addr) {

  if (!addr) return -1;

  pthread_mutex_lock(&heap_lock);

  block_header_t *block = GET_BLOCK_AT_ADDRESS(addr);

  pthread_mutex_unlock(&heap_lock);

  return block->data_size;
}

void free_memory(void *addr) { 

  if (!addr) return;

  pthread_mutex_lock(&heap_lock);
  
  block_header_t *block_to_free = GET_BLOCK_AT_ADDRESS(addr); // Moves to the header of a allocated data 
  block_to_free->is_free = true;  
  
  merge_blocks(block_to_free);
  
	if (GET_HEAP_HEADER->pages_count > 1) {
		
		void *last_page = (char *)heap_start + (PAGE_SIZE * (GET_HEAP_HEADER->pages_count - 1));

		size_t remove_count = 0;
		bool page_is_free = true;
		block_header_t *current_block = LAST_BLOCK;

		while ((void *)current_block >= last_page) {
			
			if (!current_block->is_free) {
					page_is_free = false;
					break;
			}
			remove_count++;

			current_block = current_block->previous;
		}

		if (page_is_free) {

			FIRST_BLOCK->previous = current_block;
			current_block->next = FIRST_BLOCK;
			GET_HEAP_HEADER->blocks_count -= remove_count;
			GET_HEAP_HEADER->pages_count--;
			brk(last_page);
			heap_end = last_page;
		}
	}

	pthread_mutex_unlock(&heap_lock);
}

/**
 * @brief Checks the free status of neighbor blocks and merges them togheter with 
 *        given block to one block 
 *
 * @param (block_header_t *) header address of block_header_t 
 *
 * @return (block_header_t *) address of merged block_header_t 
 */
static block_header_t *merge_blocks(block_header_t *block) {
  
  block_header_t *merge_block = block;

  // Merge previous
  if (merge_block->previous->is_free && merge_block->previous != LAST_BLOCK) {
    merge_block = merge_block->previous;

    block->next->previous = merge_block;
    merge_block->next = block->next;

    merge_block->size += block->size + sizeof(block_header_t);

    GET_HEAP_HEADER->blocks_count--;
  }

  // Merge next
  block_header_t *next_block = merge_block->next;

  if (next_block->is_free && next_block != FIRST_BLOCK) {
    merge_block->next = next_block->next;
    next_block->next->previous = merge_block;

    merge_block->size += next_block->size + sizeof(block_header_t);
    
    GET_HEAP_HEADER->blocks_count--;
  }

  return merge_block;
}

/* 
 * =================================
 * +     TEMPORARY FOR DEBUG       +
 * =================================
 */

/*
void is_free(void *addr) {

  if (GET_BLOCK_AT_ADDRESS(addr)->is_free) printf("IS FREE\n");
  else printf("NOT FREE\n");
}

void print_blocks() {
  block_header_t *current = FIRST_BLOCK;

  for (size_t i = 0; i < GET_HEAP_HEADER->blocks_count; i++) {

    // Print active blocks
    printf("| previous -> %p | current -> %p | next ->  %p | IS_FREE = %d  BLOCK SIZE = %lu\n", 
            current->previous, current, current->next, current->is_free, current->size);
    
    current = (block_header_t *)current->next;
  }
}

void heap_info() {
  printf("\nBlock header size = %lu\n\n", sizeof(block_header_t));
  printf("Heap start = %p\n", heap_start);
  printf("Heap end   = %p\n", heap_end);
  printf("Heap header size = %lu\n\n", sizeof(HEAP_HEADER));
  printf("first block = %p\n", FIRST_BLOCK);
  printf("last block = %p\n\n", LAST_BLOCK);
  printf("Blocks count = %lu\n", GET_HEAP_HEADER->blocks_count);
  printf("Pages count = %lu\n", GET_HEAP_HEADER->pages_count);
}
*/
