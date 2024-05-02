SECTIONS
{
    . = 0x80000;
    __start = .;
    .text : 
    { 
        __text_start = .;
        *(.text)
    }
    .bss : 
    {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss*)
        __bss_end = .;
    }
    __heap_start = .;
    __end = .;
}

__bss_size = (__bss_end - __bss_start) >> 3;
__startup_allocator_start = 0x3b000000;
__startup_allocator_end = __startup_allocator_start + 0x1000000;