#include "allocator.h"
#include "mini_uart.h"
#include "mmu.h"
#include "utils.h"

unsigned long mmap_addr = 0xFFFF00000000;

void finer_granularity_paging(void) {
    /* Third-level 2MB block mapping and Forth-level 4KB page mapping*/
    unsigned long *pmd_1 = (unsigned long *)ptov(0x2000ul); 
    for (size_t i = 0; i < 512; ++i) {
        unsigned long pte_table = (unsigned long)allocate(PAGE_SIZE);
        pmd_1[i] = vtop(pte_table) | PD_TABLE;
        unsigned long *pte_ptr = (unsigned long *)pte_table;
        if (i < 504) {  // 0x0  ~ 0x3F000000: normal memory
            for (size_t j = 0; j < 512; ++j) {
                unsigned long pte_addr = (j * PAGE_SIZE + i * PMD_GRANULARITY) | PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_PAGE;
                pte_ptr[j] = pte_addr; // map each 2MB block to a 4KB page
            }
        } else {        // 0x3F ~ 0x3FFFFFFF: device memory
            for (size_t j = 0; j < 512; ++j) {
                unsigned long pte_addr = (j * PAGE_SIZE + i * PMD_GRANULARITY) | PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_PAGE;
                pte_ptr[j] = pte_addr; // map each 2MB block to a 4KB page
            }
        }
    }
    
    unsigned long *pmd_2 = (unsigned long *)ptov(0x3000ul);
    for (size_t i = 0; i < 512; ++i) {
        unsigned long pte_table = (unsigned long)allocate(PAGE_SIZE);
        unsigned long *pte_ptr = (unsigned long *)pte_table;
        pmd_2[i] = vtop(pte_table) | PD_TABLE;     // all entries are reserved

        for (size_t j = 0; j < 512; ++j) {
            unsigned long pte_addr = (j * PAGE_SIZE + i * PMD_GRANULARITY + PUD_GRANULARITY) | PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_PAGE;
            pte_ptr[j] = pte_addr; // map each 2MB block to a 4KB page
        }
    }
    unsigned long *pud = (unsigned long *)ptov(0x1000ul); // PUD table
    pud[0] = 0x2000 | PD_TABLE; // 1st 1GB mapped by the 1st entry of PUD
    pud[1] = 0x3000 | PD_TABLE; // 2nd 1GB mapped by the 2nd entry of PUD
}

void mappages(unsigned long *pgd, unsigned long vaddr, unsigned long paddr, unsigned long attr) {
    unsigned long *table = pgd;
    size_t index = 0;
    for (char level = 3; level > 0; --level) {
        index = (vaddr >> (level * 9 + 12)) & 0x1FF; // 9 bits for each level, 12 bits for offset
        if (table[index] == 0) {
            void *page = allocate(PAGE_SIZE); // allocate a new page table
            memset(page, 0, PAGE_SIZE); // clear the page table
            table[index] = vtop((unsigned long)page) | PD_TABLE; // allocate a new page table
        }
        if ((table[index] & 0b11) == PD_TABLE) {
            table = (unsigned long *)ptov((table[index] & 0xFFFFFFFFF000UL)); // move to the next level
        }
    }
    index = (vaddr >> 12) & 0x1FF; // last level index for 4KB page
    table[index] = paddr | attr | PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_PAGE; // 4KB page
}


void setup_thread_peripherals(unsigned long *pgd) {
    for (size_t i = 0x3C000000; i < 0x3F000000; i += PAGE_SIZE) {
        mappages(pgd, i, i, PD_USR_ACCESS);
    }
}