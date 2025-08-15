#include "allocator.h"
#include "devicetree.h"
#include "mini_uart.h"
#include "mmu.h"
#include "rootfs.h"
#include "shell.h"
#include "thread.h"
#include "user_prog.h"
#include "vfs.h"

void kernel_main(void) {
    uart_init();
    uart_send_string("Hello, world!\r\n");

    fdt_traverse(initramfs_callback);

    buddy_init();

    finer_granularity_paging();

    init_vfs();

    unsigned long tmp;
    asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
    tmp |= 1;
    asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));

    init_thread();

    exec_prog("/initramfs/vfs1.img");
    idle();
}