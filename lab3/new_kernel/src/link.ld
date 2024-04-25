_heap_stack_size = 1000M;

SECTIONS
{
    . = 0x80000;

    __begin = .;
    .text :  
    { 
        KEEP(*(.text.boot))
        *(.text) 
        }

    .rodata : 
    { 
        *(.rodata) 
        }

    .data : 
    { 
        *(.data) 
        }

    . = ALIGN(16);

    .bss (NOLOAD) : 
    { 
    __bss_start = .;
        *(.bss*) 
    __bss_end = .;
        } 

    .heap : {
        . = ALIGN(4);
        __heap_top = .;
    }
    . = . + _heap_stack_size;
    __heap_bottom = .;
    .stack : {
        . = ALIGN(4);
        __stack_top = .;
    }
    __end = .;
}
# 64bit
__code_size = (__end - __begin);
__bss_size = (__bss_end - __bss_start) >> 3;
__boot_loader_addr = 0x3000000 ;