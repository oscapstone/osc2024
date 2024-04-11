ENTRY(_start)

SECTIONS {
    . = 0x30010000;
    .text : {
        KEEP(*(.text._start))
    }
    .rodata : {
        *(.rodata*)
    }
}
