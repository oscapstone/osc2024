SECTIONS
{
    . = 0x60000;
    __cpy_start = .;
    .text : { 
        KEEP(*(.text.boot)) *(.text .text.*) 
    }
    .rodata : { *(.rodata..*) }
    . = ALIGN(16);
    __cpy_end = .;
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
__cpy_size = (__cpy_end - __cpy_start);
