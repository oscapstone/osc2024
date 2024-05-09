_heap_stack_size = 1M;
_bootloader_size = 64K;

SECTIONS
{
    . = 0x80000;                 /* Set the starting address of the sections to 0x80000(512Kb) */
    _start = .;                  /* Define the start symbol at the current address */
    .text : { *(.text.boot) *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : {
        _bss_top = .;                    /* Mark the start of the .bss section */
         *(.bss)                          /* Place the .bss section in the .bss output section */
    }
    _bss_size = SIZEOF(.bss) >> 3;       /* Calculate the size of the .bss section (divide by 8 to get the size in 64-bit words) */
    .heap : { 
        . = ALIGN(8);                    /* Align the start of the .heap section to an 8-byte boundary */
        _heap_top = .;                   /* Mark the start of the .heap section */
    }
    . = . + _heap_stack_size;            /* Increase the location counter by the size of the heap and stack */
    .stack : {
        . = ALIGN(8);                    /* Align the start of the .stack section to an 8-byte boundary */
        _stack_top = .;                  /* Mark the top of the stack (end of the .stack section) */
    }
    _end = .;                             /* Define the end symbol at the current addres*  */
    
    . = 0x3000000;                        /* Set the starting address of the next section to 0x3000000(48Mb) */
    _bootloader_relocated_addr = 0x3000000; /* Define the relocated address of the bootloader */
    . = . + _bootloader_size;             /* Increase the location counter by the size of the bootloader */
}
__code_size = (_end - _start);           /* Calculate the size of the code (from _start to _end)  */