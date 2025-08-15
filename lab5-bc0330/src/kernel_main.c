#include "allocator.h"
#include "devicetree.h"
#include "exception_handler.h"
#include "mini_uart.h"
#include "rootfs.h"
#include "shell.h"
#include "syscall.h"
#include "thread.h"
#include "user_prog.h"

void kernel_main(void) {
    uart_init();
    uart_send_string("Hello, world!\r\n");
    uart_send_string("Loading device tree...\r\n"); 
    fdt_traverse(initramfs_callback);
    buddy_init();
    init_thread();
    // test syscall
    unsigned long tmp;
    asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
    tmp |= 1;
    asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));
    exec_prog("syscall.img");
    idle();

    while (1);
    // for (int i = 0; i < 5; ++i) {
    //     thread_create(foo, MEDIUM_PRIORITY, NULL);
    // }
    // idle();
    // shell();
    // while(1) {
    //     uart_send(uart_recv());
    // }
}
