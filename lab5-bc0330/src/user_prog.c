#include <stddef.h>
#include "allocator.h"
#include "mini_uart.h"
#include "rootfs.h"
#include "thread.h"
#include "user_prog.h"
#include "utils.h"

void jump_user_prog(void *entry, void *user_stack) {
    asm volatile (
        "mov x2, 0x0\n"
        "msr spsr_el1, x2\n" // daif = 0, interrupts enabled
        "msr elr_el1, %0\n" // set the entry point
        "msr sp_el0, %1\n"  // set the stack pointer
        "eret\n"
        : // no output
        : "r" (entry), "r" (user_stack) // input
    );
}

void exec_prog(char *filename) {
    int prog_size;
    void *user_prog = find_user_program(filename, &prog_size);
    uart_send_string("Found user program at: ");
    uart_send_num((unsigned long)user_prog, "hex");
    uart_send_string("\n");
    uart_send_string("User program size: ");
    uart_send_num(prog_size, "dec");
    uart_send_string("\n");
    if (user_prog == NULL) {
        uart_send_string("User program not found\n");
        return;
    }
    uart_send_string("allocating memory for user program...\r\n");
    void *new_user_prog = allocate(prog_size);
    uart_send_num((unsigned long)new_user_prog, "hex");
    uart_send_string("\n");
    memcpy(new_user_prog, user_prog, prog_size);
    thread_create(dummy_prog, HIGH_PRIORITY, new_user_prog, prog_size);
}

void dummy_prog(void) {
    uart_send_string("Jumping to user program...\r\n");
    if (current_thread->user_prog == NULL) {
        uart_send_string("User program not set\n");
        return;
    }
    uart_send_num((unsigned long)current_thread->user_prog, "hex");
    uart_send_string("\n");
    jump_user_prog(current_thread->user_prog, current_thread->usr_stack);
}