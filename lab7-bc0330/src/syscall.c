#include "allocator.h"
#include "exception_handler.h"
#include "framebufferfs.h"
#include "initramfs.h"
#include "mailbox.h"
#include "mini_uart.h"
#include "mmu.h"
#include "rootfs.h"
#include "syscall.h"
#include "thread.h"
#include "utils.h"
#include "vfs.h"
#include <stddef.h>

void *signal_handler[10] = {NULL};

void sys_get_pid(trapframe_t *tf) {
    tf->x[0] = current_thread->id;
}

void sys_uart_read(trapframe_t *tf) {
    char *buf = (char *)tf->x[0];
    size_t size = tf->x[1];
    for (size_t i = 0; i < size; ++i) {
        buf[i] = uart_recv();
    }
    tf->x[0] = size; // return the number of bytes read
}

void sys_uart_write(trapframe_t *tf) {
    char *buf = (char *)tf->x[0];
    size_t size = tf->x[1];
    for (size_t i = 0; i < size; ++i) {
        uart_send(buf[i]);
    }
    tf->x[0] = size; // return the number of bytes written
}

void sys_exec(trapframe_t *tf) {
    unsigned long daif = disable_interrupt();
    uart_send_string("[SYSCALL] exec\r\n");
    char *filename = (char *)tf->x[0];
    int prog_size;
    // void *entry = find_user_program(filename, &prog_size);
    struct file *entry = NULL;
    thread_t *cur_thread = current_thread;

    if (vfs_open(filename, O_RDONLY, &entry) != 0) {
        uart_send_string("[ERROR] File not found\r\n");
        enable_interrupt(daif);
        return; // File not found
    }
    prog_size = ((initramfs_internal_t *)entry->vnode->internal)->size;

    memset((char *)cur_thread->usr_stack_base, 0, 4 * thread_stack_size);
    free(cur_thread->user_prog); // Free the old user program
    memset((char *)tf, 0, sizeof(trapframe_t)); // Clear the trap frame

    cur_thread->prog_size = prog_size; // Set the new program size
    cur_thread->user_prog = allocate(prog_size); // Allocate memory for the new user program
    if (cur_thread->user_prog == NULL) {
        uart_send_string("Memory allocation failed for user program\n");
        return; // Memory allocation failed
    }
    if (vfs_read(entry, cur_thread->user_prog, prog_size) != prog_size) {
        uart_send_string("[ERROR] Failed to read user program\r\n");
        vfs_close(entry); // Close the file after reading
        enable_interrupt(daif);
        return; // Read error
    }
    vfs_close(entry); // Close the file after reading
    // memcpy(cur_thread->user_prog, entry, prog_size); // Copy the new user program to the thread
    for (size_t i = 0; i < prog_size; i += PAGE_SIZE) {
        unsigned long paddr = vtop((unsigned long)cur_thread->user_prog + i);
        mappages(cur_thread->pgd, i, paddr, PD_USR_ACCESS); // Map the user program pages
    }
    cur_thread->cwd = rootfs->root; // Set the current working directory to the root directory
    // set the trap frame so it jumps to the new program
    tf->sp_el0 = 0xFFFFFFFFF000; // Set the stack pointer to the top of the user stack
    tf->elr_el1 = 0; // Set the entry point
    tf->spsr_el1 = 0; // Set the SPSR to 0
    enable_interrupt(daif); // Enable interrupts

}

void sys_fork(trapframe_t *tf) {
    uart_send_string("[SYSCALL] fork\r\n");
    thread_t *child_thread = allocate(sizeof(thread_t));

    *child_thread = *current_thread; // Copy the current thread's context
    child_thread->id = counter++; // Assign a unique ID to the child thread
    child_thread->signal = 0;

    child_thread->pgd = allocate(PAGE_SIZE); // Allocate a new page directory for the child thread
    memset((char *)child_thread->pgd, 0, PAGE_SIZE); // Initialize the page directory to zero

    // copy the parent's user stack to the child
    child_thread->usr_stack_base = allocate(4 * PAGE_SIZE); // Allocate stack for the child thread
    child_thread->usr_stack = child_thread->usr_stack_base + 4 * PAGE_SIZE; // Set the stack pointer to the top of the stack
    memcpy(child_thread->usr_stack_base, current_thread->usr_stack_base, 4 * PAGE_SIZE); // Copy the parent's stack to the child
    for (int i = 0; i < 4; ++i) {
        mappages(child_thread->pgd, 0xFFFFFFFFB000 + i * PAGE_SIZE, 
            vtop((unsigned long)child_thread->usr_stack_base + i * PAGE_SIZE), PD_USR_ACCESS); // Map the user stack pages
    }

    // copy the parent's kernel stack to the child
    child_thread->kernel_stack_base = allocate(thread_stack_size); // Allocate kernel stack for the child thread
    child_thread->kernel_stack = child_thread->kernel_stack_base + thread_stack_size - sizeof(trapframe_t); // Set the kernel stack pointer to the top of the stack
    trapframe_t *child_tf = (trapframe_t *)child_thread->kernel_stack; // Set the child thread's trapframe
    memcpy(child_tf, tf, sizeof(trapframe_t)); // Copy the parent's trapframe to the child

    child_thread->user_prog = allocate(current_thread->prog_size); // Allocate memory for the child thread's user program
    memcpy(child_thread->user_prog, current_thread->user_prog, current_thread->prog_size); // Copy the parent's user program to the child
    for (size_t i = 0; i < child_thread->prog_size; i += PAGE_SIZE) {
        unsigned long paddr = vtop((unsigned long)child_thread->user_prog + i);
        mappages(child_thread->pgd, i, paddr, PD_USR_ACCESS); // Map the user program pages
    }

    setup_thread_peripherals(child_thread->pgd); // Set up the thread's peripherals

    // --- vfs setup ---
    child_thread->cwd = NULL;
    for (int i = 0; i < MAX_FD; ++i) {
        child_thread->files_table[i] = NULL; // Initialize the file descriptors table to NULL
    }

    child_tf->x[0] = 0; // Set the return value to 0 for the child thread

    child_thread->prev = NULL; // Set the previous thread to NULL
    child_thread->next = NULL; // Set the next thread to NULL

    child_thread->context[12] = (unsigned long)child_tf;
    child_thread->context[11] = (unsigned long)restore_context; // Set the function to execute in the context
    child_thread->context[10] = (unsigned long)child_thread->kernel_stack_base; // Set the stack pointer in the context

    thread_enqueue(child_thread); // Add the child thread to the run 
    tf->x[0] = child_thread->id; // Set the return value to the child thread's ID for the parent thread
}

void restore_context(void) {
    asm volatile (
        "ldp x0, x1, [sp, 16 * 16]\n"
        "msr elr_el1, x0\n"
        "msr spsr_el1, x1\n"
        "ldp x30, x0, [sp, 16 * 15]\n"
        "msr sp_el0, x0\n"
        "ldp x28, x29, [sp, 16 * 14]\n"
        "ldp x26, x27, [sp, 16 * 13]\n"
        "ldp x24, x25, [sp, 16 * 12]\n"
        "ldp x22, x23, [sp, 16 * 11]\n"
        "ldp x20, x21, [sp, 16 * 10]\n"
        "ldp x18, x19, [sp, 16 * 9]\n"
        "ldp x16, x17, [sp, 16 * 8]\n"
        "ldp x14, x15, [sp, 16 * 7]\n"
        "ldp x12, x13, [sp, 16 * 6]\n"
        "ldp x10, x11, [sp, 16 * 5]\n"
        "ldp x8, x9, [sp, 16 * 4]\n"
        "ldp x6, x7, [sp, 16 * 3]\n"    
        "ldp x4, x5, [sp, 16 * 2]\n"
        "ldp x2, x3, [sp, 16 * 1]\n"
        "ldp x0, x1, [sp, 16 * 0]\n"
        "add sp, sp, 16 * 17\n" // Adjust the stack pointer
        "eret\n" // Return from exception
    );
}

void sys_exit(trapframe_t *tf) {
    uart_send_string("[SYSCALL] exit\r\n");
    thread_exit();
}

void sys_mbox_call(trapframe_t *tf) {
    unsigned long mbox_addr = (tf->x[1] - 0xFFFFFFFFB000) + (unsigned long)current_thread->usr_stack_base; // Get the mailbox address
    tf->x[0] = mailbox_call(tf->x[0], (unsigned int *)mbox_addr); // return 
}

void sys_kill(trapframe_t *tf) {
    uart_send_string("[SYSCALL] kill\r\n");
    unsigned long daif = disable_interrupt();
    int pid = tf->x[0];
    int status = tf->x[1];
    thread_t *target_thread = NULL;

    if (pid == current_thread->id) {
        uart_send_string("Killing current thread (self)\r\n");
        target_thread = current_thread;
    } else {
        target_thread = find_thread_by_id(pid);
    }
    // while (1);
    if (target_thread != NULL) {
        thread_kill(target_thread, status); // Call the corrected thread_kill

        // If we killed the current thread, we must schedule away from it.
        if (target_thread == current_thread) {
            enable_interrupt(daif); // Must enable before calling schedule
            schedule();             // schedule() will switch context
            // Code should not reach here after schedule() if killing self
            uart_send_string("Error: Returned after schedule() in self-kill\r\n");
            while(1); // Safety loop
        }
        // If killing another thread, just continue execution in the current thread.
    } else {
        uart_send_string("Thread not found or cannot kill\r\n");
        // Set error return code? tf->x[0] = -ESRCH; (or similar)
    }
    enable_interrupt(daif);
}

thread_t *find_thread_by_id(int id) {
    for (int i = HIGH_PRIORITY; i >= LOW_PRIORITY; --i) {
        thread_t *thread = run_queue[i].head;
        while (thread != NULL) {
            if (thread->id == id) {
                return thread;
            }
            thread = thread->next;
        }
    }
    return NULL;
}

void default_sigkill_handler(int unused) {
    thread_kill(current_thread, 0);
}

void sys_signal(trapframe_t *tf) {
    int signum = tf->x[0];
    signal_handler[signum] = (void *)tf->x[1]; // Set the signal handler
}

void sys_sigkill(trapframe_t *tf) {
    uart_send_string("[SYSCALL] sigkill\r\n");
    unsigned long daif = disable_interrupt();
    int pid = tf->x[0];
    thread_t *target_thread = NULL;

    if (pid == current_thread->id) {
        uart_send_string("Killing current thread (self)\r\n");
        target_thread = current_thread;
    } else {
        uart_send_string("Attempting to kill thread with PID: ");
        uart_send_num(pid, "hex");
        uart_send_string("\r\n");
        target_thread = find_thread_by_id(pid);
    }

    if (target_thread != NULL) {
        target_thread->signal = tf->x[1]; // Set the signal for the target thread
    } else {
        uart_send_string("Thread not found or cannot kill\r\n");
    }
    enable_interrupt(daif);
}

void sys_sigreturn(trapframe_t *tf) {
    uart_send_string("[SYSCALL] sigreturn\r\n");

    // Restore the context from the signal handler

    free(current_thread->signal_stack_base); // Free the old user stack
    free(current_thread->signal_kernel_stack_base); // Free the old kernel stack
    current_thread->signal_stack_base = NULL;
    current_thread->signal_kernel_stack_base = NULL;

    asm volatile (
        "ldp x19, x20, [%0, 16 * 0]\n"
        "ldp x21, x22, [%0, 16 * 1]\n"
        "ldp x23, x24, [%0, 16 * 2]\n"
        "ldp x25, x26, [%0, 16 * 3]\n"
        "ldp x27, x28, [%0, 16 * 4]\n"
        "ldp fp, lr, [%0, 16 * 5]\n"
        "ldr x9, [%0, 16 * 6]\n"
        "mov sp, x9\n"
        "msr tpidr_el1, %1\n"
        "ret\n"
        : 
        : "r"(current_thread->signal_context), "r" (current_thread->context)  // Input operand
    );
}

// --- vfs syscalls ---
void sys_open(trapframe_t *tf) {
    // uart_send_string("[SYSCALL 11] open\r\n");
    char *pathname = (char *)tf->x[0];
    int flags = tf->x[1];
    for (int i = 0; i < MAX_FD; ++i) {
        if (current_thread->files_table[i] == NULL                                    // empty slot
                && vfs_open(pathname, flags, &current_thread->files_table[i]) == 0) { // successfully opened file
            tf->x[0] = i; // Return the file descriptor
            return;
        }
    }
    tf->x[0] = -1; // Return -1 if no file descriptor is available
}

void sys_close(trapframe_t *tf) {
    // uart_send_string("[SYSCALL 12] close\r\n");
    int fd = tf->x[0];

    if (fd < 0 || fd >= MAX_FD || current_thread->files_table[fd] == NULL) {
        uart_send_string("[ERROR | CLOSE] Invalid file descriptor\r\n");
        tf->x[0] = -1; // Return -1 if the file descriptor is invalid
        return;
    }

    if(vfs_close(current_thread->files_table[fd]) != 0) {
        uart_send_string("[ERROR | CLOSE] Failed to close file\r\n");
        tf->x[0] = -1; // Return -1 if the file close operation failed
        return;
    }
    current_thread->files_table[fd] = NULL; // Clear the file descriptor
    tf->x[0] = 0; // Return 0 on success
}

void sys_write(trapframe_t *tf) {
    // uart_send_string("[SYSCALL 13] write\r\n");
    int fd = tf->x[0];
    char *buf = (char *)tf->x[1];
    size_t len = tf->x[2];
    if (fd < 0 || fd >= MAX_FD || current_thread->files_table[fd] == NULL) {
        uart_send_string("[ERROR | WRITE] Invalid file descriptor\r\n");
        tf->x[0] = -1; // Return -1 if the file descriptor is invalid
        return;
    }

    int bytes_written = vfs_write(current_thread->files_table[fd], buf, len);
    if (bytes_written < 0) {
        uart_send_string("[ERROR | WRITE] Failed to write to file\r\n");
        tf->x[0] = -1; // Return -1 if the write operation failed
        return;
    }
    tf->x[0] = bytes_written; // Return the number of bytes written
}

void sys_read(trapframe_t *tf) {
    // uart_send_string("[SYSCALL 14] read\r\n");
    int fd = tf->x[0];
    char *buf = (char *)tf->x[1];
    size_t len = tf->x[2];
    if (fd < 0 || fd >= MAX_FD || current_thread->files_table[fd] == NULL) {
        uart_send_string("[ERROR | READ] Invalid file descriptor\r\n");
        tf->x[0] = -1; // Return -1 if the file descriptor is invalid
        return;
    }

    int bytes_read = vfs_read(current_thread->files_table[fd], buf, len);
    if (bytes_read < 0) {
        uart_send_string("[ERROR | READ] Failed to read from file\r\n");
        tf->x[0] = -1; // Return -1 if the read operation failed
        return;
    }
    tf->x[0] = bytes_read; // Return the number of bytes read
}

void sys_mkdir(trapframe_t *tf) {
    // uart_send_string("[SYSCALL 15] mkdir\r\n");
    char *pathname = (char *)tf->x[0];

    if (vfs_mkdir(pathname) != 0) {
        uart_send_string("[ERROR | MKDIR] Failed to create directory\r\n");
        tf->x[0] = -1; // Return -1 if the mkdir operation failed
        return;
    }
    tf->x[0] = 0; // Return 0 on success
}

void sys_mount(trapframe_t *tf) {
    // uart_send_string("[SYSCALL 16] mount\r\n");
    char *target = (char *)tf->x[1];
    char *filesystem = (char *)tf->x[2];

    tf->x[0] = vfs_mount(target, filesystem); // Call the vfs_mount function
}

void sys_chdir(trapframe_t *tf) {
    // uart_send_string("[SYSCALL 17] chdir\r\n");
    char *pathname = (char *)tf->x[0];

    if (vfs_chdir(pathname) != 0) {
        uart_send_string("[ERROR | CHDIR] Directory not found\r\n");
        tf->x[0] = -1; // Return -1 if the directory is not found
        return;
    }

    tf->x[0] = 0; // Return 0 on success
}

void sys_lseek64(trapframe_t *tf) {
    // uart_send_string("[SYSCALL 18] lseek64\r\n");
    int fd = tf->x[0];
    long offset = tf->x[1];
    int whence = tf->x[2];

    if (fd < 0 || fd >= MAX_FD || current_thread->files_table[fd] == NULL) {
        uart_send_string("[ERROR | LSEEK64] Invalid file descriptor\r\n");
        tf->x[0] = -1; // Return -1 if the file descriptor is invalid
        return;
    }

    long new_offset = current_thread->files_table[fd]->f_ops->lseek64(current_thread->files_table[fd], offset, whence);
    if (new_offset < 0) {
        uart_send_string("[ERROR | LSEEK64] Failed to seek in file\r\n");
        tf->x[0] = -1; // Return -1 if the lseek operation failed
        return;
    }
    tf->x[0] = new_offset; // Return the new offset
}

void sys_ioctl(trapframe_t *tf) {
    // // uart_send_string("[SYSCALL 19] ioctl\r\n");
    // int fd = tf->x[0];
    // unsigned long request = tf->x[1];
    struct framebuffer_info *fb_info = (struct framebuffer_info *)tf->x[2];

    framebuffer_init();
    fb_info->width = width;
    fb_info->height = height;
    fb_info->pitch = pitch;
    fb_info->isrgb = isrgb;
    uart_send_string("lfb: 0x");
    uart_send_num(lfb, "hex");
    uart_send_string("\r\n");

    tf->x[0] = 0; // Return the result of the ioctl operation
}