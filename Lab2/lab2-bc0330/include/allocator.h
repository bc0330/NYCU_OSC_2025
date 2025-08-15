#ifndef _ALLOCATOR_H_
#define _ALLOCATOR_H_


#define size_t unsigned int 
#define nullptr 0
#define MAX_HEAP_SIZE 0x10000

void *simple_alloc(size_t size);

#endif