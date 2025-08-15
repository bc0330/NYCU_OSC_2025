ENTRY(_start)

SECTIONS
{
    . = 0x80000;

    .text : { KEEP(*(.text.boot)) *(.text .text.*) }

    . = ALIGN(16);                   /* Aligned for 64-bit data/instr */
    .rodata : { *(.rodata .rodata.*) }

    
    .data : { *(.data .data.*) }

    . = ALIGN(0x8);                   /* Aligned for 64-bit data/instr */ 
  
    .bss (NOLOAD) : { 
      __bss_start = .;
      *(.bss .bss*)
      *(COMMON)
      __bss_end = .;
    }
    
    . = ALIGN(0x8);
    __stack_start = .;
    .stack () : {
      . += 0x80000;                  /* 4MB stack */
    }
    __stack_end = .;
  
}
