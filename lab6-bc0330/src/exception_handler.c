#include "allocator.h"
#include "exception_handler.h"
#include "mailbox.h"
#include "mini_uart.h"
#include "rootfs.h"
#include "syscall.h"
#include "thread.h"
#include "utils.h"

char read_buffer[MAX_BUFFER_SIZE] = {'\0'};
char write_buffer[MAX_BUFFER_SIZE]  = {'\0'};
int read_front = 0;
int read_rear = 0;
int write_front = 0;
int write_rear = 0;

void enable_interrupt(unsigned long daif) {
    asm volatile (
        "msr DAIF, x0\n" // enable IRQ
        : /* no output */
        : "r" (daif)
    );
}

unsigned long disable_interrupt(void) {
    unsigned long daif;
    asm volatile (
        "mrs %0, DAIF\n" // get current DAIF
        : "=r" (daif)
    );
    asm volatile (
        "msr DAIFSet, 0xf\n" // disable IRQ
    );
    return daif;
}

void sync_handler(trapframe_t *tf) {
    unsigned long daif = disable_interrupt();
    // uart_send_string("Sync handler called\r\n");
    
    unsigned long esr;
    asm volatile (
        "mrs %0, esr_el1\n"
        : "=r" (esr)
    );
    unsigned long type = esr >> 26;
    switch (type) {
        case 0b010101:  // SVC
            syscall_handler(tf);
            break;
        case 0b100100:  // data abort
            // uart_send_string("Data abort\r\n");
            unsigned long far_el1;
            unsigned long fault_status = (esr & 0b111111) >> 2;
            asm volatile (
                "mrs %0, far_el1\n"
                : "=r" (far_el1)
            );
            char is_segfault = 1;
            mem_region_t *cur_region = current_thread->regions;
            while (cur_region != NULL) {
                if (far_el1 >= cur_region->start && far_el1 < cur_region->end) {
                    is_segfault = 0; // not a segmentation fault
                    break;
                }
                cur_region = cur_region->next;
            }
            if (is_segfault || fault_status == 0b0011) {
                uart_send_string("[Segmentation fault]: Kill Process\r\n");
                sys_exit(tf);
            } else {
                uart_send_string("[Translation fault]: 0x");
                uart_send_num(far_el1, "hex");
                uart_send_string("\r\n");
                far_el1 = far_el1 & ~(PAGE_SIZE - 1); // align to page size
                unsigned long paddr = (unsigned long)allocate(PAGE_SIZE); // allocate a new page   
                if (paddr == 0) {
                    uart_send_string("Failed to allocate memory for data abort\r\n");
                    sys_exit(tf);
                }
                mappages(current_thread->pgd, far_el1, vtop(paddr), PD_USR_ACCESS); // map the page
            }
            break;
        case 0b100000:  // instruction abort
            // uart_send_string("Instruction abort\r\n");
            break;
        default:
            // uart_send_string("Unknown exception\r\n");
            // uart_send_num(esr, "hex");
            // uart_send_string("\r\n");
            // while (1);
            break;
    }
    enable_interrupt(daif);
}

void syscall_handler(trapframe_t *tf) {
    // uart_send_string("syscall handler\r\n");
    trapframe_t *sp = tf;

    unsigned long syscall_num = sp->x[8]; // syscall number

    // handle_signal();

    switch (syscall_num) {
        case 0:   // getpid
            sys_get_pid((trapframe_t *)sp);
            break;
        case 1: { // read
            enable_interrupt(0x0);
            sys_uart_read((trapframe_t *)sp);
            disable_interrupt();
            break;
        }
        case 2: { // write
            enable_interrupt(0x0);
            sys_uart_write((trapframe_t *)sp);
            disable_interrupt();
            break;
        }
        case 3: { // exec
            sys_exec((trapframe_t *)sp);
            break;
        }
        case 4: {        // fork
            sys_fork((trapframe_t *)sp);
            break;
        }
        case 5: {        // exit
            sys_exit((trapframe_t *)sp);
            break;
        }
        case 6: {        // mbox call
            sys_mbox_call((trapframe_t *)sp);
            break;
        }
        case 7: {        
            sys_kill((trapframe_t *)sp);
            break;
        }
        case 8: {
            sys_signal((trapframe_t *)sp);
            break;
        }
        case 9: {
            sys_sigkill((trapframe_t *)sp);
            break;
        }
        case 10: {
            sys_mmap((trapframe_t *)sp);
            break;
        }
        case 20: {      
            sys_sigreturn((trapframe_t *)sp);
            break;
        }
        default:
            uart_send_string("Unknown syscall\r\n");
            uart_send_num(syscall_num, "dec");
            uart_send_string("\r\n");
            sp->x[0] = -1; // return -1
            break;
    }
}

void irq_handler(void) {
    unsigned int cpu_irq_src;
    // uart_send_string("irq handler\r\n");
    unsigned long daif = disable_interrupt();
    handle_signal();

    cpu_irq_src = get32(CORE0_IRQ_SRC);

    if (cpu_irq_src & 0x2) {
        // timer interrupt
        timer_handler();
    } else {
        // uart_send_string("Unknown CPU interrupt\r\n");
    }
    enable_interrupt(daif);
}


void timer_handler(void) {
    // uart_send_string("timer handler\r\n");
    asm volatile (
        "mrs x0, cntfrq_el0\n"
        "lsr x0, x0, 5\n" // set timer to 1/32 sec
        "msr cntp_tval_el0, x0\n" // set expired time
    );
    schedule();
}

void handle_signal() {
//     int signal = current_thread->signal;
//     if (signal == SIGKILL) {
//         if (signal_handler[SIGKILL] != NULL) {
//             void *handler_kernel_stack_base = allocate(0x1000); // allocate a new stack for the signal handler
//             current_thread->signal_kernel_stack_base = handler_kernel_stack_base;
//             void *handler_stack_base = allocate(0x1000); // allocate a new stack for the signal handler
//             current_thread->signal_stack_base = handler_stack_base;
//             memset(handler_kernel_stack_base, 0, 0x1000);
//             memset(handler_stack_base, 0, 0x1000);
//             void *handler_stack = (void *)((unsigned long)handler_stack_base + 0x1000);
//             void *handler_kernel_stack = (void *)((unsigned long)handler_kernel_stack_base + 0x1000);
//             current_thread->signal = 0; // clear the signal

//             switch_to_signal_handler(
//                 current_thread->context,
//                 current_thread->signal_context,
//                 handler_stack,
//                 signal_handler[SIGKILL],
//                 handler_kernel_stack,
//                 handler_kernel_stack_base
//             );
//             asm volatile ( "add sp, sp, 0x30"); // discard arguments 

//         } else {
//             sys_exit(NULL);
//         }
//     }
}

void switch_to_signal_handler(
    unsigned long *normal_context,
    unsigned long *signal_context,
    void *handler_stack,
    void *handler,
    void *kernel_stack,
    void *kernel_stack_base
) {
    asm volatile (
        "stp x19, x20, [x1, 16 * 0]\n"
        "stp x21, x22, [x1, 16 * 1]\n"
        "stp x23, x24, [x1, 16 * 2]\n"
        "stp x25, x26, [x1, 16 * 3]\n"
        "stp x27, x28, [x1, 16 * 4]\n"
        "stp fp, lr, [x1, 16 * 5]\n"
        "mov x9, sp\n"
        "str x9, [x1, 16 * 6]\n" // save the normal context

        "mov fp, x5\n"
        "mov sp, x4\n"
        "mov lr, %0\n" // set the link register to the signal handler

        "mov x9, 0\n"
        "msr spsr_el1, x9\n" // set the SPSR to the signal context
        "msr elr_el1, x3\n"
        "msr sp_el0, x2\n" // set the stack pointer to the signal handler stack
        "msr tpidr_el1, x0\n" // set the thread pointer to the kernel stack
        "eret\n" // return to the signal handler
        : /* no output */
        : "r" (sigreturn)
    );
}

void sigreturn() {
    asm volatile (
        "mov x8, 20\n"
        "svc 0\n" // syscall to return from signal handler
    );
}