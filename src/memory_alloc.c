#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include "memory_alloc.h"

#define PAGE_SIZE 4096 

static void *get_block(size_t size);
static void *add_block(size_t size);

typedef struct {
	size_t size;
	void *next;
	_Bool is_free;
} Block; 

void *heap_start = NULL;
void *heap_end   = NULL;
Block *last_block = NULL;

static void *get_block(size_t size) {
	
	Block *current = heap_start;

	while (current->next != NULL && current != heap_end) {
	
		if (current->is_free && current->size >= size)
			return current;

		current = current->next; 
	}

	return add_block(size);
}

static void *add_block(size_t size) {

	if ((size_t)(heap_end - (void *)last_block) < size) {
		printf("Not enough memory on page!\n");
		return (void *)-1;
	}

	Block *new_block = (Block *)((char *)(last_block + 1) + last_block->size);

	// new_block->size = size;
	// new_block->is_free = false;
	// new_block->next = NULL;

	last_block->next = new_block;
	last_block = new_block;

	return new_block;
}

void *allocate(size_t size) {

	if (heap_start == NULL) {
		heap_start = sbrk(PAGE_SIZE);
		heap_end = sbrk(0);

		if (heap_start == (void *)-1) {
			perror("sbrk error");
			heap_start = NULL;
			return (void *)-1;
		}
		
		Block *block = heap_start;
		last_block = heap_start;

		block->size = size;
		block->is_free = false;

		return (void *)(block + 1);
	}

	Block *block = get_block(size);

	if (block == (void *)-1) return (void *)-1;
	
	block->size = size;
	block->is_free = false;

	return (void *)(block + 1); 
}

void heap_info() {
	printf("\nheader size = %lu\n\n", sizeof(Block));
	printf("Heap start = %p\n", heap_start);
	printf("Heap end   = %p\n", heap_end);
	printf("last block = %p\n\n", last_block);
}
