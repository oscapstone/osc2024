SECTIONS
{
    . = 0x80000;
    .text : { *(.text.boot) *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss) 
        __bss_end = .;
    }
}
# 64bit
__bss_size = (__bss_end - __bss_start) >> 3;
