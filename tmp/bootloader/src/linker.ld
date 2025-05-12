/* Linker script for bootloader */
ENTRY(_start)

SECTIONS
{
    . = 0x60000;                      /* Start address of bootloader, cause kernel starts at 80000*/
    __code = .;
    .text : { KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*) }
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r*) }
    .data : { *(.data .data.* .gnu.linkonce.d*) }

    . = ALIGN(0x8);                   /* Aligned for 64-bit data/instr */ 
  
    .bss (NOLOAD) : { 
      __bss_start = .;
      *(.bss .bss*)
      *(COMMON)
      __bss_end = .;
    }
    
    __end = .;
  
}
__loader_size = (__end - __code);
