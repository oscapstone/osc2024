SECTIONS
{
    . = 0x60000;
    __pseudo_start = .;
    .text : {KEEP(*(.text.boot)) *(.text .text.*)}
    .rodata : { *(.rodata .rodata.*) }
    PROVIDE(_data = .);
    .data : {*(.data .data.*)}
    .bss (NOLOAD) :
    {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
    __pseudo_end = .;
}

__bss_size = (__bss_end - __bss_start)>>3;
__pseudo_size = (__pseudo_end - __pseudo_start)>>3;