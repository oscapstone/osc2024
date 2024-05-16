
SECTIONS {
    .text : {
        KEEP(*(.text.main))
    }
    .rodata : {
        *(.rodata*)
    }
    .data : {
        *(.data*)
    }
    .bss : {
        *(.bss*)
    }
    .eh_frame_hdr : {
        *(.eh_frame_hdr)
    }
    .eh_frame : {
        *(.eh_frame)
    }

}

