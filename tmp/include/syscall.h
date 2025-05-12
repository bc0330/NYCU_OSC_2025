#ifndef __SYSCALL_H
#define __SYSCALL_H

#include <stddef.h>
#include "exception_handler.h"
#include "thread.h"

#define SIGKILL 9

extern void *signal_handler[10];

void sys_get_pid(trapframe_t *tf);
void sys_uart_read(trapframe_t *tf);
void sys_uart_write(trapframe_t *tf);;
void sys_exec(trapframe_t *tf);
void sys_fork(trapframe_t *tf);
void sys_exit(trapframe_t *tf);
void sys_mbox_call(trapframe_t *tf);
void sys_kill(trapframe_t *tf);
void sys_signal(trapframe_t *tf);
void sys_sigkill(trapframe_t *tf);
void sys_sigreturn(trapframe_t *tf);

void restore_context(void);
thread_t *find_thread_by_id(int id);
void default_sigkill_handler();

#endif