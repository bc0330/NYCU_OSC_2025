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

    adr x3, __stack_end
    mov sp, x3   // set stack pointer. since stack goes downward, set it to a higher addr


clear_bss: // loop to clear bss section
    cmp x1, x2
    beq done
    str xzr, [x1], #8      // storing zero, increment addr
    b clear_bss

done:  // initialization done
    bl kernel_main
    b idle

from_el2_to_el1: //
    mov x0, (1 << 31)
    msr hcr_el2, x0
    mov x0, 0x3c5
    msr spsr_el2, x0
    msr elr_el2, lr
    eret


do_nothing://
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


