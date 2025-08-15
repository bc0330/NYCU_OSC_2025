// system booting assembly
// placed in text section
#include "mm.h"
.section ".text.boot"

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

.section ".data"
.globl __dtb_addr
__dtb_addr: .quad 0x0


