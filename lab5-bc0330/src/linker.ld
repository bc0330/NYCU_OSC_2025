ENTRY(_start)

SECTIONS
{
    . = 0x80000;
    __start_code = .;

    .text : { KEEP(*(.text.boot)) *(.text .text.*) }
    . = ALIGN(16);                   /* Aligned for 64-bit data/instr */
    .rodata : { *(.rodata .rodata.*) }
    . = ALIGN(16);                   /* Aligned for 64-bit data/instr */
    .data : { *(.data .data.*) }

    . = ALIGN(16);                   /* Aligned for 64-bit data/instr */ 
    .bss (NOLOAD) : { 
      __bss_start = .;
      *(.bss .bss*)
      *(COMMON)
      __bss_end = .;
    }
    
    . = ALIGN(16);
    __stack_start = .;
    .stack (NOLOAD) : {
      . += 0x100000;              
    }
    __stack_end = .;
    __end_code = .;
}
