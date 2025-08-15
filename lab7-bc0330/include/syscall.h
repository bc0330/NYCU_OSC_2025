#ifndef __SYSCALL_H
#define __SYSCALL_H

#include <stddef.h>
#include "exception_handler.h"
#include "thread.h"

#define SIGKILL 9

extern void *signal_handler[10];

void sys_get_pid(trapframe_t *tf);     // 0 
void sys_uart_read(trapframe_t *tf);   // 1
void sys_uart_write(trapframe_t *tf);  // 2
void sys_exec(trapframe_t *tf);        // 3
void sys_fork(trapframe_t *tf);        // 4
void sys_exit(trapframe_t *tf);        // 5
void sys_mbox_call(trapframe_t *tf);   // 6
void sys_kill(trapframe_t *tf);        // 7
void sys_signal(trapframe_t *tf);      // 8
void sys_sigkill(trapframe_t *tf);     // 9
void sys_sigreturn(trapframe_t *tf);   // 10

// --- vfs system calls ---
void sys_open(trapframe_t *tf);        // 11
void sys_close(trapframe_t *tf);       // 12
void sys_write(trapframe_t *tf);       // 13
void sys_read(trapframe_t *tf);        // 14
void sys_mkdir(trapframe_t *tf);       // 15
void sys_mount(trapframe_t *tf);       // 16
void sys_chdir(trapframe_t *tf);       // 17
void sys_lseek64(trapframe_t *tf);     // 18
void sys_ioctl(trapframe_t *tf);       // 19

void restore_context(void);
thread_t *find_thread_by_id(int id);
void default_sigkill_handler();

#endif