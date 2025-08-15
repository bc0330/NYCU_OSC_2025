#include "devicetree.h"
#include "mini_uart.h"
#include "rootfs.h"
#include "shell.h"

char *rootfs_addr;

void kernel_main(void) {
    uart_init();
    uart_send_string("Hello, world!\n");
    uart_send_string("Loading device tree...\n");
    fdt_traverse(initramfs_callback);
    shell();
    while(1) {
        uart_send(uart_recv());
    }
}