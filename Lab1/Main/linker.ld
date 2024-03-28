SECTIONS
{
    . = 0x80000;

    .text.boot : { *(.text.boot) }

    .text :  { *(.text) }
    
    .rodata : { *(.rodata) }
    
    .data : { *(.data) }

    bss_begin = .;  
    .bss (NOLOAD) : { *(.bss) }
    bss_end = .; 
}
