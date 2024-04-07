SECTIONS {
    . = 0x80000;

    .text : { KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*) }

    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r*) }

    /* why _data ? */
    PROVIDE(_data = .);
    .data : { *(.data .data.* .gnu.linkonce.d*) }

    .bss (NOLOAD) : {
        /* str instruction accepts 8-aligned address */
        . = ALIGN(8);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }

    .heap (NOLOAD) : {
        . = ALIGN(8);
        _heap_start = .;
        . += 16 * 1024 * 1024; /* 16MB */
        _heap_end = .;
    }

    _end = .;

    /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}

__bss_size = (__bss_end - __bss_start) >> 3;
