#include "allocator.h"
#include "devicetree.h"
#include "mini_uart.h"
#include "rootfs.h"
#include "shell.h"
#include "thread.h"
#include "user_prog.h"

void kernel_main(void) {
    uart_init();
    uart_send_string("Hello, world!\n");
    uart_send_string("Loading device tree...\n");
    fdt_traverse(initramfs_callback);
    buddy_init();
    unsigned long tmp;
    asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
    tmp |= 1;
    asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));
    init_thread();
    exec_prog("syscall.img");
    idle();
    
    shell();
    while(1) {
        uart_send(uart_recv());
    }
}