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
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }

    .shstrtab : { *(.shstrtab) }
    .symtab   : { *(.symtab) }
    .strtab   : { *(.strtab) }

    /DISCARD/ : {
        *(.comment)
        *(.gnu*) *(.note*) *(.eh_frame*)
    }
}
