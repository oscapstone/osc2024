INCLUDE "src/common.ld"

ENTRY(__kernel)

SECTIONS {
    . = __kernel;
    __kernel_beg = .;

    __kernel_text_beg = .;

    .text : {
        KEEP(*(.text._start))
        *(.text .text.*)
    }

    .rodata : {
        *(.rodata .rodata.* .gnu.linkonce.r*)
    }

    __kernel_text_end = .;

    . = ALIGN(0x1000);

    .vsyscall : {
        __vsyscall_beg = .;
        *(.vsyscall)
        __vsyscall_end = .;
    }

    . = ALIGN(0x1000);

    __kernel_data_beg = .;

    .data : {
        *(.data .data.* .gnu.linkonce.d*)
    }

    .bss : {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }

    __kernel_data_end = .;

    /DISCARD/ : {
        *(.comment)
        *(.gnu*) *(.note*) *(.eh_frame*)
    }

    __kernel_end = .;
}
