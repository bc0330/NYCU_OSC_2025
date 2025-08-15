#include <stddef.h>
#include "allocator.h"
#include "mini_uart.h"
#include "mmu.h"
#include "rootfs.h"
#include "thread.h"
#include "user_prog.h"
#include "utils.h"

void jump_user_prog(void *entry, void *user_stack) {
    asm volatile (
        "mov x9, 0x0\n"
        "msr spsr_el1, x9\n" // daif = 0, interrupts enabled
        "msr elr_el1, x0\n" // set the entry point
        "msr sp_el0, x1\n"  // set the stack pointer
        "eret\n"
        : // no output
        : "r" (entry), "r" (user_stack) // input
    );
}

void exec_prog(char *filename) {
    int prog_size;
    void *user_prog = find_user_program(filename, &prog_size);
    
    if (user_prog == NULL) {
        uart_send_string("User program not found\n");
        return;
    }
    uart_send_string("allocating memory for user program...\r\n");

    void *new_user_prog = allocate(prog_size);
    
    
    memcpy(new_user_prog, user_prog, prog_size);
    thread_create(dummy_prog, HIGH_PRIORITY, new_user_prog, prog_size);
}

void dummy_prog(void) {
    uart_send_string("Jumping to user program...\r\n");
    jump_user_prog((void *)0, (void *)0xFFFFFFFFF000);
}