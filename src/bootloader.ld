SECTIONS
{
    . = 0x60000;
    .text : { 
        __text_start = .;
        KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*) 
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
