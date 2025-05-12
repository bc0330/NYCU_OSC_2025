#include "mini_uart.h"

void boot_main(void) {
    char *KERNEL_ADDR = (char *)0x80000;

    uart_init();
    uart_send_string("Hello, World!\n");
    uart_send_string("Booting the kernel...\n");

    while (1) {
        if (uart_recv() == 0xAA) {
            uart_send(0xAA);
            break;
        }
    }
    // Read the kernel size, in little endian
    // unsigned int magic = uart_recv_int();
    // unsigned int size = uart_recv_int();
    // unsigned int checksum = uart_recv_int();
    // if (magic == 0x544F4F42) {
    //     uart_send_string("Invalid kernel image!\n");
    //     return;
    // }
    unsigned int size = uart_recv_int();
    // Read the kernel
    while (size--) {
        *KERNEL_ADDR++ = uart_recv();
    }
    
    uart_send_string("Kernel loaded successfully!\n");
    uart_send_string("Jumping to the kernel...\n");
    asm volatile(
        "mov x0, x10;"
        "mov x1, x11;"
        "mov x2, x12;"
        "mov x3, x13;"
        "mov lr, 0x80000;"
        "ret;"
    );

}
