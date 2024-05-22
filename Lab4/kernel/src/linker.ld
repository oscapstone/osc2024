SECTIONS
{
    . = 0x80000;

    PROVIDE(kernel_start = .);

    .text.boot : { *(.text.boot)  }
    .text : { *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }

    .bss : {
        . = ALIGN(0x10);
        bss_begin = .;
        *(.bss*)
        bss_end = .;
    }

    PROVIDE(kernel_end = .);
}
