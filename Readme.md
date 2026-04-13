# memory_alloc

A custom heap memory allocator implemented in C, built as a learning project. 

## How it works

The allocator manages a heap using `sbrk()` to request memory from the OS in pages. Allocated blocks are tracked in a **circular doubly linked list**, where each block has a header storing its size, free status, and neighbor pointers.

## Features

- **Best-fit allocation** — finds the smallest free block that fits the request
- **Block splitting** — oversized free blocks are sliced to avoid wasting memory
- **Block merging** — adjacent free blocks are merged on `free_memory()` to reduce fragmentation
- **Mutex lock** — global mutex lock around all allocator operations

## API

```c
void *allocate(size_t size);       // allocate memory
void  free_memory(void *addr);     // free memory
ssize_t size_memory(void *addr);   // get size of allocation
```

## Build

```sh
make
```

## Status

Work in progress. Page deallocation (shrinking the heap when pages are empty) is not yet implemented.
