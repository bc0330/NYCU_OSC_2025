#ifndef _ALLOCATOR_H_
#define _ALLOCATOR_H_

#include <stddef.h>

#define nullptr 0
#define MAX_HEAP_SIZE  0x10000

#define PAGE_SIZE      0x1000
#define ALLOC_BASE     (unsigned long) 0x00000000
#define ALLOC_END      (unsigned long) 0x3B400000
#define STARTUP_BASE   (unsigned long) 0x10000000
#define STARTUP_END    (unsigned long) 0x20000000
#define PAGE_NUM       ((ALLOC_END - ALLOC_BASE) / PAGE_SIZE)

#define MAX_ORDER     6
#define MAX_CHUNK_ORDER 11
#define MIN_CHUNK_ORDER 4

#define allocated     -1
#define belongToBuddy -2
#define chunked       -3
#define reserved      -4

typedef struct page_info {
    signed char status; // -1: allocated, -2: belong to buddy, >= 0: order of the free block
    char page_order;
    char chunk_order;   // order of the chunk
    short chunk_num;     // number of free chunks in the free block
} page_info_t;

// extern page_info_t alloc_array[PAGE_NUM];
// extern page_info_t alloc_array[PAGE_NUM]; // page info array
// extern int free_list[MAX_ORDER + 1];
// extern unsigned long chunk_list[MAX_CHUNK_ORDER - MIN_CHUNK_ORDER + 1];           // free chunk list for order 4 to 8

void buddy_init();

void *startup_alloc(size_t size);
void *allocate(size_t size);
void free(void *addr);
void *page_alloc(size_t num_pages);
void free_page(void *addr);
void *chunck_alloc(size_t size);
void free_chunk(void *addr);

void print_add_page(int page_index, int order);
void print_remove_page(int page_index, int order);
void print_alloc_page(unsigned long addr, int page_index, int order, unsigned long next_addr);
void print_free_page(unsigned long addr, int page_index, int order, unsigned long next_addr);
void print_alloc_chunk(unsigned long addr, size_t size);
void print_free_chunk(unsigned long addr, size_t s);
void print_reserve_mem(unsigned long start_addr, unsigned long end_addr, int start_page, int end_page);

void memory_reserve(const unsigned long start, const unsigned long end);

void remove_from_list(int idx, int order);
void add_to_list(int idx, int order);

#endif