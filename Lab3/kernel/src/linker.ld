SECTIONS
{
    . = 0x80000;
    .text.boot : { *(.text.boot)  }
    .text : { *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }

    . = ALIGN(0x10);
    bss_begin = .;
    .bss : { *(.bss*) }
    bss_end = .;

    . = ALIGN(0x4);
    heap_begin = .;
    .heap : { }
    heap_end = heap_begin + 0x100000;
}
