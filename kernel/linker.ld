ENTRY(_start)

SECTIONS {
    . = 0x80000;
    .text : {
        KEEP(*(.text._start_kernel))
    } 
    .rodata : {
        *(.rodata*)
    }
    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
}

__bss_size = (__bss_end - __bss_start)>>3;
