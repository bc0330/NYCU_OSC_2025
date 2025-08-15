// system booting assembly
// placed in text section
#include "mm.h"
.section ".text.boot"
#define CORE0_TIMER_IRQ_CTRL 0xFFFF000040000040

#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16))
#define TCR_CONFIG_4KB ((0b00 << 14) |  (0b10 << 30))
#define TCR_CONFIG_DEFAULT (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

#define MAIR_DEVICE_nGnRnE 0b00000000
#define MAIR_NORMAL_NOCACHE 0b01000100
#define MAIR_IDX_DEVICE_nGnRnE 0
#define MAIR_IDX_NORMAL_NOCACHE 1

#define PD_TABLE 0b11
#define PD_BLOCK 0b01
#define PD_ACCESS (1 << 10)
#define BOOT_PGD_ATTR PD_TABLE
#define BOOT_PUD_ATTR (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK)

#define vtop(addr) (addr - 0xFFFF000000000000) // map virtual address to physical address
#define ptov(addr) (addr + 0xFFFF000000000000) // map physical address to virtual address

.globl _start
_start: //

    ldr x9, =vtop(__stack_end)   // set the stack pointer to the correct phys addr
    mov sp, x9
    msr sp_el1, x9

    sub sp, sp, 16               
    str x0, [sp]                 // save the dtb pointer to stack

    // check for 1st core
    mrs x1, mpidr_el1  
    and x1, x1, #0xFF    
    cmp x1, #0
    beq init
idle: //
    wfe                    // other cores sleep
    b idle

init:  /* Initialization */   
    // store the address from x0 to __dtb_addr
    ldr x1, =vtop(__bss_start)
    ldr x2, =vtop(__bss_end)
clear_bss: // loop to clear bss section
    cmp x1, x2
    beq done
    str xzr, [x1], #8      // storing zero, increment addr
    b clear_bss

done:  // initialization done

set_exception_vector_table: // set the exception vector table
    ldr x0, =exception_vector_table
    msr vbar_el1, x0

    bl from_el2_to_el1

// For Virtual Memory
setup_tcr_el1: // set TCR_EL1
    ldr x0, =TCR_CONFIG_DEFAULT
    msr tcr_el1, x0

serup_MAIR_EL1: // set MAIR_EL1
    ldr x0, =( \
    (MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8)) | \
    (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8)) \
    )
    msr mair_el1, x0

setup_PGD_PUD: // set PGD and PUD
    mov x0, 0 // PGD's page frame at 0x0
    mov x1, 0x1000 // PUD's page frame at 0x1000

    ldr x2, = BOOT_PGD_ATTR
    orr x2, x1, x2 // combine the physical address of next level page with attribute.
    str x2, [x0]

    ldr x2, = BOOT_PUD_ATTR
    mov x3, 0x00000000
    orr x3, x2, x3
    str x3, [x1] // 1st 1GB mapped by the 1st entry of PUD
    mov x3, 0x40000000
    orr x3, x2, x3
    str x3, [x1, 8] // 2nd 1GB mapped by the 2nd entry of PUD

    msr ttbr0_el1, x0 // load PGD to the bottom translation-based register.
    msr ttbr1_el1, x0 // load PGD to the top translation-based register
    mrs x2, sctlr_el1
    orr x2 , x2, 1
    msr sctlr_el1, x2 // enable MMU, cache remains disabled
// end of virtual memory setup

    ldr x9, =__stack_end
    sub sp, x9, #16
    ldr x9, [sp]
    mov x1, 0
    movk x1, 0xFFFF, lsl #48
    adr x10, __dtb_addr
    orr x9, x9, x1 // set the dtb pointer to the virtual address
    str x9, [x10] // store the dtb pointer to __dtb_addr
    add sp, sp, #16

    bl core_timer_enable
    ldr x0, =kernel_main
    br x0
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


