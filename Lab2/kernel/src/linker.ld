SECTIONS
{
    . = 0x80000;
    __start = .;

    .text : 
    { 
        KEEP(*(.text.boot)) 
        *(.text) 
    }
    .rodata : 
    { 
        *(.rodata) 
        *(.rodata*)
    }
    .data : 
    { 
        *(.data) *(.data*) 
    }
    .bss : 
    {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss) 
        __bss_end = .;
    }
    .heap :
    {
        __heap_top = .;
        . = . + __heap_size;
        *(.heap);
    }
    .stack : 
    {
        *(.stack)
        __stack_top = .;
    }

    __end = .;

    /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}

__stack_size    = 0x1000;
__heap_size     = 0x1000;
__bss_size      = (__bss_end - __bss_start) >> 3;
