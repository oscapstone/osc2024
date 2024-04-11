ENTRY(_start)

SECTIONS {
    . = 0x80000;
    .text : {
        KEEP(*(.text._start_kernel))
    } 
    .rodata : {
        *(.rodata*)
    }
}
