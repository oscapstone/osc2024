SECTIONS
{
    . = 0x80000;
    .text : { *(.text.boot) *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : { 
        . = ALIGN(8);   /* size of cache line is 64, 16B alignment may ensure it align to cache line */
        __bss_start = .;
        *(.bss)
        __bss_end = .;
    }
}

__bss_size = (__bss_end - __bss_start) >> 3;     /* number of Byte of bss segment */