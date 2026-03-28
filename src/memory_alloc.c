#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include "memory_alloc.h"

#ifdef DEBUG
	#define DEBUG_LOG(fmt, ...) printf("[DEBUG] " fmt, ##__VA_ARGS__)
#else 
	#define DEBUG_LOG(fmt, ...) ((void)0)
#endif

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
void *heap_end   = NULL;

#define GET_HEAP_HEADER ((HEAP_HEADER *)(heap_start))
#define FIRST_BLOCK ((Block *)(((HEAP_HEADER *)heap_start) + 1)) // first block after header
#define LAST_BLOCK ((Block *)(FIRST_BLOCK)->previous) 

static void *get_block(size_t size) {
	Block *current = FIRST_BLOCK;

	for (size_t i = 0; i < GET_HEAP_HEADER->blocks_count; i++) {
	
		if (current->is_free && current->size >= size)
			return current;

		current = current->next; 
	}

	return add_block(size);
}

static void *add_block(size_t size) {

	if ((size_t)(heap_end - ((void *)(LAST_BLOCK + 1))) < size + sizeof(Block)) {
		DEBUG_LOG("Not enough memory on page!\n");
		return (void *)-1;
	}

	void *new_block = (((char *)(LAST_BLOCK + 1)) + LAST_BLOCK->size);
	
	((Block *)new_block)->size = size;
	((Block *)new_block)->is_free = false;
	((Block *)new_block)->previous = LAST_BLOCK;
	((Block *)new_block)->next = FIRST_BLOCK;

	GET_HEAP_HEADER->blocks_count++;

	(FIRST_BLOCK)->previous = new_block;

	if (GET_HEAP_HEADER->blocks_count == 1) {
		FIRST_BLOCK->next = new_block;
		return new_block;
	}

	Block *current = FIRST_BLOCK->next;	

	while (current->next != FIRST_BLOCK) {
		current = current->next;
	}

	current->next = new_block;
	return new_block;
}

void free_memory(void *buffer) {
	Block *tmp = (((Block *)buffer) - 1);
	tmp->is_free = true;

	DEBUG_LOG("Freed memory at %p\n", tmp);
}

void *allocate(size_t size) {

	if (heap_start == NULL) {
		heap_start = sbrk(PAGE_SIZE);
		heap_end = sbrk(0);
		GET_HEAP_HEADER->pages_count = 1;

		if (heap_start == (void *)-1) {
			perror("sbrk error");
			heap_start = NULL;
			return (void *)-1;
		}
		
		HEAP_HEADER* heap_header = heap_start;
		heap_header->blocks_count = 1;
		heap_header->pages_count = 1;

		Block *first_block = (Block *)(heap_header + 1);

		first_block->size = size;
		first_block->is_free = false;
		first_block->previous = (void *)first_block;
		first_block->next = (void *)first_block;

		return (void *)(first_block + 1);
	}

	Block *block = get_block(size);

	if (block == (void *)-1) return (void *)-1;

	return (void *)(block + 1); 
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
