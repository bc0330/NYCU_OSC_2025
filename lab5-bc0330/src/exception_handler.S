#define CORE0_TIMER_IRQ_CTRL 0x40000040
.globl load_user_program
load_user_program: //
    mov x2, 0x340          
    /*
        0x3c0 = 0b00_1111_00_0000
                     DAIF    M[3:0]
        M = 0000 -> EL0t
        bit 4 = 0 -> AARCH64 execution state
        D debug exception
        A SError interrupt
        I IRQ interrupt
        F FIQ interrupt

        340 to enable IRQ
        0x340 = 0b00_1101_00_0000
     */
    
    msr spsr_el1, x2
    msr elr_el1, x0    // user program entry point
    msr sp_el0, x1     // user program stack pointer
    eret

.globl turn_on_timer
turn_on_timer: //
    mov x0, 0x2
    ldr x1, =CORE0_TIMER_IRQ_CTRL
    str x0, [x1]
    mov x0, 1
    msr cntp_ctl_el0, x0   // enable the timer for el0 
    ret

.globl turn_off_timer
turn_off_timer: //
    mov x0, 0x0
    ldr x1, =CORE0_TIMER_IRQ_CTRL
    str x0, [x1]
    mov x0, 0
    msr cntp_ctl_el0, x0   // enable the timer for el0 
    ret

