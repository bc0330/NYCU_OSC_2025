#include "allocator.h"
#include "devicetree.h"
#include "mini_uart.h"
#include "mmu.h"
#include "rootfs.h"
#include "shell.h"
#include "thread.h"
#include "user_prog.h"

void kernel_main(void) {
    // asm volatile (
    //     "dsb ish\n" // Ensure all memory accesses are complete
    //     "mov x2, 0x20000\n"
    //     "msr ttbr0_el1, x2\n" // Set the page table base register to the new thread's pgd
    //     "tlbi vmalle1is\n" // Invalidate the TLB for all CPUs
    //     "dsb ish\n" // Ensure the TLB invalidation is complete
    //     "isb\n" // Instruction synchronization barrier
    // );
    uart_init();
    uart_send_string("Hello, world!\r\n");
    fdt_traverse(initramfs_callback);
    buddy_init();
    finer_granularity_paging();
    unsigned long tmp;
    asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
    tmp |= 1;
    asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));
    init_thread();
    exec_prog("vm.img");
    idle();
    
    // shell();
    // while(1) {
    //     uart_send(uart_recv());
    // }
}