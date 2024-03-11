__rpi_phys_binary_load_addr = 0x80000;

ENTRY(__rpi_phys_binary_load_addr)

SECTIONS {
    . = __rpi_phys_binary_load_addr;
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
        . = ALIGN(8);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(8);
        __bss_end = .;
    }

    .shstrtab : {
        *(.shstrtab)
    }

    /DISCARD/ : {
        *(.comment)
        *(.gnu*) *(.note*) *(.eh_frame*)
        /* *(.symtab) */
        /* *(.strtab) */
    }
}
