ENTRY(main)

SECTIONS
{
    . = 0x80000;
    .text : { 
        __text_start = .;
        KEEP(*(.text.main)) *(.text .text.*) 
        __text_end = .;
    } 
    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
    _end = .;
}

__bss_size = (__bss_end - __bss_start)>>3;
__text_size = (__text_end - __text_start);
