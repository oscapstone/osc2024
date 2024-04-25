SECTIONS
{
    .text : ALIGN(8) {
        *(.text*);
    }

    .data : ALIGN(8) {
        *(.data*);
    }

    .rodata : ALIGN(8) {
        *(.rodata*);
    }

    .bss : ALIGN(8) {
        __bss_start = .;
        *(.bss*);
        __bss_end = .;
    }
}

__bss_size = (__bss_end - __bss_start);
