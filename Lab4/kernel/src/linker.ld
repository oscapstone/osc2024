SECTIONS
{
    . = 0x80000;

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

    PROVIDE(kernel_end = .);

    /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}
