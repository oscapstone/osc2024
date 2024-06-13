SECTIONS
{
    . = 0x80000;
    __start = .;
    .text : { KEEP(*(.text.boot)) *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : { 
        __bss_start = .;
        *(.bss)
        *(COMMON)
        __bss_end = .;
    }
    .heap : {
        . = ALIGN(8);
        __heap_start = .;
    }
    . = . + 8M;
    .stack : {
        . = ALIGN(8);
        __stack_start = .;
    }
    __end = .;
}
__bss_size = (__bss_end - __bss_start)>>3;