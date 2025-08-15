#ifndef __EXCEPTION_HANDLER_H
#define __EXCEPTION_HANDLER_H

#define MAX_BUFFER_SIZE 1024

#include "base.h"
#include "mmu.h"

#define IRQ_PENDING_1        (PBASE + 0x0000B204)
#define CORE0_TIMER_IRQ_CTRL (0x40000040 + KERNEL_VIRTUAL_BASE)
#define CORE0_IRQ_SRC        (0x40000060 + KERNEL_VIRTUAL_BASE)

extern char read_buffer[MAX_BUFFER_SIZE];
extern char write_buffer[MAX_BUFFER_SIZE];
extern int read_front;
extern int read_rear;
extern int write_front;
extern int write_rear;

typedef struct trapframe {
    unsigned long long x[31];
    unsigned long long sp_el0;
    unsigned long long elr_el1;
    unsigned long long spsr_el1;
} trapframe_t;

void enable_interrupt(unsigned long daif);
unsigned long disable_interrupt(void);

void sync_handler(trapframe_t *tf);

void *load_user_program(void *entry, void *stack);
void irq_handler(void);
void receive_handler(void);
void transmit_handler(void);
void timer_handler(void);
void syscall_handler(trapframe_t *tf);
void handle_signal();

void sigreturn(void);
void switch_to_signal_handler(
    unsigned long *normal_context,
    unsigned long *signal_context,
    void *handler_stack,
    void *handler,
    void *kernel_stack,
    void *kernel_stack_base
);

#endif