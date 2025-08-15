#include "allocator.h"
#include <stddef.h>
#include "devicetree.h"
#include "mini_uart.h"
#include "utils.h"

// page_info_t alloc_array[PAGE_NUM] = {{belongToBuddy, 0, 0, 0}};
page_info_t *alloc_array; // page info array
int free_list[MAX_ORDER + 1] = {-1};
unsigned long chunk_list[MAX_CHUNK_ORDER - MIN_CHUNK_ORDER + 1] = {0}; // free chunk list for order 4 to 8
char initialized = 0;
extern char *__start_code;
extern char *__end_code;

// /*
//     The free list is maintained as doubly linked list of free blocks.
//     The node info are stored directly first and second word of the page.

//         int 1. : points to the next free block, 
//         int 2. : points to the previous free block.
    
//     The free list is indexed by the order of the block size.
//     The free list is initialized to -1, indicating that there are no free blocks of that order.
// */


void buddy_init() {
    uart_send_string("Buddy system initializing...\r\n");

    uart_send_string("Allocating memory for page info array...\r\n");
    alloc_array = (page_info_t *) startup_alloc(PAGE_NUM * sizeof(page_info_t));
    if (alloc_array == nullptr) {
        uart_send_string("Alloc Error: cannot allocate memory for page info array\n");
        return;
    }
    for (int i = 0; i < PAGE_NUM; i++) {
        alloc_array[i].status = belongToBuddy;
        alloc_array[i].page_order = 0;
        alloc_array[i].chunk_order = 0;
        alloc_array[i].chunk_num = 0;
    }
    for (int i = 0; i <= MAX_ORDER; i++) {
        free_list[i] = -1;
    }

    uart_send_string("Reserving memory ...\r\n");
    memory_reserve(0, 4096);                       // spin table
    memory_reserve((unsigned long) &__start_code, (unsigned long) &__end_code + 1);          // kernel
    memory_reserve((unsigned long) rootfs_addr, (unsigned long) rootfs_end);       // initramfs
    memory_reserve((unsigned long) __dtb_addr, (unsigned long) __dtb_end);           // device tree
    memory_reserve((unsigned long) alloc_array, (unsigned long) alloc_array + PAGE_NUM * sizeof(page_info_t)); // page info array

    initialized = 1;

    // Initialize the free list
    uart_send_string("Initializing free list...\r\n");
    int prev = -1;
    for (int i = 0; i < PAGE_NUM; i++) {
        if (alloc_array[i].status != reserved) {
            if (free_list[0] == -1) {
                free_list[0] = i;
            }
            int *cur_block_ptr = (int *) (unsigned long) (ALLOC_BASE + i * PAGE_SIZE);
            *cur_block_ptr = -1;                // clear the next-block pointer
            *(cur_block_ptr + 1) = prev;        // clear the previous-block pointer
            if (prev != -1) {
                int *prev_block_ptr = (int *) (unsigned long) (ALLOC_BASE + prev * PAGE_SIZE);
                *prev_block_ptr = i;         // current block points to the previous block
            }
            prev = i;
            alloc_array[i].status = 0;          // mark the block as free
        }
    }

    // iteratively merge the free blocks into larger blocks
    for (int order = 0; order < MAX_ORDER; order++) {
        uart_send_string("Merging free blocks of order ");
        uart_send_num(order, "dec");
        uart_send_string(" into larger blocks...\r\n");
        for (int i = 0; i < PAGE_NUM; i += power(2, order)) {
            if (alloc_array[i].status == order) {
                int buddy_index = i ^ power(2, order);
                if (alloc_array[buddy_index].status == order) {
                    // Found a free buddy block, merge the two blocks
                    // uart_send_string("Merging blocks of order ");
                    // uart_send_num(order, "dec");
                    // uart_send_string(".\r\n");

                    // removing block 1 from the free list
                    // print_remove_page(i, order);
                    int *cur_block_ptr = (int *) (unsigned long) (ALLOC_BASE + i * PAGE_SIZE);
                    int *next_block_ptr = (int *) (unsigned long) (ALLOC_BASE + *cur_block_ptr * PAGE_SIZE);
                    int *prev_block_ptr = (int *) (unsigned long) (ALLOC_BASE + *(cur_block_ptr + 1) * PAGE_SIZE);
                    if (*(cur_block_ptr + 1) != -1) {
                        *(prev_block_ptr) = *cur_block_ptr; // previous block points to the next block
                    } else {
                        free_list[order] = *cur_block_ptr;   // free list points to the next block
                    }
                    if (*(cur_block_ptr) != -1) {
                        *(next_block_ptr + 1) = *(cur_block_ptr + 1); // next block points to the previous block
                    }
                    
                    // removing block 2 from the free list
                    // print_remove_page(buddy_index, order);
                    int *buddy_block_ptr = (int *) (unsigned long) (ALLOC_BASE + buddy_index * PAGE_SIZE);
                    int *buddy_next_block_ptr = (int *) (unsigned long) (ALLOC_BASE + *buddy_block_ptr * PAGE_SIZE);
                    int *buddy_prev_block_ptr = (int *) (unsigned long) (ALLOC_BASE + *(buddy_block_ptr + 1) * PAGE_SIZE);
                    if (*(buddy_block_ptr + 1)  != -1) {
                        *(buddy_prev_block_ptr) = *buddy_block_ptr; // previous block points to the next block
                    } else {
                        free_list[order] = *buddy_block_ptr;   // free list points to the next block
                    }
                    if (*(buddy_block_ptr)  != -1) {
                        *(buddy_next_block_ptr + 1) = *(buddy_block_ptr + 1); // next block points to the previous block
                    }

                    // the buddy block now belongs to the larger block
                    alloc_array[buddy_index].status = belongToBuddy;
                    alloc_array[i].status = order + 1;
                    
                    // put the merged block into the free list of the larger order
                    int *merged_block_ptr = (int *) (unsigned long) (ALLOC_BASE + i * PAGE_SIZE);
                    int *merged_next_block_ptr = (int *) (unsigned long) (ALLOC_BASE + free_list[order + 1] * PAGE_SIZE);
                    // print_add_page(i, order + 1);
                    *merged_block_ptr = free_list[order + 1];       // first word points to the next free block
                    *(merged_block_ptr + 1) = -1;               // clear the previous-block pointer
                    if (free_list[order + 1] != -1) {
                        *(merged_next_block_ptr + 1) = i; // update the previous-block pointer
                    }
                    free_list[order + 1] = i;  // add the new block to the free list
                }
            }
        }
    }

    // for (int i = 0; i < PAGE_NUM; i += power(2, MAX_ORDER)) {
    //     print_add_page(i, MAX_ORDER);
    //     int *page = (int *) (unsigned long) (ALLOC_BASE + i * PAGE_SIZE);
        
    //     alloc_array[i].status = MAX_ORDER;
    //     *page = i + power(2, MAX_ORDER);          // points to the next free block
    //     *(page + 1) = i - power(2, MAX_ORDER);    // points to the previous free block
    //     if (i == PAGE_NUM - power(2, MAX_ORDER))  // last block has not next block
    //         *page = -1;
    //     if (i == 0)                               // first block has no previous block
    //         *(page + 1) = -1;
        
    // }
    uart_send_string("Succeeded\r\n\r\n");
}

void *allocate(size_t size) {
    if (size > power(2, MAX_ORDER) * PAGE_SIZE) {
        uart_send_string("Alloc Error: cannot allocate more than ");
        uart_send_num(power(2, MAX_ORDER) * PAGE_SIZE, "dec");
        uart_send_string(" bytes\n");
        return nullptr;
    }

    if (size > power(2, MAX_CHUNK_ORDER)) {
        int page_num = size / PAGE_SIZE;
        if (size % PAGE_SIZE != 0) {
            page_num++;
        }
        return page_alloc(page_num);
    } else {
        return chunck_alloc(size);
    }
}

void free(void *addr) {
    if ((unsigned long) addr < ALLOC_BASE || (unsigned long) addr >= ALLOC_END) {
        uart_send_string("Free Error: address out of range\n");
        return;
    }
    unsigned long index = ((unsigned long) addr - ALLOC_BASE) / PAGE_SIZE;
    int status = alloc_array[index].status;
    if (status == allocated) {
        free_page(addr);
    } else if (status == chunked) {
        // uart_send_string("Page is chunked, free the chunk instead\n");
        free_chunk(addr);
    } else {
        uart_send_string("Free Error: memory not allocated\n");
    }
}

void *page_alloc(size_t num_pages) {
    if (num_pages > power(2, MAX_ORDER)) {
        uart_send_string("Alloc Error: cannot allocate more than 32 pages\n");
        return nullptr;
    }

    int order = 0;
    while (num_pages > power(2, order)) {
        order++;
    }
    // uart_send_string("Alloc: requiring a block of order ");
    // uart_send_num(order, "dec");
    // uart_send_string(".\r\n");
    
    // Find the smallest compatible contiguous block
    // block_t *block = nullptr;
    int i;                     // assigned block order
    int block_index = -1;      // assigned block index
    for (i = order; i <= MAX_ORDER; i++) {
        if (free_list[i] >= 0) {
            // uart_send_string("Alloc: found free block of order ");
            // uart_send_num(i, "dec");
            // uart_send_string(".\r\n");
            block_index = free_list[i];
            // print_remove_page(block_index, i);
            int *cur_block_ptr = (int *) (unsigned long) (ALLOC_BASE + block_index * PAGE_SIZE);
            free_list[i] = *cur_block_ptr;  // first word points to the next free block
            *cur_block_ptr = -1;            // clear the next-block pointer
            int *next_block_ptr = (int *) (unsigned long) (ALLOC_BASE + free_list[i] * PAGE_SIZE);
            if (free_list[i] != -1) {
                *(next_block_ptr + 1) = -1; // update the previous-block pointer
            }
            break;
        }
    }
    if (block_index < 0) {
        uart_send_string("Alloc Error: no free block\n");
        return nullptr;
    }

    // Split the block if necessary
    while (i > order) {
        // uart_send_string("Alloc: splitting block of order ");
        // uart_send_num(i, "dec");
        // uart_send_string(" into two blocks of order ");
        // uart_send_num(i - 1, "dec");
        // uart_send_string(".\r\n");
        int split_index = block_index + power(2, i - 1);
        // block_t *new_block = simple_alloc(sizeof(block_t));
        // new_block->index = block->index + power(2, i - 1);
        // uart_send_string("Alloc: new block index ");
        // uart_send_num(split_index, "dec");
        // uart_send_string(".\r\n");

        alloc_array[split_index].status = i - 1;          // mark the new block as free
        int *split_block_ptr = (int *) (unsigned long) (ALLOC_BASE + split_index * PAGE_SIZE);
        int *split_next_block_ptr = (int *) (unsigned long) (ALLOC_BASE + free_list[i - 1] * PAGE_SIZE);
        // print_add_page(split_index, i - 1);
        *split_block_ptr = free_list[i - 1];       // first word points to the next free block
        *(split_block_ptr + 1) = -1;               // clear the previous-block pointer
        if (free_list[i - 1] != -1) {
            *(split_next_block_ptr + 1) = split_index; // update the previous-block pointer
        }
        free_list[i - 1] = split_index;            // add the new block to the free list
        
        i--;
    }

    // Mark the allocated block
    alloc_array[block_index].page_order = order;
    for (int j = block_index; j < block_index + power(2, order); j++) {
        alloc_array[j].status = allocated;
    }

    // Return the block address
    // uart_send_string("Alloc: allocated block index ");
    // uart_send_num(block_index, "dec");
    // uart_send_string(".\r\n");
    void *addr = (void *) (unsigned long) (ALLOC_BASE + block_index * PAGE_SIZE);
    // print_alloc_page((unsigned long) addr, block_index, order, (unsigned long) (ALLOC_BASE + free_list[order] * PAGE_SIZE));
    // uart_send_string("Alloc: block address ");
    // uart_send_num((unsigned long) addr, "hex");
    // uart_send_string(".\r\n");
    
    return addr;
}

void free_page(void *addr) {
    unsigned long index = ((unsigned long) addr - ALLOC_BASE) / PAGE_SIZE;
    int order = alloc_array[index].page_order;
    if (alloc_array[index].status != allocated && alloc_array[index].status != chunked) { 
        uart_send_string("Free Error: block not allocated\n");
        return;
    }
    // uart_send_string("Free: freeing block of order ");
    // uart_send_num(order, "dec");
    // uart_send_string(" at index ");
    // uart_send_num(index, "dec");
    // uart_send_string(".\r\n");


    // Mark the block as free
    for (int j = index + 1; j < index + power(2, order); j++) {
        alloc_array[j].status = belongToBuddy;
    }

    // Coalesce adjacent blocks if possible
    while (order < MAX_ORDER && free_list[order] != -1) {
        unsigned int buddy_index = index ^ power(2, order);
        // uart_send_string("Free: checking for buddy block at index ");
        // uart_send_num(buddy_index, "dec");
        // uart_send_string(".\r\n");

        if (alloc_array[buddy_index].status != order) {
            // uart_send_string("Free: buddy block is not free.\r\n");
            break;
        }

        // uart_send_string("Free: found buddy block at index ");
        // uart_send_num(buddy_index, "dec");
        // uart_send_string(".\r\n");
        // Mark the buddy block as belongToBuddy
        
        
        int *buddy_block_ptr = (int *) (unsigned long) (ALLOC_BASE + buddy_index * PAGE_SIZE);
        int prev_idx = *(buddy_block_ptr + 1);
        int next_idx = *(buddy_block_ptr);
        int *buddy_next_block_ptr = (int *) (unsigned long) (ALLOC_BASE + next_idx * PAGE_SIZE);
        int *buddy_prev_block_ptr = (int *) (unsigned long) (ALLOC_BASE + prev_idx * PAGE_SIZE);
        // remove buddy block from free list
        // print_remove_page(buddy_index, order);
        if (prev_idx == -1) {                      // no previous block
            free_list[order] = next_idx;           // free list points to the next block
        } else {                                   // has previous block
            *(buddy_prev_block_ptr) = next_idx;    // previous block points to the next block
        }

        if (next_idx != -1) {
            *(buddy_next_block_ptr + 1) = prev_idx; // next block points to the previous block
        }
        order++;
        if (buddy_index < index) {
            int temp = index;
            index = buddy_index;
            buddy_index = temp;
        }
        alloc_array[buddy_index].status = belongToBuddy;
    }

    // Add the block to the free list
    alloc_array[index].status = order;
    // block->next = free_list[order];
    int *block_ptr = (int *) (unsigned long) (ALLOC_BASE + index * PAGE_SIZE);
    int *next_block_ptr = (int *) (unsigned long) (ALLOC_BASE + free_list[order] * PAGE_SIZE);
    *block_ptr = free_list[order];
    *(block_ptr + 1) = -1; // clear the previous-block pointer
    if (free_list[order] != -1) {
        *(next_block_ptr + 1) = index; // update the previous-block pointer
    }
    free_list[order] = index;
    // print_free_page((unsigned long) addr, index, order, (unsigned long) (ALLOC_BASE + free_list[order] * PAGE_SIZE));
}

void *chunck_alloc(size_t size) {
    int chunk_order = MIN_CHUNK_ORDER;
    while (size > power(2, chunk_order)) {
        chunk_order++;
    }
    // uart_send_string("Alloc: requiring a chunk of order ");
    // uart_send_num(chunk_order, "dec");
    // uart_send_string(".\r\n");

    // If there is no free chunk of that order, allocate a page and create chunks
    if (chunk_list[chunk_order - 4] == nullptr) {
        // uart_send_string("Alloc: allocating a page to create chuncks of order ");
        // uart_send_num(chunk_order, "dec");
        // uart_send_string(".\r\n");

        void *addr = page_alloc(1);
        int page_index = ((unsigned long) addr - ALLOC_BASE) / PAGE_SIZE;
        int chunk_size = power(2, chunk_order);
        alloc_array[page_index].status = chunked;
        alloc_array[page_index].chunk_order = chunk_order;
        alloc_array[page_index].chunk_num = PAGE_SIZE / chunk_size;
        alloc_array[page_index].page_order = 0;

        for (int i = alloc_array[page_index].chunk_num - 1; i >= 0; i--) {
            unsigned long *chunck_ptr = (unsigned long *) (unsigned long) (ALLOC_BASE + \
               page_index * PAGE_SIZE + i * chunk_size);
            if (i == alloc_array[page_index].chunk_num - 1) {
                *chunck_ptr = chunk_list[chunk_order - 4];       // first word points to the next free chunk
            } else {
                *chunck_ptr = (unsigned long) (ALLOC_BASE + \
                   page_index * PAGE_SIZE + (i + 1) * chunk_size);;
            }
        }      
        chunk_list[chunk_order - 4] = (unsigned long) (ALLOC_BASE + page_index * PAGE_SIZE);
    }

    unsigned long *chunk_ptr = (unsigned long *) chunk_list[chunk_order - 4];
    chunk_list[chunk_order - 4] = *chunk_ptr; //  points to the next free chunk
    int page_index = ((unsigned long) chunk_ptr - ALLOC_BASE) / PAGE_SIZE;
    alloc_array[page_index].chunk_num--;

    // print_alloc_chunk((unsigned long) chunk_ptr, power(2, chunk_order));
    return (void *) (unsigned long) chunk_ptr;
}

void free_chunk(void *addr) {
    int page_index = ((unsigned long) addr - ALLOC_BASE) / PAGE_SIZE;
    int chunk_order = alloc_array[page_index].chunk_order;
    unsigned long *chunk_ptr = (unsigned long *) addr;
    *chunk_ptr = chunk_list[chunk_order - 4]; // first word points to the next free chunk
    chunk_list[chunk_order - 4] = (unsigned long) addr;
    alloc_array[page_index].chunk_num++;
    // print_free_chunk((unsigned long) addr, power(2, chunk_order));
    // if (alloc_array[page_index].chunk_num == PAGE_SIZE / power(2, chunk_order)) {
    //     uart_send_string("Free: freeing page at index ");
    //     uart_send_num(page_index, "dec");
    //     uart_send_string(".\r\n");
    //     free_page((void *) (unsigned long) (ALLOC_BASE + page_index * PAGE_SIZE));
    // }
}

void *startup_alloc(size_t size) {
    // 8-byte align
    void *ptr = (void *) STARTUP_BASE;

   if ((unsigned long) ptr + size > STARTUP_END) {
        uart_send_string("Alloc Error: cannot allocate more than ");
        uart_send_num(STARTUP_END - STARTUP_BASE, "dec");
        uart_send_string(" bytes\n");
        return nullptr;
    }
    return ptr;
}


void print_add_page(int page_index, int order) {
    uart_send_string("[+] Add page ");
    uart_send_num(page_index, "dec");
    uart_send_string(" to free list of order ");
    uart_send_num(order, "dec");
    uart_send_string(".\r\n");
    uart_send_string("Range of pages:[");
    uart_send_num(page_index, "dec");
    uart_send_string(", ");
    uart_send_num(page_index + power(2, order) - 1, "dec");
    uart_send_string("]\r\n");

}

void print_remove_page(int page_index, int order) {
    uart_send_string("[-] Remove page ");
    uart_send_num(page_index, "dec");
    uart_send_string(" from free list of order ");
    uart_send_num(order, "dec");
    uart_send_string(".\r\n");
    uart_send_string("Range of pages:[");
    uart_send_num(page_index, "dec");
    uart_send_string(", ");
    uart_send_num(page_index + power(2, order) - 1, "dec");
    uart_send_string("]\r\n");
}

void print_alloc_page(unsigned long addr, int page_index, int order, unsigned long next_addr) {
    uart_send_string("[P] Allocate 0x");
    uart_send_num(addr, "hex");
    uart_send_string(" of order ");
    uart_send_num(order, "dec");
    uart_send_string(", page ");
    uart_send_num(page_index, "dec");
    uart_send_string(".\r\n");

    uart_send_string("Next address at order ");
    uart_send_num(order, "dec");
    uart_send_string(": 0x");
    uart_send_num(next_addr, "hex");
    uart_send_string(".\r\n");
}

void print_free_page(unsigned long addr, int page_index, int order, unsigned long next_addr) {
    uart_send_string("[P] Free 0x");
    uart_send_num(addr, "hex");
    uart_send_string(" and add back to order ");
    uart_send_num(order, "dec");
    uart_send_string(", page ");
    uart_send_num(page_index, "dec");
    uart_send_string(".\r\n");

    uart_send_string("Next address at order ");
    uart_send_num(order, "dec");
    uart_send_string(": 0x");
    uart_send_num(next_addr, "hex");
    uart_send_string(".\r\n");
}

void print_alloc_chunk(unsigned long addr, size_t size) {
    uart_send_string("[C] Allocate 0x");
    uart_send_num(addr, "hex");
    uart_send_string(" of chunk size ");
    uart_send_num(size, "dec");
    uart_send_string(".\r\n");
}

void print_free_chunk(unsigned long addr, size_t size) {
    uart_send_string("[C] Free 0x");
    uart_send_num(addr, "hex");
    uart_send_string(" of chunk size ");
    uart_send_num(size, "dec");
    uart_send_string(".\r\n");
}

void print_reserve_mem(unsigned long start_addr, unsigned long end_addr, int start_page, int end_page) {
    uart_send_string("[x] Reserve memory [0x");
    uart_send_num(start_addr, "hex");
    uart_send_string(", 0x");
    uart_send_num(end_addr, "hex");
    uart_send_string("].\r\n ");
    uart_send_string("Range of pages:[");
    uart_send_num(start_page, "dec");
    uart_send_string(", ");
    uart_send_num(end_page, "dec");
    uart_send_string("].\r\n");
}

void memory_reserve(unsigned long start_addr, unsigned long end_addr) {
    end_addr--;
    const unsigned long start_index = start_addr / PAGE_SIZE;
    const unsigned long end_index = end_addr / PAGE_SIZE;

    print_reserve_mem(start_addr, end_addr, start_index, end_index);
    if (!initialized) {
        // buddy system is not initialized, only set page info array, and do not touch free list
        for (unsigned long i = start_index; i <= end_index; i++) {
            alloc_array[i].status = reserved;
        }
        return;
    }

    // check if the range is valid
    for (unsigned long i = start_index; i <= end_index; i++) {
        if (alloc_array[i].status == reserved || alloc_array[i].status == allocated || alloc_array[i].status == chunked) {
            uart_send_string("Reserve Error: memory already in use\n");
            return;
        } 
    }

    // determine the order of the block which start_addr is in
    int start_order;  
    int start_block_index = -1;
    for (start_order = 0; start_order <= MAX_ORDER; start_order++) {
        start_block_index = (start_index / power(2, start_order)) * power(2, start_order);  // the index of the starting block which "start" may belong to
        if (alloc_array[start_block_index].status == start_order) {
            break;
        }
    }

    int end_order;  
    int end_block_index = -1;
    for (end_order = 0; end_order <= MAX_ORDER; end_order++) {
        end_block_index = (end_index / power(2, end_order)) * power(2, end_order);  // the index of the starting block which "start" may belong to
        if (alloc_array[end_block_index].status == end_order) {
            break;
        }
    }

    // remove the blocks between start and end blocks from the free list
    for (int i = start_block_index; i <= end_block_index;) {
        int cur_order = alloc_array[i].status;
        print_remove_page(i, cur_order);
        remove_from_list(i, cur_order);
        i += power(2, cur_order);
    }

    for (int i = start_block_index; i < end_block_index + power(2, end_order); i++) {
        alloc_array[i].status = reserved;
    }

}


void remove_from_list(int idx, int order) {
    int *block_ptr = (int *) (unsigned long) (ALLOC_BASE + idx * PAGE_SIZE);
    int *next_block_ptr = (int *) (unsigned long) (ALLOC_BASE + *(block_ptr) * PAGE_SIZE);
    int *prev_block_ptr = (int *) (unsigned long) (ALLOC_BASE + *(block_ptr + 1) * PAGE_SIZE);
    if (*(block_ptr + 1) != -1) {
        *(prev_block_ptr) = *(block_ptr); // previous block points to the next block
    } else {
        free_list[order] = *(block_ptr);  // free list points to the next block
    }
    if (*(block_ptr) != -1) {
        *(next_block_ptr + 1) = *(block_ptr + 1); // next block points to the previous block
    }
}

void add_to_list(int idx, int order) {
    int *block_ptr = (int *) (unsigned long) (ALLOC_BASE + idx * PAGE_SIZE);
    int *next_block_ptr = (int *) (unsigned long) (ALLOC_BASE + free_list[order] * PAGE_SIZE);
    *block_ptr = free_list[order];       // first word points to the next free block
    *(block_ptr + 1) = -1;               // clear the previous-block pointer
    if (free_list[order] != -1) {
        *(next_block_ptr + 1) = idx;     // update the previous-block pointer
    }
    free_list[order] = idx;              // add the new block to the free list
    alloc_array[idx].status = order;     // mark the block as free
}
