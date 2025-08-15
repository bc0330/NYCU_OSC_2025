// system booting assembly
// placed in text section

.section ".text.boot"

/*
    Run on Boot Strap Processor (BSP). Therefore no need to sleed other cores. 
 */

.globl _start
_start:             // starting the bootloader
                    // saving argument registers, 
                    // x10~13 are not used by gcc generated code
    mov x10, x0   
    mov x11, x1
    mov x12, x2
    mov x13, x3

    // Relocation
    /*
        GPU loads our bootloader at 0x80000,
        We have to move it to 0x60000, to leave space for kernel
     */
    ldr x1, = 0x80000
    ldr x2, = _start
    ldr w3, = __loader_size
reloc:  // relocation loop
    ldr x4, [x1], #8
    str x4, [x2], #8
    sub w3, w3, #8
    cbnz w3, reloc

    // loader stack pointer to 0x60000
    ldr x1, = _start
    mov sp, x1


    // clear bss
    ldr x1, = __bss_start
    ldr x2, = __bss_end
clear:   // clear bss loop
    cmp x1, x2
    beq _start_done
    str xzr, [x1], #8
    b clear


_start_done:   // bootloader done
    bl boot_main - 0x20000  // jump to relocated boot_main 

    // restore arguments
    mov x0, x10
    mov x1, x11
    mov x2, x12
    mov x3, x13
    // branch to kernel
    bl 0x80000
    b _start_done
    


