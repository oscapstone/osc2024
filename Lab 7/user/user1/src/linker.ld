SECTIONS
{
    . = 0x200000;

    .text :         { *(.text) }
    .rodata :       { *(.rodata) }
    .data :         { *(.data) }
    
    . = ALIGN(0x8);
    
    _bss_begin = .;
    .bss :          { *(.bss*) } 
    _bss_end = .;  
}