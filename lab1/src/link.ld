SECTIONS
{
    . = 0x80000;
    .text.boot : { *(.text.boot) }
    .text :  { *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    . = ALIGN(16);
    __bss_begin = .;
    .bss : { *(.bss*) } 
    __bss_end = .;
}
# 64bit
__bss_size = (__bss_end - __bss_begin) >> 3;