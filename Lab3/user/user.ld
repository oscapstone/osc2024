SECTIONS
{
    . = 0x0;
    .text :  { *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    . = ALIGN(8);
    bss_begin = .;
    .bss : { *(.bss*) }
    bss_end = .;
}
