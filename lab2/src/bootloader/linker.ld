SECTIONS
{
    . = 0x80000;
    __start = .;
    .text : { *(.text) }
    .bss : { 
        . = ALIGN(0x8);
        __bss_start = .;
        *(.bss) 
        __bss_end = .;
    }
    __heap_start = .;
    __end = .;
}
__bss_size = (__bss_end - __bss_start)>>3;