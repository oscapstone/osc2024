INCLUDE "src/common.ld"

ENTRY(__bootloader)

SECTIONS {
    . = __bootloader;
    __boot_core_stack_end = .;

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
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }

    /DISCARD/ : {
        *(.comment)
        *(.gnu*) *(.note*) *(.eh_frame*)
    }
}
