ENTRY(_start)

SECTIONS {
    . = 0x80000;
    .text : {
        KEEP(*(.text.main))
    } 
    .rodata : {
        *(.rodata*)
    }
}
