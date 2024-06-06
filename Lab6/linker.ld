SECTIONS
{
    . = 0xffff000000000000;
    . += 0x80000;
    kernel_start = .;
    .text.boot : { *(.text.boot) }
    .text :  { *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    . = ALIGN(8);
    bss_begin = .;
    .bss : { *(.bss*) }
    bss_end = .;
    kernel_end = .;
}
bss_size = (bss_end - bss_begin) >> 3;
