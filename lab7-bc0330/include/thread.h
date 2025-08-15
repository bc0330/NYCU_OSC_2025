#ifndef _THREAD_H_
#define _THREAD_H_

#include <stddef.h>
typedef unsigned long pid_t;

// priority levels
#define LOW_PRIORITY     0
#define MEDIUM_PRIORITY  1
#define HIGH_PRIORITY    2

// Thread status
#define THREAD_RUNNING   0
#define THREAD_READY     1
#define THREAD_DEAD      2
#define THREAD_WAITING   3

#define MAX_FD       16
#define thread_stack_size 0x1000 // Size of the thread stack

typedef struct thread {
    pid_t id;                   // Thread ID
    int priority;               // Thread priority
    int state;                  // Thread states (e.g., running, ready, etc.)
    int exit_code;

    void *usr_stack;            // Pointer to the thread's stack
    void *usr_stack_base;       // Base of the thread's stack
    void *kernel_stack;         // Pointer to the thread's kernel stack
    void *kernel_stack_base;    // Base of the thread's kernel stack

    unsigned long *pgd;

    void (*function)(void);     // Function to execute
    void (*user_prog)(void);    // User program to execute
    size_t prog_size;
    
    unsigned long context[13];  // Thread context (registers, etc.)
        /*
          0: x19
          1: x20
          2: x21
          3: x22
          4: x23
          5: x24
          6: x25
          7: x26
          8: x27
          9: x28
          10: fp
          11: lr
          12: sp
        */

    // --- signal handling attributes ---
    unsigned int signal; 
    unsigned long signal_context[13];
    void *signal_stack_base;
    void *signal_kernel_stack_base;

    // --- vfs attributes ---
    struct vnode *cwd;  // Current working directory
    struct file  *files_table[MAX_FD]; // File descriptors table

    struct thread* prev;
    struct thread* next;        // Pointer to the next thread in the queue
} thread_t;

typedef struct thread_queue {
    thread_t* head; // Pointer to the head of the queue
    thread_t* tail; // Pointer to the tail of the queue
} thread_queue_t;

extern thread_queue_t run_queue[3];   // Global run queue
extern thread_queue_t zombies_queue;  // Global zombies queue
extern thread_t *current_thread;      // Pointer to the currently running thread
extern unsigned long counter;         // counter for thread ID

extern void switch_to(void *prev, void *next, void *next_pgd);
extern void *get_current(void);

void init_thread(void);
thread_t *thread_create(void (*function)(void), int priority, void (*user_prog)(void), size_t prog_size);
void schedule(void);
void foo(void);
void idle(void);
void kill_zombies(void);


void thread_start(void);
void thread_kill(thread_t *t, int status);
void thread_exit(void);

void thread_enqueue(thread_t *t);
thread_t *thread_dequeue();
void thread_remove_from_queue(thread_t *t);

void print_thread_info();

#endif