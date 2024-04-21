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

    .heap : { 
        . = ALIGN(0x8);
        heap_begin = .;    
        heap_end = heap_begin + 0x100000;
    }

    PROVIDE(kernel_end = .);
}
