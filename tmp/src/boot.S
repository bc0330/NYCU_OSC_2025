// system booting assembly
// placed in text section
#include "mm.h"
.section ".text.boot"
#define CORE0_TIMER_IRQ_CTRL 0x40000040

.globl _start
_start: //
    mrs x1, mpidr_el1      // read Multiprocessor Affinity Register, use mrs cause MAR is system register

    /* Extract Affinity 0 (core ids) */
    and x1, x1, #0xFF      // get the lower 8 bits

    /* Check if core ID is 0 */
    cmp x1, #0
    beq init

idle: //
    wfe                    // other cores sleep
    b idle


init:  /* Initialization */   
    // store the address from x0 to __dtb_addr
    adr x1, __dtb_addr
    str x0, [x1]

    bl from_el2_to_el1

set_exception_vector_table: // set the exception vector table
    adr x0, exception_vector_table
    msr vbar_el1, x0

    adr x1, __bss_start
    adr x2, __bss_end

    ldr x3, =__stack_end
    mov sp, x3   // set stack pointer. since stack goes downward, set it to a higher addr


clear_bss: // loop to clear bss section
    cmp x1, x2
    beq done
    str xzr, [x1], #8      // storing zero, increment addr
    b clear_bss

done:  // initialization done
    bl core_timer_enable
    bl kernel_main
    b idle

from_el2_to_el1: //
    mov x0, (1 << 31)
    msr hcr_el2, x0
    mov x0, 0x3c5
    msr spsr_el2, x0
    msr elr_el2, lr
    eret

core_timer_enable://
    mov x0, 1
    msr cntp_ctl_el0, x0 
    mrs x0, cntfrq_el0   
    lsr x0, x0, 5 // set timer to 1/32 sec
    msr cntp_tval_el0, x0 // set expired time
    mov x0, 2
    ldr x1, =CORE0_TIMER_IRQ_CTRL
    str w0, [x1] // unmask timer interrupt
    ret

.macro save_all
    sub sp, sp, 17 * 16
    stp x0, x1, [sp ,16 * 0]
    stp x2, x3, [sp ,16 * 1]
    stp x4, x5, [sp ,16 * 2]
    stp x6, x7, [sp ,16 * 3]
    stp x8, x9, [sp ,16 * 4]
    stp x10, x11, [sp ,16 * 5]
    stp x12, x13, [sp ,16 * 6]
    stp x14, x15, [sp ,16 * 7]
    stp x16, x17, [sp ,16 * 8]
    stp x18, x19, [sp ,16 * 9]
    stp x20, x21, [sp ,16 * 10]
    stp x22, x23, [sp ,16 * 11]
    stp x24, x25, [sp ,16 * 12]
    stp x26, x27, [sp ,16 * 13]
    stp x28, x29, [sp ,16 * 14]
    mrs x0, sp_el0
    mrs x1, elr_el1
    mrs x2, spsr_el1
    stp x30, x0, [sp, 16 * 15]
    stp x1, x2, [sp, 16 * 16]
    mov x0, sp
.endm

.macro load_all
    ldp x0, x1, [sp, 16 * 16]
    msr elr_el1, x0
    msr spsr_el1, x1
    ldp x30, x0, [sp, 16 * 15]
    msr sp_el0, x0
    ldp x28, x29, [sp, 16 * 14]
    ldp x26, x27, [sp, 16 * 13]
    ldp x24, x25, [sp, 16 * 12]
    ldp x22, x23, [sp, 16 * 11]
    ldp x20, x21, [sp, 16 * 10]
    ldp x18, x19, [sp, 16 * 9]
    ldp x16, x17, [sp, 16 * 8]
    ldp x14, x15, [sp, 16 * 7]
    ldp x12, x13, [sp, 16 * 6]
    ldp x10, x11, [sp, 16 * 5]
    ldp x8, x9, [sp, 16 * 4]
    ldp x6, x7, [sp, 16 * 3]
    ldp x4, x5, [sp, 16 * 2]
    ldp x2, x3, [sp, 16 * 1]
    ldp x0, x1, [sp, 16 * 0]
    add sp, sp, 17 * 16
.endm

do_nothing://
    eret

general_irq: //
    save_all
    bl irq_handler
    load_all
    eret

general_sync: //
    save_all
    bl sync_handler
    load_all
    eret

.align 11 // vector table should be aligned to 0x800
.global exception_vector_table
exception_vector_table: // exception vector table
  b do_nothing // branch to a handler function.
  .align 7 // entry size is 0x80, .align will pad 0
  b do_nothing
  .align 7
  b do_nothing
  .align 7
  b do_nothing
  .align 7

  b do_nothing
  .align 7
  b general_irq
  .align 7
  b do_nothing
  .align 7
  b do_nothing
  .align 7

  b general_sync
  .align 7
  b general_irq
  .align 7
  b do_nothing
  .align 7
  b do_nothing
  .align 7

  b do_nothing
  .align 7
  b do_nothing
  .align 7
  b do_nothing
  .align 7
  b do_nothing
  .align 7

.section ".data"
.globl __dtb_addr
__dtb_addr: .quad 0x0


