#ifndef __MMU_H__
#define __MMU_H__

#include <stddef.h>

#define KERNEL_VIRTUAL_BASE 0xFFFF000000000000UL

#define PAGE_SIZE           0x1000       // 4KB
#define PMD_GRANULARITY     0x200000     // 2MB
#define PUD_GRANULARITY     0x40000000   // 1GB
#define PGD_GRANULARITY     0x8000000000 // 512GB

#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16))
#define TCR_CONFIG_4KB ((0b00 << 14) |  (0b10 << 30))
#define TCR_CONFIG_DEFAULT (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

#define MAIR_DEVICE_nGnRnE 0b00000000
#define MAIR_NORMAL_NOCACHE 0b01000100
#define MAIR_IDX_DEVICE_nGnRnE 0
#define MAIR_IDX_NORMAL_NOCACHE 1

#define PD_TABLE       0b11
#define PD_BLOCK       0b01
#define PD_PAGE        0b11
#define PD_RONLY      (1 << 7)
#define PD_USR_ACCESS (1 << 6) // User access
#define PD_ACCESS     (1 << 10)
#define PD_UNOX       (1ul << 54) // Unprivileged access
#define PD_KNOX       (1ul << 53)
#define BOOT_PGD_ATTR PD_TABLE
#define BOOT_PUD_ATTR (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK)
// #define BOOT_PUD_ATTR (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_TABLE)

#define vtop(addr) (addr - KERNEL_VIRTUAL_BASE) // map virtual address to physical address
#define ptov(addr) (addr + KERNEL_VIRTUAL_BASE) // map physical address to virtual address

// void set_up_mmu(void);

void finer_granularity_paging(void);
void mappages (unsigned long *page_table, unsigned long vaddr, unsigned long paddr, unsigned long attr);
void setup_thread_peripherals(unsigned long *pgd);

#endif
