
SECTIONS
{
    . = 0x80000;
    
    .text.boot :    { *(.text.boot) }
    .text :         { *(.text) }
    .rodata :       { *(.rodata) }
    .data :         { *(.data) }
    
    . = ALIGN(0x8);
    
    _bss_begin = .;
    .bss :          { *(.bss*) } 
    _bss_end = .;  
    
    /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.en_frame*) }
}

_bss_size = (_bss_end -  _bss_begin) >> 3;
