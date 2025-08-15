#include "allocator.h"
#include "exception_handler.h"
#include "mini_uart.h"
#include "mmu.h"
#include <stddef.h>
#include "thread.h"
#include "utils.h"
#include "vfs.h"

thread_queue_t run_queue[3]; // Global run queue
thread_queue_t wait_queue;  // Global wait queue
thread_queue_t zombies_queue; // Global zombies queue
thread_t *current_thread;
unsigned long counter = 0;

void init_thread(void) {
    current_thread = thread_create(idle, LOW_PRIORITY, NULL, 0);
    asm volatile (
        "msr tpidr_el1, %0\n"
        : /* no output */
        : "r" (current_thread->context)
    );
    for (int i = 0; i < 3; i++) {
        run_queue[i].head = (NULL);
        run_queue[i].tail = NULL;
    }
    run_queue[LOW_PRIORITY].head = current_thread;
    run_queue[LOW_PRIORITY].tail = current_thread;
    wait_queue.head = NULL;
    wait_queue.tail = NULL;
    zombies_queue.head = NULL;
    zombies_queue.tail = NULL;
}

void __asm_impl() {
    asm volatile (
        // context switch function
        ".globl switch_to\n"
        "switch_to:\n"
        "stp x19, x20, [x0, 16 * 0]\n"
        "stp x21, x22, [x0, 16 * 1]\n"
        "stp x23, x24, [x0, 16 * 2]\n"
        "stp x25, x26, [x0, 16 * 3]\n"
        "stp x27, x28, [x0, 16 * 4]\n"
        "stp fp, lr, [x0, 16 * 5]\n"
        "mov x9, sp\n"
        "str x9, [x0, 16 * 6]\n"

        "ldp x19, x20, [x1, 16 * 0]\n"
        "ldp x21, x22, [x1, 16 * 1]\n"
        "ldp x23, x24, [x1, 16 * 2]\n"
        "ldp x25, x26, [x1, 16 * 3]\n"
        "ldp x27, x28, [x1, 16 * 4]\n"
        "ldp fp, lr, [x1, 16 * 5]\n"
        "ldr x9, [x1, 16 * 6]\n"
        "mov sp, x9\n"
        "msr tpidr_el1, x1\n"

        "dsb ish\n" // Ensure all memory accesses are complete
        "msr ttbr0_el1, x2\n" // Set the page table base register to the new thread's pgd
        "tlbi vmalle1is\n" // Invalidate the TLB for all CPUs
        "dsb ish\n" // Ensure the TLB invalidation is complete
        "isb\n" // Instruction synchronization barrier
        "ret\n"

        // get current thread stack
        ".globl get_current\n"
        "get_current:\n"
        "mrs x0, tpidr_el1\n"
        "ret\n"
    );
}

thread_t *thread_create(void (*function)(void), int priority, void (*user_prog)(void), size_t prog_size) {
    thread_t *thread = (thread_t *)allocate(sizeof(thread_t)); // Allocate memory for the thread structure
    if (!thread) {
        uart_send_string("Memory allocation failed for thread\n");
        return NULL; // Memory allocation failed
    }

    thread->id = counter++; // Assign a unique ID to the thread
    thread->priority = priority;
    thread->function = function;
    thread->user_prog = user_prog;
    thread->prog_size = prog_size;
    thread->state = THREAD_WAITING; // Set the initial state to ready
    thread->signal = 0; // Initialize the signal to 0
    thread->usr_stack_base = allocate(4 * thread_stack_size); // Allocate stack for the thread
    thread->kernel_stack_base = allocate(thread_stack_size); // Allocate kernel stack for the thread
    thread->signal_stack_base = NULL; // Initialize the signal stack base to NULL
    thread->signal_kernel_stack_base = NULL; // Initialize the signal kernel stack base to NULL

    if (!thread->usr_stack_base || !thread->kernel_stack_base) {
        uart_send_string("Memory allocation failed for thread stack\n");
        free(thread);
        return NULL; // Memory allocation failed
    }
    // setup_thread_peripherals(thread->pgd); // Set up the thread's peripherals
    thread->pgd = allocate(PAGE_SIZE); // Allocate a page for the thread's page directory
    memset((char *)thread->pgd, 0, PAGE_SIZE); // Initialize the page directory to zero
    memset((char *)thread->usr_stack_base, 0, 4 * thread_stack_size); // Initialize the stack to zero
    memset((char *)thread->kernel_stack_base, 0, thread_stack_size); // Initialize the kernel stack to zero

    for (int i = 0; i < 4; ++i) {
        mappages(thread->pgd, 0xFFFFFFFFB000 + i * PAGE_SIZE, vtop((unsigned long)thread->usr_stack_base + i * PAGE_SIZE), PD_USR_ACCESS); // Map the user stack pages
    }
    // thread->usr_stack_base = (void *)0xFFFFFFFFB000; // Set the user stack base address
    thread->usr_stack = thread->usr_stack_base + 4 * thread_stack_size; // Set the stack pointer to the top of the stack
    thread->kernel_stack = thread->kernel_stack_base + thread_stack_size; // Set the kernel stack pointer to the top of the stack

    memset((char *)thread->context, 0, sizeof(thread->context)); // Initialize the stack to zero
    memset((char *)thread->signal_context, 0, sizeof(thread->signal_context)); // Initialize the signal context to zero

    if (user_prog != NULL) {
        for (size_t i = 0; i < prog_size; i += PAGE_SIZE) {
            unsigned long paddr = vtop((unsigned long)thread->user_prog + i);
            mappages(thread->pgd, i, paddr, PD_USR_ACCESS); // Map the user program pages
        }
    }

    setup_thread_peripherals(thread->pgd); // map gpu memory so that the thread can use it

    // --- vfs setup ---
    thread->cwd = rootfs->root; // Set the current working directory to the root directory
    for (int i = 0; i < MAX_FD; ++i) {
        thread->files_table[i] = NULL; // Initialize the file descriptors table to NULL
    }
    struct file *stdin;
    struct file *stdout;
    struct file *stderr;
    vfs_open("/dev/uart/stdin", O_RDONLY, &stdin);
    vfs_open("/dev/uart/stdout", O_WRONLY, &stdout);
    vfs_open("/dev/uart/stderr", O_WRONLY, &stderr);
    thread->files_table[0] = stdin;  // Set stdin to file descriptor 0
    thread->files_table[1] = stdout; // Set stdout to file descriptor 1
    thread->files_table[2] = stderr; // Set stderr to file descriptor 2

    thread->prev = NULL;
    thread->next = NULL;

    thread->context[12] = (unsigned long)thread->kernel_stack; // Set the stack pointer in the context
    thread->context[11] = (unsigned long)function; // Set the function to execute in the context
    thread->context[10] = (unsigned long)thread->kernel_stack_base; // Set the stack pointer in the context
    thread_enqueue(thread); // Add the thread to the run queue
    // thread->pgd = (void *)vtop((unsigned long)thread->pgd); // Convert the page directory to physical address
    return thread;
}

void thread_start(void) {
    // Call the function associated with the thread
    uart_send_string("Thread started\n");
    current_thread->function();
    thread_exit();
}

void thread_kill(thread_t *t, int status) {
    t->state = THREAD_DEAD; // Set the thread state to dead
    if (t != current_thread) {
        thread_remove_from_queue(t); // Remove the thread from the run queue
    }
    t->exit_code = status; // Set the exit code
    t->next = NULL; // Clear the next pointer
    t->prev = NULL;
    if (zombies_queue.head == NULL) {
        zombies_queue.head = t;
        zombies_queue.tail = t;
    } else {
        zombies_queue.tail->next = t;
        zombies_queue.tail = t;
    }
}

void thread_exit(void) {
    unsigned long daif = disable_interrupt();
    uart_send_num(current_thread->id, "hex");
    uart_send_string(" Thread exiting\r\n");
    vfs_close(current_thread->files_table[0]); // Close stdin
    vfs_close(current_thread->files_table[1]); // Close stdout
    vfs_close(current_thread->files_table[2]); // Close stderr
    current_thread->state = THREAD_DEAD; // Set the thread state to dead
    current_thread->next = NULL; // Clear the next pointer
    current_thread->prev = NULL; // Clear the previous pointer
    current_thread->exit_code = 0; // Set the exit code
    if (zombies_queue.head == NULL) {
        zombies_queue.head = current_thread;
        zombies_queue.tail = current_thread;
    } else {
        zombies_queue.tail->next = current_thread;
        zombies_queue.tail = current_thread;
    }
    enable_interrupt(daif);
    schedule();
    while(1);
}


void schedule(void) {
    // Check if there are any threads in the run queue
    thread_t *prev_thread = current_thread;
    thread_t *next_thread = thread_dequeue();
    if (next_thread != NULL) {
        // uart_send_string("Switching to thread: ");
        // uart_send_num(next_thread->id, "hex");
        // uart_send_string(", State: ");
        // uart_send_num(next_thread->state, "dec");
        // uart_send_string("\r\n");
        
    } else {
        next_thread = thread_create(idle, LOW_PRIORITY, NULL, 0); // Create a new idle thread if no threads are available
    }


    next_thread->state = THREAD_RUNNING; // Set the next thread state to running
    if (current_thread->state != THREAD_DEAD) {
        current_thread->state = THREAD_READY; // Set the current thread state to ready
        thread_enqueue(current_thread); // Add the current thread back to the run queue
    } 

    current_thread = next_thread; // Update the current thread
    switch_to(prev_thread->context, next_thread->context, (void *)vtop((unsigned long)next_thread->pgd)); // Switch to the next thread
}

void foo() {
    // uart_send_string("In foo thread\n");
    for (int i = 0; i < 10; ++i) {
        uart_send_string("Thread id: ");
        uart_send_num(current_thread->id, "dec");
        uart_send_string(" ");
        uart_send_num(i, "dec");
        uart_send_string("\n");
        for (int j = 0; j < 1000000; ++j) {
            asm volatile ("nop");
        }
        schedule();
    }
}

void idle() {
    while (1) {
        // uart_send_string("Idle thread running\n");
        // kill_zombies();
        schedule();
    }
}

void kill_zombies() {
    // Check if there are any zombies in the zombies queue
    // uart_send_string("Killing zombies\n");
    while (zombies_queue.head != NULL) {
        thread_t *zombie = zombies_queue.head;
        zombies_queue.head = zombie->next;

        // If the queue is now empty, set the tail to NULL
        if (zombies_queue.head == NULL) {
            zombies_queue.tail = NULL;
        }

        // Free the memory allocated for the zombie thread
        free(zombie->usr_stack_base); // Free the stack
        free(zombie->kernel_stack_base); // Free the kernel stack
        free(zombie->user_prog); // Free the user program
        free(zombie);
    }
}

void thread_enqueue(thread_t *t) {
    if (run_queue[t->priority].head == NULL) {
        run_queue[t->priority].head = t;
        run_queue[t->priority].tail = t;
        t->prev = NULL;
        t->next = NULL;
    } else {
        t->prev = run_queue[t->priority].tail;
        run_queue[t->priority].tail->next = t;
        run_queue[t->priority].tail = t;
        t->next = NULL;
    }
}


thread_t *thread_dequeue() {
    for (int i = HIGH_PRIORITY; i >= LOW_PRIORITY; i--) {
        if (run_queue[i].head != NULL) {
            thread_t *t = run_queue[i].head;
            run_queue[i].head = t->next;
            if (run_queue[i].head != NULL) {
                run_queue[i].head->prev = NULL;
            } else {
                run_queue[i].tail = NULL; // If the queue is now empty, set the tail to NULL
            }
            return t;
        }
    }
    return NULL;
}

void thread_remove_from_queue(thread_t *t) {
    if (t->prev != NULL) {
        t->prev->next = t->next;
    } else {
        run_queue[t->priority].head = t->next;
    }
    if (t->next != NULL) {
        t->next->prev = t->prev;
    } else {
        run_queue[t->priority].tail = t->prev;
    }
}

void print_thread_info() {
    for (int i = HIGH_PRIORITY; i >= LOW_PRIORITY + 1; i--) {
        thread_t *t = run_queue[i].head;
        uart_send_string("=== Priority: ");
        uart_send_num(i, "dec");
        uart_send_string(" ===\n");
        while (t != NULL) {
            uart_send_string("    Thread ID: ");
            uart_send_num(t->id, "dec");
            uart_send_string(" Priority: ");
            uart_send_num(t->priority, "dec");
            uart_send_string(" State: ");
            uart_send_num(t->state, "dec");
            uart_send_string("\n");
            t = t->next;
        }
    }
}