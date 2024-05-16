SECTIONS {
    .text : {
        KEEP(*(.text._start))
        *(.text .text.*)
    }
    .rodata : {
        *(.rodata .rodata.* .gnu.linkonce.r*)
    }
    .data : {
        *(.data .data.* .gnu.linkonce.d*)
    }

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    }

    /DISCARD/ : {
        *(.comment)
        *(.gnu*) *(.note*) *(.eh_frame*)
    }
}
