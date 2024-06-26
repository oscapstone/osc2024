SECTIONS
{
    . = 0xffff000000000000;

    . += 0x80000;
    PROVIDE(kernel_start = .);

    .text.boot : { KEEP(*(.text.boot))  }
    .text : { *(.text .text.* .gnu.linkonce.t*) }
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r*) }
    .data : { *(.data .data.* .gnu.linkonce.d*) }

    .bss : {
        . = ALIGN(0x10);
        bss_begin = .;
        *(.bss .bss.*)
        bss_end = .;
    }

    . = ALIGN(0x1000);
    PROVIDE(pg_dir = .);
    .data.pgd : { . += (4 * (1 << 12)); }


    PROVIDE(kernel_end = .);

    /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}
